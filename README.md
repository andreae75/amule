<div align="center">

<img src="https://raw.githubusercontent.com/amule-org/amule/master/org.amule.aMule.png" alt="aMule logo" width="96" />

# aMule — NeRvOuX Edition

**A personal fork of aMule 3.0.0, focused on the desktop experience — mostly on Windows.**

On the eD2k network this build is indistinguishable from stock aMule.
The fork tag exists only where a human reads it.

</div>

---

## What's different

Everything in this table is on top of the upstream tree this fork started from.
Nothing here changes the eD2k or Kad protocols.

| Area | Change | Why it matters | Commit |
| :-- | :-- | :-- | :-- |
| **UI / Layout** | The seven main views are now dockable, floatable, closable wxAUI panes instead of siblings swapped in and out of a sizer. Layout is remembered between sessions, and a **View** menu toggles panes and resets the layout. | Transfers and Statistics can finally be on screen at once, and a pane can be torn off onto a second monitor. Transfers stays as the centre pane, so the layout can never end up empty. | [`c915ba7`](../../commit/c915ba7) |
| **Tray icon** | The chroma key in the tray artwork (pure red = transparent) is resolved to a real 8-bit alpha channel, and the icon is rescaled to the size Windows actually asks for (`SM_CXSMICON`), with a dilation pass so no magenta fringe bleeds into the edges. | The icon used to render as a jagged blob: 1-bit masking meant no edge could be anti-aliased, and 22×22 artwork was resampled by a non-integer factor on every machine. | [`be53f4b`](../../commit/be53f4b) |
| **Search** | Per-theme result status colours, picked for contrast against the actual listbox background. All four states clear WCAG AA (≥ 4.5:1) on both light and dark. | The old code saturated one channel of the system text colour: pure green on white (1.37:1, effectively invisible), and on a dark theme every status collapsed to the same white. | [`47617df`](../../commit/47617df) |
| **Downloads** | Context menu → **Copy selection to clipboard**, with **Plain text**, **CSV**, **JSON** and **HTML** submenus (Ctrl+C copies as plain text). All 13 columns, all selected rows, values exactly as shown on screen. | The download list is owner-drawn, so its cells hold no text and nothing could be copied out of it. Now a selection can go straight into a spreadsheet, a report or a script. | [`9f4922b`](../../commit/9f4922b) |
| **Shared files** | Context menu → **Open containing folder**, which reveals the file in Explorer (`/select`) or opens its directory when there is nothing to highlight. Windows only, compiled out elsewhere. | Getting from a shared entry to the file on disk meant reading the *Directory Path* column and navigating there by hand. | [`857d017`](../../commit/857d017) |
| **Identity** | `AMULE_FORK_NAME` / `AMULE_FORK_VERSION`, surfaced in the About box, the startup banner, the tray menu and the Windows version resource — and nowhere else. | Tells this binary apart from an upstream one without misreporting the client to the network. See [Network identity](#network-identity). | [`f4b49cf`](../../commit/f4b49cf) |
| **Build (Windows)** | [`compile.ps1`](compile.ps1): one command from a fresh checkout to `amule.exe`, driving CMake and Ninja through an MSYS2 MINGW64 shell. `-Bootstrap` installs the whole MSYS2 dependency set; `-All` widens the build to the daemon, remote GUI, webserver and utilities. | `scripts/compile.sh` assumes a POSIX shell, so on Windows there was no one-command build path at all. | [`fbd20d0`](../../commit/fbd20d0) |
| **Docs** | [`CLAUDE.md`](CLAUDE.md): build options, the MuleUnit/ctest workflow, and the architecture that spans several files — the one-tree-many-binaries target layout, the `Notify_*` layer between engine and wx, the EC protocol and its generated headers. | Also records the two silent footguns: a missing `po/POTFILES.in` entry, and the i18n CI drift gate. | [`29c2cd9`](../../commit/29c2cd9) |

## Building

**Windows** (MSYS2 MINGW64 — the driver installs the toolchain for you):

```powershell
.\compile.ps1 -Bootstrap     # first time only: installs the MSYS2 dependencies
.\compile.ps1                # builds build\src\amule.exe
.\compile.ps1 -All           # + amuled, amulegui, amuleweb, amulecmd, utilities
```

**Linux / macOS:**

```sh
cmake -B build -DBUILD_MONOLITHIC=YES -DBUILD_REMOTEGUI=YES
cmake --build build -j"$(nproc)"
```

`scripts/compile.sh` is the full build (every option on, plus ctest). Option list and install
layout are in [`docs/INSTALL.md`](docs/INSTALL.md); build and architecture notes in
[`CLAUDE.md`](CLAUDE.md).

Tests:

```sh
ctest --test-dir build --output-on-failure --timeout 10
```

## Network identity

The upstream version macros — `VERSION_MJR/MIN/UPDATE` and `MOD_VERSION_LONG` — are the ones
that go out on the wire: to servers via `ServerConnect.cpp`, to peers via `BaseClient.cpp`.
They are left exactly as upstream shipped them, so other clients see plain aMule 3.0.0.

`AMULE_FORK_NAME` / `AMULE_FORK_VERSION` are display-only and never transmitted. Bump the fork
version for changes made here; leave the upstream macros alone. The reasoning is spelled out in
[`src/include/common/ClientVersion.h`](src/include/common/ClientVersion.h).

## Upstream

This tree is a fork of the archived [`amule-project/amule`](https://github.com/amule-project/amule)
mirror. **Upstream aMule development lives at [amule-org/amule](https://github.com/amule-org/amule)**
— bugs that are not specific to the changes listed above belong there, along with releases,
discussions and the [documentation](https://amule-org.github.io).

The original project README is preserved at [`README.old.md`](README.old.md).

aMule is free software under the [GNU GPL v2 or later](COPYING); this fork keeps the same
licence and the aMule Team copyright headers.
