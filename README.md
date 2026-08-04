# optigrab

Optical disc grabber — a small, diskpart-style CLI that rips audio CDs to MP3.

Named in homage to the **Opti-Grab** from *The Jerk*: a ridiculous little invention that actually works.

## Features

- Interactive **VERB → NOUN** REPL (`list drive`, `select drive`, `rip track`, …)
- Swappable backends via clean ports:
  - **Extract:** `cdparanoia` (default), `ffmpeg`, `libcdio_paranoia`
  - **Encode:** `ffmpeg` + libmp3lame (tags included)
  - **TOC:** libcdio (CD-TEXT when present)
- Session focus (selected drive), manual artist/album overrides
- Filenames: `<out>/<Artist> - <Album>/<NN> Title.mp3`

## Requirements (Linux)

- CMake ≥ 3.20, C++20 compiler
- `libcdio`, `libcdio_cdda`, `libcdio_paranoia` (pkg-config)
- `ffmpeg` (with `libmp3lame`; `libcdio` demuxer optional for ffmpeg extract)
- `cdparanoia` (recommended extractor)

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

Binary: `build/optigrab`

## Usage

```text
$ ./build/optigrab
OPTIGRAB> list drive
OPTIGRAB> select drive 0
OPTIGRAB> set artist "The Band"
OPTIGRAB> set album "Music From Big Pink"
OPTIGRAB> set out ~/Music
OPTIGRAB> list track
OPTIGRAB> rip track all
OPTIGRAB> exit
```

### Commands

| Command | Description |
|---------|-------------|
| `list drive` | List optical drives |
| `select drive <n\|path>` | Select drive |
| `list track` | List tracks (loads TOC) |
| `detail drive` / `detail disc` | Session / disc info |
| `rip track <all\|N\|N-M\|…>` | Extract + encode |
| `set out\|quality\|artist\|album` | Session options |
| `set extractor <ffmpeg\|cdparanoia\|libcdio>` | Swap extractor |
| `set encoder <ffmpeg>` | Swap encoder |
| `help` / `cls` / `exit` | Shell utilities |
| ↑ / ↓ | Recall previous commands (TTY only) |

## Architecture

```text
CLI (VERB NOUN) → Session + Commands
       ↓
   RipService
       ↓
 DriveEnumerator | TocReader | AudioExtractor | AudioEncoder | MetadataProvider
       ↓                ↓              ↓              ↓
   Linux /sys      libcdio     cdparanoia CLI   ffmpeg CLI
                               libcdio_paranoia
                               ffmpeg libcdio
```

Core code depends on **ports** only. Adapters are thin wrappers.

## Windows

Drive enumeration and raw CD-DA I/O differ (drive letters + SPTI). Ports are OS-agnostic; a Windows `DriveEnumerator` / extract path can be added without touching the CLI. Linux is the supported platform today.

## License

MIT (see LICENSE if present; otherwise all rights reserved by the author until declared).
