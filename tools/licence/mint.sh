#!/usr/bin/env bash
# Mints one licence token offline - no panel, no network - and prints it in the
# exact JSON shape the app reads.
#
#   tools/licence/mint.sh <access-key> <device-hwid> [days] [build]
#
# <device-hwid> is the UUID the app computes for the phone (the panel and this
# script both bind a token to it; the app rejects a token minted for another
# device). Hand the same key to a second phone and it gets its own token.
#
# [days] defaults to 30; 0 mints a permanent key (signed as 2100-01-01).
#
# Useful before the panel exists: one token is enough to sign in on a test
# device, and it proves the whole chain works.
set -euo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
priv="$here/private/licence_private.pem"

key="${1:?usage: mint.sh <access-key> <device-hwid> [days] [build]}"
hwid="${2:?usage: mint.sh <access-key> <device-hwid> [days] [build]}"
days="${3:-30}"
build="${4:-1}"

case "$key" in
    *"|"*) echo "the access key may not contain '|' (it is the payload separator)" >&2; exit 1 ;;
esac

if [ ! -f "$priv" ]; then
    echo "no private key at $priv - run tools/licence/genkey.sh first" >&2
    exit 1
fi

# 0 days means a permanent key. The token still carries a concrete timestamp
# (2100-01-01, same constant as licence::kNeverExpires) so the client keeps one
# "expired yet?" comparison instead of a zero-means-forever case.
label="$days days"
if [ "$days" -eq 0 ]; then
    exp=4102444800
    label="never"
else
    exp=$(( $(date +%s) + days * 86400 ))
fi
payload="v1|$key|$hwid|$exp|$build"
sig="$(printf '%s' "$payload" | openssl dgst -sha256 -sign "$priv" | base64 -w0)"

printf 'payload : %s\n' "$payload"
printf 'expires : %s (%s)\n' "$exp" "$label"
printf 'sig     : %s\n\n' "$sig"
printf 'reply   : {"status":true,"data":{"payload":"%s","sig":"%s","expires":%s}}\n' \
       "$payload" "$sig" "$exp"
