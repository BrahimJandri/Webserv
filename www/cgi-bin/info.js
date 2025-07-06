#!/usr/bin/env node
console.log('Content-Type: text/html\r\n\r\n');
console.log('<html><body>');
console.log('<h1>Node.js CGI Info</h1>');
console.log(`<p><b>Request Method:</b> ${process.env.REQUEST_METHOD}</p>`);
console.log('<hr><h2>Environment Variables:</h2><pre>');
for (const key in process.env) {
    console.log(`${key}: ${process.env[key]}`);
}
console.log('</pre></body></html>');