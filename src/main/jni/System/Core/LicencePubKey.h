#pragma once

// ---------------------------------------------------------------------------
// The licence signing key - PUBLIC half only
// ---------------------------------------------------------------------------
// Licence tokens are RSA-SHA256 signatures made with the PRIVATE key kept by
// tools/licence/ (and by the panel). This file holds the matching public
// modulus, which is what makes a swapped panel useless: it can answer, but it
// cannot sign.
//
// Fill this in once, from the machine that holds the private key:
//
//     tools/licence/genkey.sh
//
// prints exactly the line to paste below. Then COMMIT THIS FILE and nothing
// else - the private key (tools/licence/private/licence_private.pem, which is
// gitignored) must never enter the repo, the APK, or a chat.
//
// Empty is a valid state and it fails closed: CheckSession() answers
// "Licence key not configured in this build", so a build that was never wired
// to a key cannot be talked into letting anyone in.
//
// Rotating the key means minting new keys for your users (every token signed by
// the old private key stops verifying), so treat it like a release.

// 2048-bit modulus, hex, no "0x", no separators. Empty = not configured.
//
// This one was produced by tools/licence/genkey.sh in the working copy; its
// private half is tools/licence/private/licence_private.pem (gitignored) and is
// the file the panel must be given. BACK THAT FILE UP: with it gone this build
// can no longer be signed for, and every key already sold is stranded.
constexpr const char *kPubModulusHex =
    "C8069D102A8B2AF0325DA054B02BA3EEEDCF06A0472FA5659E0BB1B22320AF9303015ABFF11D"
    "6A4A3BC7C7EC2B0DE0DD1C5A847C56D009F9104C47A42C610C17614DEF6AE6A306A293704135"
    "77C2DDF44D318917E17413DD2DFEC52837C70085DC2AE050BFB5711E4A5286E549EFFFEC1992"
    "167FDDE2915E42F762F4A09649A4715D041780595FC7EEE25AE0B6DE2F0B9D5BB259FE417E6C"
    "C3AAA9C8BEA6760217C2DC949AA9013B085D33B3E93ADF7E132510F9C20C41E57EAAFD5EA8CE"
    "EBAFB2BDE24D032B26D6E28A76EFE625F0ECBF568A2A098BFAF78F9266421B0BC39ADC2774A3"
    "FAAA5781C41D79CE9A0C7995C97CAAE3D7F415053CBBE6CAD9034DC5";

// PKCS#1 v1.5 with SHA-256, the usual 65537.
constexpr unsigned long kPubExponent = 65537;

// What the panel's signatures are made with, in case a build ever needs to
// print it: "openssl dgst -sha256 -sign licence_private.pem".
constexpr const char *kLicenceSigningSpec = "RSA-2048 / SHA-256 / PKCS#1 v1.5";
