#!/bin/bash
inotifywait -m -r -e modify,create,delete,move /home/duri/HTTP-Server-From-Scratch |
while read path action file; do
    echo "$(date): $action on $path$file" >> /mnt/c/Users/abdur/Desktop/log.txt
done
