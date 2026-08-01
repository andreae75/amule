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

#include "KadRoutingWnd.h"	// Interface declarations

#include <algorithm>		// Needed for std::max

#include <wx/dcbuffer.h>
#include <wx/listctrl.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include <common/Format.h>	// Needed for CFormat

#include "amule.h"		// Needed for theApp
#include "MuleColour.h"		// Needed for CMuleColour
#include "NetworkFunctions.h"	// Needed for KadIPToString
#include "OtherFunctions.h"	// Needed for CastSecondsToHM

// Bucket capacity. Not pulled from the Kad headers on purpose: amulegui
// compiles this file and has no Kademlia sources, and the snapshot is
// deliberately free of Kad dependencies. It is only used to scale the fill
// display, so a stale value would misdraw, not misbehave.
static const unsigned KAD_BUCKET_SIZE = 10;

// Fired by the map when the user picks a leaf; carries the zone index.
wxDEFINE_EVENT(MULE_EVT_KAD_ZONE_SELECTED, wxCommandEvent);

namespace {

enum ContactColumn {
	ContactColumnID = 0,
	ContactColumnAddress,
	ContactColumnTCP,
	ContactColumnVersion,
	ContactColumnType,
	ContactColumnVerified,
	ContactColumnAge
};

//! Colour of a leaf cell, from empty to full.
//
// Two ramps rather than one: a colour picked to read against a white
// background disappears on a dark one, which is the bug 47617df fixed in the
// search list. Endpoints are chosen so even an empty bucket stays visible
// against the listbox background.
CMuleColour FillColour(double ratio)
{
	if (ratio < 0.0) {
		ratio = 0.0;
	} else if (ratio > 1.0) {
		ratio = 1.0;
	}

	const bool dark = CMuleColour::IsDarkBackground();

	const CMuleColour empty = dark ? CMuleColour(0x3a, 0x3a, 0x3a)
					   : CMuleColour(0xe4, 0xe4, 0xe4);
	const CMuleColour full  = dark ? CMuleColour(0x3d, 0xa5, 0xd9)
					   : CMuleColour(0x1f, 0x6f, 0xb2);

	return CMuleColour(
		(uint8_t)(empty.Red()   + (full.Red()   - empty.Red())   * ratio),
		(uint8_t)(empty.Green() + (full.Green() - empty.Green()) * ratio),
		(uint8_t)(empty.Blue()  + (full.Blue()  - empty.Blue())  * ratio));
}

//! Black or white, whichever the given background can carry.
wxColour LabelColour(const CMuleColour& background)
{
	return background.Luminance() < 140 ? *wxWHITE : *wxBLACK;
}

} // namespace


// ---------------------------------------------------------------------
// CKadZoneMapCtrl
// ---------------------------------------------------------------------

wxBEGIN_EVENT_TABLE(CKadZoneMapCtrl, wxWindow)
	EVT_PAINT(CKadZoneMapCtrl::OnPaint)
	EVT_SIZE(CKadZoneMapCtrl::OnSize)
	EVT_LEFT_DOWN(CKadZoneMapCtrl::OnMouseLeftDown)
	EVT_MOTION(CKadZoneMapCtrl::OnMouseMotion)
	EVT_LEAVE_WINDOW(CKadZoneMapCtrl::OnMouseLeave)
wxEND_EVENT_TABLE()


CKadZoneMapCtrl::CKadZoneMapCtrl(wxWindow* parent, wxWindowID id)
	: wxWindow(parent, id, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE),
	  m_snapshot(NULL),
	  m_selected(-1),
	  m_hovered(-1)
{
	// Required by wxAutoBufferedPaintDC, and the reason this control does
	// not flicker while the tree churns under it.
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetMinSize(wxSize(240, 120));
}


void CKadZoneMapCtrl::SetSnapshot(const KadRoutingSnapshot* snapshot)
{
	m_snapshot = snapshot;

	// Keep the selection across a refresh when it still means the same
	// thing. The tree is rebuilt in the same order every time, so an
	// index that still points at a leaf points at the same leaf; when it
	// does not, dropping the selection beats showing someone else's
	// contacts under the old heading.
	if (m_selected >= 0) {
		const bool stillALeaf = snapshot
			&& m_selected < (int)snapshot->zones.size()
			&& snapshot->zones[m_selected].IsLeaf();
		if (!stillALeaf) {
			m_selected = -1;
		}
	}

	m_hovered = -1;
	Refresh(false);
}


void CKadZoneMapCtrl::OnSize(wxSizeEvent& evt)
{
	Refresh(false);
	evt.Skip();
}


