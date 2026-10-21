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
    // The app's public modulus, copied from System/Core/LicencePubKey.h. The
    // panel reads its own private key and compares the two: a mismatch mints
    // keys that the app refuses with "Token was not issued by this service",
    // and this line is what turns that hour of confusion into a red row on
    // the Deployment table. Keep it in step when you rotate the key.
    'app_modulus' => 'C8069D102A8B2AF0325DA054B02BA3EEEDCF06A0472FA5659E0BB1B22320AF9303015ABFF11D6A4A3BC7C7EC2B0DE0DD1C5A847C56D009F9104C47A42C610C17614DEF6AE6A306A29370413577C2DDF44D318917E17413DD2DFEC52837C70085DC2AE050BFB5711E4A5286E549EFFFEC1992167FDDE2915E42F762F4A09649A4715D041780595FC7EEE25AE0B6DE2F0B9D5BB259FE417E6CC3AAA9C8BEA6760217C2DC949AA9013B085D33B3E93ADF7E132510F9C20C41E57EAAFD5EA8CEEBAFB2BDE24D032B26D6E28A76EFE625F0ECBF568A2A098BFAF78F9266421B0BC39ADC2774A3FAAA5781C41D79CE9A0C7995C97CAAE3D7F415053CBBE6CAD9034DC5',
];
