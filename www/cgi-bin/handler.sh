#!/bin/bash
echo "Content-Type: text/html"
echo ""
echo "<html><body><h1>Shell POST Handler</h1>"

if [ "$REQUEST_METHOD" = "POST" ]; then
    # Read the POST data from stdin
    read -r POST_STRING
    # A simple way to parse URL-encoded form data
    NAME=$(echo "$POST_STRING" | sed 's/&/ /g' | sed 's/name=//g' | sed 's/+/ /g')
    echo "<p>Hello, <strong>$NAME</strong>!</p>"
else
    echo "<p>No name was submitted.</p>"
fi

echo "<a href='/'>Go back</a>"
echo "</body></html>"