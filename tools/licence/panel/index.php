<?php

declare(strict_types=1);

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

    body { margin:0; min-height:100vh; display:grid; place-items:center;
           background:
           font:15px/1.5 ui-sans-serif,system-ui,-apple-system,"Segoe UI",sans-serif; }
    main { border:3px solid
           padding:28px 32px; max-width:420px; }
    h1 { margin:0 0 10px; font-size:22px; letter-spacing:-.02em; text-transform:uppercase; font-weight:800; }
    p { margin:0 0 18px; color:
    a { display:inline-block; padding:12px 18px; background:
        border:3px solid
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

