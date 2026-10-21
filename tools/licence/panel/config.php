<?php
/**
 * config.php - everything the panel needs to know about this deployment.
 *
 * Deploying:
 *   1. tools/licence/genkey.sh on your machine, then paste the printed modulus
 *      into src/main/jni/System/Core/LicencePubKey.h and rebuild the app.
 *   2. Upload this folder somewhere OUTSIDE the web root if your host allows it,
 *      or at least move private/ and data/ above it. Set the two paths below.
 *   3. Copy tools/licence/private/licence_private.pem to private/ on the server
 *      (chmod 600, owned by the PHP user).
 *   4. Set admin_hash - see the command in the comment below - or leave it empty
 *      and open admin.php once: it shows a first-run form and writes the login
 *      to data/auth.php. That is the only option on a host with no shell.
 *
 * On a host that only gives you public_html (x10's free tier, for example) the
 * two paths are forced inside the document root. Then the only thing keeping
 * them back is private/.htaccess and data/.htaccess - verify after uploading by
 * requesting /panel/private/licence_private.pem and /panel/data/keys.json: both
 * must answer 403. Some FTP clients skip dotfiles silently, which is exactly
 * how a signing key ends up downloadable.
 *
 * If the private key ever leaks: mint new keys against a new keypair, ship a
 * build with the new public modulus, and delete private/.
 */

declare(strict_types=1);

return [
    // Kept out of the web root: /home/you/licence-data/private/licence_private.pem
    'private_key' => __DIR__ . '/private/licence_private.pem',

    // The key store. Same idea - prefer a path above the web root, and make sure
    // it is NOT reachable over HTTP: it is every customer's key.
    'store' => __DIR__ . '/data/keys.json',

    // Must match licence::kBuild in System/Core/LicenceToken.h. Bump both when
    // the payload layout changes, so tokens from the old build stop verifying.
    'build' => 1,

    // The panel's own login. Generate the hash with:
    //   php -r 'echo password_hash("your-password-here", PASSWORD_DEFAULT), "\n";'
    // Empty (the default) means "not configured yet": admin.php then shows a
    // first-run form, and stores the login in data/auth.php - which wins over
    // these two values. Delete data/auth.php to be offered the form again.
    'admin_user' => 'admin',
    'admin_hash' => '',
];
