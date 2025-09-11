#!/usr/bin/php-cgi
<?php

// Error handling: custom error handler to display user-friendly message
set_error_handler(function($errno, $errstr) {
    if (!headers_sent()) {
        header("Status: 500 Internal Server Error");
        header("Content-Type: text/html");
    }
    echo "<h2>Internal Server Error</h2><p>An error occurred: " . htmlspecialchars($errstr) . "</p>";
    exit;
});


// Determine POST length from environment
$content_length = getenv('CONTENT_LENGTH') ?: 0;

// Validate content length with reasonable limits
if (!is_numeric($content_length) || $content_length < 0) {
    header("Status: 400 Bad Request\r\n");
    header("Content-Type: text/html\r\n");
    $return_body = "<h2>Bad Request</h2><p>Invalid content length.</p>";
    header("Content-Length: " . strlen($return_body) . "\r\n\r\n");
    echo $return_body;
    exit;
}

// Read the raw POST body from stdin
$post_data = '';
if ($content_length > 0) {
    $post_data = file_get_contents('php://input');
}

// Parse application/x-www-form-urlencoded POST data
parse_str($post_data, $fields);

// Enhanced field validation
if (empty($fields['name']) || empty($fields['email']) || empty($fields['message'])) {
    header("Status: 400 Bad Request");
    echo "<h2>Bad Request</h2><p>All fields are required.</p>";
    exit;
}

// Add email validation and length checks
if (!filter_var($fields['email'], FILTER_VALIDATE_EMAIL)) {
    header("Status: 400 Bad Request");
    echo "<h2>Bad Request</h2><p>Invalid email format.</p>";
    exit;
}

if (strlen($fields['name']) > 100 || strlen($fields['message']) > 5000) {
    header("Status: 400 Bad Request");
    echo "<h2>Bad Request</h2><p>Input too long.</p>";
    exit;
}

// Safely extract input fields
$name    = isset($fields['name'])    ? htmlspecialchars($fields['name'])    : '';
$email   = isset($fields['email'])   ? htmlspecialchars($fields['email'])   : '';
$message = isset($fields['message']) ? nl2br(htmlspecialchars($fields['message'])) : '';

// Render the reply page
$html = "<!DOCTYPE html>
<html>
<head><title>Message Received</title></head>
<body>
    <h2>Your Message Details</h2>
    <p><strong>Name:</strong> $name</p>
    <p><strong>Email:</strong> $email</p>
    <p><strong>Message:</strong><br />$message</p>
</body>
</html>";


header("Status: 200 OK\r\n");
header("Content-Type: text/html; charset=UTF-8\r\n");
header("Content-Length: " . strlen($html) . "\r\n\r\n");

// Output the HTML
echo $html;