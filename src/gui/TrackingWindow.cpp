#include "TrackingWindow.h"
#include "FMRIWindow.h"

#include "MainFrame.h"
#include "SceneManager.h"
#include "SelectionBox.h"
#include "SelectionEllipsoid.h"
#include "../main.h"
#include "../dataset/Anatomy.h"
#include "../dataset/Fibers.h"
#include "../dataset/ODFs.h"
#include "../dataset/RTTrackingHelper.h"
#include "../dataset/RTFMRIHelper.h"
#include "../dataset/Tensors.h"
#include "../dataset/Maximas.h"
#include "../misc/IsoSurface/CIsoSurface.h"
#include "../misc/IsoSurface/TriangleMesh.h"

#include <wx/checkbox.h>
#include <wx/grid.h>
#include <wx/tglbtn.h>
#include <wx/treectrl.h>


IMPLEMENT_DYNAMIC_CLASS( TrackingWindow, wxScrolledWindow )

BEGIN_EVENT_TABLE( TrackingWindow, wxScrolledWindow )
END_EVENT_TABLE()


TrackingWindow::TrackingWindow( wxWindow *pParent, MainFrame *pMf, wxWindowID id, const wxPoint &pos, const wxSize &size )
:   wxScrolledWindow( pParent, id, pos, size, wxBORDER_NONE, _T("DTI RTT Canvas") ),
    m_pMainFrame( pMf )
{
    SetBackgroundColour( *wxLIGHT_GREY );
    m_pTrackingSizer = new wxBoxSizer( wxVERTICAL );
    SetSizer( m_pTrackingSizer );
    SetAutoLayout( true );

    //Content of RTT panel
    /********************************/
#if !_USE_ZOOM_GUI
int zoomS = 100;
int zoomH = 30;
int slider1 = 70;
int slider2 = 100;
int slider3 = 55;
#else
int zoomS = 300;
int zoomH = 60;
int slider1 = 200;
int slider2 = 250;
int slider3 = 150;
#endif
    

    m_pBtnSelectFile = new wxButton( this, wxID_ANY,wxT("DTI not selected"), wxDefaultPosition, wxSize(zoomS, -1) );
    Connect( m_pBtnSelectFile->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(TrackingWindow::OnSelectFileDTI) );
    m_pBtnSelectFile->SetBackgroundColour(wxColour( 255, 147, 147 ));

    m_pBtnStart = new wxToggleButton( this, wxID_ANY,wxT("Start tracking"), wxDefaultPosition, wxSize(zoomS, -1) );
    Connect( m_pBtnStart->GetId(), wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler(TrackingWindow::OnStartTracking) );
    m_pBtnStart->Enable(false);

	m_pBtnSelectShell = new wxButton( this, wxID_ANY,wxT("Shell not selected"), wxDefaultPosition, wxSize(zoomS, -1) );
    Connect( m_pBtnSelectShell->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(TrackingWindow::OnSelectShell) );

    m_pToggleShell = new wxToggleButton( this, wxID_ANY,wxT("Shell seed OFF"), wxDefaultPosition, wxSize(zoomS, -1) );
    Connect( m_pToggleShell->GetId(), wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler(TrackingWindow::OnShellSeeding) );
	m_pToggleShell->Enable(false);

    m_pTextFA = new wxStaticText( this, wxID_ANY, wxT("Min FA"), wxDefaultPosition, wxSize(zoomH*2, -1), wxALIGN_CENTER );
    m_pSliderFA = new MySlider( this, wxID_ANY, 0, 1, 50, wxDefaultPosition, wxSize(slider2, -1), wxSL_HORIZONTAL | wxSL_AUTOTICKS );
    m_pSliderFA->SetValue( 20 );
    Connect( m_pSliderFA->GetId(), wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler(TrackingWindow::OnSliderFAMoved) );
    m_pTxtFABox = new wxTextCtrl( this, wxID_ANY, wxT("0.20"), wxDefaultPosition, wxSize(slider3, -1), wxTE_CENTRE | wxTE_READONLY );
    
    wxStaticText *m_pTextAngle = new wxStaticText( this, wxID_ANY, wxT("Max angle"), wxDefaultPosition, wxSize(zoomH*2, -1), wxALIGN_CENTER );
    m_pSliderAngle = new MySlider( this, wxID_ANY, 0, 1, 90, wxDefaultPosition, wxSize(slider2, -1), wxSL_HORIZONTAL | wxSL_AUTOTICKS );
    m_pSliderAngle->SetValue( 60 );
    Connect( m_pSliderAngle->GetId(), wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler(TrackingWindow::OnSliderAngleMoved) );
    m_pTxtAngleBox = new wxTextCtrl( this, wxID_ANY, wxT("60.0 "), wxDefaultPosition, wxSize(slider3, -1), wxTE_CENTRE | wxTE_READONLY );

    wxStaticText *m_pTextStep = new wxStaticText( this, wxID_ANY, wxT("Step"), wxDefaultPosition, wxSize(zoomH*2, -1), wxALIGN_CENTER );
    m_pSliderStep = new MySlider( this, wxID_ANY, 0, 5, 20, wxDefaultPosition, wxSize(slider2, -1), wxSL_HORIZONTAL | wxSL_AUTOTICKS );
    m_pSliderStep->SetValue( 10 );
    Connect( m_pSliderStep->GetId(), wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler(TrackingWindow::OnSliderStepMoved) );
    m_pTxtStepBox = new wxTextCtrl( this, wxID_ANY, wxT("1.0 mm"), wxDefaultPosition, wxSize(slider3, -1), wxTE_CENTRE | wxTE_READONLY );

    wxStaticText *m_pTextPuncture = new wxStaticText( this, wxID_ANY, wxT("Vin-Vout"), wxDefaultPosition, wxSize(zoomH*2, -1), wxALIGN_CENTER );
    m_pSliderPuncture = new MySlider( this, wxID_ANY, 0, 0, 10, wxDefaultPosition, wxSize(slider2, -1), wxSL_HORIZONTAL | wxSL_AUTOTICKS );
    m_pSliderPuncture->SetValue( 2 );
    Connect( m_pSliderPuncture->GetId(), wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler(TrackingWindow::OnSliderPunctureMoved) );
    m_pTxtPunctureBox = new wxTextCtrl( this, wxID_ANY, wxT("0.2"), wxDefaultPosition, wxSize(slider3, -1), wxTE_CENTRE | wxTE_READONLY );

    wxStaticText *m_pTextMinLength = new wxStaticText( this, wxID_ANY, wxT("Min length"), wxDefaultPosition, wxSize(zoomH*2, -1), wxALIGN_CENTER );
    m_pSliderMinLength = new MySlider( this, wxID_ANY, 0, 0, 400, wxDefaultPosition, wxSize(slider2, -1), wxSL_HORIZONTAL | wxSL_AUTOTICKS );
    m_pSliderMinLength->SetValue( 60 );
    Connect( m_pSliderMinLength->GetId(), wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler(TrackingWindow::OnSliderMinLengthMoved) );
    m_pTxtMinLengthBox = new wxTextCtrl( this, wxID_ANY, wxT("60 mm"), wxDefaultPosition, wxSize(slider3, -1), wxTE_CENTRE | wxTE_READONLY );

    wxStaticText *m_pTextMaxLength = new wxStaticText( this, wxID_ANY, wxT("Max length"), wxDefaultPosition, wxSize(zoomH*2, -1), wxALIGN_CENTER );
    m_pSliderMaxLength = new MySlider( this, wxID_ANY, 0, 0, 300, wxDefaultPosition, wxSize(slider2, -1), wxSL_HORIZONTAL | wxSL_AUTOTICKS );
    m_pSliderMaxLength->SetValue( 200 );
    Connect( m_pSliderMaxLength->GetId(), wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler(TrackingWindow::OnSliderMaxLengthMoved) );
    m_pTxtMaxLengthBox = new wxTextCtrl( this, wxID_ANY, wxT("200 mm"), wxDefaultPosition, wxSize(slider3, -1), wxTE_CENTRE | wxTE_READONLY );

    wxStaticText *m_pTextAxisSeedNb = new wxStaticText( this, wxID_ANY, wxT("Seed/axis"), wxDefaultPosition, wxSize(zoomH*2, -1), wxALIGN_CENTER );
	RTTrackingHelper::getInstance()->m_pSliderAxisSeedNb = new MySlider( this, wxID_ANY, 0, 1, 15, wxDefaultPosition, wxSize(130, -1), wxSL_HORIZONTAL | wxSL_AUTOTICKS );
    RTTrackingHelper::getInstance()->m_pSliderAxisSeedNb->SetValue( 10 );
    Connect( RTTrackingHelper::getInstance()->m_pSliderAxisSeedNb->GetId(), wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler(TrackingWindow::OnSliderAxisSeedNbMoved) );
    RTTrackingHelper::getInstance()->m_pTxtAxisSeedNbBox = new wxTextCtrl( this, wxID_ANY, wxT("10"), wxDefaultPosition, wxSize(slider3, -1), wxTE_CENTRE | wxTE_READONLY );

    wxStaticText *m_pTextTotalSeedNb = new wxStaticText( this, wxID_ANY, wxT("Number of current seeds"), wxDefaultPosition, wxSize(150, -1), wxALIGN_LEFT );
    RTTrackingHelper::getInstance()->m_pTxtTotalSeedNbBox = new wxTextCtrl( this, wxID_ANY, wxT("1000"), wxDefaultPosition, wxSize(slider3, -1), wxTE_CENTRE | wxTE_READONLY );

	m_pBtnConvert = new wxButton( this, wxID_ANY,wxT("Convert Fibers"), wxDefaultPosition, wxSize(140, 30) );
	Connect( m_pBtnConvert->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(TrackingWindow::OnConvertToFibers) );

    wxTextCtrl *deprecated = new wxTextCtrl( this, wxID_ANY, wxT("Not maintained anymore."), wxDefaultPosition, wxSize(230, -1), wxTE_CENTER | wxTE_READONLY );
    deprecated->SetBackgroundColour( *wxLIGHT_GREY );
    wxFont deprec_font = deprecated->GetFont();
    deprec_font.SetPointSize( 10 );
    deprec_font.SetWeight( wxFONTWEIGHT_BOLD );
    deprecated->SetFont( deprec_font );

}

