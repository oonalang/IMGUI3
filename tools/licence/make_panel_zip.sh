#!/usr/bin/env bash
# make_panel_zip.sh - one archive you can upload and extract with a web file
# manager, for hosts that give you no git and no shell (x10's free tier).
#
#   tools/licence/make_panel_zip.sh -p 'panel-password'
#   -> tools/licence/dist/licence-panel.zip
#
# Extract it straight into public_html. The archive is flat on purpose: the APK
# posts to https://<your-host>/connect, and panel/.htaccess rewrites that to
# connect.php, so nothing in the app or the config has to change.
#
# By default the archive does NOT contain the signing key or the key store:
# upload licence_private.pem into private/ afterwards. An archive is one file,
# and one forgotten archive in public_html is a downloadable signing key, so the
# default build refuses to pack a .pem.
#
# --with-key packs tools/licence/private/licence_private.pem into private/, which
# makes the upload a single extract - no second step, no FTP client that might
# skip a dotfile. It also means the archive IS the signing key: anyone who gets
# the file can mint licences. Only publish such an archive where that is
# acceptable (which is to say: not in a public repository).
set -euo pipefail

ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
PANEL="$ROOT/tools/licence/panel"
OUT="$ROOT/tools/licence/dist/licence-panel.zip"
ADMIN_USER="admin"
PASS=""
SET_HASH=1
WITH_KEY=0

usage() {
    sed -n '2,17p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
    cat <<'EOF'

Options:
  -p, --password PW   panel password; its bcrypt hash is baked into the packed
                      config.php so admin.php is ready on first load (cost 12)
      --no-password   leave the login unset: the first visit to admin.php shows
                      a setup form and stores the login in data/auth.php. Use
                      this when you would rather pick the password after
                      uploading (or when nobody can run password_hash())
      --with-key      also pack tools/licence/private/licence_private.pem into
                      private/, so extracting the archive into public_html is
                      the whole install. The archive then contains the signing
                      key: do not publish it, and delete it from the server
                      after extracting
  -u, --user NAME     panel user name (default: admin)
  -o, --out PATH      where to write the archive
  -h, --help          this text
EOF
}

die() { printf 'error: %s\n' "$*" >&2; exit 1; }

while [ $# -gt 0 ]; do
    case "$1" in
        -p|--password) PASS="${2-}"; SET_HASH=1; shift 2 ;;
        -u|--user)     ADMIN_USER="${2-}"; shift 2 ;;
        -o|--out)      OUT="${2-}"; shift 2 ;;
        --no-password) SET_HASH=0; shift ;;
        --with-key)    WITH_KEY=1; shift ;;
        -h|--help)     usage; exit 0 ;;
        *)             die "unknown option: $1 (try --help)" ;;
    esac
done

[ -f "$PANEL/connect.php" ] || die "panel sources not found at $PANEL"

# The archive is built from inside the staging directory, so a relative -o would
# land there (or nowhere). Resolve it once, up front.
mkdir -p "$(dirname "$OUT")" || die "cannot create $(dirname "$OUT")"
OUT="$(cd "$(dirname "$OUT")" && pwd)/$(basename "$OUT")"
command -v zip >/dev/null   || die "zip is not installed"
command -v unzip >/dev/null || die "unzip is not installed"

# bcrypt for the panel login. PHP when it is here (its own password_hash, so the
# hash is by definition one password_verify accepts); python3 otherwise.
hash_password() {
    local pw="$1"
    if command -v php >/dev/null 2>&1; then
        php -r 'echo password_hash($argv[1], PASSWORD_BCRYPT, ["cost" => 12]);' -- "$pw"
    elif command -v python3 >/dev/null 2>&1; then
        python3 - "$pw" <<'PY'
import crypt, secrets, string, sys
alphabet = './' + string.ascii_letters + string.digits
salt = ''.join(secrets.choice(alphabet) for _ in range(22))
print(crypt.crypt(sys.argv[1], '$2y$12$' + salt))
PY
    else
        return 1
    fi
}

verify_password() {
    local pw="$1" hash="$2"
    command -v php >/dev/null 2>&1 || return 2
    php -r 'exit(password_verify($argv[1], $argv[2]) ? 0 : 1);' -- "$pw" "$hash" >/dev/null 2>&1
}

