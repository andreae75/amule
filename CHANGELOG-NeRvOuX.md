# Changelog — aMule NeRvOuX Edition

Every version of the fork, newest first. Only what this fork changed on top of
the upstream tree; upstream aMule's own history is in
[`docs/CHANGELOG.md`](docs/CHANGELOG.md), which is theirs and not ours to edit.

**The filename is deliberate — don't rename this to `CHANGELOG.md`.** Upstream
keeps its changelog at `docs/CHANGELOG.md` today, but the root `CHANGELOG.md` is
precisely the path it would take if it ever promoted that file, and a merge
conflict in a file that is 100% fork-owned is pure noise. A name upstream will
never pick can never collide on a sync.

The fork version (`AMULE_FORK_VERSION`) is display-only and never goes out on
the network — see [Network identity](README.md#network-identity). Dates are the
date of the version bump commit.

## 0.5.0 — 2026-08-02

### Added
- **A light / dark / system colour scheme**, in Preferences → Interface.
  Defaults to System, which is also what a config file written before the
  option existed reads back as. The dark palette is sampled pixel by pixel
  from a reference theme rather than derived from a formula: the content is
  the lightest surface and the chrome recedes behind it. Lists, banded rows,
  column headers, panels, the toolbar, labels, the log pane, graphs, the AUI
  pane chrome and the Windows title bar follow it. **Menus, group box frames,
  combo dropdowns and native scrollbars do not** — those are painted by the
  OS theme, and wxWidgets 3.2 has no application-wide hook to redirect it, so
  the colours are pushed down the window tree by hand to the classes that
  honour them. wx 3.3 adds `MSWEnableDarkMode`; when it reaches a stable
  series it goes behind a version guard driven by this same option. Changing
  the scheme asks for a restart. (`d59c178`, `1dbe89f`)
- **Banded list rows** on every owner-drawn list, and **status-coloured text**
  in the download list — green only when data is actually arriving, not when a
  file is merely queued. On dark the band is a deep blue rather than a lighter
  grey: a brightness step subtle enough not to read as two lists stitched
  together is also too subtle to survive coloured text drawn on top of it.
  (`d59c178`, `1dbe89f`)
- **Copy the selection on every list.** The clipboard export written for the
  download list moved into `CMuleListCtrl`, so shared files and search results
  have it too — plain text, CSV, JSON, HTML, and Ctrl+C. JSON keys are spelled
  out per list so a language change can't rename them. (`5f2adcb`)
- **A real About dialog** instead of a message box: selectable text, a **Copy**
  button, and a build block — wxWidgets compile-time *and* run-time, compiler,
  zlib, build date, config/temp/incoming paths, OS. It opens at the size and
  position the preferences do, from one shared rule rather than two constants
  that happen to match. (`5f2adcb`, `fc1a669`)

### Changed
- **UPnP is off by default and no longer built.** `ENABLE_UPNP` now defaults to
  `OFF`, and no build driver, packaging script or CI job turns it back on;
  libupnp and libixml are gone from every dependency list. UPnP buys one
  convenience — asking the router to open a port — and pays for it with an
  XML/HTTP stack that parses SSDP announcements and device descriptions from
  anything on the local network. The feature itself is untouched:
  `-DENABLE_UPNP=YES` still builds it as before. (`6afe9b3`)
- **Geolocation is off by default and no longer built.** `ENABLE_IP2COUNTRY`
  now defaults to `OFF` and no build driver, packaging script or CI job turns
  it back on; libmaxminddb is gone from every dependency list, and the
  now-unused legacy `libgeoip-dev` went with it from the AppImage image. The
  country flag beside each peer was decoration that had largely stopped being
  accurate — behind a VPN or a relay it showed the exit node's country and
  nobody's own — and it cost a dependency, a database the user has to fetch
  from MaxMind under an account, and a lookup per connecting client. The
  feature itself is untouched: `-DENABLE_IP2COUNTRY=YES` still builds it as
  before. (`ca0f79e`)
- **The flag beside each peer is now the Straw Hat Jolly Roger**, the same one
  for everyone. It replaces nothing functional — with the country lookup
  compiled out there was no flag left to draw — and it needs no database. The
  *Show country flags for clients* setting became *Show a flag for clients*
  and still turns it off. It is stored under a new key
  (`/SkinGUIOptions/ShowClientFlag`) rather than upstream's
  `/eMule/GeoIPEnabled`, because the old one cannot be trusted: until this
  fork stopped building IP2Country, the preferences dialog forced that value
  to false on every construction with the check box greyed out, so an
  existing `amule.conf` holds a zero nobody chose. The flag shows in the peer
  lists — sources under a download, clients under a shared file — which is
  where the country flag used to be. (`0aa8dcb`, `7c3eb6c`)
- **The download list's Progress column is one filled bar** instead of eMule's
  chunk map. The map showed a coloured block per part — blue shaded by source
  count, red for the parts nobody has — which is more information than any
  other client puts in that column, and at 170 pixels wide across twenty rows
  it read as speckle rather than as progress. The per-part detail is still in
  the file detail dialog and the source counts are still in the Sources
  column. Side effect: the bar was cached in a per-row bitmap refreshed every
  five seconds because the map was expensive to draw; a rectangle is not, so
  the bar is now always in step with the percentage printed on it. (`fa9b5be`)

### Fixed
- **The preferences dialog was cramped in two ways**: every row sat against the
  next one, and the dialog opened at exactly the size its tightest page
  demanded. Rows now get a 6 DIP gap and pages 8 DIP of padding, applied by
  walking each page's sizer tree rather than by editing fifteen layout
  functions in a file shared with upstream. The dialog opens at 1000×660 DIP,
  floored by what the widest page needs and capped by the screen. (`3ee602e`)
- **Toolbar labels were unreadable on the dark scheme**, and so were check box
  and radio button labels. The native controls paint their own text in the
  system colour and never look at the window's: the toolbar needs
  `NM_CUSTOMDRAW` with `TBCDRF_USECDCOLORS`, and it has to read `uItemState`
  so a checked button — whose pale blue fill Windows still draws — keeps a
  dark label rather than a white one on blue. Check boxes and radio buttons
  have their visual styles switched off per control so they fall back to
  classic drawing, which takes the colour from the device context.
  (`1dbe89f`, `fc1a669`)

### Known issues
- **Group box frames and their labels are still native**, which on the dark
  scheme means a light frame and dark caption. Taking over the painting was
  tried and reverted (`40b02f5`): `wxStaticBox` computes a paint region that
  excludes the controls sitting inside the group, and replacing its paint
  handler loses that, so the box paints over its own contents and they come
  back only when something else invalidates them. The supported way through
  is a subclass overriding `PaintForeground()`.
- **The menu bar follows Windows, not the scheme.** It is drawn in the frame's
  non-client area and is not a window a colour can be set on; reaching it
  means the undocumented `WM_UAHDRAWMENU` messages.

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
