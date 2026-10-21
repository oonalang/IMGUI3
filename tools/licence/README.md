# Licence keys that survive a swapped panel

The app used to trust the endpoint: it POSTed `game / user_key / serial` to
`/connect` and compared the MD5 that came back against `CODMDGR-<key>-<hwid>-<secret>`
computed **inside the APK**. Two things followed from that:

* the secret was in the APK, so anyone could compute a valid token offline — no
  panel needed at all;
* a cracker only had to point the app at *their* `/connect` (DNS, hosts file,
  patching one string) and their panel could mint keys, because it could
  reproduce the same MD5.

The fix is that minting now needs an RSA private key that never ships:

```
tools/licence/private/licence_private.pem   signs      (vendor + panel only)
src/main/jni/System/Core/LicencePubKey.h    verifies   (inside every APK)
```

The panel returns `v1|key|hwid|expires|build` plus a PKCS#1 v1.5 RSA-SHA256
signature over exactly those bytes. The app verifies the signature **first**,
then checks that the key, device, build and expiry match this run. A swapped
panel can still answer — it just cannot sign, so its tokens are refused with
"Token was not issued by this service" instead of letting anyone in.

The honest limit: a determined patcher can still patch the verifier out of the
APK. That is true of every client-side check. What changed is the cost — from
"swap one URL" to "rewrite and re-sign the app" — and swapping the endpoint no
longer mints keys for anyone.

## One-time setup

```bash
tools/licence/genkey.sh        # prints the modulus to paste into LicencePubKey.h
```

Commit `LicencePubKey.h`. Keep `tools/licence/private/` (gitignored) backed up
somewhere that is not this repo: lose it and you cannot sign for this build any
more, and every key already sold is stranded.

The key was **rotated on 2026-10-21**: the previous private half was lost, so the
modulus in `LicencePubKey.h` and `app_modulus` in `panel/config.php` now belong
to a new keypair, and every token signed by the old one stops verifying. Keep
`tools/licence/private/licence_private.pem` this time — one copy on the server
(inside `private/`) and one in `tools/licence/key.enc` via `keybackup.sh`, or you
replay this rotation again. Rotating means reissuing every customer's key:
treat it like a release.

## Mint a key without any server

```bash
tools/licence/mint.sh ETHNIR-AB12CD34 <device-hwid> 30
tools/licence/mint.sh ETHNIR-PERM0001 <device-hwid> 0    # permanent
```

It prints the exact JSON the app reads, so you can sign in on a test device
before the panel exists. `<device-hwid>` is the UUID the app computes for that
phone; a token minted for one phone is refused on another.

## The panel

`panel/` is plain PHP 7.4+ (only the `openssl` extension), no database, no
composer. Drop it on any shared host:

1. `cp tools/licence/private/licence_private.pem panel/private/` (`chmod 600`).
2. In `panel/config.php` point `private_key` and `store` at absolute paths, keep
   `build` equal to `licence::kBuild`, and either set `admin_hash`:

   ```bash
   php -r 'echo password_hash("your-panel-password", PASSWORD_DEFAULT), "\n";'
   ```

   or leave it empty and let the panel ask: `admin.php` then shows a **first-run
   form** and writes the login to `data/auth.php` (which overrides `config.php`).
   That second route exists because a free host gives you no shell to run
   `password_hash()`. The same page carries a *Change password* form once you are
   in, and deleting `data/auth.php` puts the first-run form back.

3. Upload the folder. Everything in `panel/` goes together, `.htaccess` files
   included.
4. Open `admin.php`, sign in, create keys. If you left `admin_hash` empty, do
   this the minute the upload finishes: until you do, anyone who finds that URL
   can claim the panel. Devices and seat counts are explained
   below; *Revoke* kills a key without deleting its row.
   The page also prints a **Deployment** table under the key list: PHP version,
   whether `openssl` is loaded, whether the key is readable and the store
   writable, and whether either one is sitting inside the document root. Those
   five lines are what usually goes wrong on cheap hosting.

`connect.php` is the endpoint the app posts to. Same request shape as before, so
the client transport is unchanged; only the answer changed.

### Shared hosting where you only get `public_html` (x10 and friends)

Free tiers give you one document root and no SSH. Two things follow:

* **The key and the store end up inside it.** `panel/private/.htaccess` and
  `panel/data/.htaccess` deny all access, and `lib.php` creates `data/` with the
  rules already there — but a host that ignores `.htaccess`, or an FTP client
  that silently skips dotfiles, turns `https://your-host/panel/private/licence_private.pem`
  into a downloadable signing key. After uploading, **fetch that URL yourself**;
  a 403 is the only acceptable answer, and `data/keys.json` must answer 403 too.
  If you can put those two directories above the document root instead, do —
  `config.php` takes absolute paths and that removes the question entirely.
