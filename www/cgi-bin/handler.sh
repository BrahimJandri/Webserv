#!/bin/bash
echo "Content-Type: text/html"
echo ""
echo "<html><body><h1>Shell POST Handler</h1>"

if [ "$REQUEST_METHOD" = "POST" ]; then
    # Read the full POST body based on Content-Length
    read -n "$CONTENT_LENGTH" POST_STRING

    # Extract the 'name' value from the POST data
    NAME=$(echo "$POST_STRING" | sed -n 's/.*name=\([^&]*\).*/\1/p' | sed 's/+/ /g' | sed 's/%/\\x/g' | xargs -0 printf "%b")

    if [ -n "$NAME" ]; then
        echo "<p>Hello, <strong>$NAME</strong>!</p>"
    else
        echo "<p>No name was submitted.</p>"
    fi
else
    echo "<p>Invalid request method.</p>"
fi

echo "<a href='/'>Go back</a>"
echo "</body></html>"
