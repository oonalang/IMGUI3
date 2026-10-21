<?php
/**
 * index.php - what a visitor gets at https://<your-host>/ .
 *
 * The panel's files sit directly in the document root (x10's free tier gives
 * you public_html and little else), and none of them is an index. Without
 * this file the host has nothing to serve for "/" and, with directory
 * listings switched off by panel/.htaccess, answers "403 Forbidden".
 *
 * That 403 is only about the directory index - every real page of the panel
 * works - but it looks exactly like a broken deploy, and it is the first
 * thing anyone (including the person who just uploaded the panel) sees.
 *
 * So: send the root to the panel login. Relative target, because the panel
 * also runs from a subfolder when the host forces one.
 */

declare(strict_types=1);

// 302, not 301: the panel is often re-uploaded, and a 301 would be cached
// by the browser long after the page it pointed at moved.
header('Location: admin.php', true, 302);
?>
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<meta http-equiv="refresh" content="0; url=admin.php">
<title>Licence panel</title>
</head>
<body style="font:15px/1.5 system-ui,-apple-system,'Segoe UI',sans-serif;padding:2rem">
<p>Redirecting to the <a href="admin.php">licence panel</a>&hellip;</p>
</body>
</html>
