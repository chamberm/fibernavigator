#include "ConnectomicWindow.h"
#include "../dataset/ConnectomeHelper.h"
#include "MainFrame.h"
#include "SceneManager.h"

#include "../main.h"
#include "../dataset/Anatomy.h"

#include <wx/checkbox.h>
#include <wx/grid.h>
#include <wx/tglbtn.h>
#include <wx/treectrl.h>


IMPLEMENT_DYNAMIC_CLASS( ConnectomicWindow, wxScrolledWindow )

BEGIN_EVENT_TABLE( ConnectomicWindow, wxScrolledWindow )
END_EVENT_TABLE()


ConnectomicWindow::ConnectomicWindow( wxWindow *pParent, MainFrame *pMf, wxWindowID id, const wxPoint &pos, const wxSize &size)
:   wxScrolledWindow( pParent, id, pos, size, wxBORDER_NONE, _T("Connectomic") ),
    m_pMainFrame( pMf )
{
    SetBackgroundColour( *wxLIGHT_GREY );
    m_pConnectomicSizer = new wxBoxSizer( wxVERTICAL );
    SetSizer( m_pConnectomicSizer );
    SetAutoLayout( true );

    m_pNbLabels  = new wxTextCtrl( this, wxID_ANY, wxString::Format( wxT( "%i" ), 161 ), wxDefaultPosition, wxSize( 100, -1 ) );
    Connect( m_pNbLabels->GetId(),  wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( ConnectomicWindow::OnNbLabels ) );

    wxBoxSizer *pBoxRow2 = new wxBoxSizer( wxVERTICAL );
    pBoxRow2->Add( new wxStaticText( this, wxID_ANY, wxT( "Number of labels" ) ), 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 1 );
	pBoxRow2->Add( m_pNbLabels, 0, wxALIGN_CENTER | wxALL, 1 );
	m_pConnectomicSizer->Add( pBoxRow2, 0, wxFIXED_MINSIZE | wxEXPAND, 0 );

    m_pBtnSelectLabels = new wxButton( this, wxID_ANY,wxT("Labels not selected"), wxDefaultPosition, wxSize(230, -1) );
	Connect( m_pBtnSelectLabels->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ConnectomicWindow::onSelectLabels) );
    m_pBtnSelectLabels->SetBackgroundColour(wxColour( 255, 147, 147 ));

    m_pBtnSelectEdges = new wxButton( this, wxID_ANY,wxT("Tracts not selected"), wxDefaultPosition, wxSize(230, -1) );
	Connect( m_pBtnSelectEdges->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ConnectomicWindow::onSelectEdges) );
    m_pBtnSelectEdges->SetBackgroundColour(wxColour( 255, 147, 147 ));

	wxBoxSizer *pBoxRow1 = new wxBoxSizer( wxVERTICAL );
	pBoxRow1->Add( m_pBtnSelectLabels, 0, wxALIGN_CENTER | wxALL, 1 );
    pBoxRow1->Add( m_pBtnSelectEdges, 0, wxALIGN_CENTER | wxALL, 1 );
	m_pConnectomicSizer->Add( pBoxRow1, 0, wxFIXED_MINSIZE | wxEXPAND, 0 );

    m_pTextNodeSize = new wxStaticText( this, wxID_ANY, wxT("Node size"), wxDefaultPosition, wxSize(70, -1), wxALIGN_CENTER );
	m_pSliderNodeSize= new MySlider( this, wxID_ANY, 0, 1, 50, wxDefaultPosition, wxSize(100, -1), wxSL_HORIZONTAL | wxSL_AUTOTICKS );
	m_pSliderNodeSize->SetValue( 20 );
	Connect( m_pSliderNodeSize->GetId(), wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler(ConnectomicWindow::OnSliderDisplayMoved) );
    m_pTxtNodeSizeBox = new wxTextCtrl( this, wxID_ANY, wxT("2.0"), wxDefaultPosition, wxSize(55, -1), wxTE_CENTRE | wxTE_READONLY );

	wxBoxSizer *pBoxRow3 = new wxBoxSizer( wxHORIZONTAL );
    pBoxRow3->Add( m_pTextNodeSize, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxALL, 1 );
    pBoxRow3->Add( m_pSliderNodeSize,   0, wxALIGN_CENTER | wxEXPAND | wxALL, 1);
	pBoxRow3->Add( m_pTxtNodeSizeBox,   0, wxALIGN_CENTER | wxALL, 1);
	m_pConnectomicSizer->Add( pBoxRow3, 0, wxFIXED_MINSIZE | wxEXPAND, 0 );

	m_pTextNodeAlpha = new wxStaticText( this, wxID_ANY, wxT("Node Alpha"), wxDefaultPosition, wxSize(70, -1), wxALIGN_CENTER );
	m_pSliderNodeAlpha = new MySlider( this, wxID_ANY, 0, 0, 100, wxDefaultPosition, wxSize(100, -1), wxSL_HORIZONTAL | wxSL_AUTOTICKS );
	m_pSliderNodeAlpha->SetValue( 50 );
	Connect( m_pSliderNodeAlpha->GetId(), wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler(ConnectomicWindow::OnSliderDisplayMoved) );
    m_pTxtNodeAlphaBox = new wxTextCtrl( this, wxID_ANY, wxT("0.5"), wxDefaultPosition, wxSize(55, -1), wxTE_CENTRE | wxTE_READONLY );

	wxBoxSizer *pBoxRow4 = new wxBoxSizer( wxHORIZONTAL );
    pBoxRow4->Add( m_pTextNodeAlpha, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxALL, 1 );
    pBoxRow4->Add( m_pSliderNodeAlpha,   0, wxALIGN_CENTER | wxEXPAND | wxALL, 1);
	pBoxRow4->Add( m_pTxtNodeAlphaBox,   0, wxALIGN_CENTER | wxALL, 1);
	m_pConnectomicSizer->Add( pBoxRow4, 0, wxFIXED_MINSIZE | wxEXPAND, 0 );

    m_pTextEdgeSize = new wxStaticText( this, wxID_ANY, wxT("Edge size"), wxDefaultPosition, wxSize(70, -1), wxALIGN_CENTER );
	m_pSliderEdgeSize= new MySlider( this, wxID_ANY, 0, 1, 5, wxDefaultPosition, wxSize(100, -1), wxSL_HORIZONTAL | wxSL_AUTOTICKS );
	m_pSliderEdgeSize->SetValue( 2 );
	Connect( m_pSliderEdgeSize->GetId(), wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler(ConnectomicWindow::OnSliderDisplayMoved) );
    m_pTxtEdgeSizeBox = new wxTextCtrl( this, wxID_ANY, wxT("2.0"), wxDefaultPosition, wxSize(55, -1), wxTE_CENTRE | wxTE_READONLY );

	wxBoxSizer *pBoxRow5 = new wxBoxSizer( wxHORIZONTAL );
    pBoxRow5->Add( m_pTextEdgeSize, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxALL, 1 );
    pBoxRow5->Add( m_pSliderEdgeSize,   0, wxALIGN_CENTER | wxEXPAND | wxALL, 1);
	pBoxRow5->Add( m_pTxtEdgeSizeBox,   0, wxALIGN_CENTER | wxALL, 1);
	m_pConnectomicSizer->Add( pBoxRow5, 0, wxFIXED_MINSIZE | wxEXPAND, 0 );

	m_pTextEdgeAlpha = new wxStaticText( this, wxID_ANY, wxT("Edge Alpha"), wxDefaultPosition, wxSize(70, -1), wxALIGN_CENTER );
	m_pSliderEdgeAlpha = new MySlider( this, wxID_ANY, 0, 0, 100, wxDefaultPosition, wxSize(100, -1), wxSL_HORIZONTAL | wxSL_AUTOTICKS );
	m_pSliderEdgeAlpha->SetValue( 100 );
	Connect( m_pSliderEdgeAlpha->GetId(), wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler(ConnectomicWindow::OnSliderDisplayMoved) );
    m_pTxtEdgeAlphaBox = new wxTextCtrl( this, wxID_ANY, wxT("1.0"), wxDefaultPosition, wxSize(55, -1), wxTE_CENTRE | wxTE_READONLY );

	wxBoxSizer *pBoxRow6 = new wxBoxSizer( wxHORIZONTAL );
    pBoxRow6->Add( m_pTextEdgeAlpha, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxALL, 1 );
    pBoxRow6->Add( m_pSliderEdgeAlpha,   0, wxALIGN_CENTER | wxEXPAND | wxALL, 1);
	pBoxRow6->Add( m_pTxtEdgeAlphaBox,   0, wxALIGN_CENTER | wxALL, 1);
	m_pConnectomicSizer->Add( pBoxRow6, 0, wxFIXED_MINSIZE | wxEXPAND, 0 );

    m_pToggleFlashyEdges = new wxToggleButton( this, wxID_ANY,wxT("Flashy edges OFF"), wxDefaultPosition, wxSize(115, -1) );
    Connect( m_pToggleFlashyEdges->GetId(), wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler(ConnectomicWindow::OnToggleFlashyEdges) );

    m_pToggleOrientationDep = new wxToggleButton( this, wxID_ANY,wxT("Orient. Dep. OFF"), wxDefaultPosition, wxSize(115, -1) );
    Connect( m_pToggleOrientationDep->GetId(), wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler(ConnectomicWindow::OnToggleOrientationDep) );

    wxBoxSizer *pBoxRow7 = new wxBoxSizer( wxHORIZONTAL );
	pBoxRow7->Add( m_pToggleFlashyEdges, 0, wxALIGN_CENTER | wxALL, 1 );
    pBoxRow7->Add( m_pToggleOrientationDep, 0, wxALIGN_CENTER | wxALL, 1 );
	m_pConnectomicSizer->Add( pBoxRow7, 0, wxFIXED_MINSIZE | wxEXPAND, 0 );

}

