<?php

declare(strict_types=1);

const LICENCE_NEVER = 4102444800;

$config = require __DIR__ . '/config.php';

function licence_config(): array
{
    global $config;
    return $config;
}

function licence_auth(): array
{
    $cfg = licence_config();
    return [
        'user' => (string)($cfg['admin_user'] ?? ''),
        'hash' => (string)($cfg['admin_hash'] ?? ''),
    ];
}

function licence_login(string $user, string $pass): bool
{
    $auth = licence_auth();
    $userOk = $auth['user'] !== '' && hash_equals($auth['user'], $user);
    $passOk = $auth['hash'] !== '' && password_verify($pass, $auth['hash']);
    return $userOk && $passOk;
}

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

function licence_devices(array $lic): array
{
    $out = [];
    if (isset($lic['devices']) && is_array($lic['devices'])) {
        foreach ($lic['devices'] as $d) {
            if (is_string($d) && $d !== '') {
                $out[] = $d;
            }
        }
    } elseif (isset($lic['hwid']) && is_string($lic['hwid']) && $lic['hwid'] !== '') {
        $out[] = $lic['hwid'];
    }
    return array_values(array_unique($out));
}

function licence_seats(array $lic): int
{
    if (array_key_exists('seats', $lic)) {
        return max(0, (int)$lic['seats']);
    }
    return 1;
}

function licence_has_room(array $lic): bool
{
    $seats = licence_seats($lic);
    return $seats === 0 || count(licence_devices($lic)) < $seats;
}

function licence_apply_devices(array $lic, array $devices, ?int $seats = null): array
{
    $lic['devices'] = array_values(array_unique(array_filter($devices, static function ($d) {
        return is_string($d) && $d !== '';
    })));
    $lic['hwid'] = $lic['devices'][0] ?? '';
    if ($seats !== null) {
        $lic['seats'] = max(0, $seats);
    } elseif (!array_key_exists('seats', $lic)) {
        $lic['seats'] = 1;
    }
    return $lic;
}

function licence_create(string $store, string $key, int $days, string $note = '',
                        array $devices = [], int $seats = 1): array
{
    $lic = [
        'expires' => $days > 0 ? time() + $days * 86400 : 0,
        'devices' => [],
        'seats'   => max(0, $seats),
        'revoked' => 0,
        'note'    => $note,
        'created' => time(),
    ];
    $lic = licence_apply_devices($lic, $devices, max(0, $seats));
    $all = licence_all($store);
    $all[$key] = $lic;
    licence_save($store, $all);
    return $lic;
}

