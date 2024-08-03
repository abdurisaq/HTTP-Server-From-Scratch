#!/bin/bash

# Set the base IP address of your local network
baseIP="192.168.1"

# Output file
outputFile="ipaddress.txt"

# Clear the output file if it exists
> "$outputFile"

# Function to ping an IP address
ping_ip() {
    local ip=$1
    # Ping each IP address with a timeout of 1 second and check if it's up
    if ping -c 1 -W 1 "$ip" > /dev/null; then
        echo "$ip" >> "$outputFile"
    # else
    #     echo "$ip is down" >> /dev/null
    fi
}

# Loop through all possible IP addresses from 0 to 255
for i in $(seq 0 255); do
    ip="$baseIP.$i"
    ping_ip "$ip" &
    # Limit the number of parallel pings
    if [[ $(jobs -r -p | wc -l) -ge 20 ]]; then
        wait -n
    fi
done

# Wait for all background processes to finish
wait

echo $outputFile
