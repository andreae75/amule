//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#ifndef CLIENTVERSION_H
#define CLIENTVERSION_H

// RC_INVOKED is defined by windres when processing a .rc resource
// file. The Windows version-info resource (version.rc.in) only needs
// the VERSION_MJR/MIN/UPDATE macros below; pulling in config.h drags
// in -I${CMAKE_BINARY_DIR} which isn't on the windres command line for
// every Windows target (e.g. ed2k).
#ifndef RC_INVOKED
#include "config.h"		// Needed for VERSION
#endif

// eMule version used on old MuleInfo packet (unimportant).
#define	CURRENT_VERSION_SHORT			0x47

// This is only used to server login. It has no real "version" meaning anymore.
#define	EDONKEYVERSION				0x3c

// aMule version

// No more Mod Version unless we're cvs

// __GIT__ marks dev builds (MOD_VERSION_LONG = "aMule GIT").  CMake
// derives AMULE_TAGGED_RELEASE from `git describe --tags --exact-match`
// at configure time and forwards it via config.h, which makes the
// __GIT__ flag self-managing: tagged release builds suppress it
// automatically; source/dev builds keep it.  Manual override is still
// possible by setting -DAMULE_TAGGED_RELEASE on the cmake command line
// (e.g. when building from a tarball with no .git directory).
#ifndef AMULE_TAGGED_RELEASE
#define __GIT__
#endif

#ifndef VERSION
	#define VERSION "3.0.0-dev"
#endif

// ---------------------------------------------------------------------
// Fork identity
//
// This tree is a fork of upstream aMule, and carries two version numbers
// that are deliberately kept apart:
//
//   * The upstream version -- VERSION, VERSION_MJR/MIN/UPDATE and
//     MOD_VERSION_LONG. These go out on the wire: to servers via
//     ServerConnect.cpp and to peers via BaseClient.cpp. They stay
//     exactly as upstream shipped them, so on the eD2k network this
//     build is indistinguishable from stock aMule.
//   * The fork version -- AMULE_FORK_* below. Local identity only:
//     About box, startup banner, tray menu. Never transmitted.
//
// So bump AMULE_FORK_VERSION for changes made in this fork, and leave
// the upstream macros alone -- they track what this tree was forked
// from, and misreporting them would misrepresent the client to the
// network.
// ---------------------------------------------------------------------
#define	AMULE_FORK_NAME		"NeRvOuX Edition"
#define	AMULE_FORK_VERSION	"0.1.0"
#define	AMULE_FORK_TAG		" [" AMULE_FORK_NAME " " AMULE_FORK_VERSION "]"

// MOD_VERSION_LONG is what BaseClient.cpp sends to other clients as the
// eMule-lineage "mod version" tag -- it must stay upstream's string.
// MOD_VERSION_DISPLAY is the same thing plus the fork tag, for the UI.
// The two are spelled out per branch rather than composed, because the
// non-GIT MOD_VERSION_LONG is parenthesised and `(...) "literal"` is not
// string concatenation.
#ifdef __GIT__
	#define	MOD_VERSION_LONG		"aMule GIT"
	#define	MOD_VERSION_DISPLAY		("aMule GIT" AMULE_FORK_TAG)
#else
	#define	MOD_VERSION_LONG		("aMule " VERSION)
	#define	MOD_VERSION_DISPLAY		("aMule " VERSION AMULE_FORK_TAG)
#endif

#define	VERSION_MJR		3
#define	VERSION_MIN		0
#define	VERSION_UPDATE		0

#ifndef PACKAGE
#define PACKAGE "amule"
#endif

#endif // CLIENTVERSION_H
