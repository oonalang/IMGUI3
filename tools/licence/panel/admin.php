<?php
/**
 * admin.php - the panel itself: log in, create keys, see what is bound to what,
 * revoke. Deliberately tiny, deliberately server-rendered, no dependencies.
 *
 * It only ever writes the key store. The signing key stays out of this file's
 * reach beyond what lib.php does: nothing here can leak it, and the page never
 * prints a private key.
 */

declare(strict_types=1);

require __DIR__ . '/lib.php';
$config = licence_config();
$store  = licence_store_open();
$auth   = licence_auth();

// SameSite=Lax keeps the session off cross-site POSTs, and the token below is
// the belt to that braces: without it a page the signed-in admin merely visits
// could post action=create and hand itself a key.
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

$error = '';
$notice = '';

$action = $_POST['action'] ?? '';

// Every state-changing post carries the token; a mismatch is a bad request, not
// a silent no-op, so a broken deploy is visible instead of puzzling.
if ($action !== '' && !hash_equals($csrf, (string)($_POST['csrf'] ?? ''))) {
    http_response_code(400);
    echo "<!doctype html><meta charset=utf-8><p style='font:16px system-ui;padding:2rem'>"
       . "Missing or stale form token. Reload the panel and try again.</p>";
    exit;
}

// No hash anywhere yet: a fresh deploy on a host where there is no shell to run
// password_hash(). The first visitor sets the login and it is stored in
// data/auth.php; after that this form is unreachable, because $auth is no
// longer empty. Do it right after uploading - until then anyone who finds this
// URL can claim the panel.
$setupNeeded = $auth['hash'] === '';

if ($setupNeeded && $action === 'setup') {
    $user = trim((string)($_POST['user'] ?? ''));
    $pass = (string)($_POST['pass'] ?? '');
    $again = (string)($_POST['pass2'] ?? '');
    if ($user === '' || strlen($user) > 64) {
        $error = 'Pick a user name (up to 64 characters)';
    } elseif (strlen($pass) < 8) {
        $error = 'Use at least 8 characters for the password';
    } elseif (!hash_equals($pass, $again)) {
        $error = 'The two passwords do not match';
    } elseif (!licence_auth_save($user, password_hash($pass, PASSWORD_DEFAULT))) {
        $error = 'Could not write data/auth.php - is the data/ directory writable?';
    } else {
        $_SESSION['in'] = true;
        session_regenerate_id(true);
        header('Location: ' . basename(__FILE__));
        exit;
    }
} elseif ($action === 'login') {
    $user = (string)($_POST['user'] ?? '');
    $pass = (string)($_POST['pass'] ?? '');
    if ($user === $auth['user'] && password_verify($pass, $auth['hash'])) {
        $_SESSION['in'] = true;
        session_regenerate_id(true);
        header('Location: ' . basename(__FILE__));
        exit;
    }
    $error = 'Wrong username or password';
} elseif ($action === 'logout') {
    $_SESSION = [];
    session_destroy();
    header('Location: ' . basename(__FILE__));
    exit;
}

$signedIn = !empty($_SESSION['in']);

// What the deployment looks like from PHP's side. Every "bad" line here has
// broken a real panel somewhere: openssl not compiled in, a key the PHP user
// cannot read, a store directory it cannot write, or a private path sitting
// inside the document root where only .htaccess keeps it back.
$docRoot    = isset($_SERVER['DOCUMENT_ROOT']) ? rtrim((string)$_SERVER['DOCUMENT_ROOT'], '/') : '';
// "yes / no / unknown" - unknown when the host does not tell PHP its document
// root, in which case claiming "outside" would be a guess, and this table exists
// precisely because guesses about a signing key are not good enough.
$webRootNote = static function (string $path, string $guard) use ($docRoot): array {
    if ($docRoot === '') {
        return ['unknown - DOCUMENT_ROOT is not set', true];
    }
    return strpos($path, $docRoot) === 0
        ? ['NO - inside it; only ' . $guard . ' stands between it and the internet', false]
        : ['yes', true];
};
$hasOpenssl = extension_loaded('openssl');
$keyOk      = is_readable($config['private_key']);
$storeDir   = dirname($store);
$storeOk    = is_writable($storeDir);

