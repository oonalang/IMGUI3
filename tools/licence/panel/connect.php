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

// Bind the key to the first device that uses it. A different phone is a
// different licence: that is what makes a leaked key worth much less.
if ($lic['hwid'] === '') {
    $lic['hwid'] = $serial;
    licence_update($db, $key, $lic);
} elseif (!hash_equals($lic['hwid'], $serial)) {
    fail('This access key is bound to another device');
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
