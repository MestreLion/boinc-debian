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
//
#ifndef _WIZ_NOTFOUNDPAGE_H_
#define _WIZ_NOTFOUNDPAGE_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "NotFoundPage.cpp"
#endif

/*!
 * CErrNotFoundPage class declaration
 */

class CErrNotFoundPage: public wxWizardPageEx
{    
    DECLARE_DYNAMIC_CLASS( CErrNotFoundPage )
    DECLARE_EVENT_TABLE()

public:
    /// Constructors
    CErrNotFoundPage( );

    CErrNotFoundPage( CBOINCBaseWizard* parent );

    /// Creation
    bool Create( CBOINCBaseWizard* parent );

    /// Creates the controls and sizers
    void CreateControls();

////@begin CErrNotFoundPage event handler declarations

    /// wxEVT_WIZARD_PAGE_CHANGED event handler for ID_ERRACCOUNTNOTFOUNDPAGE
    void OnPageChanged( wxWizardExEvent& event );

    /// wxEVT_WIZARD_CANCEL event handler for ID_ERRACCOUNTNOTFOUNDPAGE
    void OnCancel( wxWizardExEvent& event );

////@end CErrNotFoundPage event handler declarations

////@begin CErrNotFoundPage member function declarations

    /// Gets the previous page.
    virtual wxWizardPageEx* GetPrev() const;

    /// Gets the next page.
    virtual wxWizardPageEx* GetNext() const;

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end CErrNotFoundPage member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

////@begin CErrNotFoundPage member variables
    wxStaticText* m_pTitleStaticCtrl;
    wxStaticText* m_pDirectionsStaticCtrl;
////@end CErrNotFoundPage member variables
};


#endif // _WIZ_NOTFOUNDPAGE_H_