TrackingWindow::TrackingWindow( wxWindow *pParent, MainFrame *pMf, wxWindowID id, const wxPoint &pos, const wxSize &size, int hardi)
:   wxScrolledWindow( pParent, id, pos, size, wxBORDER_NONE, _T("HARDI RTT Canvas") ),
    m_pMainFrame( pMf )
{
    SetBackgroundColour( *wxLIGHT_GREY );
    m_pTrackingSizer = new wxBoxSizer( wxVERTICAL );
    SetSizer( m_pTrackingSizer );
    SetAutoLayout( true );

    //Content of RTT panel
    /********************************/
#if !_USE_ZOOM_GUI
int zoomS = 115;
int zoomH = 30;
int slider1 = 70;
int slider2 = 100;
int slider3 = 55;
#else
int zoomS = 300;
int zoomH = 60;
int slider1 = 200;
int slider2 = 250;
int slider3 = 150;
#endif

	wxBoxSizer *pBoxRow = new wxBoxSizer( wxHORIZONTAL );
	wxStaticText *m_pTextThreshold  = new wxStaticText( this, wxID_ANY, wxT("From Chamberland et al. (2014)"), wxDefaultPosition, wxSize(zoomS*2, -1),  wxALIGN_CENTER );
	pBoxRow->Add( m_pTextThreshold, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 1);
	m_pTrackingSizer->Add( pBoxRow, 0, wxFIXED_MINSIZE | wxALL, 2 );

    m_pBtnSelectFile = new wxButton( this, wxID_ANY,wxT("Select Peaks"), wxDefaultPosition, wxSize(zoomS, -1) );
    Connect( m_pBtnSelectFile->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(TrackingWindow::OnSelectFileHARDI) );
    m_pBtnSelectFile->SetBackgroundColour(wxColour( 255, 147, 147 ));

    m_pBtnStart = new wxToggleButton( this, wxID_ANY,wxT("Start tracking"), wxDefaultPosition, wxSize(zoomS, -1));
    Connect( m_pBtnStart->GetId(), wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler(TrackingWindow::OnStartTracking) );
    m_pBtnStart->Enable(false);

	wxBoxSizer *pBoxRow1 = new wxBoxSizer( wxHORIZONTAL );
	pBoxRow1->Add( m_pBtnSelectFile, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 1 );
    pBoxRow1->Add( m_pBtnStart,   0, wxALIGN_CENTER_HORIZONTAL | wxALL, 1);
	m_pTrackingSizer->Add( pBoxRow1, 0, wxFIXED_MINSIZE | wxALL, 2 );

    m_pBtnSelectMap = new wxButton( this, wxID_ANY,wxT("Mask (e.g. WM, FA, etc) not selected"), wxDefaultPosition, wxSize(zoomS*2, -1) );
    Connect( m_pBtnSelectMap->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(TrackingWindow::OnSelectMask) );
    m_pBtnSelectMap->SetBackgroundColour(wxColour( 255, 147, 147 ));

	wxBoxSizer *pBoxRow4 = new wxBoxSizer( wxHORIZONTAL );
    pBoxRow4->Add( m_pBtnSelectMap, 0, wxALIGN_CENTER | wxALL, 1 );
	m_pTrackingSizer->Add( pBoxRow4, 0, wxFIXED_MINSIZE | wxALL, 2 );

    m_pBtnSelectGM = new wxButton( this, wxID_ANY,wxT("GM mask?"), wxDefaultPosition, wxSize(zoomS, -1) );
    Connect( m_pBtnSelectGM->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(TrackingWindow::OnSelectGM) );

    m_pToggleGMmap = new wxToggleButton( this, wxID_ANY,wxT("GM Map OFF"), wxDefaultPosition, wxSize(zoomS, -1) );
    Connect( m_pToggleGMmap->GetId(), wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler(TrackingWindow::OnToggleGM) );
	m_pToggleGMmap->Enable(false);

	wxBoxSizer *pBoxRowGM = new wxBoxSizer( wxHORIZONTAL );
    pBoxRowGM->Add( m_pBtnSelectGM, 0, wxALIGN_CENTER | wxALL, 1 );
    pBoxRowGM->Add( m_pToggleGMmap,   0, wxALIGN_CENTER | wxALL, 1);
	m_pTrackingSizer->Add( pBoxRowGM, 0, wxFIXED_MINSIZE | wxALL, 2 );

	m_pBtnSelectSeed = new wxButton( this, wxID_ANY,wxT("Seed map?"), wxDefaultPosition, wxSize(zoomS, -1) );
    Connect( m_pBtnSelectSeed->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(TrackingWindow::OnSelectSeedMap) );

    m_pToggleSeedMap = new wxToggleButton( this, wxID_ANY,wxT("Seed Map OFF"), wxDefaultPosition, wxSize(zoomS, -1) );
    Connect( m_pToggleSeedMap->GetId(), wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler(TrackingWindow::OnMapSeeding) );

	m_pToggleSeedMap->Enable(false);

	wxBoxSizer *pBoxRow3 = new wxBoxSizer( wxHORIZONTAL );
    pBoxRow3->Add( m_pBtnSelectSeed, 0, wxALIGN_CENTER | wxALL, 1 );
    pBoxRow3->Add( m_pToggleSeedMap,   0, wxALIGN_CENTER | wxALL, 1);
	m_pTrackingSizer->Add( pBoxRow3, 0, wxFIXED_MINSIZE | wxALL, 2 );

    m_pBtnSelectShell = new wxButton( this, wxID_ANY,wxT("Shell seeding?"), wxDefaultPosition, wxSize(zoomS, -1) );
    Connect( m_pBtnSelectShell->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(TrackingWindow::OnSelectShell) );

    m_pToggleShell = new wxToggleButton( this, wxID_ANY,wxT("Shell seed OFF"), wxDefaultPosition, wxSize(zoomS, -1) );
    Connect( m_pToggleShell->GetId(), wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler(TrackingWindow::OnShellSeeding) );
	m_pToggleShell->Enable(false);

	wxBoxSizer *pBoxRow2 = new wxBoxSizer( wxHORIZONTAL );
    pBoxRow2->Add( m_pBtnSelectShell, 0, wxALIGN_CENTER | wxALL, 1 );
    pBoxRow2->Add( m_pToggleShell,   0, wxALIGN_CENTER | wxALL, 1);
	m_pTrackingSizer->Add( pBoxRow2, 0, wxFIXED_MINSIZE | wxALL, 2 );  

	m_pBtnSelectNotMap = new wxButton( this, wxID_ANY,wxT("NOT map?"), wxDefaultPosition, wxSize(zoomS, -1) );
    Connect( m_pBtnSelectNotMap->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(TrackingWindow::OnSelectExclusion) );

    m_pToggleNotMap = new wxToggleButton( this, wxID_ANY,wxT("NOT Map OFF"), wxDefaultPosition, wxSize(zoomS, -1) );
    Connect( m_pToggleNotMap->GetId(), wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler(TrackingWindow::OnToggleNotMap) );
	m_pToggleGMmap->Enable(false);

	wxBoxSizer *pBoxRowEx = new wxBoxSizer( wxHORIZONTAL );
    pBoxRowEx->Add( m_pBtnSelectNotMap, 0, wxALIGN_CENTER | wxALL, 1 );
    pBoxRowEx->Add( m_pToggleNotMap,   0, wxALIGN_CENTER | wxALL, 1);
	m_pTrackingSizer->Add( pBoxRowEx, 0, wxFIXED_MINSIZE | wxALL, 2 );

    m_pBtnSelectAndMap = new wxButton( this, wxID_ANY,wxT("AND map?"), wxDefaultPosition, wxSize(zoomS, -1) );
    Connect( m_pBtnSelectAndMap->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(TrackingWindow::OnSelectInclusion) );

    m_pToggleAndMap = new wxToggleButton( this, wxID_ANY,wxT("AND Map OFF"), wxDefaultPosition, wxSize(zoomS, -1));
    Connect( m_pToggleAndMap->GetId(), wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler(TrackingWindow::OnToggleAndMap) );
	m_pToggleAndMap->Enable(false);

	wxBoxSizer *pBoxRowInc = new wxBoxSizer( wxHORIZONTAL );
    pBoxRowInc->Add( m_pBtnSelectAndMap, 0, wxALIGN_CENTER | wxALL, 1 );
    pBoxRowInc->Add( m_pToggleAndMap,   0, wxALIGN_CENTER | wxALL, 1);
	m_pTrackingSizer->Add( pBoxRowInc, 0, wxFIXED_MINSIZE | wxALL, 2 );


    m_pToggleRandomInit = new wxToggleButton( this, wxID_ANY,wxT("Randomly spaced seeds"), wxDefaultPosition, wxSize(2*zoomS, -1) );
    Connect( m_pToggleRandomInit->GetId(), wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler(TrackingWindow::OnRandomSeeding) );

    wxBoxSizer *pBoxRowRand = new wxBoxSizer( wxHORIZONTAL );
    pBoxRowRand->Add( m_pToggleRandomInit, 0, wxALIGN_CENTER | wxALL, 1 );
	m_pTrackingSizer->Add( pBoxRowRand, 0, wxFIXED_MINSIZE | wxALL, 2 );
   
    wxBoxSizer *pBoxFlips = new wxBoxSizer( wxHORIZONTAL );
    pBoxFlips->Add(new wxStaticText( this, wxID_ANY, wxT( "Init. dir." ), wxDefaultPosition, wxSize(30, -1), wxALIGN_CENTER ), 1, wxEXPAND | wxALL, 1 );
    m_pToggleTrackX = new wxToggleButton( this, wxID_ANY, wxT( "L-R" ), wxDefaultPosition, wxSize( 30, -1 ) );
    m_pToggleTrackY = new wxToggleButton( this, wxID_ANY, wxT( "A-P" ), wxDefaultPosition, wxSize( 30, -1 ) );
    m_pToggleTrackZ = new wxToggleButton( this, wxID_ANY, wxT( "I-S" ), wxDefaultPosition, wxSize( 30, -1 ) );
    Connect( m_pToggleTrackX->GetId(),         wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( TrackingWindow::OnInitX ) );
    Connect( m_pToggleTrackY->GetId(),         wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( TrackingWindow::OnInitY ) );
    Connect( m_pToggleTrackZ->GetId(),         wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( TrackingWindow::OnInitZ ) );
    
    pBoxFlips->Add( m_pToggleTrackX, 1, wxEXPAND | wxALL, 1 );
    pBoxFlips->Add( m_pToggleTrackY, 1, wxEXPAND | wxALL, 1 );
    pBoxFlips->Add( m_pToggleTrackZ, 1, wxEXPAND | wxALL, 1 );
    m_pTrackingSizer->Add( pBoxFlips,0, wxFIXED_MINSIZE | wxEXPAND, 0 );

    m_pTextFA = new wxStaticText( this, wxID_ANY, wxT("Min Mask"), wxDefaultPosition, wxSize(slider1, -1), wxALIGN_CENTER );
    m_pSliderFA = new MySlider( this, wxID_ANY, 0, 1, 100, wxDefaultPosition, wxSize(slider2, -1), wxSL_HORIZONTAL | wxSL_AUTOTICKS );
    m_pSliderFA->SetValue( 15 );
    Connect( m_pSliderFA->GetId(), wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler(TrackingWindow::OnSliderFAMoved) );
    m_pTxtFABox = new wxTextCtrl( this, wxID_ANY, wxT("0.15"), wxDefaultPosition, wxSize(slider3, -1), wxTE_CENTRE | wxTE_READONLY );
    m_pTextFA->Enable(false);
    m_pSliderFA->Enable(false);
    m_pTxtFABox->Enable(false);

	wxBoxSizer *pBoxRow5 = new wxBoxSizer( wxHORIZONTAL );
    pBoxRow5->Add( m_pTextFA, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxALL, 1 );
    pBoxRow5->Add( m_pSliderFA,   0, wxALIGN_LEFT | wxEXPAND | wxALL, 1);
	pBoxRow5->Add( m_pTxtFABox,   0, wxALIGN_LEFT | wxALL, 1);
	m_pTrackingSizer->Add( pBoxRow5, 0, wxFIXED_MINSIZE | wxEXPAND, 0 );

    wxStaticText *m_pTextAngle = new wxStaticText( this, wxID_ANY, wxT("Max angle"), wxDefaultPosition, wxSize(slider1, -1), wxALIGN_CENTER );
    m_pSliderAngle = new MySlider( this, wxID_ANY, 0, 1, 90, wxDefaultPosition, wxSize(slider2, -1), wxSL_HORIZONTAL | wxSL_AUTOTICKS );
    m_pSliderAngle->SetValue( 40 );
    Connect( m_pSliderAngle->GetId(), wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler(TrackingWindow::OnSliderAngleMoved) );
    m_pTxtAngleBox = new wxTextCtrl( this, wxID_ANY, wxT("40.0 "), wxDefaultPosition, wxSize(slider3, -1), wxTE_CENTRE | wxTE_READONLY );

		wxBoxSizer *pBoxRow6 = new wxBoxSizer( wxHORIZONTAL );
    pBoxRow6->Add( m_pTextAngle, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxALL, 1 );
    pBoxRow6->Add( m_pSliderAngle,   0, wxALIGN_LEFT | wxEXPAND | wxALL, 1);
	pBoxRow6->Add( m_pTxtAngleBox,   0, wxALIGN_LEFT | wxALL, 1);
	m_pTrackingSizer->Add( pBoxRow6, 0, wxFIXED_MINSIZE | wxEXPAND, 0 );

    wxStaticText *m_pTextStep = new wxStaticText( this, wxID_ANY, wxT("Step"), wxDefaultPosition, wxSize(slider1, -1), wxALIGN_CENTER );
    m_pSliderStep = new MySlider( this, wxID_ANY, 0, 1, 20, wxDefaultPosition, wxSize(slider2, -1), wxSL_HORIZONTAL | wxSL_AUTOTICKS );
    m_pSliderStep->SetValue( 10 );
    Connect( m_pSliderStep->GetId(), wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler(TrackingWindow::OnSliderStepMoved) );
    m_pTxtStepBox = new wxTextCtrl( this, wxID_ANY, wxT("1.0 mm"), wxDefaultPosition, wxSize(slider3, -1), wxTE_CENTRE | wxTE_READONLY );

	wxBoxSizer *pBoxRow7 = new wxBoxSizer( wxHORIZONTAL );
    pBoxRow7->Add( m_pTextStep, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxALL, 1 );
    pBoxRow7->Add( m_pSliderStep,   0, wxALIGN_LEFT | wxEXPAND | wxALL, 1);
	pBoxRow7->Add( m_pTxtStepBox,   0, wxALIGN_LEFT | wxALL, 1);
	m_pTrackingSizer->Add( pBoxRow7, 0, wxFIXED_MINSIZE | wxEXPAND, 0 );

    wxStaticText *m_pTextPuncture = new wxStaticText( this, wxID_ANY, wxT("g (weight)"), wxDefaultPosition, wxSize(slider1, -1), wxALIGN_CENTER );
    m_pSliderPuncture = new MySlider( this, wxID_ANY, 0, 40, 100, wxDefaultPosition, wxSize(slider2, -1), wxSL_HORIZONTAL | wxSL_AUTOTICKS );
    m_pSliderPuncture->SetValue( 60 );
    Connect( m_pSliderPuncture->GetId(), wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler(TrackingWindow::OnSliderPunctureMoved) );
    m_pTxtPunctureBox = new wxTextCtrl( this, wxID_ANY, wxT("0.60"), wxDefaultPosition, wxSize(slider3, -1), wxTE_CENTRE | wxTE_READONLY );

	wxBoxSizer *pBoxRow8 = new wxBoxSizer( wxHORIZONTAL );
    pBoxRow8->Add( m_pTextPuncture, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxALL, 1 );
    pBoxRow8->Add( m_pSliderPuncture,   0, wxALIGN_LEFT | wxEXPAND | wxALL, 1);
	pBoxRow8->Add( m_pTxtPunctureBox,   0, wxALIGN_LEFT | wxALL, 1);
	m_pTrackingSizer->Add( pBoxRow8, 0, wxFIXED_MINSIZE | wxEXPAND, 0 );

    wxStaticText *m_pTextGMStep = new wxStaticText( this, wxID_ANY, wxT("GM steps"), wxDefaultPosition, wxSize(slider1, -1), wxALIGN_CENTER );
    m_pSliderGMStep = new MySlider( this, wxID_ANY, 0, 0, 300, wxDefaultPosition, wxSize(slider2, -1), wxSL_HORIZONTAL | wxSL_AUTOTICKS );
    m_pSliderGMStep->SetValue( 5 );
    Connect( m_pSliderGMStep->GetId(), wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler(TrackingWindow::OnSliderGMStepMoved) );
    m_pTxtGMStepBox = new wxTextCtrl( this, wxID_ANY, wxT("5"), wxDefaultPosition, wxSize(slider3, -1), wxTE_CENTRE | wxTE_READONLY );

	wxBoxSizer *pBoxRowGMstep = new wxBoxSizer( wxHORIZONTAL );
    pBoxRowGMstep->Add( m_pTextGMStep, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxALL, 1 );
    pBoxRowGMstep->Add( m_pSliderGMStep,   0, wxALIGN_LEFT | wxEXPAND | wxALL, 1);
	pBoxRowGMstep->Add( m_pTxtGMStepBox,   0, wxALIGN_LEFT | wxALL, 1);
	m_pTrackingSizer->Add( pBoxRowGMstep, 0, wxFIXED_MINSIZE | wxEXPAND, 0 );

    wxStaticText *m_pTextMinLength = new wxStaticText( this, wxID_ANY, wxT("Min length"), wxDefaultPosition, wxSize(slider1, -1), wxALIGN_CENTER );
    m_pSliderMinLength = new MySlider( this, wxID_ANY, 0, 0, 400, wxPoint(60,240), wxSize(slider2, -1), wxSL_HORIZONTAL | wxSL_AUTOTICKS );
    m_pSliderMinLength->SetValue( 30 );
    Connect( m_pSliderMinLength->GetId(), wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler(TrackingWindow::OnSliderMinLengthMoved) );
    m_pTxtMinLengthBox = new wxTextCtrl( this, wxID_ANY, wxT("30 mm"), wxDefaultPosition, wxSize(slider3, -1), wxTE_CENTRE | wxTE_READONLY );

	wxBoxSizer *pBoxRow9 = new wxBoxSizer( wxHORIZONTAL );
    pBoxRow9->Add( m_pTextMinLength, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxALL, 1 );
    pBoxRow9->Add( m_pSliderMinLength,   0, wxALIGN_LEFT | wxEXPAND | wxALL, 1);
	pBoxRow9->Add( m_pTxtMinLengthBox,   0, wxALIGN_LEFT | wxALL, 1);
	m_pTrackingSizer->Add( pBoxRow9, 0, wxFIXED_MINSIZE | wxEXPAND, 0 );

    wxStaticText *m_pTextMaxLength = new wxStaticText( this, wxID_ANY, wxT("Max length"), wxDefaultPosition, wxSize(slider1, -1), wxALIGN_CENTER );
    m_pSliderMaxLength = new MySlider( this, wxID_ANY, 0, 0, 300, wxDefaultPosition, wxSize(slider2, -1), wxSL_HORIZONTAL | wxSL_AUTOTICKS );
    m_pSliderMaxLength->SetValue( 200 );
    Connect( m_pSliderMaxLength->GetId(), wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler(TrackingWindow::OnSliderMaxLengthMoved) );
    m_pTxtMaxLengthBox = new wxTextCtrl( this, wxID_ANY, wxT("200 mm"), wxDefaultPosition, wxSize(slider3, -1), wxTE_CENTRE | wxTE_READONLY );

	wxBoxSizer *pBoxRow10 = new wxBoxSizer( wxHORIZONTAL );
    pBoxRow10->Add( m_pTextMaxLength, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxALL, 1 );
    pBoxRow10->Add( m_pSliderMaxLength,   0, wxALIGN_LEFT | wxEXPAND | wxALL, 1);
	pBoxRow10->Add( m_pTxtMaxLengthBox,   0, wxALIGN_LEFT | wxALL, 1);
	m_pTrackingSizer->Add( pBoxRow10, 0, wxFIXED_MINSIZE | wxEXPAND, 0 );

    wxStaticText *m_pTextAxisSeedNb = new wxStaticText( this, wxID_ANY, wxT("Seeds (^3)"), wxDefaultPosition, wxSize(slider1, -1), wxALIGN_CENTER );
    RTTrackingHelper::getInstance()->m_pSliderAxisSeedNb = new MySlider( this, wxID_ANY, 0, 1, 20, wxDefaultPosition, wxSize(slider2, -1), wxSL_HORIZONTAL | wxSL_AUTOTICKS );
    RTTrackingHelper::getInstance()->m_pSliderAxisSeedNb->SetValue( 10 );
    Connect( RTTrackingHelper::getInstance()->m_pSliderAxisSeedNb->GetId(), wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler(TrackingWindow::OnSliderAxisSeedNbMoved) );
    RTTrackingHelper::getInstance()->m_pTxtAxisSeedNbBox = new wxTextCtrl( this, wxID_ANY, wxT("10"), wxDefaultPosition, wxSize(slider3, -1), wxTE_CENTRE | wxTE_READONLY );

	wxBoxSizer *pBoxRow11 = new wxBoxSizer( wxHORIZONTAL );
    pBoxRow11->Add( m_pTextAxisSeedNb, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxALL, 1 );
    pBoxRow11->Add( RTTrackingHelper::getInstance()->m_pSliderAxisSeedNb,   0, wxALIGN_LEFT | wxEXPAND | wxALL, 1);
	pBoxRow11->Add( RTTrackingHelper::getInstance()->m_pTxtAxisSeedNbBox,   0, wxALIGN_LEFT | wxALL, 1);
	m_pTrackingSizer->Add( pBoxRow11, 0, wxFIXED_MINSIZE | wxEXPAND, 0 );

    wxStaticText *m_pTextTotalSeedNb = new wxStaticText( this, wxID_ANY, wxT("Number of current seeds"), wxDefaultPosition, wxSize(slider1+slider2, -1), wxALIGN_LEFT );
    RTTrackingHelper::getInstance()->m_pTxtTotalSeedNbBox = new wxTextCtrl( this, wxID_ANY, wxT("1000"), wxDefaultPosition, wxSize(slider3, -1), wxTE_CENTRE | wxTE_READONLY );

	wxBoxSizer *pBoxRow12 = new wxBoxSizer( wxHORIZONTAL );
    pBoxRow12->Add( m_pTextTotalSeedNb, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxALL, 1 );
	pBoxRow12->Add( RTTrackingHelper::getInstance()->m_pTxtTotalSeedNbBox,   0, wxALIGN_LEFT | wxALL, 1);
	m_pTrackingSizer->Add( pBoxRow12, 0, wxFIXED_MINSIZE | wxEXPAND, 0 );

	wxStaticText *m_pTextOpacity = new wxStaticText( this, wxID_ANY, wxT("Opacity"), wxDefaultPosition, wxSize(slider1, -1), wxALIGN_CENTER );
    m_pSliderOpacity = new MySlider( this, wxID_ANY, 0, 0, 100, wxDefaultPosition, wxSize(slider2, -1), wxSL_HORIZONTAL | wxSL_AUTOTICKS );
    m_pSliderOpacity->SetValue( 100 );
    Connect( m_pSliderOpacity->GetId(), wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler(TrackingWindow::OnSliderOpacityMoved) );
    m_pTxtOpacityBox = new wxTextCtrl( this, wxID_ANY, wxT("1.0"), wxDefaultPosition, wxSize(slider3, -1), wxTE_CENTRE | wxTE_READONLY );

	wxBoxSizer *pBoxRow13 = new wxBoxSizer( wxHORIZONTAL );
    pBoxRow13->Add( m_pTextOpacity, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxALL, 1 );
    pBoxRow13->Add( m_pSliderOpacity,   0, wxALIGN_LEFT | wxEXPAND | wxALL, 1);
	pBoxRow13->Add( m_pTxtOpacityBox,   0, wxALIGN_LEFT | wxALL, 1);
	m_pTrackingSizer->Add( pBoxRow13, 0, wxFIXED_MINSIZE | wxEXPAND, 0 );

    m_pBtnToggleSrcAlpha= new wxToggleButton( this, wxID_ANY,wxT("Regular Alpha"), wxDefaultPosition, wxSize(zoomS*2, -1) );
	Connect( m_pBtnToggleSrcAlpha->GetId(), wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler(TrackingWindow::OnSrcAlpha) );
    m_pTrackingSizer->Add( m_pBtnToggleSrcAlpha, 0, wxALL, 2 );

	m_pBtnToggleColorWithOverlay= new wxToggleButton( this, wxID_ANY,wxT("Color with amplitude"), wxDefaultPosition, wxSize(zoomS*2, -1) );
	Connect( m_pBtnToggleColorWithOverlay->GetId(), wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler(TrackingWindow::OnColorWithAmplitude) );
    m_pTrackingSizer->Add( m_pBtnToggleColorWithOverlay, 0, wxALL, 2 );

	RTTrackingHelper::getInstance()->m_pBtnToggleEnableRSN = new wxToggleButton( this, wxID_ANY,wxT("Enable rs-connectivity"), wxDefaultPosition, wxSize(zoomS*2, zoomH) );
	Connect( RTTrackingHelper::getInstance()->m_pBtnToggleEnableRSN->GetId(), wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler(TrackingWindow::OnEnableRSN) );
    RTTrackingHelper::getInstance()->m_pBtnToggleEnableRSN->Enable(false);

    m_pBtnConvert = new wxButton( this, wxID_ANY,wxT("Export fibers to scene object"), wxDefaultPosition, wxSize(zoomS*2, zoomH) );
	Connect( m_pBtnConvert->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(TrackingWindow::OnConvertToFibers) );

	m_pTrackingSizer->Add( RTTrackingHelper::getInstance()->m_pBtnToggleEnableRSN, 0, wxALL, 2 );
    m_pTrackingSizer->Add( m_pBtnConvert, 0, wxALL, 2 );

    /*-----------------------ANIMATION SECTION -----------------------------------*/

    /*m_pLineSeparator = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxSize(230,-1),wxHORIZONTAL,wxT("Separator"));

    wxTextCtrl *animationZone = new wxTextCtrl( this, wxID_ANY, wxT("Animation"), wxDefaultPosition, wxSize(150, -1), wxTE_CENTER | wxTE_READONLY );
    animationZone->SetBackgroundColour( *wxLIGHT_GREY );
    wxFont font = animationZone->GetFont();
    font.SetPointSize( 10 );
    font.SetWeight( wxFONTWEIGHT_BOLD );
    animationZone->SetFont( font );

	wxBoxSizer *pBoxRowAnim1 = new wxBoxSizer( wxVERTICAL );
    pBoxRowAnim1->Add( m_pLineSeparator, 0, wxALIGN_RIGHT | wxALL, 1 );
	pBoxRowAnim1->Add( animationZone,   0, wxALIGN_CENTER | wxALL, 1);
	m_pTrackingSizer->Add( pBoxRowAnim1, 0, wxFIXED_MINSIZE | wxALL, 2 );
    
    m_bmpPlay = wxImage(MyApp::iconsPath+ wxT("play.png"), wxBITMAP_TYPE_PNG);
    m_bmpPause = wxImage(MyApp::iconsPath+ wxT("pause.png"), wxBITMAP_TYPE_PNG);

    wxImage bmpStop(MyApp::iconsPath+ wxT("stop.png"), wxBITMAP_TYPE_PNG);

    m_pPlayPause = new wxBitmapButton( this, wxID_ANY,m_bmpPlay, wxDefaultPosition, wxSize(50, -1) );
    Connect( m_pPlayPause->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(TrackingWindow::OnPlay) );

    m_pBtnStop = new wxBitmapButton( this, wxID_ANY, bmpStop, wxDefaultPosition, wxSize(50, -1) );
    Connect( m_pBtnStop->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(TrackingWindow::OnStop) );

	wxBoxSizer *pBoxRowAnim = new wxBoxSizer( wxHORIZONTAL );
    pBoxRowAnim->Add( m_pPlayPause, 0,  wxALIGN_CENTER , 1 );
	pBoxRowAnim->Add( m_pBtnStop, 0,  wxALIGN_CENTER, 1 );
	m_pTrackingSizer->Add( pBoxRowAnim, 0, wxFIXED_MINSIZE | wxALL, 2 );*/

    /*-----------------------MAGNET SECTION-----------------------------------*/

    wxTextCtrl *magnetZone = new wxTextCtrl( this, wxID_ANY, wxT("Magnet tracking"), wxDefaultPosition, wxSize(zoomS*2, -1), wxTE_CENTER | wxTE_READONLY );
    magnetZone->SetBackgroundColour( *wxLIGHT_GREY );
    wxFont magnet_font = magnetZone->GetFont();
    magnet_font.SetPointSize( 10 );
    magnet_font.SetWeight( wxFONTWEIGHT_BOLD );
    magnetZone->SetFont( magnet_font );

	wxBoxSizer *pBoxMagnetZone = new wxBoxSizer( wxVERTICAL );
    pBoxMagnetZone->Add( new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxSize(230,-1),wxHORIZONTAL,wxT("Separator")), 0, wxALIGN_RIGHT | wxALL, 1 );
	pBoxMagnetZone->Add( magnetZone,   0, wxALIGN_CENTER | wxALL, 1);
	m_pTrackingSizer->Add( pBoxMagnetZone, 0, wxFIXED_MINSIZE | wxALL, 2 );