void ConnectomicWindow::OnSize( wxSizeEvent &WXUNUSED(event) )
{
	
}

void ConnectomicWindow::OnPaint( wxPaintEvent &WXUNUSED(event) )
{
    wxPaintDC dc( this );
}

wxSizer* ConnectomicWindow::getWindowSizer()
{
    return m_pConnectomicSizer;
}

void ConnectomicWindow::onSelectLabels( wxCommandEvent& event )
{
    long item = m_pMainFrame->getCurrentListIndex();
	Anatomy* pMap = (Anatomy*)DatasetManager::getInstance()->getDataset (MyApp::frame->m_pListCtrl->GetItem( item )); 

	if( pMap != NULL && pMap->getBands() == 1 )
    {
        double nbLabels;
        m_pNbLabels->GetValue().ToDouble( &nbLabels );    
        ConnectomeHelper::getInstance()->getConnectome()->setNbLabels( nbLabels );

		m_pBtnSelectLabels->SetLabel( pMap->getName() );
        m_pBtnSelectLabels->SetBackgroundColour(wxNullColour);
        ConnectomeHelper::getInstance()->getConnectome()->setLabels( (Anatomy *)DatasetManager::getInstance()->getDataset( m_pMainFrame->m_pListCtrl->GetItem( item ) ) );
        ConnectomeHelper::getInstance()->setLabelsSelected(true);
	}

}

