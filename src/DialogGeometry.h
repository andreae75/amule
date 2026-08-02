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

#ifndef DIALOGGEOMETRY_H
#define DIALOGGEOMETRY_H

#include <wx/display.h>
#include <wx/window.h>


//! The size this fork's own top-level dialogs open at, before the screen
//! gets the last word. Chosen against a 1280x768 display -- the smallest
//! we design for -- leaving room for a task bar and a title bar. Passed
//! through wxWindow::FromDIP(), so it stays the same apparent size on a
//! scaled display.
const int MULE_DIALOG_W_DIP = 1000;
const int MULE_DIALOG_H_DIP = 660;


/**
 * Sizes and positions one of aMule's own dialogs.
 *
 * There is one rule and it lives here rather than in each dialog, so
 * that "the About box opens like the preferences do" stays true by
 * construction instead of by two constants that happen to match today.
 *
 * @param dialog the window to size; it is centred on its display.
 * @param floorSize a size the result must not go below -- what a dialog
 *        needs in order not to clip its own contents. wxDefaultSize
 *        means the dialog has nothing to protect, which is the case
 *        whenever its body scrolls.
 *
 * The screen always has the last word, in both directions: a dialog
 * taller than the display is worse than a cramped one, because its
 * buttons end up below the bottom edge and on Windows a window cannot
 * be dragged above the top of the screen to reach them.
 */
inline void MuleSizeDialog(wxWindow* dialog, const wxSize& floorSize = wxDefaultSize)
{
	wxSize target = dialog->FromDIP(wxSize(MULE_DIALOG_W_DIP, MULE_DIALOG_H_DIP));

	if (floorSize != wxDefaultSize) {
		target.IncTo(floorSize);
	}

	int displayIdx = wxDisplay::GetFromWindow(dialog);
	if (displayIdx == wxNOT_FOUND) {
		displayIdx = 0;
	}
	target.DecTo(wxDisplay(displayIdx).GetClientArea().GetSize());

	dialog->SetSize(target);
	dialog->Center();
}

#endif // DIALOGGEOMETRY_H
// File_checked_for_headers