#if _USE_ZOOM_GUI
    wxImage bmpMagnetR( MyApp::iconsPath + wxT( "magnet_r64.png" ), wxBITMAP_TYPE_PNG );
    wxBitmapButton *m_pBtnPlaceMagnetR = new wxBitmapButton( this, wxID_ANY, bmpMagnetR, wxDefaultPosition, wxSize( 30, -1 ) );
    Connect( m_pBtnPlaceMagnetR->GetId(),         wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TrackingWindow::OnPlaceMagnetR ) );
    wxImage bmpMagnetG( MyApp::iconsPath + wxT( "magnet_g64.png" ), wxBITMAP_TYPE_PNG );
    wxBitmapButton *m_pBtnPlaceMagnetG = new wxBitmapButton( this, wxID_ANY, bmpMagnetG, wxDefaultPosition, wxSize( 30, -1 ) );
    Connect( m_pBtnPlaceMagnetG->GetId(),         wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TrackingWindow::OnPlaceMagnetG ) );
    wxImage bmpMagnetB( MyApp::iconsPath + wxT( "magnet_b64.png" ), wxBITMAP_TYPE_PNG );
    wxBitmapButton *m_pBtnPlaceMagnetB = new wxBitmapButton( this, wxID_ANY, bmpMagnetB, wxDefaultPosition, wxSize( 30, -1 ) );
    Connect( m_pBtnPlaceMagnetB->GetId(),         wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TrackingWindow::OnPlaceMagnetB ) );