* **The APK's URL has no `.php`.** `panel/.htaccess` rewrites `^connect/?$` to
  `connect.php`, so uploading the folder as-is keeps `https://ethnir.x10.mx/connect`
  working. If the host has `mod_rewrite` off, put the panel in a subfolder and
  change the URL in `System/Core/Jni_Stuff.h` — the signature check does not
  care where the endpoint lives, which is rather the point.

On x10 specifically: PHP 8.1+ is selectable in cPanel, `.htaccess` is supported,
Let's Encrypt is free. Session files need a writable `session.save_path` for the
panel login — if sign-in loops back to the form, that is the reason, not the
password.

### Devices, trials, and the one check that explains a refused sign-in

A key is a **list of devices** plus a **seat count**:

* **seats = 0 - unlimited.** Any phone that signs in is added to the list. This
  is the free-trial key: hand it out and everybody's phone works. Set *Max
devices* to 0 (the default on the mint form) and every trial is like this.
* **seats = N - N phones.** The first N devices to sign in take the seats;
  the N+1th answers "This access key is already in use on N devices". For a
  key you sell, set it to 1 - that is the old one-phone-per-key rule.
* **empty list + seats = 1** - the key is `unbound`: the first phone to sign in
  claims it.

The row gives you the whole list without another page:

| control | what it does |
| --- | --- |
| **Add** | allow a device id by hand (8-64 letters, digits or hyphens), e.g. the trial phone in front of you, or a replacement for one that never signed in |
| **x** on a device chip | remove that phone - it is refused at its next sign-in, the others keep working |
| **Max** | change the seat count; lowering it below the devices already allowed is refused rather than silently dropping a phone |
| **Clear** | empty the list so the next phones to sign in claim the seats |
| **Delete** | remove the key entirely |

The trial buttons (`1-day`, `3-day`, `7-day`) are *Create key* with a fixed
lifetime, and they reuse the rest of the form: age, seat count, and any device
ids you typed. So one press can mint a 1-day key that runs on every phone, or a
1-day key already locked to the phone in your hand.

The app needed no change for any of this: the panel signs the serial of the
phone that is asking, and the app has always compared that with its own device
id. The seat count only decides which phones the panel will sign for.

The device id must match what the app computes for that phone: 8-64 letters,
digits or hyphens. Copy it from the app's own sign-in error (`Token was issued
for a different device`), or from `connect.php`'s access log if the host keeps one.

**The one check that matters.** The Deployment table has a `key matches the app`
row. It reads the modulus out of the panel's private key and compares it with
`app_modulus` in `config.php` (a copy of `System/Core/LicencePubKey.h`). If that
row is red, every key this panel mints will be refused with "Token was not issued
by this service" - the PEM and the APK do not belong together. Fix the PEM before
selling anything; no amount of panel work fixes a key mismatch.

### "403 Forbidden" when you open the panel

A 403 on the panel URL is almost never a permission problem with the files -
on a shared host it is Apache refusing to list a directory that has no index
document, because `Options -Indexes` is doing its job. Four deployments produce
it, and the curl request below tells them apart:

```bash
curl -s -o /dev/null -w 'root    %{http_code}\n' https://<host>/
curl -s -o /dev/null -w 'index   %{http_code}\n' https://<host>/index.php
curl -s -o /dev/null -w 'admin   %{http_code}\n' https://<host>/admin.php
curl -s -o /dev/null -w 'connect %{http_code}\n' https://<host>/connect
```

| root | index.php | what it means |
| --- | --- | --- |
| 403 | 200/302 | the host is not using `index.php` as a directory index. `panel/.htaccess` now sets `DirectoryIndex index.php index.html`; if the host has `AllowOverride Indexes` off, add `index.html` that redirects, or open `admin.php` directly. |
| 403 | 403 | `index.php` never got uploaded (the archive was built before it existed, or an FTP client skipped it). Re-extract the current `licence-panel.zip`. |
| 403 | 403 on `admin.php` too | the panel is in a subfolder but you are opening the domain root: `https://<host>/` is not `https://<host>/panel/`. Either extract the archive flat into `public_html` (its contents, not the folder) or use the subfolder URL. |
| 200 | 200 | the panel is fine and the 403 came from somewhere else - check `connect` next. |

A `connect` that answers **403** is the host's WAF or `mod_security` rejecting the
POST to a rewritten URL, not the panel: every failure `connect.php` itself knows
about is a 200 carrying `{"status":false,"reason":"..."}`. Try
`https://<host>/connect.php` directly and, if that answers, point the app at it
(`System/Core/Jni_Stuff.h`) - the signature check does not care where the endpoint
lives.

### Shipping it as a zip (no git, no shell)

Free hosts have a file manager and nothing else, so build the archive they can
actually use:

