<?php

declare(strict_types=1);

require __DIR__ . '/lib.php';

$config = licence_config();
$store  = licence_store_open();
$auth   = licence_auth();

session_set_cookie_params([
    'samesite' => 'Lax',
    'httponly' => true,
    'secure'   => !empty($_SERVER['HTTPS']),
]);
session_name('licencepanel');
session_start();

if (empty($_SESSION['csrf'])) {
    $_SESSION['csrf'] = bin2hex(random_bytes(16));
}
$csrf = $_SESSION['csrf'];

$error  = '';
$notice = '';
$action = $_POST['action'] ?? '';

if ($action !== '' && !hash_equals($csrf, (string)($_POST['csrf'] ?? ''))) {
    http_response_code(400);
    echo '<!doctype html><meta charset="utf-8"><p style="font:16px system-ui;padding:2rem">'
       . 'This form expired. Reload the page and try again.</p>';
    exit;
}

if ($action === 'logout') {
    $_SESSION = [];
    session_destroy();
    header('Location: ' . basename(__FILE__));
    exit;
}

if ($action === 'login') {
    $user = trim((string)($_POST['user'] ?? ''));
    $pass = (string)($_POST['pass'] ?? '');
    $slow = (int)($_SESSION['fails'] ?? 0) >= 3;
    if ($slow) {
        usleep(400000);
    }
    if (licence_login($user, $pass)) {
        $_SESSION['in'] = true;
        $_SESSION['fails'] = 0;
        session_regenerate_id(true);
        header('Location: ' . basename(__FILE__));
        exit;
    }
    $_SESSION['fails'] = (int)($_SESSION['fails'] ?? 0) + 1;
    $error = 'Wrong username or password';
}

$signedIn = !empty($_SESSION['in']);

$docRoot = isset($_SERVER['DOCUMENT_ROOT']) ? rtrim((string)$_SERVER['DOCUMENT_ROOT'], '/') : '';
$webRootNote = static function (string $path, string $guard) use ($docRoot): array {
    if ($docRoot === '') {
        return ['unknown', true];
    }
    return strpos($path, $docRoot) === 0
        ? ['inside the web root, guarded by ' . $guard, false]
        : ['outside the web root', true];
};

$hasOpenssl = extension_loaded('openssl');
$keyOk      = is_readable($config['private_key']);
$storeDir   = dirname($store);
$storeOk    = is_writable($storeDir);

$appModulus = strtoupper(preg_replace('/[^0-9a-f]/i', '', (string)($config['app_modulus'] ?? '')));
$keyModulus = '';
if ($hasOpenssl && $keyOk) {
    $pem = @file_get_contents($config['private_key']);
    $key = $pem !== false ? @openssl_pkey_get_private($pem) : false;
    if ($key !== false) {
        $details = openssl_pkey_get_details($key);
        if (is_array($details) && isset($details['rsa']['n'])) {
            $keyModulus = ltrim(strtoupper(bin2hex($details['rsa']['n'])), '0');
        }
    }
}
if ($appModulus === '') {
    $modulusNote = 'no app modulus in config.php';
    $modulusOk = true;
} elseif ($keyModulus === '') {
    $modulusNote = 'cannot read the private key';
    $modulusOk = false;
} elseif (ltrim($appModulus, '0') === $keyModulus) {
    $modulusNote = 'matches the app';
    $modulusOk = true;
} else {
    $modulusNote = 'does not match the app - every sign-in will be refused';
    $modulusOk = false;
}

$diag = [
    ['PHP', PHP_VERSION . ' (7.4 or newer)', version_compare(PHP_VERSION, '7.4.0', '>=')],
    ['openssl', $hasOpenssl ? 'loaded' : 'missing', $hasOpenssl],
    ['signing key', $keyOk ? 'readable' : 'not readable', $keyOk],
    ['key matches the app', $modulusNote, $modulusOk],
    ['key store', $storeOk ? 'writable' : 'not writable', $storeOk],
    ['payload build', (string)$config['build'], true],
    ['key file', ...$webRootNote($config['private_key'], 'private/.htaccess')],
    ['key store path', ...$webRootNote($store, 'data/.htaccess')],
];

