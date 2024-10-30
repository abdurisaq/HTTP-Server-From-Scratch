#!/usr/bin/env bash

# Directories
    
RELEVANT_PATH=""
WATCHED_DIR="/home/duri/HTTP-Server-From-Scratch"
WATCHED_DIR_LENGTH=${#WATCHED_DIR}

WINDOWS_DIR="/mnt/c/Users/abdur/Documents/HTTP-Server-From-Scratch"

LOG_FILE="/mnt/c/Users/abdur/Documents/log.txt"
TEMP_PATH=""


# Function to sync directories

process_event(){
    local event_type="$1"
    local file_path1="$2"

    echo "-${event_type}-"
    case "$event_type" in

        *"ISDIR"*)
            mkdir "${WINDOWS_DIR}${file_path1}"
            ;;
        "CREATE")
            echo "creating file"
            touch "${WINDOWS_DIR}${file_path1}"
            ;;
        "DELETE")
            rm -r "${WINDOWS_DIR}${file_path1}"
            ;;
        "MODIFY")
            cp -r "${WATCHED_DIR}${file_path1}" "${WINDOWS_DIR}${file_path1}"
            ;;

        "MOVED_FROM")
            TEMP_PATH="${file_path1}"
            ;;
        "MOVED_TO")
            mv "${WINDOWS_DIR}${TEMP_PATH}" "${WINDOWS_DIR}${file_path1}"
            ;;
        *)
            ;;
    esac
}

# [command -v rsync] && [command -v inotifywait] && [echo "confirmed rsync and inotifywait is downloaded"] || [echo "ERROR: dependencies aren't downloaded" && exit 1]
which inotifywait && echo "already installed inotifywait" || (echo "installing inotifiywait..." && sudo apt update && sudo apt install inotify-tools)

rsync -a "$WATCHED_DIR/" "$WINDOWS_DIR/"

inotifywait -m -r -e modify,create,delete,move,moved_to,moved_from "$WATCHED_DIR"|
    while read path action file; do
    echo "$(date): $action on $path$file" >> /mnt/c/Users/abdur/Desktop/log.txt
    FULLPATH="${path}${file}"
    RELEVANTPATH=${FULLPATH:${WATCHED_DIR_LENGTH}}
    echo "$RELEVANTPATH"
    process_event ${action} ${RELEVANTPATH}
done

