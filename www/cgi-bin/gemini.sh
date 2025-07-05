#!/bin/bash

# Read JSON input
read input

# Extract "text" using jq
text=$(echo "$input" | jq -r '.text')

# Set API key
API_KEY="AIzaSyDmKXFwGSIuHgtkkFmm40vOqrVkVqnsYYg"

# Send request and output only the response
echo "Content-Type: text/plain"
echo ""

curl -s "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash:generateContent" \
  -H 'Content-Type: application/json' \
  -H "X-goog-api-key: $API_KEY" \
  -X POST \
  -d "{
    \"contents\": [
      {
        \"parts\": [
          {
            \"text\": \"$text\"
          }
        ]
      }
    ]
  }" | jq -r '.candidates[0].content.parts[0].text'
