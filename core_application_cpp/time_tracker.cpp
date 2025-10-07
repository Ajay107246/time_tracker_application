/*
 * Time Tracker - C++ Implementation
 * A command-line time tracking tool with notifications and CSV logging
 *
 * Compile with:
 * g++ -std=c++17 -o time_tracker_cpp time_tracker.cpp
 * g++ -std=c++17 -Wall -Wextra -O2 -o time_tracker.exe time_tracker.cpp
 * 
 * Dependencies (linux): libnotify-dev (sudo apt-get install libnotify-dev)
 * Dependencies (windows): None (uses MessageBox)
 * make: provided for building with g++
 * g++ -std=c++17 -Wall -Wextra -O2 -pthread -c time_tracker.cpp -o time_tracker.o
 * g++ -std=c++17 -Wall -Wextra -O2 -pthread -o time_tracker.exe time_tracker.o
 * ./time_tracker.exe start "test8: time tracker"
 * ./time_tracker.exe status
 * ./time_tracker.exe report
 * ./time_tracker.exe stop
 * ./time_tracker.exe report
 * ./time_tracker.exe report 2025-10-06
 */


// without periodic notifications
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>
#include <filesystem>
#include <ctime>
#include <sstream>
#include <vector>
#include <iomanip>
#include <sys/stat.h>
#include <csignal>

#ifdef _WIN32
#include <windows.h>
#else
#include <signal.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

class TimeTracker {
private:
    fs::path config_dir;
    fs::path state_file;
    fs::path csv_file;
    fs::path daemon_pid_file;

    const int NOTIFICATION_INTERVAL = 3 * 60; // 3 minutes in seconds

public:
    TimeTracker() {
        const char* home = getenv("HOME");
        if (!home) {
#ifdef _WIN32
            home = getenv("USERPROFILE");
#endif
            if (!home) {
                throw std::runtime_error("Could not get HOME directory");
            }
        }
        
        config_dir = fs::path(home) / ".time_tracker";
        state_file = config_dir / "current_session.json";
        csv_file = config_dir / "time_logs.csv";
        daemon_pid_file = config_dir / "daemon.pid";
        
        setup_directories();
    }
    
    void setup_directories() {
        fs::create_directories(config_dir);
        
        // Initialize CSV file with headers if it doesn't exist
        if (!fs::exists(csv_file)) {
            std::ofstream file(csv_file);
            file << "name,date,start_time,end_time,duration_hours,description\n";
            file.close();
        }
    }
    
    void send_notification(const std::string& title, const std::string& message) {
#ifdef _WIN32
        // Windows notification using MessageBox
        std::string escaped_title = title;
        std::string escaped_message = message;
        
        // Replace newlines with spaces for Windows display
        for (size_t pos = 0; pos < escaped_message.length(); pos++) {
            if (escaped_message[pos] == '\n') {
                escaped_message[pos] = ' ';
            }
        }
        
        // Simple Windows message box
        std::string command = "powershell -WindowStyle Hidden -Command \""
                             "Add-Type -AssemblyName System.Windows.Forms; "
                             "[System.Windows.Forms.MessageBox]::Show('" + escaped_message + "', '" + escaped_title + "', 'OK', 'Information')\"";
        
        int result = system(command.c_str());
        if (result != 0) {
            std::cout << "NOTIFICATION: " << title << " - " << message << std::endl;
        }
#else
        // Linux notification
        std::string command = "notify-send -i time-admin -u normal -t 5000 \"" +
                              title + "\" \"" + message + "\"";
        
        int result = system(command.c_str());
        if (result != 0) {
            std::cout << "NOTIFICATION: " << title << " - " << message << std::endl;
        }
#endif
    }
    
    bool is_running() {
        return fs::exists(state_file);
    }
    
