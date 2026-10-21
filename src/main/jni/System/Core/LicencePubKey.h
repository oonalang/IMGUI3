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
// ROTATED 2026-10-21: the previous private half was lost, so this is a new
// keypair and every token signed by the old one stops verifying. The matching
// private key is tools/licence/private/licence_private.pem (gitignored) and is
// the file the panel must be given. **BACK THAT FILE UP** with
// tools/licence/keybackup.sh: lose it again and this build can never be signed
// for, and every key sold against it is stranded.
//
// Rotating again means: genkey.sh, paste the new modulus here AND into
// tools/licence/panel/config.php ('app_modulus', which is what the panel's key
// check compares against), ship a new APK, reissue keys.
constexpr const char *kPubModulusHex =
    "C61010F4D55F6586D543EE585BA0884E687FCFC6C397316664E4EB483F602D41304F60BC7BB37DA65046522663D79E81"
    "012F84492A1721FFEAA23FD32AF9A0C7762182F4708B6467ABACE56D3B9906F6F0BAE315DE04F0D12BE664A646A5D95C"
    "9CF5B538006179D8C38D03216380834513B0A9F8E85466DD8EFD0BEAD4365D28750B0430749E178304874CE50EC39E75"
    "5F772BF735683847EF9F4FD81B117FCA08D3C146C8704DE06E541B679907728BB3632B168E751B2308B7362AC2D4EF1C"
    "1168B89E168DC4C169D81AA28A23B53AC3CF514227A184E3C93C1983D37DFBF26AAE0FFECA875CAE18FA8AB00B12806C"
    "009D22BA93C6A51B592A0FB2A67FA52B";

// PKCS#1 v1.5 with SHA-256, the usual 65537.
constexpr unsigned long kPubExponent = 65537;

// What the panel's signatures are made with, in case a build ever needs to
// print it: "openssl dgst -sha256 -sign licence_private.pem".
constexpr const char *kLicenceSigningSpec = "RSA-2048 / SHA-256 / PKCS#1 v1.5";
