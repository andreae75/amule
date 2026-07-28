# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repository status

This checkout is the **archived `amule-project/amule` mirror**. Active development lives at
<https://github.com/amule-org/amule>; issues, PRs, and releases are not accepted here (see
`README.md`, `.github/pull_request_template.md`, and the `redirect-issues` / `redirect-prs`
workflows). The code is otherwise a full, buildable tree — treat local work as work against
the upstream project.

## Build

CMake only (autotools is gone). Everything is off by default except `BUILD_MONOLITHIC`,
`BUILD_ED2K`, and `BUILD_TESTING`.

```sh
cmake -B build -DBUILD_MONOLITHIC=YES -DBUILD_REMOTEGUI=YES
cmake --build build -j"$(nproc)"
```

`scripts/compile.sh [-d] [-jN]` is the canonical full build — it wipes `build/`, configures
with every `BUILD_*` / `ENABLE_*` flag on, builds, and runs ctest. It must be run from the
repo root. `.github/workflows/ccpp.yml` is the authoritative reference for platform-specific
dependency sets (Ubuntu apt, macOS brew, Windows MSYS2 MINGW64 + Ninja) and is what CI uses.

Option list and install layout: `docs/INSTALL.md`; the option definitions and their
`NEED_LIB_*` fan-out are in `cmake/options.cmake`. `cmake -LAH -B build` dumps everything.

On Windows the documented path is an **MSYS2 MINGW64 shell** (`msystem: MINGW64`, Ninja
generator). The `Bash` tool here is Git Bash, not MSYS2 — it does not have the toolchain.

`CMAKE_EXPORT_COMPILE_COMMANDS` is on, so `build/compile_commands.json` exists for clang-tidy
and clangd. ccache is picked up automatically when present.

## Tests

MuleUnit (a small in-tree EasyUnit derivative under `unittests/muleunit/`), driven by ctest.

```sh
ctest --test-dir build --output-on-failure --timeout 10   # all
ctest --test-dir build -R CTagTest --output-on-failure     # one
./build/unittests/tests/CTagTest                           # run the binary directly
```

Each test is a standalone executable that compiles the handful of `src/` translation units it
needs — it does not link the app libraries. Adding a test means adding an
`add_executable` + `add_test` block to `unittests/tests/CMakeLists.txt` listing those sources
explicitly. `unittests/README` documents the `DECLARE`/`DECLARE_SIMPLE`/`TEST`/`ASSERT_*`
macros (it still describes the removed automake workflow — ignore that part).

## Lint

`.clang-tidy` at the root is a deliberately narrow, high-signal config; its header comment
explains every suppression and is worth reading before adding checks. Vendored/generated trees
have their own opt-out `.clang-tidy` files (`src/extern/`, `src/webserver/src/`). The
flex/bison outputs `src/Scanner.cpp`, `src/Parser.cpp`, `src/IPFilterScanner.cpp` cannot be
directory-excluded — skip their warnings when triaging.

## Architecture

### One codebase, several binaries

The same `src/` sources are compiled into different executables via preprocessor defines and
per-target source lists in `cmake/source-vars.cmake`:

| Target | Sources | Key defines |
| --- | --- | --- |
| `amule` (monolithic GUI) | `COMMON_ + CORE_ + GUI_SOURCES` | — |
| `amuled` (daemon) | `COMMON_ + CORE_SOURCES` | `AMULE_DAEMON`, `wxUSE_GUI=0` |
| `amulegui` (remote GUI) | `COMMON_ + GUI_SOURCES` | `CLIENT_GUI` |
| `amulecmd`, `amuleweb`, `ed2k`, `cas`, `wxCas`, `alc`/`alcc` | small dedicated source lists | varies |

`CORE_SOURCES` is the P2P engine; `GUI_SOURCES` is the wx UI. `amulegui` gets the UI without
the engine — it drives a remote `amuled` over EC instead. **`#ifdef CLIENT_GUI` in a shared
file means "this code path runs against a remote core, not a local one"** (~35 files). The app
class hierarchy mirrors this: `CamuleAppCommon` → `CamuleApp` → `CamuleGuiApp` /
`CamuleDaemonApp` (`src/amule.h`), with `CamuleRemoteGuiApp` as the parallel remote variant
(`src/amule-remote-gui.h`). `AMULE_APP_BASE` switches between `wxApp` and `wxAppConsole`.
The global is `theApp`, whose concrete type differs per build.

