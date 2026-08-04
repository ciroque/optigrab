# optigrab

Optical disc grabber — a small, diskpart-style CLI that rips audio CDs to MP3.

Named in homage to the **Opti-Grab** from *The Jerk*: a ridiculous little invention that actually works.

## Features

- Interactive **VERB → NOUN** REPL (`list drive`, `select drive`, `rip track`, …)
- **One-shot / scriptable** mode (same verbs as flags + command on the argv)
- Command history with **↑ / ↓** (TTY / console)
- **Progress** while ripping: `[n/N]` per track, extract %, encode phase
- **Actionable device errors** (permissions, busy drive, empty tray, …)
- Swappable backends via clean ports:
  - **Linux extract:** `cdparanoia` (default), `ffmpeg`, `libcdio_paranoia`
  - **Windows extract:** `ffmpeg` (default)
  - **Encode:** `ffmpeg` + libmp3lame (tags included)
  - **TOC:** libcdio (Linux), Windows SPTI (Windows)
- Session focus (selected drive); **auto-selects when only one drive is present**
- Manual artist/album overrides
- Filenames: `<out>/<Artist> - <Album>/<NN> Title.mp3`

## Requirements

### Linux

- CMake ≥ 3.20, C++20 compiler
- `libcdio`, `libcdio_cdda`, `libcdio_paranoia` (pkg-config)
- `ffmpeg` (with `libmp3lame`)
- `cdparanoia` (default extractor)
- User in the `optical` (or `cdrom`) group for `/dev/sr*` access

### Windows

- CMake ≥ 3.20, MSVC or compatible C++20 toolchain
- `ffmpeg` on `PATH` (extract + encode)
- Optical drive visible as a CD-ROM drive letter (e.g. `D:`)

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Binary: `build/optigrab` (Linux) or `build/Release/optigrab.exe` (MSVC multi-config).

Optional version stamp:

```bash
cmake -S . -B build -DOPTIGRAB_VERSION=0.2.0
```

## Usage

### Interactive

```text
$ optigrab
OPTIGRAB> list drive
OPTIGRAB> select drive 0
OPTIGRAB> set artist "The Band"
OPTIGRAB> set album "Music From Big Pink"
OPTIGRAB> set out ~/Music
OPTIGRAB> list track
OPTIGRAB> rip track all
OPTIGRAB> exit
```

With a single drive, selection is automatic at startup (and when a command needs a drive).

### One-shot (scriptable)

Same VERB NOUN grammar after optional flags — no REPL:

```bash
optigrab list drive
optigrab --drive 0 list track
optigrab --drive 0 \
  --out ~/Music \
  --artist "The Band" \
  --album "Music From Big Pink" \
  --quality V0 \
  rip track all
```

| Flag | Meaning |
|------|---------|
| `--drive <n\|path>` | Select drive |
| `--out <dir>` | Output directory |
| `--artist` / `--album` | Tag + folder overrides |
| `--quality` | `V0` `V2` `192` `256` `320` |
| `--extractor` | Platform-dependent (`ffmpeg`, …) |
| `--encoder` | `ffmpeg` |
| `-h` / `--help` / `--version` | Meta |

Exit codes: `0` ok, `1` usage/command error, `2` rip completed with track failures.

### Commands

| Command | Description |
|---------|-------------|
| `list drive` | List optical drives |
| `select drive <n\|path>` | Select drive |
| `list track` | List tracks (loads TOC) |
| `detail drive` / `detail disc` | Session / disc info |
| `rip track <all\|N\|N-M\|…>` | Extract + encode |
| `set out\|quality\|artist\|album` | Session options |
| `set extractor …` | Swap extractor (platform-dependent) |
| `set encoder <ffmpeg>` | Swap encoder |
| `help` / `cls` / `exit` | Shell utilities |
| ↑ / ↓ | Recall previous commands |

## Architecture

```text
CLI (VERB NOUN) → Session + Commands
       ↓
   RipService
       ↓
 DriveEnumerator | TocReader | AudioExtractor | AudioEncoder | MetadataProvider
       ↓                ↓              ↓              ↓
   Linux / Windows   libcdio / SPTI   cdparanoia    ffmpeg
                                      libcdio / ffmpeg
```

Core code depends on **ports** only. Adapters are thin wrappers.

## CI & releases

- **CI** (`.github/workflows/ci.yml`): build + test on Ubuntu and Windows for every PR/push to main.
- **Release** (`.github/workflows/release.yml`): push a tag `v0.2.0` → builds both platforms → GitHub Release with archives.

```bash
git tag v0.2.0
git push origin v0.2.0
```

## Metadata naming

Track/file names use:

1. CD-TEXT when present (Linux/libcdio)
2. `set artist` / `set album` session overrides
3. Fallbacks: `Track NN`, `Unknown Artist`, `Unknown Album`

No MusicBrainz lookup yet.

## Cover art & richer tags (what “feature 10” looks like)

Not implemented yet — sketch for when we do it:

| Piece | Approach |
|-------|----------|
| **Data** | Extend `Tags` with optional `year`, `genre`, `discNumber`/`discTotal`, and `coverArtPath` or in-memory JPEG/PNG bytes |
| **Source** | MusicBrainz / Cover Art Archive after disc-id lookup, or `set cover <file.jpg>`, or user drop-in `cover.jpg` next to output |
| **Write** | ffmpeg already accepts `-metadata`; for **APIC embedded art**, either a second pass with a tagger (e.g. TagLib) or ffmpeg’s video-attached-picture pattern (`-i cover.jpg -map 0 -map 1 -c copy -c:v:1 mjpeg …`) |
| **Port** | Keep `MetadataProvider` for text; add optional `CoverArtProvider` so network/file sources stay swappable |
| **UX** | `set cover …`, auto-fetch flag later; never block a rip if art fails |

Keep it thin: tags that players care about, not a full library manager.

## Roadmap

Planned / nice-to-have (not scheduled):

1. **MusicBrainz (or similar) metadata lookup** — disc ID → titles/artist/album  
2. ~~Clearer device errors~~ — **done**  
3. ~~Progress while ripping~~ — **done**  
4. **Cancel / interrupt a rip cleanly** (Ctrl-C mid-job, keep finished tracks)  
5. **Dry-run + preview naming** before spinning the laser  
6. **Persistent settings** (+ optional history file across runs)  
7. **Gap / pregap handling** (optional, explicit)  
8. **Light verify after rip** (length/sanity; not full AccurateRip theater)  
9. ~~Scriptable one-shot mode~~ — **done**  
10. **Cover art + richer tags** — see section above  
11. Eject / close tray verbs  
12. Multi-disc disc-number tags  
13. Pipeline extract ‖ encode  
14. Windows extract without ffmpeg libcdio demuxer quirks  
15. CUE/log export for archival  

Deliberately out of scope for now: plugin frameworks, GUI, multi-format matrix as core, heavy AccurateRip graphs.

## License

MIT — see [LICENSE](LICENSE).
