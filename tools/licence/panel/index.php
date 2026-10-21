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
<style>
    /* Same paper-and-ink shell as admin.php, so the one page a visitor can
       land on before the redirect does not look like a different product. */
    body { margin:0; min-height:100vh; display:grid; place-items:center;
           background:#f4f1e8; color:#101010;
           font:15px/1.5 ui-sans-serif,system-ui,-apple-system,"Segoe UI",sans-serif; }
    main { border:3px solid #101010; background:#fff; box-shadow:6px 6px 0 #101010;
           padding:28px 32px; max-width:420px; }
    h1 { margin:0 0 10px; font-size:22px; letter-spacing:-.02em; text-transform:uppercase; font-weight:800; }
    p { margin:0 0 18px; color:#5b574e; }
    a { display:inline-block; padding:12px 18px; background:#8b5cf6; color:#fff;
        border:3px solid #101010; box-shadow:4px 4px 0 #101010;
        font-weight:800; font-size:12px; letter-spacing:.1em; text-transform:uppercase;
        text-decoration:none; }
</style>
</head>
<body>
<main>
    <h1>Licence panel</h1>
    <p>Redirecting to the panel&hellip;</p>
    <a href="admin.php">Open the panel</a>
</main>
</body>
</html>
