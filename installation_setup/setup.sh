#!/bin/bash
# Time Tracker Setup Script
# This script sets up the time tracking tool on Ubuntu

echo "Setting up Time Tracker..."

# Make the main script executable
chmod +x ../core_application_py/time_tracker.py

# Install notify-send if not present (part of libnotify-bin package)
if ! command -v notify-send &> /dev/null; then
    echo "Installing libnotify-bin for desktop notifications..."
    sudo apt-get update
    sudo apt-get install -y libnotify-bin
fi

# Create desktop entry for easy access
DESKTOP_FILE="$HOME/.local/share/applications/time-tracker.desktop"
mkdir -p "$HOME/.local/share/applications"

cat > "$DESKTOP_FILE" << EOF
[Desktop Entry]
Version=1.0
Name=Time Tracker
Comment=Track your work hours with notifications
Exec=$(pwd)/time_tracker.py
Icon=time-admin
Terminal=true
Type=Application
Categories=Office;Productivity;
EOF

# Create a convenient wrapper script
WRAPPER_SCRIPT="$HOME/.local/bin/timetrack"
mkdir -p "$HOME/.local/bin"

cat > "$WRAPPER_SCRIPT" << 'EOF'
#!/bin/bash
# Time Tracker Wrapper Script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
python3 "$SCRIPT_DIR/../../time_tracker.py" "$@"
EOF

chmod +x "$WRAPPER_SCRIPT"

# Add ~/.local/bin to PATH if not already there
if [[ ":$PATH:" != *":$HOME/.local/bin:"* ]]; then
    echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
    echo "Added ~/.local/bin to PATH. Please restart your terminal or run: source ~/.bashrc"
fi

echo "✓ Setup complete!"
echo ""
echo "Usage examples:"
echo "  ./time_tracker.py start Working on project documentation"
echo "  ./time_tracker.py status"
echo "  ./time_tracker.py stop"
echo "  ./time_tracker.py report"
echo ""
echo "Or use the wrapper command:"
echo "  timetrack start Working on project documentation"
echo "  timetrack stop"
