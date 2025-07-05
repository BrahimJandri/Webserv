#!/bin/bash
read payload
echo "Content-Type: application/json"
echo ""
cipher=$(echo "$payload" | jq -r .cipher)
alg=$(echo "$payload" | jq -r .alg)
encoded=$(echo -n "${cipher}" | base64 --wrap=0)
echo "{\"encoded\":\"$encoded\"}"