#else
	wxImage bmpMagnetR( MyApp::iconsPath + wxT( "magnet_r.png" ), wxBITMAP_TYPE_PNG );
    wxBitmapButton *m_pBtnPlaceMagnetR = new wxBitmapButton( this, wxID_ANY, bmpMagnetR, wxDefaultPosition, wxSize( 30, -1 ) );
    Connect( m_pBtnPlaceMagnetR->GetId(),         wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TrackingWindow::OnPlaceMagnetR ) );
    wxImage bmpMagnetG( MyApp::iconsPath + wxT( "magnet_g.png" ), wxBITMAP_TYPE_PNG );
    wxBitmapButton *m_pBtnPlaceMagnetG = new wxBitmapButton( this, wxID_ANY, bmpMagnetG, wxDefaultPosition, wxSize( 30, -1 ) );
    Connect( m_pBtnPlaceMagnetG->GetId(),         wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TrackingWindow::OnPlaceMagnetG ) );
    wxImage bmpMagnetB( MyApp::iconsPath + wxT( "magnet_b.png" ), wxBITMAP_TYPE_PNG );
    wxBitmapButton *m_pBtnPlaceMagnetB = new wxBitmapButton( this, wxID_ANY, bmpMagnetB, wxDefaultPosition, wxSize( 30, -1 ) );
    Connect( m_pBtnPlaceMagnetB->GetId(),         wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( TrackingWindow::OnPlaceMagnetB ) );
