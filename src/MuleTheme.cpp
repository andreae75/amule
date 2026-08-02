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
#include <wx/checkbox.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/radiobut.h>
#include <wx/splitter.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/toolbar.h>
#include <wx/toplevel.h>

#ifdef __WINDOWS__
#include <wx/msw/wrapwin.h>
#include <dwmapi.h>
#include <uxtheme.h>
#endif


namespace {

#ifdef __WINDOWS__
//! Turns visual styles off for one control.
//!
//! A themed check box, radio button or group box has its *label* painted
//! by the theme engine, which reads the system text colour and never
//! looks at the one set on the window -- so on a light Windows theme the
//! label comes out near-black on our dark panel and no amount of
//! SetOwnForegroundColour changes it. Without visual styles the control
//! falls back to classic drawing, which takes the text colour from the
//! device context, which is where wx puts ours. The glyph itself becomes
//! the classic 3D one; that is the price, and it is a legible price.
void DisableVisualStyles(wxWindow* window)
{
	if (window->GetHandle() != NULL) {
		::SetWindowTheme(static_cast<HWND>(window->GetHandle()), L"", L"");
	}
}
#endif

//! The scheme the GUI last pushed in. Read from paint handlers, written
//! only from the main thread when the preference changes -- in the
//! monolithic build the core and the GUI share that thread (see the
//! empty CALL_APP_DATA_LOCK in amule.h), so a plain variable is enough.
ColourScheme g_scheme = Scheme_System;


// The dark palette.
//
// Sampled, pixel by pixel, from the reference this fork is being made to
// look like -- not derived from a formula and not invented here. That is
// the point: the values below are already a working theme somewhere, so
// their relationships (list lighter than window, header lighter than
// list, the band blue rather than grey) have been looked at by someone
// on a real screen. Numbers picked here would only be a guess at those
// relationships.
//
// It started as canonical Qt Fusion, which puts the list interior at 25
// -- *darker* than its 53 window. This palette runs the other way: the
// content is the lightest surface and the chrome recedes behind it.
//
// HIGHLIGHTTEXT departs from Fusion, which pairs its blue selection with
// black text. That works when the selected text is the theme's own
// foreground. aMule's lists put *coloured status text* on the selected
// row -- the search-result states, the download states -- and those were
// picked to read against a dark background in 47617df. Black would fight
// them, so the selection keeps light text.
const wxColour DARK_WINDOW      (46, 46, 46);    // panels, dialog bodies, toolbar
const wxColour DARK_TEXT        (255, 255, 255);
const wxColour DARK_BASE        (57, 57, 57);    // list and text-control interiors
const wxColour DARK_ROW_ALT     (13, 36, 105);   // the banded row
const wxColour DARK_HEADER      (79, 79, 79);    // column headers
const wxColour DARK_HIGHLIGHT   (42, 130, 218);
const wxColour DARK_SHADOW      (38, 38, 38);    // borders, sunken edges, dividers
const wxColour DARK_GRAYTEXT    (127, 127, 127); // disabled labels

// Progress bars, in both schemes. See GetProgressColour().
const wxColour PROGRESS_TOP     (4, 127, 226);
const wxColour PROGRESS_BOTTOM  (4, 111, 195);

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

	// A hue change, not a brightness change. The first cut of this
	// stepped 12/255 away from the background, which is the textbook
	// answer and is what a light scheme still gets below -- but on dark
	// it disappeared under the coloured status text the download list
	// draws on every row. Blue is far enough from the greys that the
	// band survives anything painted over it, and far enough from the
	// selection blue (42,130,218) that the two never read as the same
	// state.
	if (IsDark()) {
		return DARK_ROW_ALT;
	}

	// Light keeps the neutral step: those rows are the platform's own
	// colours, and a band that came from our palette instead of the
	// system's would be the one thing on screen that matched nothing.
	const int step = 12;
	return wxColour(
		wxMax(base.Red()   - step, 0),
		wxMax(base.Green() - step, 0),
		wxMax(base.Blue()  - step, 0));
}


wxColour GetHeaderColour()
{
	if (!IsDark()) {
		return wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
	}

	return DARK_HEADER;
}


wxColour GetProgressColour(bool bottom)
{
	// Not routed through the scheme at all. Both ends of this gradient
	// clear 3:1 against every surface in either palette, so there is
	// nothing for a light variant to fix -- and one colour means a
	// screenshot of a transfer looks the same whoever took it.
	return bottom ? PROGRESS_BOTTOM : PROGRESS_TOP;
}


bool NeedsOwnerDrawnHeader()
{
	// Only when the platform theme and the palette disagree. In a light
	// scheme the native header is exactly right, and drawing it by hand
	// would throw away the platform's own look for nothing.
	return IsDark();
}


void ApplyToWindowTree(wxWindow* window)
{
	if ((window == NULL) || !IsDark()) {
		return;
	}

	// Whether a control honours SetOwnBackgroundColour on MSW is not
	// something wx will tell us, so the classes that get repainted are
	// listed rather than discovered by colouring everything and seeing
	// what sticks. Buttons, choices, combo boxes and spin controls are
	// left off deliberately: the native control draws itself through
	// the platform theme and ignores the background while honouring the
	// foreground, which is how you get white text on a light grey
	// button. Half-themed is worse than untouched.
	// Controls whose label the theme engine paints for them. They take
	// the same colours as everything else, but only after their visual
	// styles are switched off -- see DisableVisualStyles().
	const bool themePaintsTheLabel =
		(wxDynamicCast(window, wxCheckBox) != NULL)
		|| (wxDynamicCast(window, wxRadioButton) != NULL)
		|| (wxDynamicCast(window, wxStaticBox) != NULL);

	if (wxDynamicCast(window, wxTextCtrl) != NULL) {
		// Content rather than chrome -- the log pane, the search boxes
		// -- so it takes the list interior colour and sits in the same
		// family as the lists beside it.
		window->SetOwnBackgroundColour(GetColour(wxSYS_COLOUR_LISTBOX));
		window->SetOwnForegroundColour(GetColour(wxSYS_COLOUR_LISTBOXTEXT));
	} else if (themePaintsTheLabel
			|| (wxDynamicCast(window, wxPanel) != NULL)
			|| (wxDynamicCast(window, wxNotebook) != NULL)
			|| (wxDynamicCast(window, wxToolBar) != NULL)
			|| (wxDynamicCast(window, wxSplitterWindow) != NULL)
			|| (wxDynamicCast(window, wxStaticText) != NULL)
			|| (wxDynamicCast(window, wxTopLevelWindow) != NULL)) {
		window->SetOwnBackgroundColour(GetColour(wxSYS_COLOUR_WINDOW));
		window->SetOwnForegroundColour(GetColour(wxSYS_COLOUR_WINDOWTEXT));

#ifdef __WINDOWS__
		if (themePaintsTheLabel) {
			DisableVisualStyles(window);
		}
#endif
	}

	const wxWindowList& children = window->GetChildren();
	for (wxWindowList::const_iterator it = children.begin();
			it != children.end(); ++it) {
		ApplyToWindowTree(*it);
	}
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
