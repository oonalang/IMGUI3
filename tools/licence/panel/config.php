<?php
/**
 * config.php - everything the panel needs to know about this deployment.
 *
 * Deploying:
 *   1. tools/licence/genkey.sh on your machine, then paste the printed modulus
 *      into src/main/jni/System/Core/LicencePubKey.h (rebuild the app) AND into
 *      'app_modulus' below - that is what the panel's "key matches the app"
 *      check compares its private key against.
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
    // The app's public modulus, copied from System/Core/LicencePubKey.h. The
    // panel reads its own private key and compares the two: a mismatch mints
    // keys that the app refuses with "Token was not issued by this service",
    // and this line is what turns that hour of confusion into a red row on
    // the Deployment table. Keep it in step when you rotate the key.
    'app_modulus' => 'C61010F4D55F6586D543EE585BA0884E687FCFC6C397316664E4EB483F602D41304F60BC7BB37DA65046522663D79E81012F84492A1721FFEAA23FD32AF9A0C7762182F4708B6467ABACE56D3B9906F6F0BAE315DE04F0D12BE664A646A5D95C9CF5B538006179D8C38D03216380834513B0A9F8E85466DD8EFD0BEAD4365D28750B0430749E178304874CE50EC39E755F772BF735683847EF9F4FD81B117FCA08D3C146C8704DE06E541B679907728BB3632B168E751B2308B7362AC2D4EF1C1168B89E168DC4C169D81AA28A23B53AC3CF514227A184E3C93C1983D37DFBF26AAE0FFECA875CAE18FA8AB00B12806C009D22BA93C6A51B592A0FB2A67FA52B',
];
