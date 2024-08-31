#!/bin/bash

# Directories
WATCHED_DIR="/path/to/watched/directory"
WINDOWS_DIR="/mnt/c/Users/YourUsername/WindowsDirectory"

# Log file
LOG_FILE="/path/to/log.txt"

# Temporary files for processing
TEMP_LOG_FILE="/path/to/temp_log.txt"

# Function to sync directories
sync_directories() {
    # Perform synchronization using rsync
    rsync -av --delete "$WATCHED_DIR/" "$WINDOWS_DIR/"
    
    # Log the synchronization
    echo "$(date), SYNC, Full synchronization from $WATCHED_DIR to $WINDOWS_DIR" >> "$LOG_FILE"
}

# Function to process log file
process_log() {
    local event_log="$1"
    while read -r log_entry; do
        local event_type=$(echo "$log_entry" | awk -F', ' '{print $2}')
        local file_path=$(echo "$log_entry" | awk -F', ' '{print $3}')
        
        case "$event_type" in
            "CREATE")
                echo "$(date), SYNC, File created: $file_path" >> "$LOG_FILE"
                ;;
            "DELETE")
                echo "$(date), SYNC, File deleted: $file_path" >> "$LOG_FILE"
                ;;
            "MODIFY")
                echo "$(date), SYNC, File modified: $file_path" >> "$LOG_FILE"
                ;;
            "MOVE")
                echo "$(date), SYNC, File moved: $file_path" >> "$LOG_FILE"
                ;;
            *)
                echo "$(date), SYNC, Unknown event type: $event_type" >> "$LOG_FILE"
                ;;
        esac
    done < "$event_log"
}

# Function to check if a command exists
check_command() {
    command -v "$1" >/dev/null 2>&1
}

# Check if rsync is installed
if ! check_command rsync; then
    echo "Error: rsync is not installed. Please install it using your package manager."
    exit 1
fi

# Check if inotifywait is installed
if ! check_command inotifywait; then
    echo "Error: inotifywait is not installed. Please install it using your package manager."
    exit 1
fi

# Initial full synchronization
if rsync -nrc "$WATCHED_DIR/" "$WINDOWS_DIR/" | grep -q '^'; then
    sync_directories
else
    echo "$(date), SYNC, No changes detected" >> "$LOG_FILE"
fi

# Monitor changes and process them
inotifywait -m -r -e create,delete,modify,move "$WATCHED_DIR" |
while read -r directory events filename; do
    # Log the change
    echo "$(date), $events, $directory$filename" >> "$LOG_FILE"
    
    # Call the function to handle the change
    process_log "$LOG_FILE"
done