    std::string get_current_time_iso() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%dT%H:%M:%S");
        return ss.str();
    }
    
    std::string get_current_date() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d");
        return ss.str();
    }
    
    std::string get_current_time() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
        return ss.str();
    }
    
    std::string get_username() {
        const char* user = getenv("USER");
#ifdef _WIN32
        if (!user) {
            user = getenv("USERNAME");
        }
#endif
        return user ? std::string(user) : "unknown";
    }
    
    bool start_tracking(const std::string& description = "Work session") {
        if (is_running()) {
            std::cout << "Time tracking is already running.\n";
            return false;
        }
        
        // Create session state file (simplified JSON-like format)
        std::ofstream state(state_file);
        state << "{\n";
        state << "  \"name\": \"" << get_username() << "\",\n";
        state << "  \"start_time\": \"" << get_current_time_iso() << "\",\n";
        state << "  \"description\": \"" << description << "\"\n";
        state << "}\n";
        state.close();
        
        // Start daemon (simplified - would need proper daemon implementation)
        start_daemon();
        
        send_notification("Time Tracker Started", "Started tracking: " + description);
        
        std::cout << "Time tracking started at " << get_current_time() << std::endl;
        std::cout << "Description: " << description << std::endl;
        
        return true;
    }
    
    bool stop_tracking() {
        if (!is_running()) {
            std::cout << "Time tracking is not currently running.\n";
            return false;
        }
        
        // Read session data (simplified parsing)
        std::ifstream state(state_file);
        std::string line, start_time, description, name;
        
        while (std::getline(state, line)) {
            if (line.find("\"start_time\"") != std::string::npos) {
                size_t start = line.find(": \"") + 3;
                size_t end = line.find("\"", start);
                start_time = line.substr(start, end - start);
            } else if (line.find("\"description\"") != std::string::npos) {
                size_t start = line.find(": \"") + 3;
                size_t end = line.find("\"", start);
                description = line.substr(start, end - start);
            } else if (line.find("\"name\"") != std::string::npos) {
                size_t start = line.find(": \"") + 3;
                size_t end = line.find("\"", start);
                name = line.substr(start, end - start);
            }
        }
        state.close();
        
        // Calculate duration (simplified - would need proper ISO parsing)
        std::string current_time = get_current_time_iso();
        std::string current_time_only = get_current_time();
        std::string current_date = get_current_date();
        
        // For demo purposes, assume 1 hour duration
        double duration_hours = 1.0; // Needs full implementation
        
        // Log to CSV
        std::ofstream csv(csv_file, std::ios::app);
        csv << name << ","
            << current_date << ","
            << start_time.substr(11) << "," // Extract time part
            << current_time_only << ","
            << std::fixed << std::setprecision(2) << duration_hours << ","
            << description << "\n";
        csv.close();
        
        // Remove state file
        fs::remove(state_file);
        
        // Stop daemon
        stop_daemon();
        
        send_notification("Time Tracker Stopped", 
                         "Session completed\nLogged to CSV file");
        
        std::cout << "Time tracking stopped at " << current_time_only << std::endl;
        std::cout << "Duration: " << duration_hours << " hours\n";
        std::cout << "Logged to: " << csv_file << std::endl;
        
        return true;
    }
    
    void get_status() {
        if (!is_running()) {
            std::cout << "Time tracking is not currently running.\n";
            return;
        }
        
        std::ifstream state(state_file);
        std::string line;
        std::cout << "Time tracking is ACTIVE\n";
        while (std::getline(state, line)) {
            std::cout << line << std::endl;
        }
        state.close();
    }
    
    void generate_daily_report(const std::string& date = "") {
        std::string target_date = date.empty() ? get_current_date() : date;
        
        std::ifstream csv(csv_file);
        std::string line;
        std::vector<std::string> daily_entries;
        double total_hours = 0.0;
        
        // Skip header
        if (std::getline(csv, line)) {
            while (std::getline(csv, line)) {
                if (line.find(target_date) != std::string::npos) {
                    daily_entries.push_back(line);
                    
                    // Extract duration (simplified parsing)
                    std::istringstream iss(line);
                    std::string field;
                    int field_count = 0;
                    while (std::getline(iss, field, ',')) {
                        if (field_count == 4) { // duration_hours field
                            total_hours += std::stod(field);
                            break;
                        }
                        field_count++;
                    }
                }
            }
        }
        csv.close();
        
        if (daily_entries.empty()) {
            std::cout << "No entries found for " << target_date << std::endl;
            return;
        }
        
        std::cout << "\n=== Daily Report for " << target_date << " ===\n";
        std::cout << "Total Hours: " << std::fixed << std::setprecision(2) << total_hours << "\n";
        std::cout << "Total Entries: " << daily_entries.size() << "\n";
        std::cout << "\nDetails:\n";
        std::cout << std::string(70, '-') << "\n";
        
        for (const auto& entry : daily_entries) {
            std::cout << entry << "\n";
        }
        
        std::cout << std::string(70, '-') << "\n";
        std::cout << "Total: " << total_hours << " hours\n";
    }
        
    // Holds the active session’s metadata
    struct SessionData {
        std::string name;        // Username who started the session
        std::string start_time;  // ISO-8601 timestamp when tracking began
        std::string description; // User’s task description
        bool valid = false;      // True if both start_time and description were parsed
    };

    // Reads and parses the JSON-like state file.
    // Returns a SessionData with .valid=true if parsing succeeded.
    SessionData read_session_data(const fs::path& state_file) {
        SessionData data;
        if (!fs::exists(state_file)) return data;

        std::ifstream f(state_file);
        std::string line;
        while (std::getline(f, line)) {
            auto p = line.find("\"name\"");
            if (p != std::string::npos) {
                auto s = line.find(':', p) + 2;
                data.name = line.substr(s, line.find('"', s) - s);
            }
            p = line.find("\"start_time\"");
            if (p != std::string::npos) {
                auto s = line.find(':', p) + 2;
                data.start_time = line.substr(s, line.find('"', s) - s);
            }
            p = line.find("\"description\"");
            if (p != std::string::npos) {
                auto s = line.find(':', p) + 2;
                data.description = line.substr(s, line.find('"', s) - s);
            }
        }
        data.valid = !data.start_time.empty() && !data.description.empty();
        return data;
    }

    void notification_loop(const fs::path& state_file) {
        auto last = std::chrono::steady_clock::now();
        while (fs::exists(state_file)) {
            std::this_thread::sleep_for(std::chrono::seconds(30));
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - last).count() >= NOTIFICATION_INTERVAL)
            {
                auto session = read_session_data(state_file);
                if (session.valid) {
                    std::string msg = "You've been working for "
                        + std::to_string(NOTIFICATION_INTERVAL / 60)
                        + " minutes. Current task: " + session.description;
                    send_notification("Time Tracker Reminder", msg);
                }
                last = now;
            }
        }
    }
    