```bash
tools/licence/make_panel_zip.sh                  # asks for the panel password
tools/licence/make_panel_zip.sh --no-password    # or let the first-run form set it
tools/licence/make_panel_zip.sh --no-password --with-key   # key inside: one extract
```

It writes `tools/licence/dist/licence-panel.zip` with `connect.php`, `admin.php`,
`lib.php`, `config.php` and the three `.htaccess` guards **flat**, so extracting
it into `public_html` gives you `https://<host>/connect` with no URL change in
the app. Before it claims success it: hashes the password with PHP's own
`password_hash` and checks `password_verify` accepts it, lints every packed file,
unpacks its own archive to prove the dotfiles really made it in, and refuses to
build if a `.pem` or `keys.json` ever lands in the staging area.

The archive is **not committed**. It was, briefly, and built with `--with-key` so
a phone could install with a single extract - which put the signing key at a
public URL, so it was taken back out of the repo. It is a deploy artifact now:
build it with `make_panel_zip.sh` whenever `panel/` changes, and hand the file
over directly. (`git log --all -- licence-panel.zip` still carries those old
copies, so treat that keypair as public and rotate it, with a new APK, if that
ever matters.)

* **extracting is the whole install** with `--with-key`: `private/licence_private.pem`
  arrives in place, `chmod 600` it if the host lets you.
* **delete the zip from the server after extracting** - on a host that ignores
  `.htaccess` a zip sitting in `public_html` is a downloadable signing key, and
  on any host it keeps the hashed login off a public URL.
* a build without `--with-key` still refuses to pack a `.pem`, so the key cannot
  leak in by accident when you only meant to refresh the panel.

If PHP is not on the machine building the zip, the password hash is generated
with python3's `$2y$` bcrypt instead; the script says so, because it cannot run
`password_verify` there to prove the two agree (they were checked together here).

After extracting, three URLs decide whether this deploy is safe:

```bash
curl -s -o /dev/null -w '%{http_code}\n' https://<host>/private/licence_private.pem  # 403
curl -s -o /dev/null -w '%{http_code}\n' https://<host>/data/keys.json               # 403
curl -s -o /dev/null -w '%{http_code}\n' https://<host>/connect                      # not 404
```

## What the panel keeps on the server

| file | what is in it |
| --- | --- |
| `private/licence_private.pem` | the signing key. Deny-all `.htaccess` in `private/`. |
| `data/keys.json` | every customer key, its allowed devices, seat count, expiry. Deny-all `.htaccess` in `data/`. |
| `data/auth.php` | the panel login, once the first-run form or *Change password* has been used. |

All three are gitignored. `data/auth.php` is a PHP file that only returns an
array, so even a host that ignores `.htaccess` and serves it as PHP prints
nothing; on a host that serves it as plain text it would leak a bcrypt hash, so
keep checking the 403s below.

## Backing up the private key

Losing `tools/licence/private/licence_private.pem` means you can never sign for
this build again, and every key already sold is stranded - which is exactly what
happened once already. A private repo is a reasonable place for a backup — **as long as what goes in the repo is encrypted**:

```bash
LICENCE_BACKUP_PASSPHRASE='a long passphrase' tools/licence/keybackup.sh enc
git add tools/licence/key.enc        # this one is safe to commit
```

`keybackup.sh dec` restores the PEM (mode 600, round-trip verified by `enc`
before it claims success). Keep the passphrase somewhere that is not the repo —
a password manager — and remember that a repo is not durable on its own: one
encrypted copy in git + one on a USB stick beats one clever scheme.

What not to do, and why: committing the raw PEM (even to a private repo) puts
it in every clone, every CI workspace, every fork, and in the history after you
delete it. If a raw key ever lands in a repo, treat it as leaked: generate a new
pair, ship a build with the new public modulus, reissue keys, delete the old one.
And never paste a repository token into a chat or a file in the project — use
`gh auth login`, a credential helper, or an environment variable scoped to that
one command.

## Checks that run on a host machine

```bash
g++ -std=c++17 -O1 -w -Wno-deprecated-declarations -I src/main/jni \
    src/main/jni/System/Core/tests/licence_token_check.cpp -lcrypto \
    -o /tmp/licence_token_check && /tmp/licence_token_check
```

`tests/licence_token_check.cpp` generates two keypairs at run time (vendor and
cracker) and proves — among 20 checks — that the cracker's signature is refused,
which is the whole point. On a machine without `libssl-dev`, add
`-I src/main/jni/curl/openssl-android-arm64-v8a/include` and link
`/usr/lib/x86_64-linux-gnu/libcrypto.so.3`.

## If the private key leaks

Mint a new keypair, ship a build with the new modulus, issue every customer a new
key, delete the old `private/`. Old tokens die with the old key, which is the
behaviour you want.
