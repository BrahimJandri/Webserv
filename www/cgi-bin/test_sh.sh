#!/bin/bash

# Standard CGI output header
echo "Content-Type: text/plain"
echo "" # Empty line to end headers

echo "Hello from Shell CGI!"
echo "Request Method: $REQUEST_METHOD"
echo "Script Name: $SCRIPT_NAME"
echo "Query String: $QUERY_STRING"
echo "Content-Length: $CONTENT_LENGTH"
echo "Content-Type: $CONTENT_TYPE"
echo "HTTP_USER_AGENT: $HTTP_USER_AGENT"

# Check if it's a POST request and read stdin
if [ "$REQUEST_METHOD" = "POST" ]; then
    echo ""
    echo "--- Received POST Data ---"
    cat # Reads all of stdin
    echo "--------------------------"
fi