// Does the key this panel signs with actually match the public half compiled
// into the app? A mismatch is the worst failure mode this panel has: keys mint
// fine, connect.php answers 200, and every sign-in dies with "Token was not
// issued by this service" - which looks like a broken app, not a wrong PEM.
// So read the modulus out of the private key and compare it with the one in
// config.php (which mirrors System/Core/LicencePubKey.h).
$appModulus = strtoupper(preg_replace('/[^0-9a-f]/i', '', (string)($config['app_modulus'] ?? '')));
$keyModulus = '';
if ($hasOpenssl && $keyOk) {
    $pem = @file_get_contents($config['private_key']);
    $key = $pem !== false ? @openssl_pkey_get_private($pem) : false;
    if ($key !== false) {
        $details = openssl_pkey_get_details($key);
        if (is_array($details) && isset($details['rsa']['n'])) {
            $keyModulus = strtoupper(bin2hex($details['rsa']['n']));
            $keyModulus = ltrim($keyModulus, '0');
        }
    }
}
if ($appModulus === '') {
    $modulusNote = 'config.php app_modulus is empty - cannot check the key';
    $modulusOk = true;
} elseif ($keyModulus === '') {
    $modulusNote = 'cannot read a modulus from the private key';
    $modulusOk = false;
} elseif (ltrim($appModulus, '0') === $keyModulus) {
    $modulusNote = 'matches LicencePubKey.h - tokens will verify';
    $modulusOk = true;
} else {
    $modulusNote = 'DOES NOT MATCH LicencePubKey.h - every sign-in will be refused (' . substr($keyModulus, 0, 24) . '...) ';
    $modulusOk = false;
}

$diag = [
    ['PHP', PHP_VERSION . ' (7.4 or newer)', version_compare(PHP_VERSION, '7.4.0', '>=')],
    ['openssl', $hasOpenssl ? 'loaded' : 'MISSING - tokens cannot be signed', $hasOpenssl],
    ['signing key', $keyOk ? 'readable by PHP' : 'not readable: ' . $config['private_key'], $keyOk],
    ['key matches the app', $modulusNote, $modulusOk],
    ['key store', $storeOk ? 'writable: ' . $storeDir : 'not writable: ' . $storeDir, $storeOk],
    ['panel login', !$setupNeeded ? (is_file(dirname($store) . '/auth.php') ? 'data/auth.php' : 'config.php admin_hash')
                                 : 'NOT SET - this page offers the setup form', !$setupNeeded],
    ['payload build', (string)$config['build'] . ' (must equal licence::kBuild)', true],
    ['key outside web root', ...$webRootNote($config['private_key'], 'private/.htaccess')],
    ['store outside web root', ...$webRootNote($store, 'data/.htaccess')],
];

if ($signedIn && $action === 'password') {
    $current = (string)($_POST['current'] ?? '');
    $pass    = (string)($_POST['pass'] ?? '');
    $again   = (string)($_POST['pass2'] ?? '');
    if (!password_verify($current, $auth['hash'])) {
        $error = 'Current password is wrong';
    } elseif (strlen($pass) < 8) {
        $error = 'Use at least 8 characters for the new password';
    } elseif (!hash_equals($pass, $again)) {
        $error = 'The two new passwords do not match';
    } elseif (!licence_auth_save($auth['user'], password_hash($pass, PASSWORD_DEFAULT))) {
        $error = 'Could not write data/auth.php - is the data/ directory writable?';
    } else {
        $notice = 'Panel password changed';
    }
}

// The device id the app sends is its own UUID: letters, digits and hyphens,
// 8-64 of them (connect.php enforces the same shape). One validator for every
// place a key can be bound by hand, so a typo is refused here instead of
// producing a key that can never be used.
function valid_device(string $hwid): bool
{
    return (bool)preg_match('/^[A-Za-z0-9-]{8,64}$/', $hwid);
}

