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

$diag = [
    ['PHP', PHP_VERSION . ' (7.4 or newer)', version_compare(PHP_VERSION, '7.4.0', '>=')],
    ['openssl', $hasOpenssl ? 'loaded' : 'MISSING - tokens cannot be signed', $hasOpenssl],
    ['signing key', $keyOk ? 'readable by PHP' : 'not readable: ' . $config['private_key'], $keyOk],
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

if ($signedIn && $action === 'create') {
    $name = trim((string)($_POST['key'] ?? ''));
    $days = max(0, (int)($_POST['days'] ?? 30));
    $note = trim((string)($_POST['note'] ?? ''));
    if ($name === '') {
        $name = 'ETHNIR-' . strtoupper(bin2hex(random_bytes(4))) . '-' . strtoupper(bin2hex(random_bytes(2)));
    }
    if (strpos($name, '|') !== false) {
        $error = "A key may not contain '|'";
    } elseif (licence_find($store, $name) !== null) {
        $error = 'That key already exists';
    } else {
        licence_create($store, $name, $days, $note);
        $notice = 'Created ' . $name;
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

if ($signedIn && $action === 'rebind') {
    $key = (string)($_POST['key'] ?? '');
    $lic = licence_find($store, $key);
    if ($lic !== null) {
        $lic['hwid'] = '';
        licence_update($store, $key, $lic);
        $notice = 'Cleared the device binding on ' . $key;
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
    echo "<section><table><tr><th>Deployment</th><th>State</th></tr>";
    foreach ($rows as [$name, $detail, $ok]) {
        printf('<tr><td>%s</td><td class="%s">%s</td></tr>',
            h((string)$name), $ok ? 'ok' : 'bad', h((string)$detail));
    }
    echo '</table></section>';
}
?>
<!doctype html>
<html lang="en">
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Licence panel</title>
<style>
    :root { color-scheme: dark }
    body { font: 15px/1.45 system-ui, -apple-system, "Segoe UI", sans-serif; margin: 0;
           background: #120b1d; color: #f4f0fc; }
    main { max-width: 1080px; margin: 0 auto; padding: 28px 20px 60px; }
    h1 { font-size: 22px; margin: 0 0 4px; }
    p.sub { color: #aca0c6; margin: 0 0 24px; }
    section { background: #1b1229; border: 1px solid #34264a; border-radius: 14px;
              padding: 18px; margin-bottom: 18px; }
    label { display: block; font-size: 12px; letter-spacing: .08em; text-transform: uppercase;
            color: #aca0c6; margin-bottom: 6px; }
    input, select { background: #120b1d; border: 1px solid #3d2d58; color: inherit;
                    border-radius: 9px; padding: 9px 11px; font: inherit; width: 100%;
                    box-sizing: border-box; }
    .row { display: flex; gap: 12px; flex-wrap: wrap; }
    .row > div { flex: 1 1 200px; }
    button { background: #8b5cf6; border: 0; color: #fff; font: 600 15px system-ui;
             padding: 10px 18px; border-radius: 999px; cursor: pointer; }
    button.ghost { background: transparent; border: 1px solid #4a3766; color: #cfc3e6; }
    table { width: 100%; border-collapse: collapse; font-size: 14px; }
    th, td { text-align: left; padding: 9px 8px; border-bottom: 1px solid #2a1d3d; }
    th { color: #aca0c6; font-weight: 600; font-size: 12px; letter-spacing: .06em;
         text-transform: uppercase; }
    code { font: 13px ui-monospace, SFMono-Regular, Menlo, monospace; }
    .ok { color: #7ee2b8 } .bad { color: #ff7a94 } .dim { color: #8c80a8 }
    form.inline { display: inline }
</style>
<main>
    <h1>Licence panel</h1>
    <p class="sub">Keys minted here are what the app accepts: every token is signed with the
        private key, and the app only trusts the matching public one.</p>

    <?php if ($error): ?><p class="bad"><?= h($error) ?></p><?php endif; ?>
    <?php if ($notice): ?><p class="ok"><?= h($notice) ?></p><?php endif; ?>

    <?php if ($setupNeeded): ?>
        <section>
            <h1 style="font-size:17px;margin:0 0 6px">First run</h1>
            <p class="dim" style="margin:0 0 16px">There is no panel login yet. Pick one now:
                this form disappears once it is set, and anyone who reaches this page
                before you do can claim it.</p>
            <form method="post">
                <input type="hidden" name="action" value="setup">
                <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
                <div class="row">
                    <div><label>User</label><input name="user" value="admin" autocomplete="username" autofocus></div>
                    <div><label>Password (8+ characters)</label><input name="pass" type="password" autocomplete="new-password"></div>
                    <div><label>Repeat password</label><input name="pass2" type="password" autocomplete="new-password"></div>
                </div>
                <p><button>Save and sign in</button></p>
            </form>
        </section>
        <?php diag_table($diag); ?>
    <?php elseif (!$signedIn): ?>
        <section>
            <form method="post">
                <input type="hidden" name="action" value="login">
                <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
                <div class="row">
                    <div><label>User</label><input name="user" autocomplete="username" autofocus></div>
                    <div><label>Password</label><input name="pass" type="password" autocomplete="current-password"></div>
                    <div style="flex:0 0 auto;align-self:end"><button>Sign in</button></div>
                </div>
            </form>
        </section>
    <?php else: ?>
        <section>
            <form method="post">
                <input type="hidden" name="action" value="create">
                <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
                <div class="row">
                    <div style="flex:2 1 320px"><label>Access key (blank = generate)</label>
                        <input name="key" placeholder="ETHNIR-XXXXXXXX"></div>
                    <div><label>Days (0 = never)</label><input name="days" type="number" value="30" min="0"></div>
                    <div style="flex:2 1 240px"><label>Note</label><input name="note" placeholder="who / what"></div>
                    <div style="flex:0 0 auto;align-self:end"><button>Create key</button></div>
                </div>
            </form>
        </section>

        <section>
            <table>
                <tr>
                    <th>Key</th><th>State</th><th>Expires</th><th>Device</th><th>Note</th><th></th>
                </tr>
                <?php if (!$keys): ?>
                    <tr><td colspan="6" class="dim">No keys yet.</td></tr>
                <?php endif; ?>
                <?php foreach ($keys as $key => $lic): ?>
                    <tr>
                        <td><code><?= h((string)$key) ?></code></td>
                        <td>
                            <?php if (!empty($lic['revoked'])): ?><span class="bad">revoked</span>
                            <?php elseif ($lic['expires'] !== 0 && $lic['expires'] < $now): ?><span class="bad">expired</span>
                            <?php else: ?><span class="ok">active</span><?php endif; ?>
                        </td>
                        <td class="dim"><?= $lic['expires'] === 0
                            ? 'never'
                            : h(date('Y-m-d H:i', (int)$lic['expires'])) ?></td>
                        <td class="dim"><code><?= $lic['hwid'] === '' ? 'unbound' : h(substr($lic['hwid'], 0, 8) . '...') ?></code></td>
                        <td class="dim"><?= h((string)($lic['note'] ?? '')) ?></td>
                        <td>
                            <form class="inline" method="post">
                                <input type="hidden" name="key" value="<?= h((string)$key) ?>">
                                <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
                                <button class="ghost" name="action" value="revoke">
                                    <?= empty($lic['revoked']) ? 'Revoke' : 'Re-enable' ?></button>
                            </form>
                            <form class="inline" method="post">
                                <input type="hidden" name="key" value="<?= h((string)$key) ?>">
                                <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
                                <button class="ghost" name="action" value="rebind">Unbind</button>
                            </form>
                        </td>
                    </tr>
                <?php endforeach; ?>
            </table>
        </section>

        <?php diag_table($diag); ?>

        <section>
            <h1 style="font-size:17px;margin:0 0 12px">Panel login</h1>
            <form method="post">
                <input type="hidden" name="action" value="password">
                <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
                <div class="row">
                    <div><label>Current password</label><input name="current" type="password" autocomplete="current-password"></div>
                    <div><label>New password</label><input name="pass" type="password" autocomplete="new-password"></div>
                    <div><label>Repeat new password</label><input name="pass2" type="password" autocomplete="new-password"></div>
                    <div style="flex:0 0 auto;align-self:end"><button class="ghost">Change</button></div>
                </div>
            </form>
        </section>

        <form method="post">
            <input type="hidden" name="csrf" value="<?= h($csrf) ?>">
            <button class="ghost" name="action" value="logout">Sign out</button>
        </form>
    <?php endif; ?>
</main>
</html>
