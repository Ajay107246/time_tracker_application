#!/usr/bin/env python3
"""
Time Reporting Tool
A comprehensive time tracking application with desktop notifications and CSV logging.

Usage:
    python time_tracker.py start [description] - Start time tracking
    python time_tracker.py stop - Stop time tracking  
    python time_tracker.py status - Check current status
    python time_tracker.py report - Generate daily report
"""

import os
import sys
import csv
import json
import time
import argparse
import threading
import subprocess
import signal
from datetime import datetime, timedelta
from pathlib import Path
#import fcntl
import platform
import portalocker

# Configuration
CONFIG_DIR = Path.home() / ".time_tracker"
STATE_FILE = CONFIG_DIR / "current_session.json"
CSV_FILE = CONFIG_DIR / "time_logs.csv"
LOCK_FILE = CONFIG_DIR / "tracker.lock"
NOTIFICATION_INTERVAL = 2 * 60 * 60  # 2 hours in seconds

class TimeTracker:
    def __init__(self):
        self.config_dir = CONFIG_DIR
        self.state_file = STATE_FILE
        self.csv_file = CSV_FILE
        self.lock_file = LOCK_FILE
        self.notification_interval = NOTIFICATION_INTERVAL
        self.setup_directories()
        self.lock = threading.Lock()
        
    def setup_directories(self):
        """Create necessary directories and files"""
        self.config_dir.mkdir(exist_ok=True)
        
        # Initialize CSV file with headers if it doesn't exist
        if not self.csv_file.exists():
            with open(self.csv_file, 'w', newline='', encoding='utf-8') as f:
                writer = csv.writer(f)
                writer.writerow(['name', 'date', 'start_time', 'end_time', 'duration_hours', 'description'])
    
    #def send_notification(self, title, message):
    def send_notification(self, title, message):
        """Send desktop notification using notify-send"""
        system = platform.system()
        if system == "Windows":
            try:
                from win10toast import ToastNotifier
                toaster = ToastNotifier()
                toaster.show_toast(title, message, duration=10)
            except ImportError:
                print(f"NOTIFICATION: {title} - {message}")
        else:
            try:
                subprocess.run([
                    'notify-send', 
                    '-i', 'time-admin',
                    '-u', 'normal',
                    '-t', '5000',
                    title, 
                    message
                ], check=False)
            except FileNotFoundError:
                print(f"NOTIFICATION: {title} - {message}")
    
    def is_running(self):
        """Check if time tracking is currently active"""
        return self.state_file.exists()
    
    def get_current_session(self):
        """Get current session information"""
        if not self.state_file.exists():
            return None
            
        try:
            with open(self.state_file, 'r', encoding='utf-8') as f:
                return json.load(f)
        except (json.JSONDecodeError, IOError):
            return None
    
    def start_tracking(self, description="Work session"):
        """Start time tracking"""
        if self.is_running():
            current = self.get_current_session()
            if current:
                start_time = datetime.fromisoformat(current['start_time'])
                duration = datetime.now() - start_time
                print(f"Time tracking already running for {duration}")
                print(f"Description: {current['description']}")
                return False
        
        # Create session data
        session_data = {
            'name': os.getlogin(),
            'start_time': datetime.now().isoformat(),
            'description': description,
            'last_notification': datetime.now().isoformat()
        }
        
        # Save session state
        with open(self.state_file, 'w', encoding='utf-8') as f:
            json.dump(session_data, f, indent=2)
        
        # Start background notification daemon
        self.start_notification_daemon()
        
        # Send start notification
        self.send_notification(
            "Time Tracker Started", 
            f"Started tracking: {description}"
        )
        
        print(f"Time tracking started at {datetime.now().strftime('%H:%M:%S')}")
        print(f"Description: {description}")
        return True
    
    def stop_tracking(self):
        """Stop time tracking and log to CSV"""
        if not self.is_running():
            print("Time tracking is not currently running.")
            return False
        
        session_data = self.get_current_session()
        if not session_data:
            print("Error: Could not read session data.")
            return False
        
        # Calculate duration
        start_time = datetime.fromisoformat(session_data['start_time'])
        end_time = datetime.now()
        duration = end_time - start_time
        duration_hours = duration.total_seconds() / 3600
        
        # Log to CSV
        with self.lock:
            with open(self.csv_file, 'a', newline='', encoding='utf-8') as f:
                # Use file locking to prevent concurrent writes
                #fcntl.flock(f.fileno(), fcntl.LOCK_EX)
                portalocker.lock(f.fileno(), portalocker.LockFlags.EXCLUSIVE)
                writer = csv.writer(f)
                writer.writerow([
                    session_data['name'],
                    start_time.strftime('%Y-%m-%d'),
                    start_time.strftime('%H:%M:%S'),
                    end_time.strftime('%H:%M:%S'),
                    round(duration_hours, 2),
                    session_data['description']
                ])
                #fcntl.flock(f.fileno(), fcntl.LOCK_UN)
                #portalocker.lock(f.fileno(), portalocker.LockFlags.UNLOCK)
                portalocker.unlock(f)#.fileno(), portalocker.LockFlags.UNLOCK)

        # Remove state file
        self.state_file.unlink()
        
        # Stop notification daemon
        self.stop_notification_daemon()
        
        # Send stop notification
        self.send_notification(
            "Time Tracker Stopped", 
            f"Worked for {duration}\nLogged to CSV file"
        )
        
        print(f"Time tracking stopped at {end_time.strftime('%H:%M:%S')}")
        print(f"Duration: {duration}")
        print(f"Hours: {duration_hours:.2f}")
        print(f"Logged to: {self.csv_file}")
        return True
    
    def get_status(self):
        """Get current tracking status"""
        if not self.is_running():
            print("Time tracking is not currently running.")
            return
        session_data = self.get_current_session()
        if not session_data:
            print("Error: Could not read session data.")
            return
        
        start_time = datetime.fromisoformat(session_data['start_time'])
        duration = datetime.now() - start_time
        
        print("Time tracking is ACTIVE")
        print(f"Started: {start_time.strftime('%Y-%m-%d %H:%M:%S')}")
        print(f"Duration: {duration}")
        print(f"Description: {session_data['description']}")
        print(f"User: {session_data['name']}")
    
    def start_notification_daemon(self):
        """Start background daemon for periodic notifications"""
        daemon_script = self.config_dir / "notification_daemon.py"
        
        # Daemon script content
        daemon_code = '''#!/usr/bin/env python3
import os
import sys
import time
import json
import subprocess
import signal
import platform
from datetime import datetime, timedelta
from pathlib import Path

CONFIG_DIR = Path.home() / ".time_tracker"
STATE_FILE = CONFIG_DIR / "current_session.json"
PID_FILE = CONFIG_DIR / "daemon.pid"
# NOTIFICATION_INTERVAL = 2 * 60 * 60  # 2 hours
NOTIFICATION_INTERVAL = 3 * 60  # 3 minutes

# def send_notification(self, title, message):
#     try:
#         subprocess.run([
#             'notify-send', 
#             '-i', 'clock',
#             '-u', 'normal',
#             '-t', '10000',
#             title, 
#             message
#         ], check=False)
#     except FileNotFoundError:
#         print(f"NOTIFICATION: {title} - {message}")

def send_notification(self, title, message):
    system = platform.system()
    if system == "Windows":
        try:
            from win10toast import ToastNotifier
            toaster = ToastNotifier()
            toaster.show_toast(title, message, duration=10)
        except ImportError:
            print(f"NOTIFICATION: {title} - {message}")
    else:
        try:
            import subprocess
            subprocess.run([
                'notify-send', 
                '-i', 'time-admin',
                '-u', 'normal',
                '-t', '5000',
                title, 
                message
            ], check=False)
        except FileNotFoundError:
            print(f"NOTIFICATION: {title} - {message}")

def cleanup_and_exit(signum=None, frame=None):
    """Clean up on exit"""
    if PID_FILE.exists():
        PID_FILE.unlink()
    sys.exit(0)

def main():
    # Handle signals for clean shutdown
    signal.signal(signal.SIGTERM, cleanup_and_exit)
    signal.signal(signal.SIGINT, cleanup_and_exit)
    
    # Ensure config directory exists
    CONFIG_DIR.mkdir(exist_ok=True)
    
    # Write PID file
    with open(PID_FILE, 'w') as f:
        f.write(str(os.getpid()))
    
    last_notification = datetime.now()
    
    try:
        while True:
            # Check if session still exists
            if not STATE_FILE.exists():
                cleanup_and_exit()
            
            # Check if it's time for notification
            if datetime.now() - last_notification >= timedelta(seconds=NOTIFICATION_INTERVAL):
                try:
                    with open(STATE_FILE, 'r') as f:
                        session_data = json.load(f)
                    
                    start_time = datetime.fromisoformat(session_data['start_time'])
                    duration = datetime.now() - start_time
                    hours = duration.total_seconds() / 3600
                    
                    send_notification(
                        "Time Tracker Reminder", 
                        f"You've been working for {hours:.1f} hours\\nCurrent task: {session_data['description']}"
                    )
                    
                    last_notification = datetime.now()
                    
                    # Update last notification time in session
                    session_data['last_notification'] = last_notification.isoformat()
                    with open(STATE_FILE, 'w') as f:
                        json.dump(session_data, f, indent=2)
                        
                except Exception:
                    # If there's an error reading session, exit daemon
                    cleanup_and_exit()
            
            time.sleep(60)  # Check every minute
            
    except KeyboardInterrupt:
        cleanup_and_exit()

if __name__ == "__main__":
    main()
'''
        with open(daemon_script, 'w', encoding='utf-8') as f:
            f.write(daemon_code)
        daemon_script.chmod(0o755)
        
        # Start daemon in background
        subprocess.Popen([sys.executable, str(daemon_script)], 
                        stdout=subprocess.DEVNULL, 
                        stderr=subprocess.DEVNULL)
    
    def stop_notification_daemon(self):
        """Stop background notification daemon"""
        pid_file = self.config_dir / "daemon.pid"
        if pid_file.exists():
            try:
                with open(pid_file, 'r') as f:
                    pid = int(f.read().strip())
                os.kill(pid, signal.SIGTERM)
                time.sleep(0.5)
                if pid_file.exists():
                    pid_file.unlink()
            except (ValueError, ProcessLookupError, PermissionError):
                # Process might already be dead
                if pid_file.exists():
                    pid_file.unlink()
    
    def generate_daily_report(self, date_str=None):
        """Generate report for a specific date"""
        if date_str is None:
            date_str = datetime.now().strftime('%Y-%m-%d')
        
        if not self.csv_file.exists():
            print("No time logs found.")
            return
        
        daily_entries = []
        total_hours = 0
        
        with open(self.csv_file, 'r', encoding='utf-8') as f:
            reader = csv.DictReader(f)
            for row in reader:
                if row['date'] == date_str:
                    daily_entries.append(row)
                    total_hours += float(row['duration_hours'])
        
        if not daily_entries:
            print(f"No entries found for {date_str}")
            return
        
        print(f"\n=== Daily Report for {date_str} ===")
        print(f"Total Hours: {total_hours:.2f}")
        print(f"Total Entries: {len(daily_entries)}")
        print("\nDetails:")
        print("-" * 70)
        
        for entry in daily_entries:
            print(f"{entry['start_time']} - {entry['end_time']} "
                  f"({entry['duration_hours']}h): {entry['description']}")
        
        print("-" * 70)
        print(f"Total: {total_hours:.2f} hours")

def main():
    parser = argparse.ArgumentParser(description='Time Reporting Tool')
    subparsers = parser.add_subparsers(dest='command', help='Available commands')
    
    # Start command
    start_parser = subparsers.add_parser('start', help='Start time tracking')
    start_parser.add_argument('description', nargs='*', default=['Work session'], 
                             help='Description of the work session')
    
    # Stop command
    subparsers.add_parser('stop', help='Stop time tracking')
    
    # Status command
    subparsers.add_parser('status', help='Check current tracking status')
    
    # Report command
    report_parser = subparsers.add_parser('report', help='Generate daily report')
    report_parser.add_argument('--date', help='Date in YYYY-MM-DD format (default: today)')
    
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        return
    
    tracker = TimeTracker()
    
    if args.command == 'start':
        description = ' '.join(args.description)
        tracker.start_tracking(description)
    elif args.command == 'stop':
        tracker.stop_tracking()
    elif args.command == 'status':
        tracker.get_status()
    elif args.command == 'report':
        tracker.generate_daily_report(args.date)

if __name__ == "__main__":
    main()
