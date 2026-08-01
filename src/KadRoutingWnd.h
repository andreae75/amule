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

#ifndef KADROUTINGWND_H
#define KADROUTINGWND_H

#include <vector>

#include <wx/panel.h>
#include <wx/window.h>

#include "KadRoutingSnapshot.h"

class wxListCtrl;
class wxStaticText;


/**
 * The zone tree, drawn as an icicle plot.
 *
 * One row per level of the tree; the root spans the full width and every
 * split halves the space, so the horizontal axis is the 128 bit ID space and
 * a cell's width is the share of it that zone covers. Leaves are filled by
 * how full their bucket is, and reach down to the bottom of the plot, so
 * every column is accounted for at every depth.
 *
 * Owns nothing: the snapshot belongs to the parent panel and is handed over
 * by pointer, which stays valid because both are refreshed together.
 */
class CKadZoneMapCtrl : public wxWindow
{
public:
	CKadZoneMapCtrl(wxWindow* parent, wxWindowID id);

	/**
	 * Points the control at a new snapshot and redraws.
	 *
	 * The selected zone is kept if it still exists and is still a leaf,
	 * so a refresh does not yank the contact list out from under a user
	 * who is reading it.
	 */
	void SetSnapshot(const KadRoutingSnapshot* snapshot);

	//! Index into the snapshot's zone vector, or -1 for none.
	int GetSelection() const { return m_selected; }

private:
	void OnPaint(wxPaintEvent& evt);
	void OnSize(wxSizeEvent& evt);
	void OnMouseLeftDown(wxMouseEvent& evt);
	void OnMouseMotion(wxMouseEvent& evt);
	void OnMouseLeave(wxMouseEvent& evt);

	/**
	 * Draws one zone and recurses into its children.
	 *
	 * @param rowHeight Height of a single level's band.
	 * @param bottom Y coordinate the leaves extend down to.
	 *
	 * Also (re)builds the hit-test table, so painting and hit-testing
	 * can never disagree about where a cell is.
	 */
	void DrawZone(wxDC& dc, int zone, const wxRect& rect, int rowHeight, int bottom);

	//! The zone whose cell contains the point, or -1.
	int HitTest(const wxPoint& point) const;

	//! Cell rectangles, in paint order. Rebuilt by every OnPaint.
	struct ZoneCell {
		wxRect	rect;
		int	zone;
	};
	std::vector<ZoneCell>	m_cells;

	const KadRoutingSnapshot*	m_snapshot;
	int				m_selected;
	//! Zone under the pointer, for the tooltip. -1 when outside.
	int				m_hovered;

	wxDECLARE_EVENT_TABLE();
};


/**
 * Kad routing table view: summary, zone map, and the contacts of the
 * selected leaf.
 *
 * Read-only, and refreshed from the GUI timer rather than from Kad, so it
 * costs nothing while it is hidden.
 */
class CKadRoutingWnd : public wxPanel
{
public:
	CKadRoutingWnd(wxWindow* parent);

	/**
	 * Takes a fresh snapshot from the core and redraws.
	 *
	 * Cheap enough to call on a timer -- a few hundred contacts copied --
	 * but the caller should still skip it while the pane is hidden.
	 */
	void UpdateSnapshot();

private:
	void OnZoneSelected(wxCommandEvent& evt);

	//! Fills the contact list from a leaf zone, or empties it.
	void ShowZoneContacts(int zone);

	//! The summary line above the map.
	void UpdateSummary();

	KadRoutingSnapshot	m_snapshot;

	wxStaticText*		m_summary;
	CKadZoneMapCtrl*	m_map;
	wxListCtrl*		m_contacts;

	wxDECLARE_EVENT_TABLE();
};

#endif // KADROUTINGWND_H
// File_checked_for_headers