Static libraries assembled in `src/CMakeLists.txt`: `mulecommon` (`src/libs/common/` —
Path, Format, TextFile, StringFunctions, MD5), `mulesocket` (`LibSocket.cpp`, boost::asio),
`muleappcommon`, `muleappcore`, `muleappgui`, `ec`.

### Core → GUI decoupling

The engine never touches wx widgets directly. `src/GuiEvents.h` defines ~100
`Notify_Xxx(...)` macros that expand to `MuleNotify::DoNotify(&MuleNotify::Xxx, args...)`.
In the monolithic build these marshal onto the GUI thread as a `CMuleGUIEvent`; in `amuled`
they compile away or route through EC. Engine code calls `Notify_*` — it does not call into
`src/*Ctrl.cpp` / `src/*Dlg.cpp`.

### External Connections (EC)

`src/libs/ec/` is the wire protocol shared by `amuled` and its remote clients (`amulegui`,
`amuleweb`, `amulecmd`). Tag-based, documented in `docs/EC_Protocol.md`; server side is
`src/ExternalConn.cpp`. **`ECCodes.h` and `ECTagTypes.h` are generated at build time** by a
CMake script from `src/libs/ec/abstracts/*.abstract` — edit the `.abstract` files, not the
headers (the committed `.h` copies are regenerated). Java mirrors in `src/libs/ec/java/`.

### Other subsystems

- `src/kademlia/` — Kad DHT (routing zones, contacts, search, `UInt128`), largely an eMule port.
- `src/webserver/` — `amuleweb`: an embedded PHP-subset interpreter (`php_lexer.l`,
  `php_parser.y`, `php_syntree.cpp`) rendering the templates in `src/webserver/default/`.
- `src/utils/` — `cas`, `wxCas`, `aLinkCreator`, `fileview`, plus maintenance scripts in
  `src/utils/scripts/`.
- `src/include/protocol/` — eD2k / Kad / Kad2 opcode headers, the network contract with other
  clients. `src/include/common/DataFileVersion.h` versions the on-disk formats.

### Generated and semi-generated files

- `src/Scanner.cpp` / `src/Parser.cpp` (search expression grammar) and
  `src/IPFilterScanner.cpp` — regenerated from `.l` / `.y` when flex/bison are found at
  configure time, otherwise the committed copies are compiled as-is. Edit the `.l` / `.y`.
- `src/icons/icon_data.c` — PNGs under `src/icons/` embedded as byte arrays by
  `src/icons/embed_icons.py` (Python3 found) or taken from the committed fallback.
  Icons reach the UI through `CamuleArtProvider` (a `wxArtProvider`), not `#include`d XPMs.
- `src/muuli_wdr.{cpp,h}` — the whole GUI layout. Originally wxDesigner output from
  `muuli.wdr`; that source is gone and the file is **maintained by hand**.
- `config.h` from `config.h.cm`; `version.rc` from `version.rc.in` on Windows. The version
  string comes from `git describe` on every configure (see the long comment in the root
  `CMakeLists.txt`); an exact tag flips `AMULE_TAGGED_RELEASE` and drops the snapshot suffix.

## Translations

`./scripts/update-po.sh` from the repo root regenerates `po/amule.pot` and msgmerges every
`po/*.po`. CI (`.github/workflows/i18n.yml`) fails the build on catalog drift, so run it and
commit the result whenever translatable strings change.

**A new `.cpp` containing `_()` / `wxTRANSLATE` / `wxPLURAL` strings must be added to
`po/POTFILES.in`** — xgettext only scans that list and the omission is silent. New languages
also go in `po/LINGUAS`. Full workflow: `docs/translations.md`.

Translated manpages under `docs/man/` are committed po4a artifacts and are also drift-checked
by CI: `cd docs/man && touch po/manpages-*.po && po4a po4a.config`.

## Conventions

- Tabs for indentation in both C++ and CMake, matching the surrounding file.
- wx types throughout (`wxString`, `wxThread`, `wxEvtHandler`); `CPath` (`src/libs/common/Path.h`)
  rather than raw strings for filesystem paths.
- Every source file carries the GPL header with the aMule Team copyright line; new files
  follow the same block.
- Packaging lives outside the CMake build: `packaging/{linux,macos,windows}/build.sh` with
  per-platform `versions.env` (AppImage, Flatpak, NSIS installer, macOS notarization).
