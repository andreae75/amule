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

#ifndef MULETHEME_H
#define MULETHEME_H

#include <wx/colour.h>
#include <wx/settings.h>


/**
 * The colour scheme the UI is drawn in.
 *
 * The values are written to the config file as plain integers, so they may
 * be appended to but never reordered. Scheme_System is deliberately zero:
 * a config file written by an older build has no key at all, and reads back
 * as 0 -- which has to mean "whatever this machine was already doing".
 */
enum ColourScheme
{
	Scheme_System = 0,
	Scheme_Light  = 1,
	Scheme_Dark   = 2
};


/**
 * Resolves the colours the UI paints itself with.
 *
 * aMule draws most of what the user actually looks at itself -- the lists
 * are the generic wxWidgets control (src/extern/wxWidgets/listctrl.cpp),
 * so even their column headers are ours -- and all of that painting asks
 * wxSystemSettings for its colours, directly or through CMuleColour. This
 * sits in front of those calls: when a scheme is in force, GetColour()
 * answers from a fixed palette instead of from the OS.
 *
 * That indirection is the whole mechanism. Every owner-drawn row, header,
 * progress bar and graph follows from here without knowing a theme exists.
 *
 * Deliberately independent of Preferences: this header is pulled in by
 * MuleColour.h, which reaches into the core, and a dependency on the
 * preference machinery would be a cycle. The GUI pushes the scheme in with
 * SetScheme() at startup and whenever the setting changes; a build that
 * never calls it -- amuled -- stays on Scheme_System forever, which is
 * exactly right for a program that draws nothing.
 */
namespace MuleTheme
{
	/** The scheme currently in force. */
	ColourScheme GetScheme();

	/**
	 * Sets the scheme. Only changes what later GetColour() calls answer;
	 * repainting whatever is already on screen is the caller's problem.
	 */
	void SetScheme(ColourScheme scheme);

	/**
	 * Whether the UI should currently be drawn dark.
	 *
	 * Scheme_System resolves here rather than at the call sites, because
	 * "system" is a question only the OS can answer and the answer can
	 * change while aMule is running.
	 */
	bool IsDark();

	/**
	 * The colour to paint with, standing in for
	 * wxSystemSettings::GetColour().
	 *
	 * Falls back to the system value for anything the palette does not
	 * spell out, so an unmapped colour degrades to the old behaviour
	 * rather than to black-on-black.
	 */
	wxColour GetColour(wxSystemColour index);

	/**
	 * The background for one row of a list, given whether it is an odd
	 * one.
	 *
	 * Banding is what makes a wide row scannable: the eye follows a
	 * stripe across ten columns where it loses a plain one. The lists
	 * are owner-drawn, so there is no wxLC_ALTERNATE to switch on --
	 * each of them fills its own row background, and this is the single
	 * definition they share.
	 *
	 * In a dark scheme the alternate band is a deep blue rather than a
	 * lighter grey. A neutral step has to stay subtle or it reads as two
	 * lists stitched together, and subtle is exactly what does not
	 * survive a row of coloured status text drawn on top of it. A hue
	 * change separates the bands at a glance without competing with the
	 * foreground. A light scheme keeps the neutral step: there the rows
	 * carry the platform's own colours and a blue band would be the one
	 * thing on screen that came from nowhere.
	 */
	wxColour GetRowColour(bool alternate);

	/**
	 * The background of a list's column headers.
	 *
	 * Its own colour rather than BTNFACE, because the header has to sit
	 * above both the window it is in and the rows below it -- three
	 * surfaces, three values. Only meaningful when
	 * NeedsOwnerDrawnHeader() is true; in a light scheme the platform
	 * draws the header and this is never consulted.
	 */
	wxColour GetHeaderColour();

	/**
	 * One end of the gradient a progress bar is filled with -- the top
	 * edge, or the bottom when @a bottom is set.
	 *
	 * The same blue in both schemes. A progress bar is not a surface the
	 * UI sits on, it is a measurement drawn on one, and it stays legible
	 * against either background.
	 */
	wxColour GetProgressColour(bool bottom);

	/**
	 * Whether the header of a list has to be painted by hand.
	 *
	 * wxRendererNative draws headers through the platform theme, which
	 * on Windows means a light header no matter what the rest of the
	 * window is doing -- and the label on top of it, drawn in the
	 * theme's own foreground, comes out invisible. When this returns
	 * true the caller must draw the header itself.
	 */
	bool NeedsOwnerDrawnHeader();

	/**
	 * Paints @a window and everything under it in the current scheme.
	 *
	 * The owner-drawn lists pick the palette up on their own, because
	 * they ask for every colour they use. Everything around them -- the
	 * panels the panes sit on, the toolbar, the labels, the log pane --
	 * is a stock wx control that asks the platform instead, and in
	 * wxWidgets 3.2 there is no application-wide switch to redirect
	 * that (3.3 adds MSWEnableDarkMode; see the note in GetColour()).
	 * So the colours are pushed down the tree by hand.
	 *
	 * Only the classes known to honour a colour on MSW are touched --
	 * see the body -- so buttons, choices and combo boxes keep their
	 * native look rather than ending up with an unreadable half-themed
	 * one. Call once the window's children all exist, and again on
	 * anything built later.
	 *
	 * Does nothing unless the scheme is dark: a light scheme is the
	 * platform's own colours, and setting them explicitly would only
	 * freeze the window against a later system theme change.
	 */
	void ApplyToWindowTree(wxWindow* window);

	/**
	 * Asks the window manager to draw this window's title bar dark.
	 *
	 * The title bar is not ours to paint -- it lives outside the client
	 * area -- so a dark window with a white caption is the one seam the
	 * palette cannot cover. Windows exposes a switch for it; everywhere
	 * else this does nothing, because the desktop environment already
	 * decorates windows to match its own theme.
	 *
	 * Call after the window has a native handle, and again if the scheme
	 * changes. Silently does nothing when the scheme is not dark or the
	 * OS is too old to know the attribute.
	 */
	void ApplyTitleBar(wxWindow* window);
}

#endif // MULETHEME_H
// File_checked_for_headers
