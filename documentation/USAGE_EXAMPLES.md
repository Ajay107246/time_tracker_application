# Time Tracker Usage Examples

This document provides comprehensive examples of how to use the time tracking tool effectively.

## Basic Usage Examples

### Starting Time Tracking
```bash
# Simple start with default description
./time_tracker.py start

# Start with custom description
./time_tracker.py start "Debugging authentication module"

# Start with multi-word description
./time_tracker.py start Working on sprint planning meeting

# Start with quoted description for special characters
./time_tracker.py start "Client call - Q4 requirements review"
```

### Checking Status
```bash
# Check current tracking status
./time_tracker.py status

# Example output:
# Time tracking is ACTIVE
# Started: 2025-10-03 14:30:00
# Duration: 2:15:00
# Description: Working on time tracker implementation
# User: john_doe
```

### Stopping Time Tracking
```bash
# Stop current session
./time_tracker.py stop

# Example output:
# Time tracking stopped at 16:45:00
# Duration: 2:15:00
# Hours: 2.25
# Logged to: /home/john_doe/.time_tracker/time_logs.csv
```

### Daily Reports
```bash
# Generate today's report
./time_tracker.py report

# Generate report for specific date
./time_tracker.py report --date 2025-10-01

# Example output:
# === Daily Report for 2025-10-03 ===
# Total Hours: 7.25
# Total Entries: 2
# 
# Details:
# ----------------------------------------------------------------------
# 09:00:00 - 12:00:00 (3.0h): Sprint planning and team standup
# 13:00:00 - 17:15:00 (4.25h): Implementing user authentication API
# ----------------------------------------------------------------------
# Total: 7.25 hours
```

## Workflow Examples

### Typical Work Day
```bash
# Start of work day
./time_tracker.py start "Daily standup and planning"

# After standup, switch tasks
./time_tracker.py stop
./time_tracker.py start "Code review - authentication module"

# Lunch break
./time_tracker.py stop

# Afternoon work
./time_tracker.py start "Implementing user profile API"

# End of day
./time_tracker.py stop

# Review the day
./time_tracker.py report
```

### Project-based Tracking
```bash
# Client work
./time_tracker.py start "Project Alpha - Frontend development"
./time_tracker.py stop

# Internal work  
./time_tracker.py start "Internal tools - Database optimization"
./time_tracker.py stop

# Meeting time
./time_tracker.py start "Client meeting - Project Alpha requirements"
./time_tracker.py stop
```

### Development Sprint Tracking
```bash
# Sprint planning
./time_tracker.py start "Sprint 23 - Planning and story estimation"

# Development work
./time_tracker.py start "USER-123 - Implement login functionality"

# Bug fixes
./time_tracker.py start "BUG-456 - Fix password reset email issue"

# Code review
./time_tracker.py start "Code review - Authentication module"

# Sprint retrospective
./time_tracker.py start "Sprint 23 - Retrospective meeting"
```

## C++ Version Examples

### Basic Commands
```bash
# Compile first
make

# Start tracking
./time_tracker_cpp start "Working on C++ implementation"

# Check status
./time_tracker_cpp status

# Stop tracking
./time_tracker_cpp stop

# Generate report
./time_tracker_cpp report
```

### Installation and Usage
```bash
# Install dependencies
make deps

# Build the application
make

# Install to system
make install

# Now use from anywhere
time_tracker_cpp start "System-wide time tracking"
```

## Wrapper Script Examples

After running setup.sh, you can use the convenient wrapper:

```bash
# These commands work from any directory
timetrack start "Documentation writing"
timetrack status
timetrack stop
timetrack report

# Weekly time summary (after accumulating data)
timetrack report --date 2025-10-01
timetrack report --date 2025-10-02
timetrack report --date 2025-10-03
```

## Data Analysis Examples

### CSV Analysis with Command Line Tools
```bash
# View raw CSV data
cat ~/.time_tracker/time_logs.csv

# Count total entries
wc -l ~/.time_tracker/time_logs.csv

# Filter by date
grep "2025-10-03" ~/.time_tracker/time_logs.csv

# Calculate weekly hours (requires date range)
grep -E "2025-10-0[1-7]" ~/.time_tracker/time_logs.csv | awk -F, '{sum+=$5} END {print "Total hours:", sum}'
```

### CSV Analysis with Spreadsheets
1. Open `~/.time_tracker/time_logs.csv` in LibreOffice Calc or Excel
2. Create pivot tables for:
   - Hours per day
   - Hours per project (based on description keywords)
   - Average session length
   - Most productive times

## Notification Examples

### What You'll See
- **Start notification**: "Time Tracker Started - Started tracking: Project work"
- **2-hour reminder**: "Time Tracker Reminder - You've been working for 2.1 hours. Current task: Project work"
- **Stop notification**: "Time Tracker Stopped - Worked for 3:15:00. Logged to CSV file"

### Customizing Notifications
The notification interval is set to 2 hours by default. To modify:

1. Edit `time_tracker.py`
2. Change `NOTIFICATION_INTERVAL = 2 * 60 * 60` (in seconds)
3. Restart any active sessions

## Error Handling Examples

### Common Issues and Solutions

**Already running session:**
```bash
./time_tracker.py start "New task"
# Output: Time tracking already running for 1:30:00
#         Description: Previous task

# Solution: Stop current session first
./time_tracker.py stop
./time_tracker.py start "New task"
```

**No active session:**
```bash
./time_tracker.py stop
# Output: Time tracking is not currently running.

# Solution: Start a session first
./time_tracker.py start "Some work"
```

**Missing notifications:**
```bash
# If notifications don't appear, check:
which notify-send

# If missing, install:
sudo apt-get install libnotify-bin

# Test notifications:
notify-send "Test" "This is a test notification"
```

## Advanced Usage

### Background Operation
The tool works in the background - you can:
- Close the terminal after starting
- Restart your computer (session persists)  
- Log out and back in (notifications continue)

### Multiple Sessions Protection
The tool prevents accidentally starting multiple sessions:
```bash
./time_tracker.py start "Task 1"
./time_tracker.py start "Task 2"  # This will show current session info instead
```

### File Locations
Understanding where data is stored:
```bash
# Configuration directory
ls -la ~/.time_tracker/

# Current session (if active)
cat ~/.time_tracker/current_session.json

# Historical data
cat ~/.time_tracker/time_logs.csv

# Daemon process ID (if notifications running)
cat ~/.time_tracker/daemon.pid
```

## Integration Examples

### Shell Aliases
Add to your `~/.bashrc` or `~/.zshrc`:
```bash
alias tt='./time_tracker.py'
alias tts='./time_tracker.py start'
alias tto='./time_tracker.py stop'  
alias ttstat='./time_tracker.py status'
alias ttr='./time_tracker.py report'
```

Usage:
```bash
tts "New feature implementation"
ttstat
tto
ttr
```

### Cron Job for Automated Reports
Add to crontab for daily email reports:
```bash
# Edit crontab
crontab -e

# Add line for daily 6 PM report
0 18 * * * /path/to/time_tracker.py report | mail -s "Daily Time Report" your-email@company.com
```

### Git Integration
Track time per commit/branch:
```bash
# Before starting work on a feature
git checkout -b feature/user-auth
./time_tracker.py start "Feature: User authentication system"

# When committing
git commit -m "Implement login endpoint"
./time_tracker.py stop

# Track time across branches
git checkout feature/notifications  
./time_tracker.py start "Feature: Push notification system"
```

These examples should cover most use cases and help users get the most out of the time tracking tool.
