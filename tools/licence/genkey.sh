#!/usr/bin/env bash
# Creates the licence signing keypair and prints the public key to paste into
# src/main/jni/System/Core/LicencePubKey.h.
#
#   tools/licence/genkey.sh
#
# The private key lands in tools/licence/private/ (gitignored, chmod 600). Copy
# it to the panel when you deploy (panel/config.php points at it) and keep a
# backup somewhere that is not this repo: lose it and every key you have issued
# stops signing; leak it and anyone can mint keys again, which is the exact hole
# this replaces.
set -euo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
priv="$here/private/licence_private.pem"

mkdir -p "$here/private"

if [ -e "$priv" ]; then
    echo "refusing to overwrite $priv" >&2
    echo "delete it deliberately if you really mean to rotate the key" >&2
    exit 1
fi

openssl genrsa -out "$priv" 2048 2>/dev/null
chmod 600 "$priv"

mod="$(openssl rsa -in "$priv" -noout -modulus 2>/dev/null | sed 's/^Modulus=//')"

if [ -z "$mod" ]; then
    echo "openssl did not produce a modulus" >&2
    exit 1
fi

echo "private key : $priv"
echo "             (keep it out of the repo; it is gitignored)"
echo
echo "Paste everything between the markers into"
echo "src/main/jni/System/Core/LicencePubKey.h, replacing the empty default:"
echo
echo "-----8<-----"
printf 'constexpr const char *kPubModulusHex =\n'
printf '%s\n' "$mod" | fold -w 96 | sed 's/^/    "/; s/$/"/'
printf '    ;\n'
echo "-----8<-----"
echo
echo "Then mint a key to test with:"
echo "    tools/licence/mint.sh MY-KEY-1234 <device-hwid> 30"
