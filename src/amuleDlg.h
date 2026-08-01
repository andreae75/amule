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

#ifndef AMULEDLG_H
#define AMULEDLG_H


#include <wx/archive.h>
#include <wx/aui/framemanager.h>	// Needed for wxAuiManager (dock layout)
#include <wx/filename.h>
#include <wx/frame.h>			// Needed for wxFrame
#include <wx/imaglist.h>
#include <wx/timer.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>

#include "Types.h"			// Needed for uint32
#include "StatisticsDlg.h"

class wxTimerEvent;
class wxTextCtrl;

class CIP2Country;
class CTransferWnd;
class CServerWnd;
class CSharedFilesWnd;
class CSearchDlg;
class CChatWnd;
class CKadDlg;
class CKadRoutingWnd;
class PrefsUnifiedDlg;


class CMuleTrayIcon;

struct PageType {
	wxWindow* page;
	wxString name;
};

#define MP_RESTORE	4001
#define MP_CONNECT	4002
#define MP_DISCONNECT	4003
#define MP_EXIT		4004

// View menu: one contiguous id per dockable view (indexed by
// CamuleDlg::DialogType), plus the layout reset. Kept in the same
// private 4xxx block as the tray menu ids above.
#define MP_VIEW_PANE_FIRST	4020
#define MP_VIEW_PANE_LAST	4029
#define MP_VIEW_RESET_LAYOUT	4030


#define DEFAULT_SIZE_X  800
#define DEFAULT_SIZE_Y  600


enum ClientSkinEnum {
	Client_Green_Smiley = 0,
	Client_Red_Smiley,
	Client_Yellow_Smiley,
	Client_Grey_Smiley,
	Client_White_Smiley,
	Client_ExtendedProtocol_Smiley,
	Client_SecIdent_Smiley,
	Client_BadGuy_Smiley,
	Client_CreditsGrey_Smiley,
	Client_CreditsYellow_Smiley,
	Client_Upload_Smiley,
	Client_Friend_Smiley,
	Client_eMule_Smiley,
	Client_mlDonkey_Smiley,
	Client_eDonkeyHybrid_Smiley,
	Client_aMule_Smiley,
	Client_lphant_Smiley,
	Client_Shareaza_Smiley,
	Client_xMule_Smiley,
	Client_Unknown,
	Client_InvalidRating_Smiley,
	Client_PoorRating_Smiley,
	Client_FairRating_Smiley,
	Client_GoodRating_Smiley,
	Client_ExcellentRating_Smiley,
	Client_CommentOnly_Smiley,
	Client_Encryption_Smiley,
	// Add items here.
	CLIENT_SKIN_SIZE
};


// CamuleDlg Dialogfeld
class CamuleDlg : public wxFrame
{
public:
	CamuleDlg(
		wxWindow *pParent = NULL,
		const wxString &title = "",
		wxPoint where = wxDefaultPosition,
		wxSize dlg_size = wxSize(DEFAULT_SIZE_X,DEFAULT_SIZE_Y));
	~CamuleDlg();

	void AddLogLine(const wxString& line);
	void AddServerMessageLine(wxString& message);
	void ResetLog(int id);

	void ShowUserCount(const wxString& info = "");
	void ShowConnectionState(bool skinChanged = false);
	void ShowTransferRate();

	bool StatisticsWindowActive()
		{ return IsDialogVisible(DT_STATS_WND); }

	/* Returns the active dialog. Needed to check what to redraw. */
	enum DialogType {
		DT_TRANSFER_WND,
		DT_NETWORKS_WND,
		DT_SEARCH_WND,
		DT_SHARED_WND,
		DT_CHAT_WND,
		DT_STATS_WND,
		DT_KAD_WND,	// this one is still unused
		DT_KADROUTING_WND
	};
	DialogType GetActiveDialog()
		{ return m_nActiveDialog; }

	/**
	 * Brings a view to the front: shows its pane if it was closed, gives
	 * it focus, and records it as the active one.
	 *
	 * The @a dlg argument is redundant now that panes are looked up by
	 * type, but the signature is kept so the toolbar handler and
	 * CChatWnd don't have to change.
	 */
	void SetActiveDialog(DialogType type, wxWindow* dlg);

	/**
	 * Helper function for deciding if a certain dlg is visible.
	 *
	 * Under the dock layout several views can be on screen at once, so
	 * this asks the pane rather than comparing against a single "active"
	 * window. Callers use it to skip expensive redraws, and a view
	 * that is docked next to the active one genuinely does need
	 * repainting.
	 *
	 * @return True if the dialog is visible to the user, false otherwise.
	 */
	bool IsDialogVisible( DialogType dlg );

	void ShowED2KLinksHandler( bool show );

	void DlgShutDown();
	void OnClose(wxCloseEvent& evt);
	void OnBnConnect(wxCommandEvent& evt);

	bool SafeState()	{ return m_is_safe_state; }

	void LaunchUrl(const wxString &url);

	void CreateSystray();
	void RemoveSystray();

	void StartGuiTimer()	{ gui_timer->Start(100); }
	void StopGuiTimer()	{ gui_timer->Stop(); }

	/**
	 * This function ensures that _all_ list widgets are properly sorted.
	 */
	void InitSort();