HASH=""
if [ "$SET_HASH" = 1 ]; then
    if [ -z "$PASS" ]; then
        if [ -t 0 ]; then
            read -r -s -p 'panel password: ' PASS; echo
            [ -n "$PASS" ] || die "empty password"
        else
            die "no password: pass -p PW, or --no-password to leave the login disabled"
        fi
    fi
    [ ${#PASS} -ge 8 ] || die "password is shorter than 8 characters"
    HASH=$(hash_password "$PASS") || die "no php or python3 to hash the password; use --no-password"
    case "$HASH" in
        \$2*) ;;
        *) die "unexpected hash format: $HASH" ;;
    esac
    if verify_password "$PASS" "$HASH"; then
        :
    elif [ $? -eq 1 ]; then
        die "password_verify rejects the generated hash - refusing to build a panel nobody can sign into"
    else
        printf 'note: php is not installed, so the hash was generated by python3 and not\n' >&2
        printf '      verified with password_verify. It is a $2y$ bcrypt hash, which\n' >&2
        printf '      password_verify accepts; check the panel login once it is uploaded.\n' >&2
    fi
fi

STAGE=$(mktemp -d)
trap 'rm -rf "$STAGE"' EXIT

cp "$PANEL/connect.php" "$PANEL/lib.php" "$PANEL/admin.php" "$PANEL/config.php" "$PANEL/index.php" "$STAGE/"
cp "$PANEL/.htaccess" "$STAGE/.htaccess"
mkdir -p "$STAGE/private" "$STAGE/data"
cp "$PANEL/private/.htaccess" "$STAGE/private/.htaccess"
cp "$PANEL/data/.htaccess"    "$STAGE/data/.htaccess"

KEY="$ROOT/tools/licence/private/licence_private.pem"
if [ "$WITH_KEY" = 1 ]; then
    [ -f "$KEY" ] || die "--with-key: $KEY does not exist (run tools/licence/genkey.sh first)"
    cp "$KEY" "$STAGE/private/licence_private.pem"
    chmod 600 "$STAGE/private/licence_private.pem"
fi

python3 - "$STAGE/config.php" "$ADMIN_USER" "$HASH" <<'PY'
import re, sys

path, user, hashed = sys.argv[1], sys.argv[2], sys.argv[3]

def php_str(s):
    return "'" + s.replace('\\', '\\\\').replace("'", "\\'") + "'"

src = open(path).read()

# Replace the whole line, comment included: the stock one describes the
# config.php default, and shipping it next to a baked hash would describe the
# wrong thing.
src = re.sub(r"^\s*'admin_user'\s*=>\s*'[^']*',.*$",
             "    'admin_user' => " + php_str(user) + ",",
             src, count=1, flags=re.M)
if hashed:
    note = "   // password_hash(), cost 12 - baked at build time by make_panel_zip.sh"
else:
    note = "   // empty: admin.php offers a one-time setup form and writes data/auth.php"

src = re.sub(r"^\s*'admin_hash'\s*=>\s*'[^']*',.*$",
             "    'admin_hash' => " + php_str(hashed) + "," + note,
             src, count=1, flags=re.M)
open(path, "w").write(src)
PY

grep -q "'admin_user' => '" "$STAGE/config.php" || die "could not set admin_user in the packed config.php"
if [ "$SET_HASH" = 1 ]; then
    grep -qF "$HASH" "$STAGE/config.php" || die "could not set admin_hash in the packed config.php"
fi