function valid_device(string $hwid): bool
{
    return (bool)preg_match('/^[A-Za-z0-9-]{8,64}$/', $hwid);
}

function split_devices(string $raw): array
{
    $devices = [];
    $bad     = null;
    foreach (preg_split('/[\s,;]+/', trim($raw)) ?: [] as $d) {
        if ($d === '') {
            continue;
        }
        if (!valid_device($d)) {
            $bad = $d;
            break;
        }
        $devices[] = $d;
    }
    return [array_values(array_unique($devices)), $bad];
}

if ($signedIn && $action === 'create') {
    $name  = trim((string)($_POST['key'] ?? ''));
    $days  = max(0, (int)($_POST['days'] ?? 30));
    $note  = trim((string)($_POST['note'] ?? ''));
    $seats = max(0, (int)($_POST['seats'] ?? 1));
    [$devices, $badDevice] = split_devices((string)($_POST['hwid'] ?? ''));
    if ($name === '') {
        $name = 'ETHNIR-' . strtoupper(bin2hex(random_bytes(4))) . '-' . strtoupper(bin2hex(random_bytes(2)));
    }
    if (strpos($name, '|') !== false) {
        $error = "A key may not contain '|'";
    } elseif ($badDevice !== null) {
        $error = 'Not a device id: ' . $badDevice;
    } elseif ($seats !== 0 && count($devices) > $seats) {
        $error = 'You listed ' . count($devices) . ' devices but allowed only ' . $seats;
    } elseif (licence_find($store, $name) !== null) {
        $error = 'That key already exists';
    } else {
        licence_create($store, $name, $days, $note, $devices, $seats);
        $notice = 'Created ' . $name . ($seats === 0
            ? ' - unlimited devices'
            : ' - ' . $seats . ' device' . ($seats === 1 ? '' : 's'));
    }
}

if ($signedIn && $action === 'delete') {
    $key = (string)($_POST['key'] ?? '');
    $all = licence_all($store);
    if (isset($all[$key])) {
        unset($all[$key]);
        licence_save($store, $all);
        $notice = 'Deleted ' . $key;
    }
}

if ($signedIn && $action === 'revoke') {
    $key = (string)($_POST['key'] ?? '');
    $lic = licence_find($store, $key);
    if ($lic !== null) {
        $lic['revoked'] = empty($lic['revoked']) ? 1 : 0;
        licence_update($store, $key, $lic);
        $notice = ($lic['revoked'] ? 'Revoked ' : 'Re-enabled ') . $key;
    }
}

if ($signedIn && $action === 'bind') {
    $key = (string)($_POST['key'] ?? '');
    $lic = licence_find($store, $key);
    [$devices, $badDevice] = split_devices((string)($_POST['hwid'] ?? ''));
    if ($lic === null) {
        $error = 'That key is gone - reload the page';
    } elseif ($badDevice !== null) {
        $error = 'Not a device id: ' . $badDevice;
    } elseif (!$devices) {
        $error = 'Type a device id first';
    } else {
        $current = licence_devices($lic);
        $seats   = licence_seats($lic);
        $merged  = array_values(array_unique(array_merge($current, $devices)));
        if ($seats !== 0 && count($merged) > $seats) {
            $error = 'This key allows ' . $seats . ' device' . ($seats === 1 ? '' : 's') . ' - raise the limit first';
        } else {
            $lic = licence_apply_devices($lic, $merged);
            licence_update($store, $key, $lic);
            $added = array_values(array_diff($merged, $current));
            $notice = $added ? 'Added ' . implode(', ', $added) : 'Already allowed';
        }
    }
}

if ($signedIn && $action === 'dropdevice') {
    $key  = (string)($_POST['key'] ?? '');
    $hwid = trim((string)($_POST['hwid'] ?? ''));
    $lic  = licence_find($store, $key);
    if ($lic !== null && $hwid !== '') {
        $lic = licence_apply_devices($lic, array_diff(licence_devices($lic), [$hwid]));
        licence_update($store, $key, $lic);
        $notice = 'Removed ' . $hwid;
    }
}

