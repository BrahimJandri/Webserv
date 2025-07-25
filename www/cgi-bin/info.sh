#!/bin/bash
echo "Content-Type: text/html"
echo ""
echo "<html><body>"
echo "<h1>Shell Script CGI Info</h1>"
echo "<p><b>Request Method:</b> $REQUEST_METHOD</p>"
echo "<hr><h2>Environment Variables:</h2><pre>"
printenv
echo "</pre></body></html>"