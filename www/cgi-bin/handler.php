#!/usr/bin/php-cgi
<?php
header("Content-Type: text/html");
echo "<html><body><h1>PHP POST Handler</h1>";

if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_POST['name'])) {
    $name = htmlspecialchars($_POST['name']);
    echo "<p>Hello, <strong>$name</strong>!</p>";
} else {
    echo "<p>No name was submitted.</p>";
}

echo "<a href='/'>Go back</a>";
echo "</body></html>";
?>