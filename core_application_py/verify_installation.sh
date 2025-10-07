#!/bin/bash
# Time Tracker Installation Verification

echo "=== Time Tracker Installation Verification ==="
echo

# Check Python version
echo "🐍 Python Version:"
python --version
echo

# Check if main script exists and is executable
echo "📄 Main Script:"
if [ -f "time_tracker.py" ]; then
    echo "✓ time_tracker.py found"
    if [ -x "time_tracker.py" ]; then
        echo "✓ time_tracker.py is executable"
    else
        echo "⚠ time_tracker.py is not executable (run chmod +x time_tracker.py)"
    fi
else
    echo "❌ time_tracker.py not found"
fi
echo

# Check for notify-send
echo "🔔 Notification System:"

OS_NAME=$(uname -s)

if [[ "$OS_NAME" == "Linux" ]]; then
    if command -v notify-send >/dev/null 2>&1; then
        echo "✓ notify-send is available"
        notify-send "Time Tracker" "Verification test notification" 2>/dev/null && echo "✓ Test notification sent"
    else
        echo "⚠ notify-send not found (install with: sudo apt-get install libnotify-bin)"
    fi
elif [[ "$OS_NAME" == "MINGW"* || "$OS_NAME" == "CYGWIN"* || "$OS_NAME" == "MSYS"* ]]; then
    echo "✓ Windows detected - notify-send test skipped"
    echo "Run the Python script to test Windows notifications via win10toast."
else
    echo "⚠ Unsupported OS for notification test"
fi
echo

#echo "🔔 Notification System:"
#if command -v notify-send >/dev/null 2>&1; then
#    echo "✓ notify-send is available"
#    # Test notification
#    notify-send "Time Tracker" "Verification test notification" 2>/dev/null && echo "✓ Test notification sent"
#else
#    echo "⚠ notify-send not found (install with: sudo apt-get install libnotify-bin)"
#fi
#echo

# Check C++ compiler if C++ version exists
echo "⚙️ C++ Build Environment:"
if [ -f "../core_application_cpp/time_tracker.cpp" ]; then
    echo "✓ time_tracker.cpp found"
    if command -v g++ >/dev/null 2>&1; then
        echo "✓ g++ compiler available"
        g++ --version | head -1
    else
        echo "⚠ g++ not found (install with: sudo apt-get install build-essential)"
    fi
else
    echo "ℹ C++ version not present"
fi
echo

# Test basic functionality
echo "🧪 Basic Functionality Test:"
if [ -x "time_tracker.py" ]; then
    echo "Testing help command..."
    python time_tracker.py --help >/dev/null 2>&1 && echo "✓ Help command works" || echo "❌ Help command failed"
    #./time_tracker.py --help >/dev/null 2>&1 && echo "✓ Help command works" || echo "❌ Help command failed"

    # Quick start/stop test
    echo "Testing quick start/stop cycle..."
    python time_tracker.py start "Verification test" >/dev/null 2>&1
    sleep 1
    python time_tracker.py status >/dev/null 2>&1 && echo "✓ Status command works" || echo "❌ Status command failed"
    python time_tracker.py stop >/dev/null 2>&1 && echo "✓ Start/stop cycle works" || echo "❌ Start/stop cycle failed"
else
    echo "⚠ Cannot test - time_tracker.py not executable"
fi
echo

echo "=== Verification Complete ==="
echo
echo "If all items show ✓, your installation is ready!"
echo "If you see ⚠ or ❌, please address the issues shown above."
echo
echo "Quick start:"
echo "  ./time_tracker.py start 'My first task'"
echo "  ./time_tracker.py stop"
echo "  ./time_tracker.py report"