private:
    void start_daemon() {
        // Simplified: Here you'd fork a proper daemon process in production
        // For example, spawning a separate process to handle notifications
        // Spawn background thread for periodic reminders
        std::thread([this]() {
            notification_loop(state_file);
        }).detach();
        std::cout << "Notification daemon started (simplified placeholder).\n";
    }
    
    void stop_daemon() {
        if (fs::exists(daemon_pid_file)) {
            std::ifstream pid_file(daemon_pid_file);
            std::string pid_str;
            if (std::getline(pid_file, pid_str)) {
                int pid = std::stoi(pid_str);
#ifdef _WIN32
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
                if (hProcess != NULL) {
                    TerminateProcess(hProcess, 1); // exit code 1
                    CloseHandle(hProcess);
                }
#else
                kill(pid, SIGTERM);
#endif
            }
            pid_file.close();
            fs::remove(daemon_pid_file);
        }
    }
};

void print_usage(const char* program_name) {
    std::cout << "Time Reporting Tool - C++ Version\n\n";
    std::cout << "Usage:\n";
    std::cout << "  " << program_name << " start [description]  - Start time tracking\n";
    std::cout << "  " << program_name << " stop                 - Stop time tracking\n";
    std::cout << "  " << program_name << " status               - Check current status\n";
    std::cout << "  " << program_name << " report [date]        - Generate daily report\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << program_name << " start \"Coding new features\"\n";
    std::cout << "  " << program_name << " stop\n";
    std::cout << "  " << program_name << " report 2025-10-03\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    try {
        TimeTracker tracker;
        std::string command = argv[1];
        
        if (command == "start") {
            std::string description = "Work session";
            if (argc > 2) {
                description = "";
                for (int i = 2; i < argc; ++i) {
                    if (i > 2) description += " ";
                    description += argv[i];
                }
            }
            tracker.start_tracking(description);
            
        } else if (command == "stop") {
            tracker.stop_tracking();
            
        } else if (command == "status") {
            tracker.get_status();
            
        } else if (command == "report") {
            std::string date = (argc > 2) ? argv[2] : "";
            tracker.generate_daily_report(date);
            
        } else {
            std::cout << "Unknown command: " << command << "\n";
            print_usage(argv[0]);
            return 1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

#if 0
// with daemon, but not recording data after start
// status does not work

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>
#include <filesystem>
#include <ctime>
#include <sstream>
#include <vector>
#include <iomanip>
#include <csignal>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace fs = std::filesystem;

// Notification interval in seconds (3 minutes)
static constexpr int NOTIFICATION_INTERVAL = 3 * 60;

struct SessionData {
    std::string name;
    std::string start_time;
    std::string description;
    bool valid = false;
};

void send_notification(const std::string& title, const std::string& message) {
#ifdef _WIN32
    std::string escaped_title = title;
    std::string escaped_message = message;
    // Replace newlines and quotes
    for (auto& c : escaped_message) if (c=='\n') c=' ';
    for (auto& c : escaped_title) if (c=='"') c='\'';
    for (auto& c : escaped_message) if (c=='"') c='\'';
    std::string cmd = "powershell -WindowStyle Hidden -Command \""
        "Add-Type -AssemblyName System.Windows.Forms;"
        "[System.Windows.Forms.MessageBox]::Show('" + escaped_message + "','" + escaped_title + "','OK','Information')\"";
    if (system(cmd.c_str()) != 0) {
        std::cout << "NOTIFICATION: " << title << " - " << message << std::endl;
    }
#else
    std::string cmd = "notify-send -i time-admin -u normal -t 5000 \""
        + title + "\" \"" + message + "\"";
    if (system(cmd.c_str()) != 0) {
        std::cout << "NOTIFICATION: " << title << " - " << message << std::endl;
    }
#endif
}

SessionData read_session_data(const fs::path& state_file) {
    SessionData data;
    if (!fs::exists(state_file)) return data;
    std::ifstream f(state_file);
    std::string line;
    while (std::getline(f,line)) {
        auto p = line.find("\"name\"");
        if (p!=std::string::npos) {
            auto s = line.find(':',p)+2;
            data.name = line.substr(s, line.find('"',s)-s);
        }
        p = line.find("\"start_time\"");
        if (p!=std::string::npos) {
            auto s = line.find(':',p)+2;
            data.start_time = line.substr(s, line.find('"',s)-s);
        }
        p = line.find("\"description\"");
        if (p!=std::string::npos) {
            auto s = line.find(':',p)+2;
            data.description = line.substr(s, line.find('"',s)-s);
        }
    }
    data.valid = !data.start_time.empty() && !data.description.empty();
    return data;
}

void notification_loop(const fs::path& state_file) {
    auto last = std::chrono::steady_clock::now();
    while (fs::exists(state_file)) {
        std::this_thread::sleep_for(std::chrono::seconds(30));
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now-last).count() >= NOTIFICATION_INTERVAL) {
            auto session = read_session_data(state_file);
            if (session.valid) {
                std::string msg = "You've been working for "
                    + std::to_string(NOTIFICATION_INTERVAL/60)
                    + " minutes. Current task: " + session.description;
                send_notification("Time Tracker Reminder", msg);
            }
            last = now;
        }
    }
}