#endif

    m_pToggleMagnetMode = new wxToggleButton( this, wxID_ANY,wxT("Start magnet"), wxDefaultPosition, wxSize(zoomS, -1) );
    Connect( m_pToggleMagnetMode->GetId(), wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler(TrackingWindow::OnToggleMagnetMode) );

    wxBoxSizer *pBoxMagnet1 = new wxBoxSizer( wxHORIZONTAL );
    pBoxMagnet1->Add( m_pBtnPlaceMagnetR, 1, wxEXPAND | wxALL, 1 );
    pBoxMagnet1->Add( m_pBtnPlaceMagnetG, 1, wxEXPAND | wxALL, 1 );
    pBoxMagnet1->Add( m_pBtnPlaceMagnetB, 1, wxEXPAND | wxALL, 1 );
	m_pTrackingSizer->Add( pBoxMagnet1, 0, wxFIXED_MINSIZE | wxEXPAND, 0 );

    m_pTrackingSizer->Add( m_pToggleMagnetMode, 1, wxEXPAND | wxALL, 1 );

}

void TrackingWindow::OnSize( wxSizeEvent &WXUNUSED(event) )
{
}

void TrackingWindow::OnPaint( wxPaintEvent &WXUNUSED(event) )
{
    wxPaintDC dc( this );
}

wxSizer* TrackingWindow::getWindowSizer()
{
    return m_pTrackingSizer;
}

void TrackingWindow::OnStartTracking( wxCommandEvent& WXUNUSED(event) )
{
    RTTrackingHelper::getInstance()->toggleRTTReady();
    RTTrackingHelper::getInstance()->setRTTDirty( true );

    if( !RTTrackingHelper::getInstance()->isRTTReady() )
    {
        SceneManager::getInstance()->getScene()->getRTTfibers()->clearFibersRTT();
        //RTTrackingHelper::getInstance()->setRTTDirty( false );

        RTFMRIHelper::getInstance()->setTractoDrivenRSN(false);
		RTTrackingHelper::getInstance()->setRTTDirty(true);
        RTFMRIHelper::getInstance()->setRTFMRIDirty( true );
        RTTrackingHelper::getInstance()->m_pBtnToggleEnableRSN->SetValue(false);
        m_pBtnStart->SetLabel(wxT("Start tracking"));
    }
    else
    {
        m_pBtnStart->SetLabel(wxT("Stop tracking"));
        RTFMRIHelper::getInstance()->setTractoDrivenRSN(true);
    }
}

void TrackingWindow::OnClearBox( wxTreeEvent&    event )
{
    m_pMainFrame->onDeleteTreeItem( event );
    SceneManager::getInstance()->getScene()->getRTTfibers()->clearFibersRTT();
    RTTrackingHelper::getInstance()->setRTTDirty( false );
    RTTrackingHelper::getInstance()->setRTTReady( false );
    m_pBtnStart->SetValue( false );
}

void TrackingWindow::OnSliderFAMoved(wxCommandEvent& WXUNUSED(event))
{
    float sliderValue = m_pSliderFA->GetValue() / 100.0f;
    m_pTxtFABox->SetValue( wxString::Format( wxT( "%.2f"), sliderValue ) );
    SceneManager::getInstance()->getScene()->getRTTfibers()->setFAThreshold( sliderValue );
    RTTrackingHelper::getInstance()->setRTTDirty( true );
}

void TrackingWindow::OnSliderAngleMoved( wxCommandEvent& WXUNUSED(event) )
{
    float sliderValue = m_pSliderAngle->GetValue();
    m_pTxtAngleBox->SetValue(wxString::Format( wxT( "%.1f "), sliderValue ) );
    SceneManager::getInstance()->getScene()->getRTTfibers()->setAngleThreshold( sliderValue );
    RTTrackingHelper::getInstance()->setRTTDirty( true );
}

void TrackingWindow::OnSliderStepMoved( wxCommandEvent& WXUNUSED(event) )
{
    float sliderValue = m_pSliderStep->GetValue() / 10.0f;
    m_pTxtStepBox->SetValue(wxString::Format( wxT( "%.1f mm"), sliderValue) );
    SceneManager::getInstance()->getScene()->getRTTfibers()->setStep( sliderValue );
    RTTrackingHelper::getInstance()->setRTTDirty( true );
}

void TrackingWindow::OnSliderGMStepMoved( wxCommandEvent& WXUNUSED(event) )
{
    int sliderValue = m_pSliderGMStep->GetValue();
    m_pTxtGMStepBox->SetValue(wxString::Format( wxT( "%i"), sliderValue) );
    SceneManager::getInstance()->getScene()->getRTTfibers()->setGMStep( sliderValue );
    RTTrackingHelper::getInstance()->setRTTDirty( true );
}

void TrackingWindow::OnSliderPunctureMoved( wxCommandEvent& WXUNUSED(event) )
{
    float sliderValue = m_pSliderPuncture->GetValue() / 100.0f;
    m_pTxtPunctureBox->SetValue(wxString::Format( wxT( "%.2f"), sliderValue) );
    SceneManager::getInstance()->getScene()->getRTTfibers()->setPuncture( sliderValue );
    SceneManager::getInstance()->getScene()->getRTTfibers()->setVinVout( sliderValue );
    RTTrackingHelper::getInstance()->setRTTDirty( true );
}

