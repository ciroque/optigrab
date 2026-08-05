# Verifying optigrab releases

Every GitHub Release ships platform archives plus integrity files:

| File | Purpose |
|------|---------|
| `optigrab-*.tar.gz` / `optigrab-*.zip` | Binaries |
| `SHA256SUMS` | SHA-256 digests of the archives |
| `SHA256SUMS.asc` | Detached GPG signature over `SHA256SUMS` (when signing is configured) |
| `KEYS.asc` | Release public key (if committed in the repo) |

## 1. Checksums only

```bash
# Download the archives you care about + SHA256SUMS into the same directory, then:
sha256sum -c SHA256SUMS
# macOS:
# shasum -a 256 -c SHA256SUMS
```

Only lines for files present in the directory are required; if you did not download every platform, either download all archives listed in `SHA256SUMS` or check individual lines:

```bash
grep linux-x64 SHA256SUMS | sha256sum -c -
```

## 2. GPG signature (recommended)

### Import the release public key

Prefer the key shipped with the release (or from the repo):

```bash
# From a release asset:
gpg --import KEYS.asc

# Or from the source tree:
gpg --import packaging/KEYS.asc
```

Fingerprint should match what the project documents in `packaging/KEYS.asc` comments / README.

### Verify the signature, then the sums

```bash
gpg --verify SHA256SUMS.asc SHA256SUMS
sha256sum -c SHA256SUMS
```

A good result looks like:

```text
gpg: Good signature from "optigrab release signing <…>"
```

If you see `WARNING: This key is not certified with a trusted signature!`, the signature is still cryptographically valid — you have not marked the key as trusted in your local web of trust. That is normal for a first import.

## 3. What this does *not* cover

- **Windows SmartScreen** / **macOS Gatekeeper** — different systems (Authenticode / Apple notarization). Checksums + GPG prove the file matches what the project published; they do not replace OS vendor code-signing.
- **Compromised GitHub account** that can create tags and push secrets — mitigate with 2FA, limited token scope, and offline verification of the public key from a second channel when possible.
