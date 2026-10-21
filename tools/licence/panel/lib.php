<?php
/**
 * lib.php - shared helpers for the licence panel: config, the key store, and the
 * private key held in strict confidence.
 *
 * Storage is one JSON file with an flock around every write. That is deliberate
 * for a small vendor panel: it needs no MySQL account, it survives shared
 * hosting, and the whole store is a file you can back up by copying. If the
 * panel outgrows a few thousand keys, swap the four licence_* functions for
 * PDO and nothing else has to change.
 */

declare(strict_types=1);

// A key sold with days=0 is permanent. The store keeps 0 ("never" in the panel
// table, no date to recompute), but every token still travels with a concrete
// timestamp so the app needs only one "expired yet?" comparison. 2100-01-01,
// the same value as licence::kNeverExpires in System/Core/LicenceToken.h.
const LICENCE_NEVER = 4102444800;

$config = require __DIR__ . '/config.php';

function licence_config(): array
{
    global $config;
    return $config;
}

/**
 * The panel's own login. config.php can carry a hash, but a host with no shell
 * cannot run password_hash() to make one, so the setup form in admin.php writes
 * data/auth.php instead and that file wins. Keeping the credentials in the
 * store directory rather than rewriting config.php means the deploy never has
 * to make this file writable - and data/.htaccess already denies the folder, so
 * the hash is no more exposed than keys.json next to it.
 *
 * @return array{user:string, hash:string}
 */
function licence_auth(): array
{
    static $auth = null;
    if ($auth !== null) {
        return $auth;
    }

    $cfg  = licence_config();
    $auth = [
        'user' => (string)($cfg['admin_user'] ?? 'admin'),
        'hash' => (string)($cfg['admin_hash'] ?? ''),
    ];

    $file = dirname((string)$cfg['store']) . '/auth.php';
    if (is_readable($file)) {
        $override = include $file;
        if (is_array($override)) {
            $auth['user'] = (string)($override['user'] ?? $auth['user']);
            $auth['hash'] = (string)($override['hash'] ?? $auth['hash']);
        }
    }

    return $auth;
}

/** Store the panel login. Returns false when data/ is not writable. */
function licence_auth_save(string $user, string $hash): bool
{
    $file = dirname((string)licence_config()['store']) . '/auth.php';
    $body = "<?php\n// Written by admin.php - the panel's own login. Delete this file to fall back\n"
          . "// to config.php's admin_hash, or to run the setup form again.\n"
          . "return ['user' => " . var_export($user, true)
          . ", 'hash' => " . var_export($hash, true) . "];\n";

    if (file_put_contents($file, $body, LOCK_EX) === false) {
        return false;
    }
    @chmod($file, 0660);
    return true;
}

/** The private key, read once per request. It must never be served. */
function licence_private_key(): string
{
    static $pem = null;
    if ($pem !== null) {
        return $pem;
    }

    $path = licence_config()['private_key'];
    if (!is_readable($path)) {
        http_response_code(500);
        echo json_encode(['status' => false, 'reason' => 'Licence service is not configured']);
        exit;
    }

    $pem = (string)file_get_contents($path);
    return $pem;
}

function licence_store_open(): string
{
    $store = licence_config()['store'];
    $dir = dirname($store);
    if (!is_dir($dir) && !@mkdir($dir, 0770, true) && !is_dir($dir)) {
        http_response_code(500);
        echo json_encode(['status' => false, 'reason' => 'Licence store unavailable']);
        exit;
    }
    if (!file_exists($store)) {
        file_put_contents($store, "{}\n", LOCK_EX);
        @chmod($store, 0660);
    }
    return $store;
}

/** @return array<string,array{ expires:int, hwid:string, revoked:int, note:string, created:int }> */
function licence_all(string $store): array
{
    $raw = @file_get_contents($store);
    if ($raw === false || trim($raw) === '') {
        return [];
    }
    $data = json_decode($raw, true);
    return is_array($data) ? $data : [];
}

function licence_save(string $store, array $all): bool
{
    $tmp = $store . '.tmp';
    if (file_put_contents($tmp, json_encode($all, JSON_PRETTY_PRINT) . "\n", LOCK_EX) === false) {
        return false;
    }
    @chmod($tmp, 0660);
    return rename($tmp, $store);
}

/** The timestamp to sign for a stored licence: its expiry, or the sentinel. */
function licence_signable_expiry(int $stored): int
{
    return $stored === 0 ? LICENCE_NEVER : $stored;
}

function licence_find(string $store, string $key): ?array
{
    $all = licence_all($store);
    return $all[$key] ?? null;
}

function licence_update(string $store, string $key, array $lic): bool
{
    $all = licence_all($store);
    $all[$key] = $lic;
    return licence_save($store, $all);
}

function licence_create(string $store, string $key, int $days, string $note = '', string $hwid = ''): array
{
    $lic = [
        'expires' => $days > 0 ? time() + $days * 86400 : 0,   // 0 = never
        'hwid'    => $hwid,                                   // '' = bound on first use
        'revoked' => 0,
        'note'    => $note,
        'created' => time(),
    ];
    $all = licence_all($store);
    $all[$key] = $lic;
    licence_save($store, $all);
    return $lic;
}