void CKadZoneMapCtrl::OnPaint(wxPaintEvent& WXUNUSED(evt))
{
	wxAutoBufferedPaintDC dc(this);

	const CMuleColour background(wxSYS_COLOUR_LISTBOX);
	dc.SetBackground(background.GetBrush());
	dc.Clear();

	m_cells.clear();

	const wxSize size = GetClientSize();

	if (!m_snapshot || !m_snapshot->valid || m_snapshot->zones.empty()) {
		// Nothing to draw is not an error: Kad may simply be off. The
		// panel's summary line says which, so here just stay quiet.
		return;
	}

	dc.SetTextForeground(MuleTheme::GetColour(wxSYS_COLOUR_WINDOWTEXT));

	// One band per level. Bands get a floor so a deep tree stays legible
	// rather than collapsing into hairlines; the plot then simply uses
	// more room than it has and clips, which beats being unreadable.
	const int levels = (int)m_snapshot->maxDepth + 1;
	const int rowHeight = std::max(10, size.GetHeight() / std::max(1, levels));

	DrawZone(dc, 0, wxRect(0, 0, size.GetWidth(), rowHeight), rowHeight, size.GetHeight());
}


void CKadZoneMapCtrl::DrawZone(wxDC& dc, int zone, const wxRect& rect, int rowHeight, int bottom)
{
	if (zone < 0 || zone >= (int)m_snapshot->zones.size() || rect.GetWidth() <= 0) {
		return;
	}

	const KadZoneInfo& info = m_snapshot->zones[zone];

	if (!info.IsLeaf()) {
		// A split zone is not drawn itself: its two halves cover the
		// same span one level down, and drawing both would just paint
		// over it. Only the divider is worth a pixel.
		const int half = rect.GetWidth() / 2;
		const wxRect left(rect.GetX(), rect.GetY() + rowHeight, half, rowHeight);
		const wxRect right(rect.GetX() + half, rect.GetY() + rowHeight,
				   rect.GetWidth() - half, rowHeight);

		DrawZone(dc, info.child[0], left, rowHeight, bottom);
		DrawZone(dc, info.child[1], right, rowHeight, bottom);
		return;
	}

	// Leaves reach down to the bottom of the plot, so every column is
	// accounted for at every depth and the picture reads as a partition
	// of the ID space rather than as a ragged staircase.
	wxRect cell(rect.GetX(), rect.GetY(), rect.GetWidth(), std::max(rowHeight, bottom - rect.GetY()));

	const double ratio = (double)info.contacts.size() / (double)KAD_BUCKET_SIZE;
	CMuleColour fill = FillColour(ratio);

	dc.SetBrush(fill.GetBrush());
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.DrawRectangle(cell);

	// Hairline between neighbours: without it two equally full buckets
	// side by side look like one wide bucket.
	dc.SetPen(CMuleColour(wxSYS_COLOUR_LISTBOX).GetPen());
	dc.DrawLine(cell.GetRight(), cell.GetTop(), cell.GetRight(), cell.GetBottom());

	if (zone == m_selected) {
		dc.SetPen(wxPen(MuleTheme::GetColour(wxSYS_COLOUR_HIGHLIGHT), 2));
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		dc.DrawRectangle(cell.Deflate(1, 1));
	}

	// "n/K" if it fits. A cell too narrow for its own label is common
	// near the root of a well filled table, and a clipped number reads
	// as a wrong number.
	const wxString label = CFormat("%u/%u") % (unsigned)info.contacts.size() % KAD_BUCKET_SIZE;
	wxCoord textWidth, textHeight;
	dc.GetTextExtent(label, &textWidth, &textHeight);

	if (textWidth + 4 <= cell.GetWidth() && textHeight + 2 <= cell.GetHeight()) {
		dc.SetTextForeground(LabelColour(fill));
		dc.DrawText(label,
			cell.GetX() + (cell.GetWidth() - textWidth) / 2,
			cell.GetY() + (cell.GetHeight() - textHeight) / 2);
	}

	ZoneCell entry;
	entry.rect = cell;
	entry.zone = zone;
	m_cells.push_back(entry);
}


int CKadZoneMapCtrl::HitTest(const wxPoint& point) const
{
	for (std::vector<ZoneCell>::const_iterator it = m_cells.begin(); it != m_cells.end(); ++it) {
		if (it->rect.Contains(point)) {
			return it->zone;
		}
	}

	return -1;
}


void CKadZoneMapCtrl::OnMouseLeftDown(wxMouseEvent& evt)
{
	const int zone = HitTest(evt.GetPosition());
	if (zone == m_selected) {
		return;
	}

	m_selected = zone;
	Refresh(false);

	wxCommandEvent notify(MULE_EVT_KAD_ZONE_SELECTED, GetId());
	notify.SetEventObject(this);
	notify.SetInt(zone);
	ProcessWindowEvent(notify);
}


void CKadZoneMapCtrl::OnMouseMotion(wxMouseEvent& evt)
{
	const int zone = HitTest(evt.GetPosition());
	if (zone == m_hovered) {
		return;
	}

	m_hovered = zone;

	if (zone < 0 || !m_snapshot) {
		UnsetToolTip();
		return;
	}

	const KadZoneInfo& info = m_snapshot->zones[zone];
	SetToolTip(CFormat(_("Zone %s (level %u): %u of %u contacts"))
		% (info.zoneIndex.IsEmpty() ? wxString("root") : info.zoneIndex)
		% info.level
		% (unsigned)info.contacts.size()
		% KAD_BUCKET_SIZE);
}


