/////////////////////////////////////////////////////////////////////////////
// Name:            ConnectomicWindow.h
// Author:          ---
// Creation Date:   ---
//
// Description: This is the implementation file for the Connectomic method.
/////////////////////////////////////////////////////////////////////////////


#ifndef CONNECTOMIC_H_
#define CONNECTOMIC_H_

#include "MainCanvas.h"
#include "MyListCtrl.h"

#include "../misc/Algorithms/Helper.h"

#include <wx/scrolwin.h>
#include <wx/statline.h>

class MainFrame;
class wxToggleButton;

class ConnectomicWindow: public wxScrolledWindow
{
public:
    ConnectomicWindow(){};
    ConnectomicWindow( wxWindow *pParent, MainFrame *pMf, wxWindowID id, const wxPoint &pos, const wxSize &size );

    ~ConnectomicWindow(){};
    void OnPaint( wxPaintEvent &event );
    void OnSize( wxSizeEvent &event );
    wxSizer* getWindowSizer();
	
public:
	void onSelectLabels                ( wxCommandEvent& event );
    void onSelectEdges                 ( wxCommandEvent& event );
    void OnNbLabels                    ( wxCommandEvent& event );
    void OnSliderDisplayMoved          ( wxCommandEvent& event );
    void OnToggleFlashyEdges           ( wxCommandEvent& event );
    void OnToggleOrientationDep        ( wxCommandEvent& event );
	
	
	
private:
    MainFrame           *m_pMainFrame;
	wxButton            *m_pBtnSelectLabels;
    wxButton            *m_pBtnSelectEdges;
    wxTextCtrl          *m_pNbLabels;
    wxTextCtrl          *m_pTxtBoxNbLabels;

    wxStaticText        *m_pTextNodeSize;
	wxSlider            *m_pSliderNodeSize;
	wxTextCtrl          *m_pTxtNodeSizeBox;
    wxStaticText        *m_pTextNodeAlpha;
	wxSlider            *m_pSliderNodeAlpha;
	wxTextCtrl          *m_pTxtNodeAlphaBox;

    wxStaticText        *m_pTextEdgeSize;
	wxSlider            *m_pSliderEdgeSize;
	wxTextCtrl          *m_pTxtEdgeSizeBox;
    wxStaticText        *m_pTextEdgeAlpha;
	wxSlider            *m_pSliderEdgeAlpha;
	wxTextCtrl          *m_pTxtEdgeAlphaBox;

    wxToggleButton		*m_pToggleFlashyEdges;
    wxToggleButton      *m_pToggleOrientationDep;
	
    
private:
    wxSizer *m_pConnectomicSizer;
    ConnectomicWindow( wxWindow *pParent, wxWindowID id, const wxPoint &pos, const wxSize &size );
    DECLARE_DYNAMIC_CLASS( ConnectomicWindow )
    DECLARE_EVENT_TABLE()
};

#endif /*CONNECTOMIC*/
