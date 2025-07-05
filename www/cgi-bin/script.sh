#!/bin/bash

# Simple script to show system info and greet the user

# Greet the user
echo "Hello, $USER!"

# Show current date and time
echo "Current date and time: $(date)"

# Show system uptime
echo "System uptime:"
uptime

# Show disk usage
echo "Disk usage:"
df -h

# End of script
echo "Done!"
