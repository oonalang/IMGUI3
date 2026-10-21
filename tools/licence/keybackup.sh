#!/usr/bin/env bash
# Encrypts the licence signing key so an encrypted copy can live in a private
# repo (or a USB stick, or both) while the raw PEM stays out of git.
#
#   tools/licence/keybackup.sh enc    # -> tools/licence/key.enc   (commit this)
#   tools/licence/keybackup.sh dec    # -> tools/licence/private/licence_private.pem
#
# The passphrase is never an argument (argv is world-readable in ps) and never
# written to disk: it comes from $LICENCE_BACKUP_PASSPHRASE, or is prompted for.
# Keep it in a password manager. Without it the .enc file is a paperweight; with
# it, so is everyone else's copy of your key.
#
# Committing the RAW pem to a repo - private or not - is the one thing not to do:
# it ends up in someone's clone, in a CI workspace, in a backup, in the history
# after you delete it. Encrypted blob + passphrase held elsewhere is the shape
# that survives a leak of the repo alone.
set -euo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
priv="$here/private/licence_private.pem"
blob="$here/key.enc"

pass_from_env=false
if [ -n "${LICENCE_BACKUP_PASSPHRASE:-}" ]; then
    pass_from_env=true
    pass="$LICENCE_BACKUP_PASSPHRASE"
fi

prompt() {
    local var="$1" text="$2"
    printf '%s' "$text" >&2
    IFS= read -r -s "$var"
    printf '\n' >&2
}

# 1.1.0 (the version vendored for the app) has no -pbkdf2; a modern openssl does,
# and the older EVP_BytesToKey derivation is not something to fall back to
# silently for a file that holds a signing key.
if ! printf 'probe' | openssl enc -aes-256-cbc -pbkdf2 -salt -pass pass:probe >/dev/null 2>&1; then
    echo "this openssl has no -pbkdf2 support; upgrade OpenSSL (1.1.1 or newer)" >&2
    exit 1
fi

case "${1:-}" in
    enc)
        if [ ! -f "$priv" ]; then
            echo "no key at $priv - run tools/licence/genkey.sh first" >&2
            exit 1
        fi
        if [ -e "$blob" ]; then
            echo "refusing to overwrite $blob - move it aside if you really mean to" >&2
            exit 1
        fi

        if ! $pass_from_env; then
            prompt pass "passphrase: "
            prompt again "again: "
            if [ "$pass" != "$again" ]; then
                echo "passphrases differ" >&2
                exit 1
            fi
            unset again
        fi
        if [ "${#pass}" -lt 12 ]; then
            echo "use at least 12 characters; this is the only thing protecting the key" >&2
            exit 1
        fi

        export LICENCE_BACKUP_PASSPHRASE="$pass"
        # On any failure remove the half-written blob: a truncated backup that
        # looks like a backup is worse than none.
        if ! openssl enc -aes-256-cbc -pbkdf2 -salt -in "$priv" -out "$blob" \
                -pass env:LICENCE_BACKUP_PASSPHRASE; then
            rm -f "$blob"
            echo "encryption failed - nothing written" >&2
            exit 1
        fi
        chmod 644 "$blob"

        # Prove the blob decrypts back to the same bytes before saying so: a
        # backup nobody has restored is a backup nobody should trust.
        tmp="$(mktemp)"
        openssl enc -d -aes-256-cbc -pbkdf2 -in "$blob" -out "$tmp" \
            -pass env:LICENCE_BACKUP_PASSPHRASE
        if cmp -s "$priv" "$tmp"; then
            echo "wrote $blob"
            echo "verified: it decrypts back to the exact same key"
            echo
            echo "Commit $blob (it is encrypted). Keep the passphrase in your"
            echo "password manager - and note that losing it means no restore."
        else
            echo "round-trip check FAILED - do not trust $blob" >&2
            rm -f "$tmp"
            exit 1
        fi
        rm -f "$tmp"
        ;;
    dec)
        if [ ! -f "$blob" ]; then
            echo "no $blob to restore from" >&2
            exit 1
        fi
        if [ -e "$priv" ]; then
            echo "refusing to overwrite $priv - move it aside first" >&2
            exit 1
        fi

        if ! $pass_from_env; then
            prompt pass "passphrase: "
        fi

        export LICENCE_BACKUP_PASSPHRASE="$pass"
        mkdir -p "$(dirname "$priv")"
        umask 077
        # Decrypt to a temporary file and only move it into place once openssl
        # has succeeded. Writing straight to $priv meant a wrong passphrase left
        # an empty licence_private.pem behind, and the next attempt then refused
        # to "overwrite" it - a restore that fails once could not be retried.
        tmp="$(mktemp)"
        trap 'rm -f "$tmp"' EXIT
        if ! openssl enc -d -aes-256-cbc -pbkdf2 -in "$blob" -out "$tmp" \
                -pass env:LICENCE_BACKUP_PASSPHRASE; then
            echo "wrong passphrase, or $blob is corrupt - nothing restored" >&2
            exit 1
        fi
        mv "$tmp" "$priv"
        chmod 600 "$priv"
        echo "restored $priv (mode 600)"
        ;;
    *)
        echo "usage: tools/licence/keybackup.sh enc|dec" >&2
        exit 2
        ;;
esac
