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

#include "MuleTheme.h"

#include <wx/window.h>

#ifdef __WINDOWS__
#include <wx/msw/wrapwin.h>
#include <dwmapi.h>
#endif


namespace {

//! The scheme the GUI last pushed in. Read from paint handlers, written
//! only from the main thread when the preference changes -- in the
//! monolithic build the core and the GUI share that thread (see the
//! empty CALL_APP_DATA_LOCK in amule.h), so a plain variable is enough.
ColourScheme g_scheme = Scheme_System;


// The dark palette.
//
// These are the canonical Qt Fusion dark values, which is deliberate:
// Fusion dark is what the reference this was modelled on looks like, and
// several thousand Qt applications have already had their contrast
// argued over on them. Inventing a palette here would mean re-litigating
// that with no new information.
//
// The one departure is HIGHLIGHTTEXT. Fusion pairs its blue selection
// with black text, which works when the selected text is the theme's own
// foreground. aMule's lists put *coloured status text* on the selected
// row -- the search-result states, the download states -- and those were
// picked to read against a dark background in 47617df. Black would fight
// them, so the selection keeps light text.
const wxColour DARK_WINDOW      (53, 53, 53);    // panels, dialog bodies
const wxColour DARK_TEXT        (255, 255, 255);
const wxColour DARK_BASE        (25, 25, 25);    // list and text-control interiors
const wxColour DARK_HIGHLIGHT   (42, 130, 218);
const wxColour DARK_SHADOW      (35, 35, 35);    // borders, sunken edges
const wxColour DARK_GRAYTEXT    (127, 127, 127); // disabled labels

} // namespace


namespace MuleTheme {

ColourScheme GetScheme()
{
	return g_scheme;
}


void SetScheme(ColourScheme scheme)
{
	g_scheme = scheme;
}


bool IsDark()
{
	switch (g_scheme) {
		case Scheme_Dark:
			return true;
		case Scheme_Light:
			return false;
		case Scheme_System:
		default:
			// Only the OS can answer this, and the answer can change
			// while aMule runs, so it is asked every time rather than
			// cached at startup.
			return wxSystemSettings::GetAppearance().IsDark();
	}
}


wxColour GetColour(wxSystemColour index)
{
	// Light is not a palette of its own: it is the platform's own
	// colours, which is what aMule has always drawn with. Keeping it
	// that way means opting out of the theme restores the previous
	// appearance exactly, rather than approximately.
	if (!IsDark()) {
		return wxSystemSettings::GetColour(index);
	}

	switch (index) {
		// Surfaces the UI sits on.
		case wxSYS_COLOUR_WINDOW:
		case wxSYS_COLOUR_BTNFACE:
		case wxSYS_COLOUR_MENU:
		case wxSYS_COLOUR_3DLIGHT:
		case wxSYS_COLOUR_APPWORKSPACE:
			return DARK_WINDOW;

		// Interiors that hold content: lists, text controls.
		case wxSYS_COLOUR_LISTBOX:
			return DARK_BASE;

		// Foregrounds.
		case wxSYS_COLOUR_WINDOWTEXT:
		case wxSYS_COLOUR_BTNTEXT:
		case wxSYS_COLOUR_LISTBOXTEXT:
		case wxSYS_COLOUR_MENUTEXT:
		case wxSYS_COLOUR_INFOTEXT:
			return DARK_TEXT;

		// Selection. See the note on HIGHLIGHTTEXT above.
		case wxSYS_COLOUR_HIGHLIGHT:
			return DARK_HIGHLIGHT;
		case wxSYS_COLOUR_HIGHLIGHTTEXT:
			return DARK_TEXT;

		// Edges and dividers.
		case wxSYS_COLOUR_BTNSHADOW:
		case wxSYS_COLOUR_3DDKSHADOW:
		case wxSYS_COLOUR_WINDOWFRAME:
			return DARK_SHADOW;

		case wxSYS_COLOUR_GRAYTEXT:
			return DARK_GRAYTEXT;

		default:
			// Anything not spelled out falls back to the platform.
			// That can look wrong, but it looks wrong in a way the
			// user can see and report; the alternative -- guessing --
			// produces black text on a black background and no clue
			// as to where it came from.
			return wxSystemSettings::GetColour(index);
	}
}

wxColour GetRowColour(bool alternate)
{
	const wxColour base = GetColour(wxSYS_COLOUR_LISTBOX);
	if (!alternate) {
		return base;
	}

	// Step away from the background rather than towards a fixed colour,
	// so the banding survives someone pointing this at a different
	// palette later: lift a dark base, deepen a light one. 12 out of 255
	// is roughly where the stripe stops being subliminal and stops short
	// of looking like two different lists stitched together.
	const int step = 12;
	if (IsDark()) {
		return wxColour(
			wxMin(base.Red()   + step, 255),
			wxMin(base.Green() + step, 255),
			wxMin(base.Blue()  + step, 255));
	}

	return wxColour(
		wxMax(base.Red()   - step, 0),
		wxMax(base.Green() - step, 0),
		wxMax(base.Blue()  - step, 0));
}


bool NeedsOwnerDrawnHeader()
{
	// Only when the platform theme and the palette disagree. In a light
	// scheme the native header is exactly right, and drawing it by hand
	// would throw away the platform's own look for nothing.
	return IsDark();
}


void ApplyTitleBar(wxWindow* window)
{
#ifdef __WINDOWS__
	if ((window == NULL) || !window->GetHandle()) {
		return;
	}

	const BOOL dark = IsDark() ? TRUE : FALSE;

	// DWMWA_USE_IMMERSIVE_DARK_MODE settled on 20 in Windows 10 build
	// 2004. Between 18985 and that it was 19, and before 18985 the
	// attribute did not exist. Ask for 20 and fall back to 19: a wrong
	// attribute number is refused with an error rather than acted on,
	// so trying both is safe and cheaper than version-sniffing the OS.
	if (FAILED(::DwmSetWindowAttribute(window->GetHandle(),
			DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark)))) {
		const DWORD DWMWA_USE_IMMERSIVE_DARK_MODE_PRE_20H1 = 19;
		::DwmSetWindowAttribute(window->GetHandle(),
			DWMWA_USE_IMMERSIVE_DARK_MODE_PRE_20H1, &dark, sizeof(dark));
	}
#else
	// Every other desktop draws window decorations from its own theme
	// and offers no per-window override worth reaching for.
	(void)window;
#endif
}

} // namespace MuleTheme
// File_checked_for_headers
