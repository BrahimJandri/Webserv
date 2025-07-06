#!/usr/bin/php-cgi
<?php
header("Content-Type: text/html");
echo "<html><body><h1>PHP CGI Info</h1>";
echo "<p><b>Request Method:</b> " . $_SERVER['REQUEST_METHOD'] . "</p>";
echo "<hr><h2>Environment & Server Variables:</h2>";
phpinfo(INFO_VARIABLES);
echo "</body></html>";
?>