void TrackingWindow::OnSliderMinLengthMoved( wxCommandEvent& WXUNUSED(event) )
{
    float sliderValue = m_pSliderMinLength->GetValue();
    m_pTxtMinLengthBox->SetValue(wxString::Format( wxT( "%.1f mm"), sliderValue) );
    SceneManager::getInstance()->getScene()->getRTTfibers()->setMinFiberLength( sliderValue );
    RTTrackingHelper::getInstance()->setRTTDirty( true );
}

void TrackingWindow::OnSliderOpacityMoved( wxCommandEvent& WXUNUSED(event) )
{
    float sliderValue = m_pSliderOpacity->GetValue() / 100.0f;
    m_pTxtOpacityBox->SetValue(wxString::Format( wxT( "%.2f"), sliderValue) );
    SceneManager::getInstance()->getScene()->getRTTfibers()->setOpacity( sliderValue );
    SceneManager::getInstance()->getScene()->getRTTfibers()->renderRTTFibers(true, false, true);
}

void TrackingWindow::OnSliderMaxLengthMoved( wxCommandEvent& WXUNUSED(event) )
{
    float sliderValue = m_pSliderMaxLength->GetValue();
    m_pTxtMaxLengthBox->SetValue(wxString::Format( wxT( "%.1f mm"), sliderValue) );
    SceneManager::getInstance()->getScene()->getRTTfibers()->setMaxFiberLength( sliderValue );
    RTTrackingHelper::getInstance()->setRTTDirty( true );
}

void TrackingWindow::OnSelectFileDTI( wxCommandEvent& WXUNUSED(event) )
{
    //Tensor data
    long item = m_pMainFrame->getCurrentListIndex();
    Tensors* pTensorInfo = (Tensors *)DatasetManager::getInstance()->getDataset( m_pMainFrame->m_pListCtrl->GetItem( item ) );
	
	if( pTensorInfo != NULL && TENSORS == pTensorInfo->getType() && 6 == pTensorInfo->getBands()  )
    {
		//Hide tensor data
		pTensorInfo->setShow(false);
		m_pMainFrame->m_pListCtrl->UpdateSelected();
		m_pMainFrame->refreshAllGLWidgets();
        m_pBtnSelectFile->SetLabel( pTensorInfo->getName() );
        m_pBtnSelectFile->SetBackgroundColour(wxNullColour);

        //Set Step
        float step = DatasetManager::getInstance()->getVoxelX() / 2.0f;
        SceneManager::getInstance()->getScene()->getRTTfibers()->setTensorsInfo( (Tensors *)DatasetManager::getInstance()->getDataset( m_pMainFrame->m_pListCtrl->GetItem( item ) ) );

        if(SceneManager::getInstance()->getSelectionTree().isEmpty())
        {
            m_pMainFrame->createNewSelectionObject( BOX_TYPE, Vector(0,0,0) );
            
            SelectionObject* pNewSelObj(m_pMainFrame->getCurrentSelectionObject());
            
            Vector boxSize(2/step,2/step,2/step);
            
            pNewSelObj->setSize(boxSize);
        }
        m_pMainFrame->m_pTrackingWindow->m_pBtnStart->SetBackgroundColour(wxColour( 147, 255, 239 ));
        m_pMainFrame->m_pTrackingWindow->m_pBtnStart->Enable( true );
    }
}

void TrackingWindow::OnSelectFileHARDI( wxCommandEvent& WXUNUSED(event) )
{
    //HARDI data
    long item = m_pMainFrame->getCurrentListIndex();
    Maximas* pMaximasInfo = (Maximas *)DatasetManager::getInstance()->getDataset( m_pMainFrame->m_pListCtrl->GetItem( item ) );
	
	if( pMaximasInfo != NULL && pMaximasInfo->getType() == MAXIMAS )
    {
		//Hide hardi data
		pMaximasInfo->setShow(false);
		m_pMainFrame->m_pListCtrl->UpdateSelected();
		m_pMainFrame->refreshAllGLWidgets();

        m_pBtnSelectFile->SetLabel( pMaximasInfo->getName() );
        m_pBtnSelectFile->SetBackgroundColour(wxNullColour);

        //Set Step
        float step = DatasetManager::getInstance()->getVoxelX() / 2.0f;
        SceneManager::getInstance()->getScene()->getRTTfibers()->setIsHardi( true );
        SceneManager::getInstance()->getScene()->getRTTfibers()->setHARDIInfo( (Maximas *)DatasetManager::getInstance()->getDataset( m_pMainFrame->m_pListCtrl->GetItem( item ) ) );
        
        if(SceneManager::getInstance()->getSelectionTree().isEmpty())
        {
            m_pMainFrame->createNewSelectionObject( BOX_TYPE, Vector(0,0,0) );
            
            SelectionObject* pNewSelObj = m_pMainFrame->getCurrentSelectionObject();
            
            Vector boxSize(2/step,2/step,2/step);
            pNewSelObj->setSize(boxSize);
        }
        if(m_pTextFA->IsEnabled())
        {
            m_pMainFrame->m_pTrackingWindowHardi->m_pBtnStart->SetBackgroundColour(wxColour( 147, 255, 239 ));
            m_pMainFrame->m_pTrackingWindowHardi->m_pBtnStart->Enable( true );

        }
    }
}

void TrackingWindow::OnSelectShell( wxCommandEvent& WXUNUSED(event) )
{
	//Select surface for seeding
    long item = m_pMainFrame->getCurrentListIndex();
	DatasetInfo* pMesh = DatasetManager::getInstance()->getDataset (MyApp::frame->m_pListCtrl->GetItem( item )); 

	if( pMesh != NULL && pMesh->getType() == ISO_SURFACE )
    {
		m_pBtnSelectShell->SetLabel( pMesh->getName() );
		SceneManager::getInstance()->getScene()->getRTTfibers()->setShellInfo( (DatasetInfo *)DatasetManager::getInstance()->getDataset( m_pMainFrame->m_pListCtrl->GetItem( item ) ) );
		m_pToggleShell->Enable(true);
        m_pToggleShell->SetValue(true);

        RTTrackingHelper::getInstance()->setShellSeed(true);
        RTTrackingHelper::getInstance()->setRTTDirty( true );
        float sliderValue = RTTrackingHelper::getInstance()->m_pSliderAxisSeedNb->GetValue();

        //Set nb of seeds depending on the seeding mode
        if( !RTTrackingHelper::getInstance()->isShellSeeds() )
        {
            RTTrackingHelper::getInstance()->m_pTxtTotalSeedNbBox->SetValue(wxString::Format( wxT( "%.1f"), sliderValue*sliderValue*sliderValue) );
            m_pToggleShell->SetLabel(wxT("Shell seed OFF"));
        }
        else
        {
            float shellSeedNb = SceneManager::getInstance()->getScene()->getRTTfibers()->getShellSeedNb();
            RTTrackingHelper::getInstance()->m_pTxtTotalSeedNbBox->SetValue(wxString::Format( wxT( "%.1f"), shellSeedNb) ); 
            m_pToggleShell->SetLabel(wxT( "Shell seed ON"));
        } 
	}
}

void TrackingWindow::OnSelectSeedMap( wxCommandEvent& WXUNUSED(event) )
{
	//Select map for threshold seeding
    long item = m_pMainFrame->getCurrentListIndex();
	Anatomy* pSeedMap = (Anatomy*)DatasetManager::getInstance()->getDataset (MyApp::frame->m_pListCtrl->GetItem( item )); 

	if( pSeedMap != NULL && pSeedMap->getBands() == 1 )
    {
		m_pBtnSelectSeed->SetLabel( pSeedMap->getName() );
		SceneManager::getInstance()->getScene()->getRTTfibers()->setSeedMapInfo( (Anatomy *)DatasetManager::getInstance()->getDataset( m_pMainFrame->m_pListCtrl->GetItem( item ) ) );

		m_pToggleSeedMap->Enable(true);
        m_pToggleSeedMap->SetValue(true);

        RTTrackingHelper::getInstance()->setSeedMap(true);
        RTTrackingHelper::getInstance()->setRTTDirty( true );
        float sliderValue = RTTrackingHelper::getInstance()->m_pSliderAxisSeedNb->GetValue();

        //Set nb of seeds depending on the seeding mode
        if( !RTTrackingHelper::getInstance()->isSeedMap() )
        {
			RTTrackingHelper::getInstance()->m_pSliderAxisSeedNb->SetValue( sliderValue );
            RTTrackingHelper::getInstance()->m_pTxtTotalSeedNbBox->SetValue(wxString::Format( wxT( "%.1f"), sliderValue*sliderValue*sliderValue) );
            m_pToggleSeedMap->SetLabel(wxT("Seed map OFF"));
        }
        else
        {
			RTTrackingHelper::getInstance()->m_pSliderAxisSeedNb->SetValue( 1 );
			RTTrackingHelper::getInstance()->m_pTxtAxisSeedNbBox->SetValue(wxString::Format( wxT( "%.1f"), 1.0f) );
			SceneManager::getInstance()->getScene()->getRTTfibers()->setNbSeed( 1 );
            float seedMapNb = SceneManager::getInstance()->getScene()->getRTTfibers()->getSeedMapNb();
            RTTrackingHelper::getInstance()->m_pTxtTotalSeedNbBox->SetValue(wxString::Format( wxT( "%.1f"), seedMapNb) ); 
            m_pToggleSeedMap->SetLabel(wxT( "Seed map ON"));
        } 
	}
}

