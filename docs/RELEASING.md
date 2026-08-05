# Releasing optigrab

## Cut a release

```bash
git tag v0.2.0
git push origin v0.2.0
```

Or run the **Release** workflow from the Actions UI (`workflow_dispatch`) with tag `v0.2.0`.

CI will:

1. Build + test on Linux, Windows, and macOS  
2. Package `optigrab-linux-x64`, `optigrab-windows-x64`, `optigrab-macos-arm64`  
3. Build `SHA256SUMS` over all archives  
4. Detach-sign `SHA256SUMS` with GPG when secrets are configured  
5. Publish a GitHub Release with archives + `SHA256SUMS` + `SHA256SUMS.asc` (+ `KEYS.asc` if present)

## Enable GPG release signing (one-time)

### 1. Create a dedicated release key (recommended)

Do **not** use your daily commit key if you can avoid it. A separate “release signing” key is easier to rotate and has a clearer purpose.

```bash
gpg --full-generate-key
# RSA 4096 or Ed25519, no expiry or long expiry
# Real name:  optigrab release signing
# Email:      your-project-or-noreply address
```

List the key id / fingerprint:

```bash
gpg --list-secret-keys --keyid-format LONG
```

### 2. Export secrets for GitHub Actions

**Private key** (repo secret `GPG_PRIVATE_KEY`):

```bash
# Replace KEYID with the 16+ hex key id
gpg --export-secret-keys --armor KEYID
```

Copy the full armored block (including `BEGIN` / `END` lines) into a repository secret named:

| Secret | Required | Description |
|--------|----------|-------------|
| `GPG_PRIVATE_KEY` | yes (for `.asc`) | Armored secret key |
| `GPG_PASSPHRASE` | if key has one | Passphrase for the key |
| `GPG_KEY_ID` | optional | Key id/fingerprint if the keyring would be ambiguous |

GitHub → **Settings → Secrets and variables → Actions → New repository secret**.

### 3. Commit the public key (so users can verify offline)

```bash
gpg --export --armor KEYID > packaging/KEYS.asc
# Optional: add a short comment at the top of KEYS.asc with the fingerprint
git add packaging/KEYS.asc
git commit -m "chore: add release signing public key"
```

The release job copies `packaging/KEYS.asc` into the release assets when the file exists.

### 4. Publish the fingerprint

Put the fingerprint in the README or release notes so users can cross-check:

```bash
gpg --fingerprint KEYID
```

### 5. Smoke-test locally

```bash
sha256sum optigrab-*.tar.gz optigrab-*.zip > SHA256SUMS
gpg --detach-sign --armor -o SHA256SUMS.asc SHA256SUMS
gpg --verify SHA256SUMS.asc SHA256SUMS
```

## Without GPG secrets

Releases still publish **`SHA256SUMS`**. The workflow logs a warning and skips `SHA256SUMS.asc` until `GPG_PRIVATE_KEY` is set.

## Rotating a compromised key

1. Generate a new key; export new secrets; replace GitHub secrets.  
2. Commit the new `packaging/KEYS.asc` (keep the old public key in git history or a `KEYS.old.asc` note for verifying historical releases).  
3. Announce the rotation (README / release notes).  
4. Revoke the old key and publish the revocation certificate if it was widely distributed.