void CKadZoneMapCtrl::OnMouseLeave(wxMouseEvent& WXUNUSED(evt))
{
	m_hovered = -1;
	UnsetToolTip();
}


// ---------------------------------------------------------------------
// CKadRoutingWnd
// ---------------------------------------------------------------------

wxBEGIN_EVENT_TABLE(CKadRoutingWnd, wxPanel)
wxEND_EVENT_TABLE()


CKadRoutingWnd::CKadRoutingWnd(wxWindow* parent)
	: wxPanel(parent, wxID_ANY),
	  m_summary(NULL),
	  m_map(NULL),
	  m_contacts(NULL)
{
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

	m_summary = new wxStaticText(this, wxID_ANY, wxEmptyString);
	sizer->Add(m_summary, 0, wxALL | wxEXPAND, 4);

	m_map = new CKadZoneMapCtrl(this, wxID_ANY);
	sizer->Add(m_map, 2, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 4);

	m_contacts = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
		wxLC_REPORT | wxLC_SINGLE_SEL);
	m_contacts->InsertColumn(ContactColumnID,	_("Kad ID"),	wxLIST_FORMAT_LEFT, 260);
	m_contacts->InsertColumn(ContactColumnAddress,	_("Address"),	wxLIST_FORMAT_LEFT, 150);
	m_contacts->InsertColumn(ContactColumnTCP,	_("TCP port"),	wxLIST_FORMAT_LEFT,  70);
	m_contacts->InsertColumn(ContactColumnVersion,	_("Version"),	wxLIST_FORMAT_LEFT,  60);
	m_contacts->InsertColumn(ContactColumnType,	_("Type"),	wxLIST_FORMAT_LEFT,  50);
	m_contacts->InsertColumn(ContactColumnVerified,	_("Verified"),	wxLIST_FORMAT_LEFT,  70);
	m_contacts->InsertColumn(ContactColumnAge,	_("Known for"),	wxLIST_FORMAT_LEFT,  90);
	sizer->Add(m_contacts, 3, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 4);

	SetSizer(sizer);

	// Bound rather than put in the event table: the event type is created
	// at run time, and a static event table entry would depend on the
	// order two static initialisers happen to run in.
	m_map->Bind(MULE_EVT_KAD_ZONE_SELECTED, &CKadRoutingWnd::OnZoneSelected, this);

	UpdateSummary();
}


void CKadRoutingWnd::UpdateSnapshot()
{
	theApp->GetKadRoutingSnapshot(m_snapshot);

	m_map->SetSnapshot(&m_snapshot);
	UpdateSummary();
	ShowZoneContacts(m_map->GetSelection());
}


void CKadRoutingWnd::UpdateSummary()
{
	if (!m_snapshot.valid) {
#ifdef CLIENT_GUI
		// EC does not carry the routing table, and this build has no
		// Kad of its own. Say that, rather than show an empty tree
		// that would read as "Kad knows nobody".
		m_summary->SetLabel(_("The Kad routing table is only available with a local core."));
#else
		m_summary->SetLabel(_("Kad is not running."));
#endif
		return;
	}

	m_summary->SetLabel(CFormat(_("%u contacts in %u buckets, tree depth %u, %u per bucket - own ID %s"))
		% m_snapshot.totalContacts
		% m_snapshot.leafCount
		% m_snapshot.maxDepth
		% KAD_BUCKET_SIZE
		% m_snapshot.selfId);
}


void CKadRoutingWnd::OnZoneSelected(wxCommandEvent& evt)
{
	ShowZoneContacts(evt.GetInt());
}


void CKadRoutingWnd::ShowZoneContacts(int zone)
{
	m_contacts->DeleteAllItems();

	if (zone < 0 || zone >= (int)m_snapshot.zones.size()) {
		return;
	}

	const KadZoneInfo& info = m_snapshot.zones[zone];
	const time_t now = time(NULL);

	long row = 0;
	for (std::vector<KadContactInfo>::const_iterator it = info.contacts.begin();
		it != info.contacts.end(); ++it)
	{
		m_contacts->InsertItem(row, it->id);
		m_contacts->SetItem(row, ContactColumnAddress,
			CFormat("%s:%u") % KadIPToString(it->ip) % it->udpPort);
		m_contacts->SetItem(row, ContactColumnTCP, CFormat("%u") % it->tcpPort);
		m_contacts->SetItem(row, ContactColumnVersion, CFormat("%u") % it->version);
		m_contacts->SetItem(row, ContactColumnType, CFormat("%u") % it->type);
		m_contacts->SetItem(row, ContactColumnVerified, it->verified ? _("Yes") : _("No"));
		m_contacts->SetItem(row, ContactColumnAge,
			(it->created && it->created <= now)
				? CastSecondsToHM((uint32)(now - it->created))
				: _("Unknown"));
		++row;
	}
}
// File_checked_for_headers