void ConnectomicWindow::onSelectEdges( wxCommandEvent& event )
{
    long item = m_pMainFrame->getCurrentListIndex();
    Fibers* pFibers = DatasetManager::getInstance()->getSelectedFibers( m_pMainFrame->m_pListCtrl->GetItem( item ) );
    if( pFibers != NULL )
    {
        m_pBtnSelectEdges->SetLabel( pFibers->getName() );
        m_pBtnSelectEdges->SetBackgroundColour(wxNullColour);
        ConnectomeHelper::getInstance()->getConnectome()->setEdges( (Fibers *)DatasetManager::getInstance()->getDataset( m_pMainFrame->m_pListCtrl->GetItem( item ) ) );
        ConnectomeHelper::getInstance()->setEdgesSelected(true);
    }
}

void ConnectomicWindow::OnNbLabels( wxCommandEvent& event )
{
    double nbLabels;
    m_pNbLabels->GetValue().ToDouble( &nbLabels );    
    ConnectomeHelper::getInstance()->getConnectome()->setNbLabels( nbLabels );
    
}

void ConnectomicWindow::OnSliderDisplayMoved( wxCommandEvent& event )
{
	float sliderValue = m_pSliderNodeSize->GetValue() / 10.0f;
	m_pTxtNodeSizeBox->SetValue( wxString::Format( wxT( "%.1f"), sliderValue ) );
	ConnectomeHelper::getInstance()->getConnectome()->setNodeSize( sliderValue );

    sliderValue = m_pSliderNodeAlpha->GetValue() / 100.0f;
	m_pTxtNodeAlphaBox->SetValue( wxString::Format( wxT( "%.2f"), sliderValue ) );
	ConnectomeHelper::getInstance()->getConnectome()->setNodeAlpha( sliderValue );

    sliderValue = m_pSliderEdgeSize->GetValue();
	m_pTxtEdgeSizeBox->SetValue( wxString::Format( wxT( "%.1f"), sliderValue ) );
	ConnectomeHelper::getInstance()->getConnectome()->setEdgeSize( sliderValue );

    sliderValue = m_pSliderEdgeAlpha->GetValue() / 100.0f;
	m_pTxtEdgeAlphaBox->SetValue( wxString::Format( wxT( "%.2f"), sliderValue ) );
	ConnectomeHelper::getInstance()->getConnectome()->setEdgeAlpha( sliderValue );

    ConnectomeHelper::getInstance()->setDirty( true );
}

void ConnectomicWindow::OnToggleFlashyEdges( wxCommandEvent& event )
{
    ConnectomeHelper::getInstance()->getConnectome()->toggleFlashyEdges();

    if( !ConnectomeHelper::getInstance()->getConnectome()->isFlashyEdges())
    {
        m_pToggleFlashyEdges->SetLabel(wxT( "Flashy edges OFF"));
    }
    else
    {
		m_pToggleFlashyEdges->SetLabel(wxT( "Flashy edges ON"));
    }
}

void ConnectomicWindow::OnToggleOrientationDep( wxCommandEvent& event )
{
    ConnectomeHelper::getInstance()->getConnectome()->toggleOrientationDep();

    if( !ConnectomeHelper::getInstance()->getConnectome()->isOrientationDep())
    {
        m_pToggleOrientationDep->SetLabel(wxT( "Orient. Dep. OFF"));
    }
    else
    {
		m_pToggleOrientationDep->SetLabel(wxT( "Orient. Dep. ON"));
    }
}