void TrackingWindow::OnMapSeeding( wxCommandEvent& WXUNUSED(event) )
{
    RTTrackingHelper::getInstance()->toggleSeedMap();
    RTTrackingHelper::getInstance()->setRTTDirty( true );
	m_pBtnStart->Enable( true );
    
	//Set nb of seeds depending on the seeding mode 
	if( !RTTrackingHelper::getInstance()->isSeedMap() )
    {
		RTTrackingHelper::getInstance()->m_pSliderAxisSeedNb->SetValue( 10 );
        RTTrackingHelper::getInstance()->m_pTxtTotalSeedNbBox->SetValue(wxString::Format( wxT( "%.1f"), 1000.0f) );
		SceneManager::getInstance()->getScene()->getRTTfibers()->setNbSeed( 10 );
	    RTTrackingHelper::getInstance()->m_pTxtAxisSeedNbBox->SetValue( wxString::Format( wxT( "%.1f"), 10.0f) );
        m_pToggleSeedMap->SetLabel(wxT( "Seed map OFF"));
    }
    else
    {
		RTTrackingHelper::getInstance()->m_pSliderAxisSeedNb->SetValue( 1 );
		RTTrackingHelper::getInstance()->m_pTxtAxisSeedNbBox->SetValue(wxString::Format( wxT( "%.1f"), 1.0f) );
		SceneManager::getInstance()->getScene()->getRTTfibers()->setNbSeed( 1 );
        float seedMapNb = SceneManager::getInstance()->getScene()->getRTTfibers()->getSeedMapNb();
        RTTrackingHelper::getInstance()->m_pTxtTotalSeedNbBox->SetValue(wxString::Format( wxT( "%.1f"), seedMapNb) ); 
        m_pToggleSeedMap->SetLabel(wxT( "Seed map ON"));
    }
}

void TrackingWindow::OnToggleGM( wxCommandEvent& WXUNUSED(event) )
{
    RTTrackingHelper::getInstance()->toggleGMmap();
    RTTrackingHelper::getInstance()->setRTTDirty( true );
    
	//Set nb of seeds depending on the seeding mode 
	if( !RTTrackingHelper::getInstance()->isGMAllowed() )
    {
        m_pToggleGMmap->SetLabel(wxT( "GM map OFF"));
    }
    else
    {
        m_pToggleGMmap->SetLabel(wxT( "GM map ON"));
    }
}

void TrackingWindow::OnSelectMask( wxCommandEvent& WXUNUSED(event) )
{
	//Select map for threshold seeding
    long item = m_pMainFrame->getCurrentListIndex();
	Anatomy* pMap = (Anatomy*)DatasetManager::getInstance()->getDataset (MyApp::frame->m_pListCtrl->GetItem( item )); 

	if( pMap != NULL && pMap->getBands() == 1 )
    {
		m_pBtnSelectMap->SetLabel( pMap->getName() );
        m_pBtnSelectMap->SetBackgroundColour(wxNullColour);
		SceneManager::getInstance()->getScene()->getRTTfibers()->setMaskInfo( (Anatomy *)DatasetManager::getInstance()->getDataset( m_pMainFrame->m_pListCtrl->GetItem( item ) ) );
        m_pTextFA->Enable(true);
        m_pSliderFA->Enable(true);
        m_pTxtFABox->Enable(true);
	}
    if(SceneManager::getInstance()->getScene()->getRTTfibers()->isHardiSelected())
    {
        m_pMainFrame->m_pTrackingWindowHardi->m_pBtnStart->Enable( true );
        m_pMainFrame->m_pTrackingWindowHardi->m_pBtnStart->SetBackgroundColour(wxColour( 147, 255, 239 ));
		//m_pMainFrame->m_pFMRIWindow->setInitiateTractoBtn();
    }
}

void TrackingWindow::OnInitX( wxCommandEvent& event )
{
    RTTrackingHelper::getInstance()->toggleInitSeed();
    Vector init = Vector(1,0,0);
    SceneManager::getInstance()->getScene()->getRTTfibers()->setInitSeed( init );
    
    RTTrackingHelper::getInstance()->setRTTDirty( true );
}

void TrackingWindow::OnInitY( wxCommandEvent& event )
{
    RTTrackingHelper::getInstance()->toggleInitSeed();
    Vector init = Vector(0,1,0);
    SceneManager::getInstance()->getScene()->getRTTfibers()->setInitSeed( init );
    
    RTTrackingHelper::getInstance()->setRTTDirty( true );
}

void TrackingWindow::OnInitZ( wxCommandEvent& event )
{
    RTTrackingHelper::getInstance()->toggleInitSeed();
    Vector init = Vector(0,0,1);
    SceneManager::getInstance()->getScene()->getRTTfibers()->setInitSeed( init );
    
    RTTrackingHelper::getInstance()->setRTTDirty( true );
}

void TrackingWindow::OnSelectGM( wxCommandEvent& WXUNUSED(event) )
{
	//Select map for threshold seeding
    long item = m_pMainFrame->getCurrentListIndex();
	Anatomy* pMap = (Anatomy*)DatasetManager::getInstance()->getDataset (MyApp::frame->m_pListCtrl->GetItem( item )); 

    

	if( pMap != NULL && pMap->getBands() == 1 )
    {
		m_pBtnSelectGM->SetLabel( pMap->getName() );
		SceneManager::getInstance()->getScene()->getRTTfibers()->setGMInfo( (Anatomy *)DatasetManager::getInstance()->getDataset( m_pMainFrame->m_pListCtrl->GetItem( item ) ) );

        m_pToggleGMmap->Enable(true);
        m_pToggleGMmap->SetValue(true);

        RTTrackingHelper::getInstance()->setGMmap(true);
        RTTrackingHelper::getInstance()->setRTTDirty( true );
	}

    if( !RTTrackingHelper::getInstance()->isGMAllowed() )
    {
        m_pToggleGMmap->SetLabel(wxT( "GM map OFF"));
    }
    else
    {
        m_pToggleGMmap->SetLabel(wxT( "GM map ON"));
    }
 }

void TrackingWindow::OnSelectExclusion( wxCommandEvent& WXUNUSED(event) )
{
	//Select map for threshold seeding
    long item = m_pMainFrame->getCurrentListIndex();
	Anatomy* pMap = (Anatomy*)DatasetManager::getInstance()->getDataset (MyApp::frame->m_pListCtrl->GetItem( item )); 

	if( pMap != NULL && pMap->getBands() == 1 )
    {
		m_pBtnSelectNotMap->SetLabel( pMap->getName() );
		SceneManager::getInstance()->getScene()->getRTTfibers()->setExcludeInfo( (Anatomy *)DatasetManager::getInstance()->getDataset( m_pMainFrame->m_pListCtrl->GetItem( item ) ) );

        m_pToggleNotMap->Enable(true);
        m_pToggleNotMap->SetValue(true);

        RTTrackingHelper::getInstance()->setNotmap(true);
        RTTrackingHelper::getInstance()->setRTTDirty( true );
	}

    if( !RTTrackingHelper::getInstance()->isNotMapOn() )
    {
        m_pToggleNotMap->SetLabel(wxT( "NOT map OFF"));
    }
    else
    {
        m_pToggleNotMap->SetLabel(wxT( "NOT map ON"));
    }
}

void TrackingWindow::OnToggleNotMap( wxCommandEvent& WXUNUSED(event) )
{
    RTTrackingHelper::getInstance()->toggleNotMap();
    RTTrackingHelper::getInstance()->setRTTDirty( true );
    
	if( !RTTrackingHelper::getInstance()->isNotMapOn() )
    {
        m_pToggleNotMap->SetLabel(wxT( "NOT map OFF"));
    }
    else
    {
        m_pToggleNotMap->SetLabel(wxT( "NOT map ON"));
    }
}

void TrackingWindow::OnSelectInclusion( wxCommandEvent& WXUNUSED(event) )
{
	//Select map for threshold seeding
    long item = m_pMainFrame->getCurrentListIndex();
	Anatomy* pMap = (Anatomy*)DatasetManager::getInstance()->getDataset (MyApp::frame->m_pListCtrl->GetItem( item )); 

	if( pMap != NULL && pMap->getBands() == 1 )
    {
		m_pBtnSelectAndMap->SetLabel( pMap->getName() );
		SceneManager::getInstance()->getScene()->getRTTfibers()->setIncludeInfo( (Anatomy *)DatasetManager::getInstance()->getDataset( m_pMainFrame->m_pListCtrl->GetItem( item ) ) );

        m_pToggleAndMap->Enable(true);
        m_pToggleAndMap->SetValue(true);

        RTTrackingHelper::getInstance()->setAndmap(true);
        RTTrackingHelper::getInstance()->setRTTDirty( true );
	}

    if( !RTTrackingHelper::getInstance()->isAndMapOn() )
    {
        m_pToggleAndMap->SetLabel(wxT( "AND map OFF"));
        SceneManager::getInstance()->getScene()->getRTTfibers()->setAnd(true);
    }
    else
    {
        m_pToggleAndMap->SetLabel(wxT( "AND map ON"));
        SceneManager::getInstance()->getScene()->getRTTfibers()->setAnd(false);
    }
}

void TrackingWindow::OnToggleAndMap( wxCommandEvent& WXUNUSED(event) )
{
    RTTrackingHelper::getInstance()->toggleAndMap();
    RTTrackingHelper::getInstance()->setRTTDirty( true );
    
	if( !RTTrackingHelper::getInstance()->isAndMapOn() )
    {
        m_pToggleAndMap->SetLabel(wxT( "AND map OFF"));
        SceneManager::getInstance()->getScene()->getRTTfibers()->setAnd(true);
    }
    else
    {
        m_pToggleAndMap->SetLabel(wxT( "AND map ON"));
        SceneManager::getInstance()->getScene()->getRTTfibers()->setAnd(false);
    }
}

