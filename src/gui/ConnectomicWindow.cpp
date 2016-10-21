#include "ConnectomicWindow.h"
#include "../dataset/ConnectomeHelper.h"
#include "MainFrame.h"
#include "SceneManager.h"

#include "../main.h"
#include "../dataset/Anatomy.h"

#include <wx/checkbox.h>
#include <wx/tglbtn.h>
#include <wx/treectrl.h>
#include <wx/colordlg.h>





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

    m_pConnectomicSizer->AddSpacer( 8 );

    m_pNbLabels  = new wxTextCtrl( this, wxID_ANY, wxString::Format( wxT( "%i" ), 161 ), wxDefaultPosition, wxSize( 75, -1 ) );
    Connect( m_pNbLabels->GetId(),  wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( ConnectomicWindow::OnNbLabels ) );

    wxBoxSizer *pBoxRow2 = new wxBoxSizer( wxHORIZONTAL );
    pBoxRow2->Add( new wxStaticText( this, wxID_ANY, wxT("Number of labels"), wxDefaultPosition, wxSize(110, -1), wxALIGN_CENTER ), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxALL, 1 );
	pBoxRow2->Add( m_pNbLabels, 0, wxALIGN_CENTER | wxALL, 1 );
	m_pConnectomicSizer->Add( pBoxRow2, 0, wxFIXED_MINSIZE | wxEXPAND, 0 );

    m_pBtnSelectLabels = new wxButton( this, wxID_ANY,wxT("Nodes not selected"), wxDefaultPosition, wxSize(230, -1) );
	Connect( m_pBtnSelectLabels->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ConnectomicWindow::onSelectLabels) );
    m_pBtnSelectLabels->SetBackgroundColour(wxColour( 255, 147, 147 ));

    m_pBtnLoadLabels = new wxButton( this, wxID_ANY,wxT("Load labels (.txt)"), wxDefaultPosition, wxSize(230, -1) );
	pMf->Connect( m_pBtnLoadLabels->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MainFrame::onLoadLabels) );


    m_pBtnSelectEdges = new wxButton( this, wxID_ANY,wxT("Edges not selected"), wxDefaultPosition, wxSize(230, -1) );
	Connect( m_pBtnSelectEdges->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ConnectomicWindow::onSelectEdges) );
    m_pBtnSelectEdges->SetBackgroundColour(wxColour( 255, 147, 147 ));

    m_pBtnClearConnectome = new wxButton( this, wxID_ANY,wxT("Clear Connectome"), wxDefaultPosition, wxSize(230, -1) );
	Connect( m_pBtnClearConnectome->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ConnectomicWindow::onClearConnectome) );

	wxBoxSizer *pBoxRow1 = new wxBoxSizer( wxVERTICAL );
	pBoxRow1->Add( m_pBtnSelectLabels, 0, wxALIGN_CENTER | wxALL, 1 );
    pBoxRow1->Add( m_pBtnLoadLabels, 0, wxALIGN_CENTER | wxALL, 1 );
    pBoxRow1->Add( m_pBtnSelectEdges, 0, wxALIGN_CENTER | wxALL, 1 );
    pBoxRow1->Add( m_pBtnClearConnectome, 0, wxALIGN_CENTER | wxALL, 1 );
    //pBoxRow1->Add( new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxSize(230,-1),wxHORIZONTAL,wxT("Separator")), 0, wxALIGN_RIGHT | wxALL, 1 );
	m_pConnectomicSizer->Add( pBoxRow1, 0, wxFIXED_MINSIZE | wxEXPAND, 0 );

    m_pConnectomicSizer->AddSpacer( 8 );

    m_pTextEdgeThreshold = new wxStaticText( this, wxID_ANY, wxT("Threshold"), wxDefaultPosition, wxSize(70, -1), wxALIGN_CENTER );
	m_pSliderEdgeThreshold= new MySlider( this, wxID_ANY, 0, 0, 200, wxDefaultPosition, wxSize(100, -1), wxSL_HORIZONTAL | wxSL_AUTOTICKS );
	m_pSliderEdgeThreshold->SetValue( 0 );
	Connect( m_pSliderEdgeThreshold->GetId(), wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler(ConnectomicWindow::onSliderEdgeThreshold) );
    m_pTxtEdgeThresholdBox = new wxTextCtrl( this, wxID_ANY, wxT("0.0"), wxDefaultPosition, wxSize(55, -1), wxTE_CENTRE | wxTE_READONLY );

	wxBoxSizer *pBoxThresh = new wxBoxSizer( wxHORIZONTAL );
    pBoxThresh->Add( m_pTextEdgeThreshold, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxALL, 1 );
    pBoxThresh->Add( m_pSliderEdgeThreshold,   0, wxALIGN_CENTER | wxEXPAND | wxALL, 1);
	pBoxThresh->Add( m_pTxtEdgeThresholdBox,   0, wxALIGN_CENTER | wxALL, 1);
	m_pConnectomicSizer->Add( pBoxThresh, 0, wxFIXED_MINSIZE | wxEXPAND, 0 );

    m_pTextNodeSize = new wxStaticText( this, wxID_ANY, wxT("Node size"), wxDefaultPosition, wxSize(70, -1), wxALIGN_CENTER );
	m_pSliderNodeSize= new MySlider( this, wxID_ANY, 0, 1, 100, wxDefaultPosition, wxSize(100, -1), wxSL_HORIZONTAL | wxSL_AUTOTICKS );
	m_pSliderNodeSize->SetValue( 20 );
	Connect( m_pSliderNodeSize->GetId(), wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler(ConnectomicWindow::OnSliderDisplayMoved) );
    m_pTxtNodeSizeBox = new wxTextCtrl( this, wxID_ANY, wxT("2.0"), wxDefaultPosition, wxSize(55, -1), wxTE_CENTRE | wxTE_READONLY );

	wxBoxSizer *pBoxRow3 = new wxBoxSizer( wxHORIZONTAL );
    pBoxRow3->Add( m_pTextNodeSize, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxALL, 1 );
    pBoxRow3->Add( m_pSliderNodeSize,   0, wxALIGN_CENTER | wxEXPAND | wxALL, 1);
	pBoxRow3->Add( m_pTxtNodeSizeBox,   0, wxALIGN_CENTER | wxALL, 1);
	m_pConnectomicSizer->Add( pBoxRow3, 0, wxFIXED_MINSIZE | wxEXPAND, 0 );

	m_pTextNodeAlpha = new wxStaticText( this, wxID_ANY, wxT("Node Alpha"), wxDefaultPosition, wxSize(70, -1), wxALIGN_CENTER );
	m_pSliderNodeAlpha = new MySlider( this, wxID_ANY, 0, 0, 100, wxDefaultPosition, wxSize(60, -1), wxSL_HORIZONTAL | wxSL_AUTOTICKS );
	m_pSliderNodeAlpha->SetValue( 100 );
	Connect( m_pSliderNodeAlpha->GetId(), wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler(ConnectomicWindow::OnSliderDisplayMoved) );
    m_pTxtNodeAlphaBox = new wxTextCtrl( this, wxID_ANY, wxT("1.0"), wxDefaultPosition, wxSize(55, -1), wxTE_CENTRE | wxTE_READONLY );

    wxImage bmpColor(MyApp::iconsPath+ wxT("colorSelect.png" ), wxBITMAP_TYPE_PNG);
    m_pbtnSelectColor = new wxBitmapButton(this, wxID_ANY, bmpColor, wxDefaultPosition, wxSize(40,-1));
    Connect(m_pbtnSelectColor->GetId(),wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ConnectomicWindow::OnAssignColorNode ));

	wxBoxSizer *pBoxRow4 = new wxBoxSizer( wxHORIZONTAL );
    pBoxRow4->Add( m_pTextNodeAlpha, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxALL, 1 );
    pBoxRow4->Add( m_pSliderNodeAlpha,   0, wxALIGN_CENTER | wxEXPAND | wxALL, 1);
    pBoxRow4->Add( m_pbtnSelectColor,   0, wxALIGN_CENTER | wxALL, 1);
	pBoxRow4->Add( m_pTxtNodeAlphaBox,   0, wxALIGN_CENTER | wxALL, 1);
	m_pConnectomicSizer->Add( pBoxRow4, 0, wxFIXED_MINSIZE | wxEXPAND, 0 );

    m_pTextEdgeSize = new wxStaticText( this, wxID_ANY, wxT("Edge size"), wxDefaultPosition, wxSize(70, -1), wxALIGN_CENTER );
	m_pSliderEdgeSize= new MySlider( this, wxID_ANY, 0, 1, 7, wxDefaultPosition, wxSize(100, -1), wxSL_HORIZONTAL | wxSL_AUTOTICKS );
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

 //   m_pToggleFlashyEdges = new wxToggleButton( this, wxID_ANY,wxT("Flashy edges OFF"), wxDefaultPosition, wxSize(120, -1) );
 //   Connect( m_pToggleFlashyEdges->GetId(), wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler(ConnectomicWindow::OnToggleFlashyEdges) );

    m_pToggleOrientationDep = new wxToggleButton( this, wxID_ANY,wxT("Enable depth sorting"), wxDefaultPosition, wxSize(230, -1) );
    Connect( m_pToggleOrientationDep->GetId(), wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler(ConnectomicWindow::OnToggleOrientationDep) );

    m_pToggleShowFibers = new wxToggleButton( this, wxID_ANY,wxT("Show selected streamlines"), wxDefaultPosition, wxSize(230, -1) );
    Connect( m_pToggleShowFibers->GetId(), wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler(ConnectomicWindow::OnToggleShowFibers) );

    wxBoxSizer *pBoxRow7 = new wxBoxSizer( wxVERTICAL );
    pBoxRow7->Add( m_pToggleOrientationDep, 0, wxALIGN_CENTER | wxALL, 1 );
    pBoxRow7->Add( m_pToggleShowFibers, 0, wxALIGN_RIGHT | wxALL, 1 );
    m_pConnectomicSizer->Add( pBoxRow7, 0,  wxFIXED_MINSIZE | wxALL, 2 );

    wxTextCtrl *metricZone = new wxTextCtrl( this, wxID_ANY, wxT("Global graph metrics"), wxDefaultPosition, wxSize(230, -1), wxTE_CENTER | wxTE_READONLY );
    metricZone->SetBackgroundColour( *wxLIGHT_GREY );
    wxFont metric_font = metricZone->GetFont();
    metric_font.SetPointSize( 10 );
    metric_font.SetWeight( wxFONTWEIGHT_BOLD );
    metricZone->SetFont( metric_font );

	wxBoxSizer *pBoxMetricZone = new wxBoxSizer( wxVERTICAL );
    pBoxMetricZone->Add( new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxSize(230,-1),wxHORIZONTAL,wxT("Separator")), 0, wxALIGN_RIGHT | wxALL, 1 );
	pBoxMetricZone->Add( metricZone,   0, wxALIGN_CENTER | wxALL, 1);
	m_pConnectomicSizer->Add( pBoxMetricZone, 0, wxFIXED_MINSIZE | wxALL, 2 );

    m_pConnectomicSizer->AddSpacer( 8 );

    //////////////////////////////////////////////////////////////////////////

    m_pGridGlobalInfo = new wxGrid( this, wxID_ANY );
    m_pGridGlobalInfo->SetRowLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
    wxFont font = m_pGridGlobalInfo->GetFont();
    font.SetPointSize( 8 );
    font.SetWeight( wxFONTWEIGHT_BOLD );
    m_pGridGlobalInfo->SetFont( font );
    m_pGridGlobalInfo->SetColLabelSize( 2 );
    m_pGridGlobalInfo->CreateGrid( 6, 1, wxGrid::wxGridSelectCells );
    m_pGridGlobalInfo->SetColLabelValue( 0, wxT( "" ) );
    m_pGridGlobalInfo->SetRowLabelValue( 0, wxT( "# of nodes" ) );
    m_pGridGlobalInfo->SetRowLabelValue( 1, wxT( "# of edges" ) );
    m_pGridGlobalInfo->SetRowLabelValue( 2, wxT( "Density" ) );
    m_pGridGlobalInfo->SetRowLabelValue( 3, wxT( "Mean degree" ) );
    m_pGridGlobalInfo->SetRowLabelValue( 4, wxT( "Global efficiency" ) );
    m_pGridGlobalInfo->SetRowLabelValue( 5, wxT( "Transitivity" ) );

    m_pGridGlobalInfo->SetRowLabelSize( 150 );

    m_pConnectomicSizer->Add( m_pGridGlobalInfo, 0, wxALIGN_CENTER | wxALL, 0 );

    wxTextCtrl *nodeZone = new wxTextCtrl( this, wxID_ANY, wxT("Node metrics"), wxDefaultPosition, wxSize(230, -1), wxTE_CENTER | wxTE_READONLY );
    nodeZone->SetBackgroundColour( *wxLIGHT_GREY );
    wxFont node_font = nodeZone->GetFont();
    node_font.SetPointSize( 10 );
    node_font.SetWeight( wxFONTWEIGHT_BOLD );
    nodeZone->SetFont( node_font );

	wxBoxSizer *pBoxNodeZone = new wxBoxSizer( wxVERTICAL );
    pBoxNodeZone->Add( new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxSize(230,-1),wxHORIZONTAL,wxT("Separator")), 0, wxALIGN_RIGHT | wxALL, 1 );
	pBoxNodeZone->Add( nodeZone,   0, wxALIGN_CENTER | wxALL, 1);
	m_pConnectomicSizer->Add( pBoxNodeZone, 0, wxFIXED_MINSIZE | wxALL, 2 );

    m_pConnectomicSizer->AddSpacer( 8 );

    //////////////////////////////////////////////////////////////////////////

    ConnectomeHelper::getInstance()->m_pGridNodeInfo = new wxGrid( this, wxID_ANY );
    ConnectomeHelper::getInstance()->m_pGridNodeInfo->SetRowLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
    wxFont nodefont = ConnectomeHelper::getInstance()->m_pGridNodeInfo->GetFont();
    nodefont.SetPointSize( 8 );
    nodefont.SetWeight( wxFONTWEIGHT_BOLD );
    ConnectomeHelper::getInstance()->m_pGridNodeInfo->SetFont( nodefont );
    ConnectomeHelper::getInstance()->m_pGridNodeInfo->SetColLabelSize( 2 );
    ConnectomeHelper::getInstance()->m_pGridNodeInfo->CreateGrid( 9, 1, wxGrid::wxGridSelectCells );
    ConnectomeHelper::getInstance()->m_pGridNodeInfo->SetColLabelValue( 0, wxT( "" ) );
    ConnectomeHelper::getInstance()->m_pGridNodeInfo->SetRowLabelValue( 0, wxT( "Name" ) );
    ConnectomeHelper::getInstance()->m_pGridNodeInfo->SetRowLabelValue( 1, wxT( "ID" ) );
    ConnectomeHelper::getInstance()->m_pGridNodeInfo->SetRowLabelValue( 2, wxT( "Degree" ) );
    ConnectomeHelper::getInstance()->m_pGridNodeInfo->SetRowLabelValue( 3, wxT( "Strength" ) );
    ConnectomeHelper::getInstance()->m_pGridNodeInfo->SetRowLabelValue( 4, wxT( "Eigen centrality" ) );
    ConnectomeHelper::getInstance()->m_pGridNodeInfo->SetRowLabelValue( 5, wxT( "Closeness centrality" ) );
    ConnectomeHelper::getInstance()->m_pGridNodeInfo->SetRowLabelValue( 6, wxT( "Betweenness centrality" ) );
    ConnectomeHelper::getInstance()->m_pGridNodeInfo->SetRowLabelValue( 7, wxT( "Local efficiency" ) );
    ConnectomeHelper::getInstance()->m_pGridNodeInfo->SetRowLabelValue( 8, wxT( "Clustering coefficient" ) );

    ConnectomeHelper::getInstance()->m_pGridNodeInfo->SetRowLabelSize( 150 );

    m_pConnectomicSizer->Add( ConnectomeHelper::getInstance()->m_pGridNodeInfo, 0, wxALIGN_CENTER | wxALL, 0 );

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
        ConnectomeHelper::getInstance()->createConnectome();
        double nbLabels;
        m_pNbLabels->GetValue().ToDouble( &nbLabels );    
        ConnectomeHelper::getInstance()->getConnectome()->setNbLabels( nbLabels );
        m_pGridGlobalInfo->SetCellValue( 0,  0, wxString::Format( wxT( "%i" ),     int(nbLabels)        ) );
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
        Fibers* fibers = (Fibers *)DatasetManager::getInstance()->getDataset( m_pMainFrame->m_pListCtrl->GetItem( item ) );

        std::vector<bool> selected(fibers->getFibersCount(), false);
        fibers->setSelected(selected);

        ConnectomeHelper::getInstance()->getConnectome()->setEdges( fibers );

        m_pGridGlobalInfo->SetCellValue( 1,  0, wxString::Format( wxT( "%i" ), ConnectomeHelper::getInstance()->getConnectome()->getGlobalStats().m_NbEdges        ) );
        m_pGridGlobalInfo->SetCellValue( 2,  0, wxString::Format( wxT( "%.2f" ), ConnectomeHelper::getInstance()->getConnectome()->getGlobalStats().m_Density       ) );
        m_pGridGlobalInfo->SetCellValue( 3,  0, wxString::Format( wxT( "%.2f" ), ConnectomeHelper::getInstance()->getConnectome()->getGlobalStats().m_meanDegree       ) );
        ConnectomeHelper::getInstance()->setEdgesSelected(true);
    }
}