/**
 * "PHONE-AAAA-1111, PHONE-BBBB-2222" -> the ids to allow. Holds at most one
 * lookup bad entry so the first mistake is the one reported.
 *
 * @return array{0:string[],1:?string}
 */
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
        $error = 'Not a device id: ' . $badDevice . ' - ids are 8-64 letters, digits or hyphens';
    } elseif ($seats !== 0 && count($devices) > $seats) {
        $error = 'You listed ' . count($devices) . ' devices but allowed only ' . $seats;
    } elseif (licence_find($store, $name) !== null) {
        $error = 'That key already exists';
    } else {
        licence_create($store, $name, $days, $note, $devices, $seats);
        $notice = 'Created ' . $name . ($seats === 0
            ? ' - unlimited devices'
            : ' - ' . $seats . ' device' . ($seats === 1 ? '' : 's'))
            . ($devices ? ', starting with ' . implode(', ', $devices) : '');
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

// Allow a device by hand - the trial phone in front of you, a customer whose
// serial you already know, or a replacement for one that died before it ever
// signed in. Adds to the list rather than replacing it; the seat count decides
// how long that list may get (0 = unlimited, which is the free-trial key).
if ($signedIn && $action === 'bind') {
    $key  = (string)($_POST['key'] ?? '');
    $lic  = licence_find($store, $key);
    [$devices, $badDevice] = split_devices((string)($_POST['hwid'] ?? ''));
    if ($lic === null) {
        $error = 'That key is gone - reload the panel';
    } elseif ($badDevice !== null) {
        $error = 'Not a device id: ' . $badDevice . ' - ids are 8-64 letters, digits or hyphens';
    } elseif (!$devices) {
        $error = 'Type the device id to allow';
    } else {
        $current = licence_devices($lic);
        $seats   = licence_seats($lic);
        $merged  = array_values(array_unique(array_merge($current, $devices)));
        if ($seats !== 0 && count($merged) > $seats) {
            $error = 'This key allows ' . $seats . ' device' . ($seats === 1 ? '' : 's')
                   . ' - raise the limit first, or use 0 for a trial key';
        } else {
            $lic = licence_apply_devices($lic, $merged);
            licence_update($store, $key, $lic);
            $added = array_values(array_diff($merged, $current));
            $notice = $added
                ? 'Added ' . implode(', ', $added) . ' to ' . $key
                : $key . ' already allows ' . implode(', ', $devices);
        }
    }
}

// Take one phone off the list. That phone is refused on its next sign-in; the
// others keep working.
if ($signedIn && $action === 'dropdevice') {
    $key  = (string)($_POST['key'] ?? '');
    $hwid = trim((string)($_POST['hwid'] ?? ''));
    $lic  = licence_find($store, $key);
    if ($lic !== null && $hwid !== '') {
        $lic = licence_apply_devices($lic, array_diff(licence_devices($lic), [$hwid]));
        licence_update($store, $key, $lic);
        $notice = 'Removed ' . $hwid . ' from ' . $key;
    }
}

// The seat count: 0 = unlimited (hand a trial key around), 1 = the old
// one-phone rule, N = a family/team key. Lowering it below the number of
// devices already allowed is refused rather than silently dropping phones.
if ($signedIn && $action === 'seats') {
    $key   = (string)($_POST['key'] ?? '');
    $seats = max(0, (int)($_POST['seats'] ?? 1));
    $lic   = licence_find($store, $key);
    if ($lic === null) {
        $error = 'That key is gone - reload the panel';
    } elseif ($seats !== 0 && $seats < count(licence_devices($lic))) {
        $error = 'This key already allows ' . count(licence_devices($lic))
               . ' device(s) - remove one first';
    } else {
        $lic = licence_apply_devices($lic, licence_devices($lic), $seats);
        licence_update($store, $key, $lic);
        $notice = $key . ($seats === 0
            ? ' now works on unlimited devices'
            : ' now allows ' . $seats . ' device' . ($seats === 1 ? '' : 's'));
    }
}

// Clearing every device puts the key back to "the first phones that use it".
if ($signedIn && $action === 'rebind') {
    $key = (string)($_POST['key'] ?? '');
    $lic = licence_find($store, $key);
    if ($lic !== null) {
        $lic = licence_apply_devices($lic, []);
        licence_update($store, $key, $lic);
        $notice = 'Cleared the device list on ' . $key;
    }
}

$keys = licence_all($store);
ksort($keys);
$now = time();

function h(string $s): string { return htmlspecialchars($s, ENT_QUOTES, 'UTF-8'); }

/** Shared by the first-run page and the panel: the same five answers matter in
 *  both places, and on a fresh upload the setup page is the only one you see. */
function diag_table(array $rows): void
{
    echo '<section class="diag"><h2>Deployment</h2><div class="tablewrap"><table>'
       . '<thead><tr><th>Check</th><th>State</th></tr></thead><tbody>';
    foreach ($rows as [$name, $detail, $ok]) {
        printf('<tr><td>%s</td><td class="%s">%s</td></tr>',
            h((string)$name), $ok ? 'ok' : 'bad', h((string)$detail));
    }
    echo '</tbody></table></div></section>';
}
?>
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Licence panel</title>
<style>
    /* Neo-brutalist shell, minimal interior: paper and ink, one violet, hard
       black rules and offset shadows, no gradients and no rounded corners.
       Everything that is data (keys, device ids, dates) is monospace so a
       mistyped character is visible; everything that is a label is uppercase
       and letter-spaced so the page reads as a form, not prose. */
    :root {
        --paper: #f4f1e8;
        --ink:   #101010;
        --violet:#8b5cf6;
        --acid:  #e4ff4f;
        --muted: #5b574e;
        --bad:   #b00020;
        color-scheme: light;
    }
    * { box-sizing: border-box }
    body {
        margin: 0; background: var(--paper); color: var(--ink);
        font: 15px/1.5 ui-sans-serif, system-ui, -apple-system, "Segoe UI", sans-serif;
        -webkit-font-smoothing: antialiased;
    }
    ::selection { background: var(--violet); color: #fff }
    main { max-width: 1120px; margin: 0 auto; padding: 40px 20px 90px }

    header.top {
        display: flex; align-items: flex-end; justify-content: space-between;
        gap: 20px; flex-wrap: wrap;
        border-bottom: 3px solid var(--ink); padding-bottom: 16px; margin-bottom: 28px;
    }
    h1 {
        margin: 0 0 8px; font-size: clamp(28px, 4.4vw, 44px); line-height: .92;
        letter-spacing: -.035em; text-transform: uppercase; font-weight: 800;
    }
    p.sub { margin: 0; max-width: 62ch; color: var(--muted) }

    section {
        background: #fff; border: 3px solid var(--ink); box-shadow: 6px 6px 0 var(--ink);
        padding: 22px; margin: 0 0 24px;
    }
    h2 {
        margin: 0 0 16px; font-size: 12px; letter-spacing: .16em;
        text-transform: uppercase; font-weight: 800;
    }
    h2 .count { color: var(--muted); font-weight: 600; letter-spacing: .06em }

    label {
        display: block; margin-bottom: 6px; font-size: 10px; letter-spacing: .16em;
        text-transform: uppercase; font-weight: 800;
    }
    input, select {
        width: 100%; padding: 11px 12px; font: inherit; color: var(--ink);
        background: #fff; border: 3px solid var(--ink); border-radius: 0;
    }
    input::placeholder { color: #9b968a }
    input:focus, select:focus { outline: none; box-shadow: 4px 4px 0 var(--violet) }

    .row { display: flex; gap: 14px; flex-wrap: wrap; align-items: flex-end }
    .row > div { flex: 1 1 200px; min-width: 0 }

    button {
        font: 800 12px/1 ui-sans-serif, system-ui, sans-serif; letter-spacing: .1em;
        text-transform: uppercase; padding: 13px 18px; cursor: pointer;
        color: #fff; background: var(--violet);
        border: 3px solid var(--ink); border-radius: 0; box-shadow: 4px 4px 0 var(--ink);
        transition: transform .06s ease, box-shadow .06s ease;
    }
    button:hover { transform: translate(-1px, -1px); box-shadow: 5px 5px 0 var(--ink) }
    button:active { transform: translate(3px, 3px); box-shadow: 1px 1px 0 var(--ink) }
    button.ghost { background: #fff; color: var(--ink) }
    button.acid  { background: var(--acid); color: var(--ink) }
    button.danger{ background: #fff; color: var(--bad) }
    button.small { padding: 9px 12px; font-size: 11px; box-shadow: 3px 3px 0 var(--ink) }

    .flash {
        margin: 0 0 24px; padding: 14px 16px; border: 3px solid var(--ink);
        box-shadow: 4px 4px 0 var(--ink); font-weight: 700;
    }
    .flash.ok  { background: var(--acid) }
    .flash.bad { background: #ffd6dc }

    .tablewrap { overflow-x: auto; border: 3px solid var(--ink) }
    table { width: 100%; min-width: 900px; border-collapse: collapse; font-size: 14px }
    th, td { text-align: left; padding: 12px 12px; border-bottom: 2px solid var(--ink); vertical-align: top }
    tr:last-child td { border-bottom: 0 }
    th {
        background: var(--ink); color: var(--paper); font-size: 10px;
        letter-spacing: .14em; text-transform: uppercase; font-weight: 800;
    }
    td.actions { white-space: nowrap }

    code, .mono { font: 13px/1.45 ui-monospace, SFMono-Regular, Menlo, Consolas, monospace }
    code { word-break: break-all }

    .pill {
        display: inline-block; padding: 4px 9px; border: 2px solid var(--ink);
        font-size: 10px; letter-spacing: .12em; text-transform: uppercase; font-weight: 800;
    }
    .pill.ok  { background: var(--acid) }
    .pill.bad { background: #ffd6dc; color: var(--bad) }
    .dim { color: var(--muted) }

    .device-form { display: flex; gap: 8px; margin-top: 8px; min-width: 240px }
    .device-form input { padding: 8px 10px; border-width: 2px; font-size: 12px }
    .device-form input[type=number] { flex: 0 0 74px }
    .device-form button { flex: 0 0 auto }

    .device-chip {
        display: inline-flex; align-items: center; gap: 6px; margin: 0 6px 6px 0;
        padding: 3px 4px 3px 8px; border: 2px solid var(--ink); background: #fff;
    }
    .device-chip code { font-size: 12px }
    .device-chip button { padding: 3px 7px; font-size: 10px; box-shadow: none; border-width: 2px }
    .seatnote {
        font-size: 10px; letter-spacing: .12em; text-transform: uppercase;
        color: var(--muted); font-weight: 700; margin: 0 0 6px;
    }

    .inline { display: inline }
    .diag td:first-child { font-weight: 700; width: 34% }
    .diag td.ok  { color: var(--ink) }
    .diag td.bad { color: var(--bad); font-weight: 700 }

    p.note { color: var(--muted); font-size: 13px; margin: 36px 0 0; max-width: 70ch }
    @media (max-width: 640px) {
        main { padding: 24px 14px 70px }
        section { padding: 18px; box-shadow: 4px 4px 0 var(--ink) }
        header.top { align-items: flex-start }
    }
</style>
</head>
<body>
<main>
    <header class="top">
        <div>
            <h1>Licence panel</h1>
            <p class="sub">Keys minted here are what the app accepts: every token is signed
                with the private key, and the app trusts only the matching public one.</p>
        </div>
        <?php if ($signedIn): ?>
            <form method="post">
                <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
                <button class="ghost" name="action" value="logout">Sign out</button>
            </form>
        <?php endif; ?>
    </header>

    <?php if ($error): ?><p class="flash bad"><?= h($error) ?></p><?php endif; ?>
    <?php if ($notice): ?><p class="flash ok"><?= h($notice) ?></p><?php endif; ?>

    <?php if ($setupNeeded): ?>
        <section>
            <h2>First run</h2>
            <p class="dim" style="margin:0 0 18px">There is no panel login yet. Pick one now:
                this form disappears once it is set, and anyone who reaches this page
                before you do can claim it.</p>
            <form method="post">
                <input type="hidden" name="action" value="setup">
                <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
                <div class="row">
                    <div><label>User</label><input name="user" value="admin" autocomplete="username" autofocus></div>
                    <div><label>Password (8+ characters)</label><input name="pass" type="password" autocomplete="new-password"></div>
                    <div><label>Repeat password</label><input name="pass2" type="password" autocomplete="new-password"></div>
                    <div style="flex:0 0 auto"><button>Save and sign in</button></div>
                </div>
            </form>
        </section>
        <?php diag_table($diag); ?>
    <?php elseif (!$signedIn): ?>
        <section>
            <h2>Sign in</h2>
            <form method="post">
                <input type="hidden" name="action" value="login">
                <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
                <div class="row">
                    <div><label>User</label><input name="user" autocomplete="username" autofocus></div>
                    <div><label>Password</label><input name="pass" type="password" autocomplete="current-password"></div>
                    <div style="flex:0 0 auto"><button>Sign in</button></div>
                </div>
            </form>
        </section>
    <?php else: ?>
        <section>
            <h2>Mint a key</h2>
            <form method="post">
                <input type="hidden" name="action" value="create">
                <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
                <div class="row">
                    <div style="flex:2 1 300px"><label>Access key (blank = generate)</label>
                        <input name="key" placeholder="ETHNIR-XXXXXXXX" class="mono"></div>
                    <div style="flex:0 1 130px"><label>Days (0 = never)</label>
                        <input name="days" type="number" value="30" min="0"></div>
                    <div><label>Max devices (0 = unlimited)</label>
                        <input name="seats" type="number" value="0" min="0"></div>
                    <div style="flex:2 1 260px"><label>Device ids (comma separated, blank = first sign-in)</label>
                        <input name="hwid" placeholder="device id, optional" class="mono"></div>
                    <div style="flex:2 1 220px"><label>Note</label><input name="note" placeholder="who / what"></div>
                </div>
                <div class="row" style="margin-top:16px">
                    <div style="flex:0 0 auto"><button>Create key</button></div>
                    <div style="flex:0 0 auto"><button class="acid" name="days" value="1">1-day trial</button></div>
                    <div style="flex:0 0 auto"><button class="acid" name="days" value="3">3-day trial</button></div>
                    <div style="flex:0 0 auto"><button class="acid" name="days" value="7">7-day trial</button></div>
                </div>
                <p class="dim" style="margin:14px 0 0"><strong>Max devices</strong> is the number of
                    phones one key may sign in on: <code>0</code> is unlimited, which is what a
                    free-trial key wants; leave it at 1 for a key you sell. A trial button creates
                    the key with that lifetime, that device limit and any device ids you typed -
                    a trial can be handed over already locked to one phone, or open to any.</p>
            </form>
        </section>

        <section>
            <h2>Keys <span class="count">(<?= count($keys) ?>)</span></h2>
            <div class="tablewrap">
            <table>
                <thead>
                    <tr><th>Key</th><th>State</th><th>Expires</th><th>Device</th><th>Note</th><th>Actions</th></tr>
                </thead>
                <tbody>
                <?php if (!$keys): ?>
                    <tr><td colspan="6" class="dim">No keys yet.</td></tr>
                <?php endif; ?>
                <?php foreach ($keys as $key => $lic): ?>
                    <tr>
                        <td><code><?= h((string)$key) ?></code></td>
                        <td>
                            <?php if (!empty($lic['revoked'])): ?><span class="pill bad">revoked</span>
                            <?php elseif ($lic['expires'] !== 0 && $lic['expires'] < $now): ?><span class="pill bad">expired</span>
                            <?php else: ?><span class="pill ok">active</span><?php endif; ?>
                        </td>
                        <td class="mono dim"><?= $lic['expires'] === 0
                            ? 'never'
                            : h(date('Y-m-d H:i', (int)$lic['expires'])) ?></td>
                        <td>
                            <?php
                            $devs  = licence_devices($lic);
                            $seats = licence_seats($lic);
                            ?>
                            <?php if (!$devs): ?>
                                <code class="dim">unbound - first sign-in claims it</code>
                            <?php else: ?>
                                <?php foreach ($devs as $d): ?>
                                    <form class="inline" method="post">
                                        <input type="hidden" name="key" value="<?= h((string)$key) ?>">
                                        <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
                                        <input type="hidden" name="action" value="dropdevice">
                                        <span class="device-chip">
                                            <code><?= h($d) ?></code>
                                            <button class="small ghost" name="hwid" value="<?= h($d) ?>"
                                                    aria-label="Remove device <?= h($d) ?>"
                                                    title="Remove this device">x</button>
                                        </span>
                                    </form>
                                <?php endforeach; ?>
                            <?php endif; ?>
                            <div class="seatnote"><?= $seats === 0
                                ? 'unlimited devices'
                                : $seats . ' device' . ($seats === 1 ? '' : 's') . ' max' ?></div>
                            <form class="device-form" method="post">
                                <input type="hidden" name="key" value="<?= h((string)$key) ?>">
                                <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
                                <input name="hwid" class="mono" aria-label="Device id to allow on <?= h((string)$key) ?>"
                                       placeholder="device id">
                                <button class="small ghost" name="action" value="bind">Add</button>
                            </form>
                            <form class="device-form" method="post">
                                <input type="hidden" name="key" value="<?= h((string)$key) ?>">
                                <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
                                <input name="seats" type="number" min="0" value="<?= $seats ?>"
                                       aria-label="Max devices for <?= h((string)$key) ?>">
                                <button class="small ghost" name="action" value="seats">Max</button>
                            </form>
                        </td>
                        <td class="dim"><?= h((string)($lic['note'] ?? '')) ?></td>
                        <td class="actions">
                            <form class="inline" method="post">
                                <input type="hidden" name="key" value="<?= h((string)$key) ?>">
                                <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
                                <button class="small ghost" name="action" value="revoke">
                                    <?= empty($lic['revoked']) ? 'Revoke' : 'Re-enable' ?></button>
                            </form>
                            <?php if (licence_devices($lic)): ?>
                            <form class="inline" method="post">
                                <input type="hidden" name="key" value="<?= h((string)$key) ?>">
                                <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
                                <button class="small ghost" name="action" value="rebind">Clear</button>
                            </form>
                            <?php endif; ?>
                            <form class="inline" method="post"
                                  onsubmit="return confirm('Delete this key? It cannot be undone.');">
                                <input type="hidden" name="key" value="<?= h((string)$key) ?>">
                                <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
                                <button class="small danger" name="action" value="delete">Delete</button>
                            </form>
                        </td>
                    </tr>
                <?php endforeach; ?>
                </tbody>
            </table>
            </div>
        </section>

        <?php diag_table($diag); ?>

        <section>
            <h2>Panel login</h2>
            <form method="post">
                <input type="hidden" name="action" value="password">
                <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
                <div class="row">
                    <div><label>Current password</label><input name="current" type="password" autocomplete="current-password"></div>
                    <div><label>New password</label><input name="pass" type="password" autocomplete="new-password"></div>
                    <div><label>Repeat new password</label><input name="pass2" type="password" autocomplete="new-password"></div>
                    <div style="flex:0 0 auto"><button class="ghost">Change password</button></div>
                </div>
            </form>
        </section>
    <?php endif; ?>

    <p class="note">Device ids are the serial the app computes for that phone: 8-64 letters,
        digits or hyphens. A key starts unbound and the first phone to sign in takes a seat;
        <em>Add</em> allows another phone by hand, <em>x</em> takes one away, and
        <em>Max</em> is how many seats the key has (0 = unlimited, for a free trial).</p>
</main>
</body>
</html>