void print_usage(const char* prog) {
    std::cout<<"Usage:\n"
        << prog << " start [description]\n"
        << prog << " stop\n"
        << prog << " status\n"
        << prog << " report [YYYY-MM-DD]\n";
}

void print_status(const fs::path& state_file) {
    if (!fs::exists(state_file)) {
        std::cout << "Time tracking is not currently running.\n";
        return;
    }
    SessionData session = read_session_data(state_file);
    if (!session.valid) {
        std::cout << "Error reading session data.\n";
        return;
    }
    std::cout << "Time tracking is ACTIVE\n";
    std::cout << "Started: " << session.start_time << "\n";
    std::cout << "Description: " << session.description << "\n";
    std::cout << "User: " << session.name << "\n";
}

void print_report(const fs::path& csv_file, const std::string& date="") {
    std::ifstream f(csv_file);
    if (!f) {
        std::cout << "Could not open CSV file: " << csv_file << "\n";
        return;
    }
    std::string target;
    if (date.empty()) {
        std::ostringstream oss;
        std::time_t t = std::time(nullptr);
        oss << std::put_time(std::localtime(&t), "%Y-%m-%d");
        target = oss.str();
    } else {
        target = date;
    }
    std::string line;
    // Skip header
    if (!std::getline(f, line)) {
        std::cout << "CSV is empty.\n";
        return;
    }
    std::vector<std::string> entries;
    double total = 0.0;
    while (std::getline(f, line)) {
        if (line.find(target) != std::string::npos) {
            entries.push_back(line);
            // Parse duration (5th field)
            std::istringstream ss(line);
            std::string field;
            for (int i=0; i<=4; ++i) {
                std::getline(ss, field, ',');
            }
            total += std::stod(field);
        }
    }
    if (entries.empty()) {
        std::cout << "No entries for " << target << "\n";
        return;
    }
    std::cout << "=== Report for " << target << " ===\n"
              << "Total Entries: " << entries.size() << "\n"
              << "Total Hours: " << std::fixed << std::setprecision(2) << total << "\n\n"
              << "Details:\n";
    for (auto &e : entries) {
        std::cout << e << "\n";
    }
}