void ConnectomicWindow::onClearConnectome( wxCommandEvent& event )
{
    ConnectomeHelper::getInstance()->deleteConnectome();
    //ConnectomeHelper::getInstance()->getConnectome()->clearConnectome();

	m_pBtnSelectLabels->SetLabel( wxT( "Nodes not selected") );
    m_pBtnSelectLabels->SetBackgroundColour(wxColour(255, 147, 147));
    
    m_pBtnSelectEdges->SetLabel( wxT( "Edges not selected") );
    m_pBtnSelectEdges->SetBackgroundColour(wxColour(255, 147, 147));

    m_pGridGlobalInfo->SetCellValue( 0,  0, wxT( "" ) );
    m_pGridGlobalInfo->SetCellValue( 1,  0, wxT( "" ) );
    m_pGridGlobalInfo->SetCellValue( 2,  0, wxT( "" ) );
    m_pGridGlobalInfo->SetCellValue( 3,  0, wxT( "" ) );

    ConnectomeHelper::getInstance()->m_pGridNodeInfo->SetCellValue( 0,  0, wxT( "" )) ;
    ConnectomeHelper::getInstance()->m_pGridNodeInfo->SetCellValue( 1,  0, wxT( "" ));
    ConnectomeHelper::getInstance()->m_pGridNodeInfo->SetCellValue( 2,  0, wxT( "" ));
    ConnectomeHelper::getInstance()->m_pGridNodeInfo->SetCellValue( 3,  0, wxT( "" ));

}

