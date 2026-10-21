<?php

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

