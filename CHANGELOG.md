# Changelog — aMule NeRvOuX Edition

Every version of the fork, newest first. Only what this fork changed on top of
the upstream tree; upstream aMule's own history is in
[`docs/CHANGELOG.md`](docs/CHANGELOG.md).

The fork version (`AMULE_FORK_VERSION`) is display-only and never goes out on
the network — see [Network identity](README.md#network-identity). Dates are the
date of the version bump commit.

## Unreleased

### Changed
- **UPnP is off by default and no longer built.** `ENABLE_UPNP` now defaults to
  `OFF`, and no build driver, packaging script or CI job turns it back on;
  libupnp and libixml are gone from every dependency list. UPnP buys one
  convenience — asking the router to open a port — and pays for it with an
  XML/HTTP stack that parses SSDP announcements and device descriptions from
  anything on the local network. The feature itself is untouched:
  `-DENABLE_UPNP=YES` still builds it as before. (`6afe9b3`)

### Added
- **Copy the selection on every list.** The clipboard export written for the
  download list moved into `CMuleListCtrl`, so shared files and search results
  have it too — plain text, CSV, JSON, HTML, and Ctrl+C. JSON keys are spelled
  out per list so a language change can't rename them. (`5f2adcb`)
- **A real About dialog** instead of a message box: selectable text, a **Copy**
  button, and a build block — wxWidgets compile-time *and* run-time, compiler,
  zlib, build date, config/temp/incoming paths, OS. (`5f2adcb`)

## 0.4.0 — 2026-08-01

### Added
- **Auto-fit columns to window**, in the column header menu of every list. The
  visible columns are scaled to fill the list's width and follow it on every
  resize; remembered per list across restarts, and dragging a divider hands
  control back. Content-measuring auto-size isn't an option here — the lists are
  owner-drawn, so their cells hold no text to measure. (`f17f60d`)

## 0.3.0 — 2026-08-01

### Added
- **Kad Routing Table pane** (View menu). The bucket tree drawn as an icicle
  plot — horizontal axis is the 128-bit ID space, each leaf filled by how full
  its bucket is — with the contacts of the selected leaf listed below (ID,
  address, version, liveliness, verified, age). Read-only, local core only;
  `amulegui` says so instead of guessing. Every client of this family exposes
  exactly one number from this structure: the node count. (`074a98d`)
- The fork name and version in the About box. (`f8e5d52`)

## 0.2.0 — 2026-08-01

### Added
- **Copy the selection to the clipboard** from the download list, as plain
  text, CSV, JSON or HTML — every column, every selected row, values exactly as
  shown on screen. The list is owner-drawn, so until now its cells held no text
  and nothing could be copied out of them. (`9f4922b`)
- **Open containing folder** in the shared files context menu: reveals the file
  in Explorer (`/select`), or opens its directory when there is nothing to
  highlight. Windows only, compiled out elsewhere. (`857d017`)

### Changed
- The README is now the fork's home page, tracking what differs from upstream.
  (`5ecd400`)

### Fixed
- `compile.ps1` stopped killing aMule processes running from outside the build
  directory — it matched on process name, so it took down the user's own
  running client. It now matches on the executable's path. (`5efb5f1`)

## 0.1.0 — 2026-07-28

First tagged version of the fork.

### Added
- **Dockable pane layout** (wxAUI). The seven main views became dockable,
  floatable, closable panes instead of siblings swapped in and out of a sizer.
  The layout is remembered between sessions and a **View** menu toggles panes
  and resets it. Transfers stays the centre pane, so the layout can never end
  up empty. (`c915ba7`)
- `AMULE_FORK_NAME` / `AMULE_FORK_VERSION`, distinct from the upstream version
  macros that go out on the wire. (`f4b49cf`)
- [`compile.ps1`](compile.ps1): one command from a fresh checkout to
  `amule.exe`, driving CMake and Ninja through an MSYS2 MINGW64 shell.
  `scripts/compile.sh` assumes a POSIX shell, so on Windows there was no
  one-command build path at all. (`fbd20d0`)
- [`CLAUDE.md`](CLAUDE.md): build options, the MuleUnit/ctest workflow, and the
  architecture that spans several files. (`29c2cd9`)

### Fixed
- **Tray icon.** The chroma key in the tray artwork (pure red = transparent) is
  resolved to a real 8-bit alpha channel, and the icon is rescaled to the size
  Windows actually asks for (`SM_CXSMICON`), with a dilation pass so no magenta
  fringe bleeds into the edges. It used to render as a jagged blob: 1-bit
  masking meant no edge could be anti-aliased, and 22×22 artwork was resampled
  by a non-integer factor on every machine. (`be53f4b`)
- **Search result status colours**, per theme, picked for contrast against the
  actual listbox background — all four states clear WCAG AA (≥ 4.5:1) on light
  and dark. The old code saturated one channel of the system text colour: pure
  green on white (1.37:1, effectively invisible), and on a dark theme every
  status collapsed to the same white. (`47617df`)