	void SetMessageBlink(bool state) { m_BlinkMessages = state; }
	void Create_Toolbar(bool orientation);

	void DoNetworkRearrange();

	CIP2Country*		m_IP2Country;
	void IP2CountryDownloadFinished(uint32 result);
	void EnableIP2Country();

	//! Last view brought to the front. Only meaningful as "what the user
	//! looked at most recently" -- use IsDialogVisible() to ask whether
	//! something is actually on screen.
	wxWindow*		m_activewnd;
	CTransferWnd*		m_transferwnd;
	CServerWnd*		m_serverwnd;
	CSharedFilesWnd*	m_sharedfileswnd;
	CSearchDlg*		m_searchwnd;
	CChatWnd*		m_chatwnd;
	CStatisticsDlg*		m_statisticswnd;
	CKadDlg*		m_kademliawnd;
	//! Kad routing table view. Its own pane, unlike m_kademliawnd.
	CKadRoutingWnd*		m_kadroutingwnd;
	//! Pointer to the current preference dialog, if any.
	PrefsUnifiedDlg*	m_prefsDialog;

	int			m_srv_split_pos;

	wxImageList m_imagelist;
	wxImageList m_tblist;

protected:
	void OnToolBarButton(wxCommandEvent& ev);
	void OnViewPane(wxCommandEvent& ev);
	void OnResetLayout(wxCommandEvent& ev);
	void OnAboutButton(wxCommandEvent& ev);

	/**
	 * The build's own description: versions, paths, host.
	 *
	 * Shown in the About box, where it can be selected and copied, so a
	 * bug report can carry it without anyone having to ask.
	 */
	wxString GetBuildInfo() const;
	void OnPrefButton(wxCommandEvent& ev);
	void OnImportButton(wxCommandEvent& ev);
	void OnMinimize(wxIconizeEvent& evt);
	void OnShow(wxShowEvent& evt);
	void OnBnClickedFast(wxCommandEvent& evt);
	void OnGUITimer(wxTimerEvent& evt);
	void OnMainGUISizeChange(wxSizeEvent& evt);
	void OnExit(wxCommandEvent& evt);

private:
	// ---- Dock layout ------------------------------------------------
	//
	// The seven views used to be siblings swapped in and out of a sizer,
	// one visible at a time. They are now panes of a wxAuiManager, so
	// any combination can be on screen, docked or floating.

	//! Panel the manager owns. It sits inside the old contentSizer slot
	//! so the eD2k link bar and the status bar below it stay ordinary
	//! sizer content -- a wxAuiManager claims its window's whole client
	//! area and would fight them for it.
	wxPanel* m_auiHost;
	wxAuiManager m_auiMgr;

	//! Stable, untranslated pane id. This is the key LoadPerspective()
	//! matches on, so it must never be run through _(): a language
	//! change would otherwise orphan every saved pane.
	static const wxString PaneName(DialogType type);
	//! Translated pane caption, for the title bar and the View menu.
	static const wxString PaneCaption(DialogType type);
	//! The window behind a view, or NULL for types without a pane
	//! (DT_KAD_WND lives inside the networks notebook, not in a pane).
	wxWindow* PaneWindow(DialogType type) const;

	void CreateAuiPanes();
	void ApplyDefaultPerspective();
	void CreateViewMenu();
	void SyncViewMenu();

	//! Specifies if the prefs-dialog was shown before minimizing.
	bool m_prefsVisible;
	wxToolBar *m_wndToolbar;
	wxTimer *gui_timer;
	CMuleTrayIcon *m_wndTaskbarNotifier;
	DialogType m_nActiveDialog;
	bool m_is_safe_state;
	bool m_BlinkMessages;
	int m_CurrentBlinkBitmap;
	uint32 m_last_iconizing;

public:
	// Track iconize state from wxIconizeEvent::IsIconized(), which is
	// reliable across platforms — unlike wxFrame::IsIconized() which
	// can return false on wxGTK after a minimize-button click while
	// the OS still has the window iconized. Tray menu and DoShowHide
	// consult this to decide whether the window is "visible to the
	// user" so the "Show aMule"/"Hide aMule" label and the click
	// action stay in sync with reality.
	bool IsTrayLogicallyIconized() const { return m_iconized_logical; }

private:
	bool m_iconized_logical = false;
	wxFileName m_skinFileName;
	std::vector<wxString> m_clientSkinNames;
	bool m_GeoIPavailable;

	WX_DECLARE_STRING_HASH_MAP(wxZipEntry*, ZipCatalog);
	ZipCatalog cat;

	PageType m_logpages[4];
	PageType m_networkpages[2];

	bool LoadGUIPrefs(bool override_pos, bool override_size);
	bool SaveGUIPrefs();

	void UpdateTrayIcon(int percent);

	void Apply_Clients_Skin();
	void Apply_Toolbar_Skin(wxToolBar *wndToolbar);
	bool Check_and_Init_Skin();
	void Add_Skin_Icon(const wxString &iconName, const wxBitmap &stdIcon, bool useSkins);
	void ToogleED2KLinksHandler();
	void SetMessagesTool();
	void OnKeyPressed(wxKeyEvent& evt);

	wxDECLARE_EVENT_TABLE();
};

#endif

// File_checked_for_headers