void TrackingWindow::OnShellSeeding( wxCommandEvent& WXUNUSED(event) )
{
    RTTrackingHelper::getInstance()->toggleShellSeeds();
    RTTrackingHelper::getInstance()->setRTTDirty( true );
    float sliderValue = RTTrackingHelper::getInstance()->m_pSliderAxisSeedNb->GetValue();
	m_pBtnStart->Enable( true );
    
	//Set nb of seeds depending on the seeding mode
	if( !RTTrackingHelper::getInstance()->isShellSeeds() )
    {
        RTTrackingHelper::getInstance()->m_pTxtTotalSeedNbBox->SetValue(wxString::Format( wxT( "%.1f"), sliderValue*sliderValue*sliderValue) );
        m_pToggleShell->SetLabel(wxT( "Shell seed OFF"));
    }
    else
    {
        float shellSeedNb = SceneManager::getInstance()->getScene()->getRTTfibers()->getShellSeedNb();
        RTTrackingHelper::getInstance()->m_pTxtTotalSeedNbBox->SetValue(wxString::Format( wxT( "%.1f"), shellSeedNb) ); 
        m_pToggleShell->SetLabel(wxT( "Shell seed ON"));
    }
}

//Deprecated
void TrackingWindow::OnRandomSeeding( wxCommandEvent& WXUNUSED(event) )
{
    RTTrackingHelper::getInstance()->toggleRandomInit();
    RTTrackingHelper::getInstance()->setRTTDirty( true );
    
	//Set nb of seeds depending on the seeding mode 
    if( !RTTrackingHelper::getInstance()->isRandomInit() )
    {
        m_pToggleRandomInit->SetLabel(wxT( "Evenly spaced seeds"));
    }
    else
    {
        m_pToggleRandomInit->SetLabel(wxT( "Randomly spaced seeds"));
    }
}

//Deprecated
void TrackingWindow::OnInterpolate( wxCommandEvent& WXUNUSED(event) )
{
    RTTrackingHelper::getInstance()->toggleInterpolateTensors();
    RTTrackingHelper::getInstance()->setRTTDirty( true );
}

void TrackingWindow::OnSliderAxisSeedNbMoved( wxCommandEvent& WXUNUSED(event) )
{
    float sliderValue = RTTrackingHelper::getInstance()->m_pSliderAxisSeedNb->GetValue();
    SceneManager::getInstance()->getScene()->getRTTfibers()->setNbSeed( sliderValue );
    RTTrackingHelper::getInstance()->m_pTxtAxisSeedNbBox->SetValue(wxString::Format( wxT( "%.1f"), sliderValue) );

	if( !RTTrackingHelper::getInstance()->isShellSeeds() && !RTTrackingHelper::getInstance()->isSeedMap())
    {
        RTTrackingHelper::getInstance()->m_pTxtTotalSeedNbBox->SetValue(wxString::Format( wxT( "%.1f"), sliderValue*sliderValue*sliderValue) );
    }
	else if( RTTrackingHelper::getInstance()->isSeedMap())
	{
		float mapSeedNb = SceneManager::getInstance()->getScene()->getRTTfibers()->getSeedMapNb();
		RTTrackingHelper::getInstance()->m_pTxtTotalSeedNbBox->SetValue(wxString::Format( wxT( "%.1f"), mapSeedNb) );
	}
    else
    {
        float shellSeedNb = SceneManager::getInstance()->getScene()->getRTTfibers()->getNbMeshPoint();
        RTTrackingHelper::getInstance()->m_pTxtTotalSeedNbBox->SetValue(wxString::Format( wxT( "%.1f"), shellSeedNb) );
    }
    RTTrackingHelper::getInstance()->setRTTDirty( true );
}

void TrackingWindow::OnConvertToFibers( wxCommandEvent& WXUNUSED(event) )
{
    if(!SceneManager::getInstance()->getScene()->getRTTfibers()->getRTTFibers()->empty())
    { 
	    //Convert fibers
	    DatasetIndex index = DatasetManager::getInstance()->createFibers();

	    if( !DatasetManager::getInstance()->isFibersGroupLoaded() )
        {
            DatasetIndex result = DatasetManager::getInstance()->createFibersGroup();
            m_pMainFrame->m_pListCtrl->InsertItem( result );
        }

	    m_pMainFrame->m_pListCtrl->InsertItem( index );

        RTTrackingHelper::getInstance()->setRTTReady(false);

        SceneManager::getInstance()->getScene()->getRTTfibers()->clearFibersRTT();
        //RTTrackingHelper::getInstance()->setRTTDirty( false );

        RTFMRIHelper::getInstance()->setRTFMRIDirty( false );
	    RTFMRIHelper::getInstance()->setTractoDrivenRSN(false);
	    RTTrackingHelper::getInstance()->setRTTDirty(true);
        RTTrackingHelper::getInstance()->m_pBtnToggleEnableRSN->SetValue(false);

        m_pBtnStart->SetLabel(wxT("Start tracking"));
        m_pBtnStart->SetValue(false);
    }
}

//void TrackingWindow::OnPlay( wxCommandEvent& WXUNUSED(event) )
//{
//    RTTrackingHelper::getInstance()->setTrackAction(true);
//    RTTrackingHelper::getInstance()->togglePlayStop();
//    if(!RTTrackingHelper::getInstance()->isTrackActionPaused())
//    {
//        m_pPlayPause->SetBitmapLabel(m_bmpPause);
//        m_pMainFrame->setTimerSpeed();
//    }
//    else
//    {
//        m_pPlayPause->SetBitmapLabel(m_bmpPlay);
//        m_pMainFrame->setTimerSpeed();
//    }
//}
//
//void TrackingWindow::OnStop( wxCommandEvent& WXUNUSED(event) )
//{
//    SceneManager::getInstance()->getScene()->getRTTfibers()->m_trackActionStep = 0;
//    RTTrackingHelper::getInstance()->setTrackAction(false);
//    RTTrackingHelper::getInstance()->setTrackActionPause(true);
//    m_pPlayPause->SetBitmapLabel(m_bmpPlay);
//    m_pMainFrame->setTimerSpeed();
//}

void TrackingWindow::OnSrcAlpha( wxCommandEvent& event )
{
    RTTrackingHelper::getInstance()->toggleSrcAlpha();
    if( !RTTrackingHelper::getInstance()->isSrcAlpha() )
    {
        m_pBtnToggleSrcAlpha->SetLabel(wxT( "Flashy alpha"));
    }
    else
    {
        m_pBtnToggleSrcAlpha->SetLabel(wxT( "Regular alpha"));
    }
    RTTrackingHelper::getInstance()->setRTTDirty( true );
}
 void TrackingWindow::OnColorWithAmplitude( wxCommandEvent& event )
 {
	 
	 DatasetInfo* pInfo = SceneManager::getInstance()->getScene()->getRTTfibers();
	 pInfo->toggleUseTex();
	 pInfo->toggleShowFS();
	 RTTrackingHelper::getInstance()->setRTTDirty( true );

 }
void TrackingWindow::OnEnableRSN( wxCommandEvent& WXUNUSED(event) )
{
    RTTrackingHelper::getInstance()->toogleTractoDrivenRSN();
	RTTrackingHelper::getInstance()->setRTTDirty(true);
	RTFMRIHelper::getInstance()->setRTFMRIDirty(true);

	if( RTTrackingHelper::getInstance()->isTractoDrivenRSN() )
		RTFMRIHelper::getInstance()->setTractoDrivenRSN(true);
	else
		RTFMRIHelper::getInstance()->setTractoDrivenRSN(false);
}

void TrackingWindow::OnPlaceMagnetR( wxCommandEvent& WXUNUSED(event) )
{
    m_pMainFrame->createNewSelectionObject( BOX_TYPE, Vector(1,0,0) );
            
    SelectionObject* pNewSelObj = m_pMainFrame->getCurrentSelectionObject();       
    m_pMainFrame->m_pTreeWidget->SetItemImage( pNewSelObj->getTreeId(), pNewSelObj->getIcon() );
}

void TrackingWindow::OnPlaceMagnetG( wxCommandEvent& WXUNUSED(event) )
{
    m_pMainFrame->createNewSelectionObject( BOX_TYPE, Vector(0,1,0) );
            
    SelectionObject* pNewSelObj = m_pMainFrame->getCurrentSelectionObject();       
    m_pMainFrame->m_pTreeWidget->SetItemImage( pNewSelObj->getTreeId(), pNewSelObj->getIcon() );
}

void TrackingWindow::OnPlaceMagnetB( wxCommandEvent& WXUNUSED(event) )
{
    m_pMainFrame->createNewSelectionObject( BOX_TYPE, Vector(0,0,1) );
            
    SelectionObject* pNewSelObj = m_pMainFrame->getCurrentSelectionObject();       
    m_pMainFrame->m_pTreeWidget->SetItemImage( pNewSelObj->getTreeId(), pNewSelObj->getIcon() );
}

void TrackingWindow::OnToggleMagnetMode( wxCommandEvent& WXUNUSED(event) )
{
    RTTrackingHelper::getInstance()->toggleMagnet();
    RTTrackingHelper::getInstance()->setRTTDirty( true );
 
	if( !RTTrackingHelper::getInstance()->isMagnetOn() )
    {
        m_pToggleMagnetMode->SetLabel(wxT( "Start magnet"));
    }
    else
    {
        m_pToggleMagnetMode->SetLabel(wxT( "Stop magnet"));
    }
}