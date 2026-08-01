//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
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

#ifndef KADROUTINGSNAPSHOT_H
#define KADROUTINGSNAPSHOT_H

#include <vector>
#include <wx/string.h>

#include "Types.h"		// Needed for uint8, uint16, uint32


/**
 * A copy of the Kad routing table, taken at one instant, for display.
 *
 * The routing table is a binary tree of zones over the 128 bit ID space,
 * centered on our own ID, whose leaves hold up to K contacts each. It lives
 * in the core (src/kademlia/routing/) and is rewritten by the Kad timers as
 * nodes come and go; the GUI must not hold on to any part of it.
 *
 * So this is values only -- no CContact pointers survive the snapshot, and a
 * snapshot stays valid and consistent no matter what Kad does afterwards.
 * These are plain structs with no Kademlia dependency, which is also what
 * lets the display code compile into amulegui, where there is no Kad at all.
 */

//! One contact, as a leaf zone holds it.
struct KadContactInfo
{
	KadContactInfo()
		: ip(0), udpPort(0), tcpPort(0), version(0), type(0),
		  verified(false), created(0)
	{ }

	//! Kad ID, as a hex string.
	wxString	id;
	//! Host byte order, ready for Uint32toStringIP().
	uint32		ip;
	uint16		udpPort;
	uint16		tcpPort;
	//! Kad protocol version spoken by the contact.
	uint8		version;
	//! Liveliness, 0 (just seen) to 4 (about to be dropped).
	uint8		type;
	//! Whether the contact's IP has been verified by a challenge.
	bool		verified;
	//! When we first learned of the contact.
	time_t		created;
};


//! One node of the zone tree. Either a leaf with contacts, or a split zone
//! with two children -- never both, and never something in between.
struct KadZoneInfo
{
	KadZoneInfo()
		: level(0)
	{
		child[0] = child[1] = -1;
	}

	bool IsLeaf() const { return child[0] < 0; }

	//! 0 is the whole ID space, 1 is a half, 2 a quarter, and so on.
	uint32		level;
	//! Index of this zone within its level, as a binary string of
	//! `level` digits. Empty for the root.
	wxString	zoneIndex;
	//! Indices into KadRoutingSnapshot::zones, or -1 for a leaf. The
	//! tree is flattened into a vector and addressed by index rather
	//! than by pointer, so the whole snapshot copies and dies as one
	//! object with no ownership to track.
	int		child[2];
	//! Contacts, for a leaf zone. Always empty for a split zone.
	std::vector<KadContactInfo>	contacts;
};


//! The whole tree, plus the totals worth showing next to it.
struct KadRoutingSnapshot
{
	KadRoutingSnapshot()
		: valid(false), totalContacts(0), leafCount(0), maxDepth(0)
	{ }

	//! False when Kad is not running: there is no table to show, which
	//! is a different thing from a table that happens to be empty.
	bool		valid;
	//! Our own Kad ID, the centre the tree is organised around.
	wxString	selfId;
	uint32		totalContacts;
	uint32		leafCount;
	//! Number of edges on the longest path from the root to a leaf.
	uint32		maxDepth;
	//! The zone tree, flattened. Index 0 is the root when non-empty.
	std::vector<KadZoneInfo>	zones;
};

#endif // KADROUTINGSNAPSHOT_H
