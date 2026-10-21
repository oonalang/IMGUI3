<?php
/**
 * connect.php - the endpoint the app's sign-in posts to.
 *
 * This is the same request the app already sends
 * (game / user_key / serial), so the client's transport did not have to change:
 * what changed is the ANSWER. It no longer returns an MD5 that the app could
 * also have computed by itself; it returns a payload plus an RSA-SHA256
 * signature made with the private key in config.php, and the app verifies that
 * against the public key compiled into it.
 *
 * A swapped panel - the attack this replaces - can still answer whatever it
 * likes. It cannot sign with your key, so the app refuses its tokens
 * ("Token was not issued by this service") instead of accepting them.
 *
 *   POST game=CODMGR&user_key=...&serial=<device hwid>
 *
 *   200 {"status":true,"data":{"payload":"v1|KEY|HWID|EXP|BUILD","sig":"<b64>","expires":EXP}}
 *   200 {"status":false,"reason":"..."}
 *
 * Requires PHP 7.4+ with the openssl extension (standard on shared hosting).
 */

declare(strict_types=1);

require __DIR__ . '/lib.php';

header('Content-Type: application/json; charset=utf-8');

$game   = isset($_POST['game'])     ? (string)$_POST['game']     : '';
$key    = isset($_POST['user_key']) ? trim((string)$_POST['user_key']) : '';
$serial = isset($_POST['serial'])   ? trim((string)$_POST['serial'])   : '';

function fail(string $reason): void
{
    echo json_encode(['status' => false, 'reason' => $reason]);
    exit;
}

if ($game !== 'CODMGR') {
    fail('Unknown client');
}
if ($key === '' || strlen($key) > 128) {
    fail('No access key');
}
if (strpos($key, '|') !== false) {
    fail('Malformed access key');
}
if ($serial === '' || !preg_match('/^[A-Za-z0-9-]{8,64}$/', $serial)) {
    fail('No device id');
}

$db   = licence_store_open();
$lic  = licence_find($db, $key);
if ($lic === null) {
    fail('Invalid access key');
}
if (!empty($lic['revoked'])) {
    fail('This access key has been disabled');
}
if ($lic['expires'] !== 0 && $lic['expires'] < time()) {
    fail('This licence key has expired');
}

// Bind this phone the first time it signs in. A key carries a list of allowed
// devices and a seat count (0 = unlimited, the free-trial key): a phone that is
// already in the list is let through forever, a new one is added while there is
// room, and past that the key is simply in use elsewhere.
$devices = licence_devices($lic);
if (!in_array($serial, $devices, true)) {
    if (!licence_has_room($lic)) {
        $seats = licence_seats($lic);
        fail($seats === 1
            ? 'This access key is already in use on another device'
            : 'This access key is already in use on ' . $seats . ' devices');
    }
    $devices[] = $serial;
    $lic = licence_apply_devices($lic, $devices);
    licence_update($db, $key, $lic);
}

$build   = $config['build'];
// exp 0 in the store means "never" - it is signed as a concrete 2100-01-01 so
// the app never has a zero-means-forever branch of its own.
$expires = licence_signable_expiry((int)$lic['expires']);
$payload = 'v1|' . $key . '|' . $serial . '|' . (string)$expires . '|' . (string)$build;

$signature = '';
if (!openssl_sign($payload, $signature, licence_private_key(), OPENSSL_ALGO_SHA256)) {
    fail('Server could not sign the token');
}

echo json_encode([
    'status' => true,
    'data'   => [
        'payload' => $payload,
        'sig'     => base64_encode($signature),
        'expires' => $expires,
    ],
]);