void ConnectomicWindow::OnNbLabels( wxCommandEvent& event )
{
    ConnectomeHelper::getInstance()->createConnectome();

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

void ConnectomicWindow::onSliderEdgeThreshold( wxCommandEvent& event )
{
    float sliderValue = m_pSliderEdgeThreshold->GetValue() / 1000.0f;
	m_pTxtEdgeThresholdBox->SetValue( wxString::Format( wxT( "%.3f"), sliderValue ) );
	ConnectomeHelper::getInstance()->getConnectome()->setEdgeThreshold( sliderValue );
    ConnectomeHelper::getInstance()->getConnectome()->computeNodeDegreeAndStrength();
    ConnectomeHelper::getInstance()->getConnectome()->computeGlobalMetrics();

    m_pGridGlobalInfo->SetCellValue( 0,  0, wxString::Format( wxT( "%i" ),   ConnectomeHelper::getInstance()->getConnectome()->getGlobalStats().m_NbNodes            ) );
    m_pGridGlobalInfo->SetCellValue( 1,  0, wxString::Format( wxT( "%i" ), ConnectomeHelper::getInstance()->getConnectome()->getGlobalStats().m_NbEdges        ) );
    m_pGridGlobalInfo->SetCellValue( 2,  0, wxString::Format( wxT( "%.2f" ), ConnectomeHelper::getInstance()->getConnectome()->getGlobalStats().m_Density       ) );
    m_pGridGlobalInfo->SetCellValue( 3,  0, wxString::Format( wxT( "%.2f" ), ConnectomeHelper::getInstance()->getConnectome()->getGlobalStats().m_meanDegree       ) );

}

bool ConnectomicWindow::SelectColor( wxColour &col )
{
    wxColourData colorData;
    
    for( int i = 0; i < 10; ++i )
    {
        wxColour colorTemp(i * 28, i * 28, i * 28);
        colorData.SetCustomColour(i, colorTemp);
    }
    
    int i = 10;
    wxColour colorTemp ( 255, 0, 0 );
    colorData.SetCustomColour( i++, colorTemp );
    wxColour colorTemp1( 0, 255, 0 );
    colorData.SetCustomColour( i++, colorTemp1 );
    wxColour colorTemp2( 0, 0, 255 );
    colorData.SetCustomColour( i++, colorTemp2 );
    wxColour colorTemp3( 255, 255, 0 );
    colorData.SetCustomColour( i++, colorTemp3 );
    wxColour colorTemp4( 255, 0, 255 );
    colorData.SetCustomColour( i++, colorTemp4 );
    wxColour colorTemp5( 0, 255, 255 );
    colorData.SetCustomColour( i++, colorTemp5 );
    
    wxColourDialog dialog( this, &colorData );

    if( dialog.ShowModal() == wxID_OK )
    {
        wxColourData retData = dialog.GetColourData();
        col = retData.GetColour();
        return true;
    }

    return false;
}

void ConnectomicWindow::OnAssignColorNode( wxCommandEvent& WXUNUSED(event) )
{

    wxColour newCol;
    
    bool success = SelectColor( newCol );
    
    if( !success )
    {
        return;
    }
   
    ConnectomeHelper::getInstance()->getConnectome()->setNodeColor( newCol );

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

void ConnectomicWindow::OnToggleShowFibers( wxCommandEvent& event )
{
    ConnectomeHelper::getInstance()->toggleShowStreamlines();
    ConnectomeHelper::getInstance()->getConnectome()->setSelectedStreamlines();

    if( !ConnectomeHelper::getInstance()->isShowStreamlines())
    {
        m_pToggleShowFibers->SetLabel(wxT( "Show selected streamlines"));
    }
    else
    {
		m_pToggleShowFibers->SetLabel(wxT( "Hide selected streamlines"));
    }
}


void ConnectomicWindow::OnToggleOrientationDep( wxCommandEvent& event )
{
    ConnectomeHelper::getInstance()->getConnectome()->toggleOrientationDep();

    if( !ConnectomeHelper::getInstance()->getConnectome()->isOrientationDep())
    {
        m_pToggleOrientationDep->SetLabel(wxT( "Enable depth sorting"));
    }
    else
    {
		m_pToggleOrientationDep->SetLabel(wxT( "Disable depth sorting"));
    }
}