# Lint whatever PHP we have before shipping it: a syntax error here means a
# 500 on the host, where you cannot see the log.
if command -v php >/dev/null 2>&1; then
    for f in "$STAGE"/*.php; do
        php -l "$f" >/dev/null || die "php -l failed on $f"
    done
fi

if [ "$WITH_KEY" = 0 ] && find "$STAGE" -name '*.pem' | grep -q .; then
    die "staging area picked up a key - refusing to build (use --with-key if that is what you want)"
fi
if find "$STAGE" -name 'keys.json' | grep -q .; then
    die "staging area picked up the key store - refusing to build"
fi

mkdir -p "$(dirname "$OUT")"
rm -f "$OUT"
# -X drops the uid/gid/extra fields: cleaner on a host you do not own.
( cd "$STAGE" && zip -q -X -r "$OUT" . )

if [ "$WITH_KEY" = 1 ]; then
    printf '%s\n' \
        "Licence panel with its signing key. Extract into public_html and the panel" \
        "is complete. This file IS the signing key: keep it private and delete it" \
        "from the server once it has been extracted." \
        | zip -z "$OUT" >/dev/null 2>&1 || true
else
    printf '%s\n' \
        "Licence panel (signed-token edition). Extract into public_html, then upload" \
        "licence_private.pem into private/. Contains no signing key and no key store." \
        "Delete this archive from the server once it has been extracted." \
        | zip -z "$OUT" >/dev/null 2>&1 || true
fi

# The archive is only worth anything if the dotfiles are really inside it: a
# panel without private/.htaccess serves the signing key as text/plain.
entries=$(unzip -Z1 "$OUT")
for want in connect.php lib.php admin.php config.php index.php .htaccess private/.htaccess data/.htaccess private/ data/; do
    printf '%s\n' "$entries" | grep -qx -- "$want" || die "$want is missing from the archive"
done
if printf '%s\n' "$entries" | grep -qi 'keys\.json'; then
    die "archive contains the key store - refusing to ship it"
fi
if [ "$WITH_KEY" = 0 ] && printf '%s\n' "$entries" | grep -qi '\.pem$'; then
    die "archive contains a signing key - refusing to ship it"
fi
if [ "$WITH_KEY" = 1 ]; then
    printf '%s\n' "$entries" | grep -qx 'private/licence_private.pem' \
        || die "--with-key was given but private/licence_private.pem is not in the archive"
fi

# Unpack the archive and prove the result is a working panel, not just a zip
# that lists the right names.
CHECK=$(mktemp -d)
unzip -q "$OUT" -d "$CHECK"
if command -v php >/dev/null 2>&1; then
    for f in "$CHECK"/*.php; do
        php -l "$f" >/dev/null || die "extracted $(basename "$f") does not parse"
    done
    if [ "$SET_HASH" = 1 ]; then
        verify_password "$PASS" "$(php -r 'echo (require $argv[1])["admin_hash"];' -- "$CHECK/config.php")" \
            || die "extracted config.php does not verify the password"
    fi
fi
rm -rf "$CHECK"

NAMED_HTACCESS=$(printf '%s\n' "$entries" | grep -c '\.htaccess$')

cat <<EOF

built:  $OUT
inside: $(printf '%s\n' "$entries" | grep -vc '/$') files, $NAMED_HTACCESS .htaccess guards
login:  $ADMIN_USER / $([ "$SET_HASH" = 1 ] && echo '(the password you just typed)' || echo '(unset - admin.php asks for one on first visit)')

Upload and extract:
  1. cPanel -> File Manager -> public_html, enable "Show Hidden Files".
  2. Upload the zip, Extract it here. You should end up with connect.php,
     admin.php, lib.php, config.php, .htaccess, private/ and data/ directly
     in public_html - not inside a second folder.
$([ "$WITH_KEY" = 1 ] \
    && echo '  3. The signing key is already in private/ - nothing else to upload.' \
    || echo '  3. Upload tools/licence/private/licence_private.pem into private/.')
  4. Delete the zip.$([ "$WITH_KEY" = 1 ] && echo ' (it contains the signing key)' || echo '')
$([ "$SET_HASH" = 1 ] && echo '  5. Open /admin.php and sign in.' || echo '  5. Open /admin.php and set the panel login straight away: until you do,\n     whoever finds that URL first can claim it.')

Then check, before you sell anything:
  curl -s -o /dev/null -w '%{http_code}\n' https://<host>/private/licence_private.pem   # 403
  curl -s -o /dev/null -w '%{http_code}\n' https://<host>/data/keys.json                # 403
  curl -s -o /dev/null -w '%{http_code}\n' https://<host>/connect                       # not 404
  curl -s -o /dev/null -w '%{http_code}\n' https://<host>/                              # 302 to admin.php

If a guard answers 200, the key is public: move private/ and data/ above
public_html, point config.php at the new paths, and treat the old key as leaked.
EOF