// Helper to get config directory
fs::path get_config_dir() {
    const char* home = getenv("HOME");
#ifdef _WIN32
    if (!home) home = getenv("USERPROFILE");
#endif
    if (!home) throw std::runtime_error("HOME/USERPROFILE not set");
    return fs::path(home) / ".time_tracker";
}

int main(int argc,char** argv) {
    if (argc<2) { print_usage(argv[0]); return 1; }
    fs::path config = get_config_dir();
    fs::path state = config/"current_session.json";
    fs::path pidf  = config/"daemon.pid";
    fs::path csv   = config/"time_logs.csv";
    std::string cmd=argv[1];
    if (cmd=="daemon") {
        notification_loop(state);
        return 0;
    }
    // other commands: start/stop/status/report
    // start: write state_file, write pid, spawn daemon
    if (cmd=="start") {
        fs::create_directories(config);
        std::ofstream s(state);
        s<<"{\n"
         <<" \"name\": \""<<getenv("USER")<<"\",\n"
         <<" \"start_time\": \"";
        {
            auto now = std::chrono::system_clock::now();
            auto t = std::chrono::system_clock::to_time_t(now);
            std::ostringstream oss;
            oss << std::put_time(std::localtime(&t), "%Y-%m-%dT%H:%M:%S");
            s << oss.str();
        }
        s << "\",\n"
         <<" \"description\": \""<<(argc>2?argv[2]:"Work session")<<"\"\n}";
        s.close();
        std::ofstream p(pidf); p<<getpid(); p.close();
#ifdef _WIN32
        std::string cmd = "start /B \"\" \"" + std::string(argv[0]) + "\" daemon";
        system(cmd.c_str());
#else
        if (fork()==0) { execlp(argv[0],argv[0],"daemon",nullptr); exit(1);} 
#endif
        send_notification("Time Tracker Started", "Started tracking");
        return 0;
    }
    if(cmd=="stop"){
        if (fs::exists(pidf)){
            int pid; std::ifstream p(pidf); p>>pid; p.close();
#ifdef _WIN32
            TerminateProcess(OpenProcess(PROCESS_TERMINATE,FALSE,pid),1);
#else
            kill(pid,SIGTERM);
#endif
            fs::remove(pidf);
        }
        if (fs::exists(state)) fs::remove(state);
        send_notification("Time Tracker Stopped","Session completed and logged");
        return 0;
    }
    if(cmd=="status"){
        //if(fs::exists(state)) std::cout<<"Tracking active\n";
        //else std::cout<<"Not running\n";
        print_status(state);
        return 0;
    }
    if(cmd=="report"){
        // report logic omitted for brevity
        std::string date = (argc>2 ? argv[2] : "");
        print_report(csv, date);
        return 0;
    }
    print_usage(argv[0]);
    return 1;
}
#endif