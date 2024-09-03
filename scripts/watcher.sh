#!/bin/bash
RELEVANT_PATH=""
WATCHED_DIR="/home/duri/HTTP-Server-From-Scratch"
WATCHED_DIR_LENGTH=${#WATCHED_DIR}

inotifywait -m -r -e modify,create,delete,move /home/duri/HTTP-Server-From-Scratch |
    while read path action file; do
    echo "$(date): $action on $path$file" >> /mnt/c/Users/abdur/Desktop/log.txt
    FULLPATH="${path}${file}"
    RELEVANTPATH=${FULLPATH:${WATCHED_DIR_LENGTH}}
    echo "$RELEVANTPATH"
done