if ($signedIn && $action === 'seats') {
    $key   = (string)($_POST['key'] ?? '');
    $seats = max(0, (int)($_POST['seats'] ?? 1));
    $lic   = licence_find($store, $key);
    if ($lic === null) {
        $error = 'That key is gone - reload the page';
    } elseif ($seats !== 0 && $seats < count(licence_devices($lic))) {
        $error = 'Remove a device first - this key already allows ' . count(licence_devices($lic));
    } else {
        $lic = licence_apply_devices($lic, licence_devices($lic), $seats);
        licence_update($store, $key, $lic);
        $notice = $key . ($seats === 0 ? ' now works on any device' : ' now allows ' . $seats);
    }
}

if ($signedIn && $action === 'rebind') {
    $key = (string)($_POST['key'] ?? '');
    $lic = licence_find($store, $key);
    if ($lic !== null) {
        $lic = licence_apply_devices($lic, []);
        licence_update($store, $key, $lic);
        $notice = 'Cleared the device list on ' . $key;
    }
}

$keys = $signedIn ? licence_all($store) : [];
ksort($keys);
$now = time();

function h(string $s): string
{
    return htmlspecialchars($s, ENT_QUOTES, 'UTF-8');
}
?>
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<meta name="theme-color" content="#efe9ff">
<title><?= $signedIn ? 'Licence keys' : 'Sign in' ?></title>
<style>
    :root {
        --bg: #efe9ff;
        --bg-2: #e3d6ff;
        --clay: #f6f1ff;
        --ink: #241a3d;
        --muted: #6e6390;
        --violet: #7c3aed;
        --violet-soft: #b79bff;
        --mint: #c8f7e4;
        --rose: #ffd6e7;
        --lilac: #e9dcff;
        --shadow:
            14px 14px 30px rgba(124, 58, 237, .16),
            -12px -12px 26px rgba(255, 255, 255, .95),
            inset 2px 2px 5px rgba(255, 255, 255, .9),
            inset -3px -3px 9px rgba(124, 58, 237, .10);
        --shadow-sm:
            8px 8px 18px rgba(124, 58, 237, .14),
            -7px -7px 16px rgba(255, 255, 255, .95);
        --press:
            inset 7px 7px 14px rgba(124, 58, 237, .18),
            inset -7px -7px 14px rgba(255, 255, 255, .92);
        color-scheme: light;
    }

    * { box-sizing: border-box }

    html, body { min-height: 100% }

    body {
        margin: 0;
        background:
            radial-gradient(1100px 620px at 12% -10%, #f8f2ff 0%, rgba(248, 242, 255, 0) 60%),
            radial-gradient(900px 640px at 108% 12%, #d9c6ff 0%, rgba(217, 198, 255, 0) 62%),
            linear-gradient(168deg, var(--bg) 0%, var(--bg-2) 100%);
        background-attachment: fixed;
        color: var(--ink);
        font: 15px/1.55 ui-sans-serif, system-ui, -apple-system, "Segoe UI", Roboto, sans-serif;
        -webkit-font-smoothing: antialiased;
        overflow-x: hidden;
    }

    ::selection { background: var(--violet); color: #fff }

    a { color: var(--violet) }

    .shell { max-width: 1080px; margin: 0 auto; padding: 34px 18px 80px }

    .clay {
        background: var(--clay);
        border-radius: 30px;
        box-shadow: var(--shadow);
    }

    .rise { animation: rise .62s cubic-bezier(.19, 1, .22, 1) both }
    .rise:nth-of-type(2) { animation-delay: .07s }
    .rise:nth-of-type(3) { animation-delay: .14s }
    .rise:nth-of-type(4) { animation-delay: .21s }

    @keyframes rise {
        from { opacity: 0; transform: translateY(18px) scale(.985) }
        to   { opacity: 1; transform: none }
    }

    @keyframes float {
        0%, 100% { transform: translate3d(0, 0, 0) }
        50%      { transform: translate3d(0, -16px, 0) }
    }

    @keyframes shake {
        10%, 90% { transform: translateX(-2px) }
        30%, 70% { transform: translateX(4px) }
        50%      { transform: translateX(-5px) }
    }

    @keyframes spin { to { transform: rotate(360deg) } }

    @keyframes shimmer {
        from { background-position: -180% 0 }
        to   { background-position: 180% 0 }
    }

    .login-shell {
        min-height: 100dvh;
        display: grid;
        place-items: center;
        padding: 24px 16px 40px;
        position: relative;
        isolation: isolate;
    }

    .orb {
        position: fixed;
        border-radius: 50%;
        filter: blur(2px);
        opacity: .55;
        z-index: -1;
        animation: float 9s ease-in-out infinite;
    }
    .orb.a { width: 260px; height: 260px; background: #cdb4ff; top: -70px; left: -60px }
    .orb.b { width: 320px; height: 320px; background: #e7d5ff; bottom: -120px; right: -90px; animation-delay: -3.5s }
    .orb.c { width: 150px; height: 150px; background: #f3d9ff; top: 42%; right: -60px; animation-delay: -6s }

    .login-card {
        width: 100%;
        max-width: 392px;
        padding: 34px 30px 28px;
        animation: rise .7s cubic-bezier(.19, 1, .22, 1) both;
    }
    .login-card.bad { animation: shake .5s ease-in-out }

    .badge {
        display: inline-flex;
        align-items: center;
        gap: 8px;
        padding: 8px 14px;
        border-radius: 999px;
        background: var(--lilac);
        box-shadow: var(--shadow-sm);
        color: var(--violet);
        font-size: 10px;
        font-weight: 800;
        letter-spacing: .18em;
        text-transform: uppercase;
    }
    .badge i {
        width: 7px; height: 7px; border-radius: 50%;
        background: var(--violet);
        box-shadow: 0 0 0 4px rgba(124, 58, 237, .18);
    }

    h1 {
        margin: 20px 0 6px;
        font-size: clamp(26px, 7vw, 32px);
        line-height: 1.08;
        letter-spacing: -.03em;
    }

    .sub { margin: 0 0 24px; color: var(--muted); font-size: 14px }

    label { display: block; margin: 0 0 16px }

    .label-text {
        display: block;
        margin: 0 0 8px 6px;
        font-size: 10px;
        font-weight: 800;
        letter-spacing: .16em;
        text-transform: uppercase;
        color: var(--muted);
    }

    input {
        width: 100%;
        padding: 15px 18px;
        font: inherit;
        color: var(--ink);
        background: #fbf8ff;
        border: 0;
        border-radius: 18px;
        box-shadow: var(--press);
        transition: box-shadow .2s ease, background .2s ease;
        -webkit-appearance: none;
    }
    input::placeholder { color: #a79bc4 }
    input:focus {
        outline: none;
        background: #fff;
        box-shadow: var(--press), 0 0 0 3px rgba(124, 58, 237, .28);
    }

    .btn {
        position: relative;
        display: inline-flex;
        align-items: center;
        justify-content: center;
        gap: 10px;
        width: 100%;
        min-height: 52px;
        padding: 15px 22px;
        font: 800 13px/1 ui-sans-serif, system-ui, sans-serif;
        letter-spacing: .12em;
        text-transform: uppercase;
        color: #fff;
        background: linear-gradient(145deg, #8b5cf6, #6d28d9);
        border: 0;
        border-radius: 18px;
        cursor: pointer;
        box-shadow:
            10px 10px 22px rgba(109, 40, 217, .28),
            -6px -6px 16px rgba(255, 255, 255, .7),
            inset 1px 1px 2px rgba(255, 255, 255, .45);
        transition: transform .16s ease, box-shadow .16s ease, filter .16s ease;
    }
    .btn:hover { transform: translateY(-2px); filter: saturate(1.05) }
    .btn:active { transform: translateY(2px); box-shadow: var(--press) }
    .btn:focus-visible { outline: 3px solid rgba(124, 58, 237, .45); outline-offset: 3px }

    .btn.ghost {
        width: auto;
        min-height: 44px;
        padding: 11px 18px;
        color: var(--violet);
        background: var(--clay);
        box-shadow: var(--shadow-sm);
    }
    .btn.ghost:active { box-shadow: var(--press) }

    .btn.danger { color: #b4235a; background: var(--rose) }
    .btn.danger:active { box-shadow: var(--press) }

    .btn.small {
        width: auto;
        min-height: 38px;
        padding: 9px 14px;
        font-size: 11px;
        border-radius: 14px;
        box-shadow: var(--shadow-sm);
    }
    .btn.small:active { box-shadow: var(--press) }

    .btn .spin {
        position: absolute;
        width: 18px; height: 18px;
        border-radius: 50%;
        border: 2.5px solid rgba(255, 255, 255, .4);
        border-top-color: #fff;
        opacity: 0;
        animation: spin .7s linear infinite;
    }
    .btn.busy { color: transparent; pointer-events: none }
    .btn.busy .spin { opacity: 1 }

    .flash {
        margin: 0 0 18px;
        padding: 14px 16px;
        border-radius: 16px;
        font-weight: 700;
        font-size: 13px;
        animation: rise .35s ease both;
    }
    .flash.bad  { background: var(--rose); color: #8e1d44; box-shadow: var(--press) }
    .flash.good { background: var(--mint); color: #0d5e44; box-shadow: var(--press) }

    .tiny { margin: 18px 0 0; text-align: center; color: var(--muted); font-size: 12px }

    .topbar {
        display: flex;
        align-items: center;
        justify-content: space-between;
        gap: 16px;
        flex-wrap: wrap;
        margin: 0 0 26px;
        padding: 18px 22px;
    }
    .brand { display: flex; align-items: center; gap: 14px }
    .brand .mark {
        display: grid;
        place-items: center;
        width: 46px; height: 46px;
        border-radius: 16px;
        background: linear-gradient(145deg, #a78bfa, #7c3aed);
        box-shadow: var(--shadow-sm);
        color: #fff;
        font-weight: 900;
        font-size: 15px;
        letter-spacing: .02em;
    }
    .brand h2 { margin: 0; font-size: 16px; letter-spacing: -.01em }
    .brand p { margin: 2px 0 0; color: var(--muted); font-size: 12px }

    section.card {
        padding: 26px 24px;
        margin: 0 0 22px;
    }

    .section-head {
        display: flex;
        align-items: baseline;
        justify-content: space-between;
        gap: 12px;
        margin: 0 0 20px;
    }
    .section-head h3 {
        margin: 0;
        font-size: 12px;
        letter-spacing: .18em;
        text-transform: uppercase;
        color: var(--violet);
    }
    .section-head .count { color: var(--muted); font-size: 12px; font-weight: 700 }

    .grid { display: flex; flex-wrap: wrap; gap: 14px }
    .grid > .field { flex: 1 1 200px; min-width: 0 }
    .grid > .field.grow { flex: 2 1 300px }
    .grid > .field.slim { flex: 0 1 132px }

    .trial-row { display: flex; flex-wrap: wrap; gap: 10px; margin-top: 18px }
    .trial-row .btn { width: auto; min-width: 132px }

    .hint { margin: 18px 0 0; color: var(--muted); font-size: 12.5px; line-height: 1.6 }
    .hint code {
        padding: 2px 7px;
        border-radius: 8px;
        background: var(--lilac);
        font-size: 12px;
        color: var(--violet);
    }

    .table-wrap {
        position: relative;
        border-radius: 24px;
        background: #fbf8ff;
        box-shadow: var(--press);
        padding: 6px;
        overflow-x: auto;
    }

    table {
        width: 100%;
        min-width: 860px;
        border-collapse: separate;
        border-spacing: 0 6px;
        font-size: 13.5px;
    }
    thead th {
        padding: 10px 14px;
        text-align: left;
        font-size: 10px;
        letter-spacing: .16em;
        text-transform: uppercase;
        color: var(--muted);
        font-weight: 800;
        white-space: nowrap;
    }
    tbody tr {
        background: #f4eeff;
        box-shadow: 0 6px 14px rgba(124, 58, 237, .07);
        transition: transform .16s ease, box-shadow .16s ease;
    }
    tbody tr:hover {
        transform: translateY(-1px);
        box-shadow: 0 10px 20px rgba(124, 58, 237, .13);
    }
    tbody td {
        padding: 14px;
        vertical-align: top;
        border-top: 1px solid rgba(124, 58, 237, .07);
        border-bottom: 1px solid rgba(124, 58, 237, .07);
    }
    tbody td:first-child { border-left: 1px solid rgba(124, 58, 237, .07); border-radius: 16px 0 0 16px }
    tbody td:last-child  { border-right: 1px solid rgba(124, 58, 237, .07); border-radius: 0 16px 16px 0 }

    .mono {
        font: 12.5px/1.5 ui-monospace, SFMono-Regular, Menlo, Consolas, monospace;
        word-break: break-all;
    }

    .pill {
        display: inline-block;
        padding: 5px 11px;
        border-radius: 999px;
        font-size: 9.5px;
        font-weight: 800;
        letter-spacing: .12em;
        text-transform: uppercase;
        box-shadow: var(--shadow-sm);
    }
    .pill.ok  { background: var(--mint); color: #0d5e44 }
    .pill.bad { background: var(--rose); color: #8e1d44 }

    .dim { color: var(--muted) }

    .chip {
        display: inline-flex;
        align-items: center;
        gap: 6px;
        margin: 0 6px 6px 0;
        padding: 5px 6px 5px 10px;
        border-radius: 12px;
        background: #fbf8ff;
        box-shadow: var(--shadow-sm);
        font: 11.5px/1.3 ui-monospace, SFMono-Regular, Menlo, Consolas, monospace;
    }
    .chip button {
        width: 24px; height: 24px;
        display: grid; place-items: center;
        border: 0;
        border-radius: 8px;
        background: var(--rose);
        color: #8e1d44;
        font-size: 12px;
        font-weight: 900;
        cursor: pointer;
        line-height: 1;
    }
    .chip button:hover { filter: brightness(.97) }

    .dev-forms { display: flex; flex-wrap: wrap; gap: 8px; margin-top: 10px }
    .dev-forms input { width: auto; flex: 1 1 150px; padding: 9px 12px; border-radius: 12px; font-size: 12px }
    .dev-forms input[type=number] { flex: 0 0 66px }
    .dev-forms button { flex: 0 0 auto }

    .actions { display: flex; flex-wrap: wrap; gap: 8px }

    .diag td:first-child { font-weight: 700; width: 40% }
    .diag td.ok  { color: #0d5e44 }
    .diag td.bad { color: #8e1d44; font-weight: 700 }

    .skeleton {
        position: absolute;
        inset: 6px;
        border-radius: 20px;
        background: #fbf8ff;
        display: grid;
        gap: 10px;
        padding: 18px;
        align-content: start;
        transition: opacity .28s ease;
        z-index: 2;
    }
    .skeleton span {
        display: block;
        height: 44px;
        border-radius: 14px;
        background:
            linear-gradient(100deg, rgba(233, 220, 255, .55) 20%, rgba(255, 255, 255, .95) 40%, rgba(233, 220, 255, .55) 60%)
            0 0 / 220% 100%;
        animation: shimmer 1.5s linear infinite;
    }
    body.ready .skeleton { opacity: 0; pointer-events: none }
    body.ready .table-wrap table { animation: rise .5s ease both }

    .foot { margin: 30px 0 0; text-align: center; color: var(--muted); font-size: 12px }

    @media (max-width: 620px) {
        .shell { padding: 20px 12px 64px }
        section.card { padding: 20px 16px }
        .topbar { padding: 16px }
        .login-card { padding: 28px 22px 24px }
        .trial-row .btn { flex: 1 1 130px; min-width: 0 }
    }

    @media (prefers-reduced-motion: reduce) {
        *, *::before, *::after {
            animation-duration: .001ms !important;
            animation-iteration-count: 1 !important;
            transition-duration: .001ms !important;
        }
    }
</style>
</head>
<body class="<?= $signedIn ? '' : 'guest' ?>">
<?php if (!$signedIn): ?>
    <span class="orb a"></span><span class="orb b"></span><span class="orb c"></span>
    <main class="login-shell">
        <form class="login-card clay <?= $error ? 'bad' : '' ?>" method="post" data-busy>
            <input type="hidden" name="action" value="login">
            <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
            <span class="badge"><i></i>Licence panel</span>
            <h1>Sign in</h1>
            <p class="sub">Manage the keys this build accepts.</p>

            <?php if ($error): ?>
                <p class="flash bad" role="alert"><?= h($error) ?></p>
            <?php endif; ?>

            <label>
                <span class="label-text">Username</span>
                <input name="user" autocomplete="username" autocapitalize="none" spellcheck="false"
                       placeholder="your username" required autofocus>
            </label>

            <label>
                <span class="label-text">Password</span>
                <input name="pass" type="password" autocomplete="current-password"
                       placeholder="your password" required>
            </label>

            <button class="btn" type="submit">
                <span class="label">Sign in</span>
                <span class="spin" aria-hidden="true"></span>
            </button>

            <p class="tiny">One account. No registration.</p>
        </form>
    </main>
<?php else: ?>
    <main class="shell">
        <header class="topbar clay rise">
            <div class="brand">
                <span class="mark">EK</span>
                <div>
                    <h2>Licence keys</h2>
                    <p><?= h((string)$auth['user']) ?> · every token here is signed</p>
                </div>
            </div>
            <form method="post">
                <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
                <button class="btn ghost small" name="action" value="logout">Sign out</button>
            </form>
        </header>

        <?php if ($error): ?><p class="flash bad" role="alert"><?= h($error) ?></p><?php endif; ?>
        <?php if ($notice): ?><p class="flash good"><?= h($notice) ?></p><?php endif; ?>

        <section class="card clay rise">
            <div class="section-head"><h3>Mint a key</h3><span class="count">signed on creation</span></div>
            <form method="post" data-busy>
                <input type="hidden" name="action" value="create">
                <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
                <div class="grid">
                    <div class="field grow">
                        <span class="label-text">Access key</span>
                        <input name="key" class="mono" placeholder="blank = generate one">
                    </div>
                    <div class="field slim">
                        <span class="label-text">Days</span>
                        <input name="days" type="number" min="0" value="30" inputmode="numeric">
                    </div>
                    <div class="field slim">
                        <span class="label-text">Max devices</span>
                        <input name="seats" type="number" min="0" value="0" inputmode="numeric">
                    </div>
                    <div class="field grow">
                        <span class="label-text">Device ids</span>
                        <input name="hwid" class="mono" placeholder="comma separated, optional">
                    </div>
                    <div class="field grow">
                        <span class="label-text">Note</span>
                        <input name="note" placeholder="who or what">
                    </div>
                </div>
                <div class="trial-row">
                    <button class="btn" type="submit"><span class="label">Create key</span><span class="spin" aria-hidden="true"></span></button>
                    <button class="btn ghost" name="days" value="1"><span class="label">1-day trial</span><span class="spin" aria-hidden="true"></span></button>
                    <button class="btn ghost" name="days" value="3"><span class="label">3-day trial</span><span class="spin" aria-hidden="true"></span></button>
                    <button class="btn ghost" name="days" value="7"><span class="label">7-day trial</span><span class="spin" aria-hidden="true"></span></button>
                </div>
                <p class="hint"><code>0</code> max devices means the key works on any phone, which is what a
                    free trial wants. Set <code>1</code> for a key you sell. A trial button keeps the
                    other fields as they are.</p>
            </form>
        </section>

        <section class="card clay rise">
            <div class="section-head">
                <h3>Keys</h3>
                <span class="count"><?= count($keys) ?> total</span>
            </div>
            <div class="table-wrap">
                <div class="skeleton" aria-hidden="true"><span></span><span></span><span></span></div>
                <table>
                    <thead>
                        <tr><th>Key</th><th>State</th><th>Expires</th><th>Devices</th><th>Note</th><th>Actions</th></tr>
                    </thead>
                    <tbody>
                    <?php if (!$keys): ?>
                        <tr><td colspan="6" class="dim">No keys yet.</td></tr>
                    <?php endif; ?>
                    <?php foreach ($keys as $key => $lic): ?>
                        <?php
                        $devs  = licence_devices($lic);
                        $seats = licence_seats($lic);
                        ?>
                        <tr>
                            <td class="mono"><?= h((string)$key) ?></td>
                            <td>
                                <?php if (!empty($lic['revoked'])): ?><span class="pill bad">revoked</span>
                                <?php elseif ($lic['expires'] !== 0 && $lic['expires'] < $now): ?><span class="pill bad">expired</span>
                                <?php else: ?><span class="pill ok">active</span><?php endif; ?>
                            </td>
                            <td class="mono dim"><?= $lic['expires'] === 0 ? 'never' : h(date('Y-m-d H:i', (int)$lic['expires'])) ?></td>
                            <td>
                                <?php if (!$devs): ?>
                                    <span class="dim">unbound</span>
                                <?php else: ?>
                                    <?php foreach ($devs as $d): ?>
                                        <form method="post" style="display:inline">
                                            <input type="hidden" name="key" value="<?= h((string)$key) ?>">
                                            <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
                                            <input type="hidden" name="action" value="dropdevice">
                                            <span class="chip"><?= h($d) ?><button name="hwid" value="<?= h($d) ?>"
                                                aria-label="Remove <?= h($d) ?>" title="Remove">&times;</button></span>
                                        </form>
                                    <?php endforeach; ?>
                                <?php endif; ?>
                                <div class="dim" style="font-size:11px;margin:2px 0 6px">
                                    <?= $seats === 0 ? 'any device' : $seats . ' device' . ($seats === 1 ? '' : 's') ?>
                                </div>
                                <div class="dev-forms">
                                    <form method="post" style="display:flex;gap:8px;flex:1 1 160px">
                                        <input type="hidden" name="key" value="<?= h((string)$key) ?>">
                                        <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
                                        <input name="hwid" class="mono" placeholder="add device id" aria-label="Device to allow">
                                        <button class="btn ghost small" name="action" value="bind">Add</button>
                                    </form>
                                    <form method="post" style="display:flex;gap:8px">
                                        <input type="hidden" name="key" value="<?= h((string)$key) ?>">
                                        <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
                                        <input name="seats" type="number" min="0" value="<?= $seats ?>" aria-label="Max devices">
                                        <button class="btn ghost small" name="action" value="seats">Max</button>
                                    </form>
                                </div>
                            </td>
                            <td class="dim"><?= h((string)($lic['note'] ?? '')) ?></td>
                            <td>
                                <div class="actions">
                                    <form method="post">
                                        <input type="hidden" name="key" value="<?= h((string)$key) ?>">
                                        <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
                                        <button class="btn ghost small" name="action" value="revoke">
                                            <?= empty($lic['revoked']) ? 'Revoke' : 'Enable' ?></button>
                                    </form>
                                    <?php if ($devs): ?>
                                        <form method="post">
                                            <input type="hidden" name="key" value="<?= h((string)$key) ?>">
                                            <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
                                            <button class="btn ghost small" name="action" value="rebind">Clear</button>
                                        </form>
                                    <?php endif; ?>
                                    <form method="post" onsubmit="return confirm('Delete this key?')">
                                        <input type="hidden" name="key" value="<?= h((string)$key) ?>">
                                        <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
                                        <button class="btn danger small" name="action" value="delete">Delete</button>
                                    </form>
                                </div>
                            </td>
                        </tr>
                    <?php endforeach; ?>
                    </tbody>
                </table>
            </div>
        </section>

        <section class="card clay rise">
            <div class="section-head"><h3>Deployment</h3><span class="count">server checks</span></div>
            <div class="table-wrap">
                <table class="diag" style="min-width:560px">
                    <thead><tr><th>Check</th><th>State</th></tr></thead>
                    <tbody>
                    <?php foreach ($diag as [$name, $detail, $ok]): ?>
                        <tr>
                            <td><?= h((string)$name) ?></td>
                            <td class="<?= $ok ? 'ok' : 'bad' ?>"><?= h((string)$detail) ?></td>
                        </tr>
                    <?php endforeach; ?>
                    </tbody>
                </table>
            </div>
        </section>

        <p class="foot">Signed with RSA-2048 · build <?= h((string)$config['build']) ?></p>
    </main>
<?php endif; ?>
<script>
    document.querySelectorAll('form[data-busy]').forEach(function (form) {
        form.addEventListener('submit', function () {
            var btn = form.querySelector('button[type=submit], .btn');
            if (btn) { btn.classList.add('busy'); }
        });
    });
    window.addEventListener('load', function () {
        window.setTimeout(function () { document.body.classList.add('ready'); }, 500);
    });
</script>
</body>
</html>
