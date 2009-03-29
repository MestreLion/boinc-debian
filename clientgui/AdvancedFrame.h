// Berkeley Open Infrastructure for Network Computing
// http://boinc.berkeley.edu
// Copyright (C) 2005 University of California
//
// This is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation;
// either version 2.1 of the License, or (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU Lesser General Public License for more details.
//
// To view the GNU Lesser General Public License visit
// http://www.gnu.org/copyleft/lesser.html
// or write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA


#ifndef _ADVANCEDFRAME_H_
#define _ADVANCEDFRAME_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "AdvancedFrame.cpp"
#endif

class CStatusBar : public wxStatusBar
{
    DECLARE_DYNAMIC_CLASS(CStatusBar)

public:
    CStatusBar();
    CStatusBar(wxWindow *parent);
    ~CStatusBar();

    void OnSize(wxSizeEvent& event);

    wxStaticBitmap* m_pbmpConnected;
    wxStaticText*   m_ptxtConnected;
    wxStaticBitmap* m_pbmpDisconnect;
    wxStaticText*   m_ptxtDisconnect;

private:
    DECLARE_EVENT_TABLE()
};


class CAdvancedFrame : public CBOINCBaseFrame
{
    DECLARE_DYNAMIC_CLASS(CAdvancedFrame)

public:
    CAdvancedFrame();
    CAdvancedFrame(wxString title, wxIcon* icon, wxIcon* icon32);

    ~CAdvancedFrame(void);

    void OnSwitchView( wxCommandEvent& event );
    void OnSwitchGUI( wxCommandEvent& event );

    void OnActivitySelection( wxCommandEvent& event );
    void OnNetworkSelection( wxCommandEvent& event );

    void OnProjectsAttachToProject( wxCommandEvent& event );
    void OnProjectsAttachToAccountManager( wxCommandEvent& event );
    void OnAccountManagerUpdate( wxCommandEvent& event );
    void OnAccountManagerDetach( wxCommandEvent& event );

    void OnOptionsOptions( wxCommandEvent& event );
	void OnDlgPreferences( wxCommandEvent& event );
    void OnSelectComputer( wxCommandEvent& event );
    void OnClientShutdown( wxCommandEvent& event );
    void OnRunBenchmarks( wxCommandEvent& event );
    void OnCommandsRetryCommunications( wxCommandEvent& event );
    void Onread_prefs( wxCommandEvent& event );
    void Onread_config( wxCommandEvent& event );

    void OnHelp( wxHelpEvent& event );
    void OnHelpBOINC( wxCommandEvent& event );
    void OnHelpAbout( wxCommandEvent& event );

    void OnShow( wxShowEvent& event );

    void OnRefreshState( wxTimerEvent& event );
    void OnFrameRender( wxTimerEvent& event );
    void OnListPanelRender( wxTimerEvent& event );

    void OnNotebookSelectionChanged( wxNotebookEvent& event );

    void OnRefreshView( CFrameEvent& event );
    void OnConnect( CFrameEvent& event );
    void OnUpdateStatus( CFrameEvent& event );

    void ResetReminderTimers();

    wxTimer*        m_pRefreshStateTimer;
    wxTimer*        m_pFrameRenderTimer;
    wxTimer*        m_pFrameListPanelRenderTimer;

private:

    wxMenuBar*      m_pMenubar;
    wxNotebook*     m_pNotebook;
    CStatusBar*     m_pStatusbar;

    bool            m_bDisplayShutdownClientWarning;
	wxInt32			m_iDisplayViewType;

    wxString        m_strBaseTitle;

    bool            CreateMenu();
    bool            DeleteMenu();

    bool            CreateNotebook();
    bool            RepopulateNotebook();
    template < class T >
        bool        CreateNotebookPage( T pwndNewNotebookPage );
    bool            DeleteNotebook();

    bool            CreateStatusbar();
    bool            DeleteStatusbar();

    bool            SaveState();
    bool            SaveViewState();
    bool            RestoreState();
    bool            RestoreViewState();

    void            SaveWindowDimensions();
    void            RestoreWindowDimensions();

    void            UpdateActivityModeControls( CC_STATUS& status );
    void            UpdateNetworkModeControls( CC_STATUS& status );
    void            UpdateRefreshTimerInterval( wxInt32 iCurrentNotebookPage );

    void            StartTimers();
    void            StopTimers();

#ifdef __WXMAC__
protected:

    wxAcceleratorEntry  m_Shortcuts[1];     // For HELP keyboard shortcut
    wxAcceleratorTable* m_pAccelTable;
#endif

    DECLARE_EVENT_TABLE()
};


#endif

