/////////////////////////////////////////////////////////////////////////////
// Name:            Maximas.cpp
// Author:          Maxime Chamberland
/////////////////////////////////////////////////////////////////////////////

#include "Maximas.h"

#include "DatasetManager.h"
#include "RTTrackingHelper.h"
#include "../Logger.h"
#include "../gfx/ShaderHelper.h"
#include "../gui/MyListCtrl.h"
#include "../gui/SceneManager.h"
#include "../misc/nifti/nifti1_io.h"

#include <GL/glew.h>
#include <wx/math.h>
#include <wx/xml/xml.h>

#include <algorithm>
#include <fstream>
#include <limits>
#include <vector>
using std::vector;

#if defined(__WXMAC__) || defined(__WXMSW__)
#ifndef isnan
inline bool isnan(double x) {
    return x != x;
}
#endif
#endif

///////////////////////////////////////////
Maximas::Maximas( const wxString &filename )
: Glyph(),
m_displayType( SLICES ),
m_dataType( 16 ),
m_doRotate(false)
{
    m_fullPath = filename;
    m_scalingFactor = 3.0f;


#ifdef __WXMSW__
    m_name = filename.AfterLast( '\\' );
#else
    m_name = filename.AfterLast( '/' );
#endif

}

//////////////////////////////////////////////////////////////////////////
Maximas::~Maximas()
{
    Logger::getInstance()->print( wxT( "Maximas destructor called but nothing to do." ), LOGLEVEL_DEBUG );
}

//////////////////////////////////////////////////////////////////////////
bool Maximas::load( nifti_image *pHeader, nifti_image *pBody )
{
    m_columns  = pHeader->dim[1]; //XSlice
    m_rows     = pHeader->dim[2]; //YSlice
    m_frames   = pHeader->dim[3]; //ZSlice
    m_bands    = pHeader->dim[4]; // 3 * Number of sticks.
    m_dataType = pHeader->datatype;//16

    m_voxelSizeX = pHeader->dx;
    m_voxelSizeY = pHeader->dy;
    m_voxelSizeZ = pHeader->dz;

    float voxelX = DatasetManager::getInstance()->getVoxelX();
    float voxelY = DatasetManager::getInstance()->getVoxelY();
    float voxelZ = DatasetManager::getInstance()->getVoxelZ();
	const float VOXEL_SIZE_EPSILON(0.0001f);

    if( m_voxelSizeX != voxelX || m_voxelSizeY != voxelY || m_voxelSizeZ != voxelZ )
    {
		// NOTE TO THE TEAM: THIS IS NOT A VERY GOOD THING TO DO. We do it to support
        // different software that save the metadata with incorrect rounding / conversion.
        if( std::abs(m_voxelSizeX - voxelX) < VOXEL_SIZE_EPSILON &&
            std::abs(m_voxelSizeY - voxelY) < VOXEL_SIZE_EPSILON &&
            std::abs(m_voxelSizeZ - voxelZ) < VOXEL_SIZE_EPSILON )
        {
            // In this case, we are in the expected error range between something coming from an integer
            // and something coming from a float. We accept it, make sure thesizes fit for the new anatomy, 
            // and still display a debug message for developers.
            m_voxelSizeX = voxelX;
            m_voxelSizeY = voxelY;
            m_voxelSizeZ = voxelZ;
                
            Logger::getInstance()->print( wxT( "Voxel sizes did not exactly fit. In expected float range error. Using the already loaded voxel sizes." ),
                                            LOGLEVEL_DEBUG );
        }
        else
        {
            Logger::getInstance()->print( wxT( "Voxel size different from anatomy" ), LOGLEVEL_ERROR );
            return false;
        }
    }

    m_type = MAXIMAS;

    int datasetSize = pHeader->dim[1] * pHeader->dim[2] * pHeader->dim[3];
    
    l_fileFloatData.assign( datasetSize * m_bands, 0.0f);
    float* pData = (float*)pBody->data;

    for( int i( 0 ); i < datasetSize; ++i )
    {
        for( int j( 0 ); j < m_bands; ++j )
        {
            if(!isnan(pData[j * datasetSize + i]))
                l_fileFloatData[i * m_bands + j] = pData[j * datasetSize + i];
        }
    }
    
    createStructure( l_fileFloatData );
    bool m_originalAxialOrientation;
    bool m_originalSagOrientation;
    if( pHeader->sform_code > 0 )
    {
        if( pHeader->sto_xyz.m[0][0] < 0.0 )
        {
            m_originalAxialOrientation = ORIENTATION_RIGHT_TO_LEFT;
        }
        else
        {
            m_originalAxialOrientation = ORIENTATION_LEFT_TO_RIGHT;
        }
        if( pHeader->sto_xyz.m[1][1] < 0.0 )
        {
            m_originalSagOrientation = ORIENTATION_ANT_TO_POST;
        }
        else
        {
            m_originalSagOrientation = ORIENTATION_POST_TO_ANT;
        }
    }
    else if( pHeader->qform_code > 0 )
    {
        if( pHeader->qto_xyz.m[0][0] < 0.0 )
        {
            m_originalAxialOrientation = ORIENTATION_RIGHT_TO_LEFT;
        }
        else
        {
            m_originalAxialOrientation = ORIENTATION_LEFT_TO_RIGHT;
        }
        if( pHeader->qto_xyz.m[1][1] < 0.0 )
        {
            m_originalSagOrientation = ORIENTATION_ANT_TO_POST;
        }
        else
        {
            m_originalSagOrientation = ORIENTATION_POST_TO_ANT;
        }
    }

    if( m_originalAxialOrientation == ORIENTATION_RIGHT_TO_LEFT )
    { 
        flipAxis( X_AXIS, true );
        RTTrackingHelper::getInstance()->setMaximaFlip(Vector(-1,1,1));
        flipAnat( X_AXIS );     
    }
    if( m_originalSagOrientation == ORIENTATION_ANT_TO_POST )
    {
        flipAxis( Y_AXIS, true );
        RTTrackingHelper::getInstance()->setMaximaFlip(Vector(1,-1,1));
        flipAnat( Y_AXIS );
    }

    m_isLoaded = true;
    return true;
}



//////////////////////////////////////////////////////////////////////////
void Maximas::saveNifti( wxString fileName )
{
    // Prevents copying the whole vector
    vector<float> *pDataset = &l_fileFloatData;

    int dims[] = { 4, m_columns, m_rows, m_frames, m_bands, 0, 0, 0 };
    nifti_image* pImage(NULL);
    pImage = nifti_make_new_nim( dims, m_dataType, 1 );
    
    if( !fileName.EndsWith( _T( ".nii" ) ) && !fileName.EndsWith( _T( ".nii.gz" ) ) )
    {
        fileName += _T( ".nii.gz" );
    }   

    char fn[1024];
    strcpy( fn, (const char*)fileName.mb_str( wxConvUTF8 ) );

    pImage->datatype   = m_dataType;
    pImage->fname = fn;
    pImage->dx = m_voxelSizeX;
    pImage->dy = m_voxelSizeY;
    pImage->dz = m_voxelSizeZ;
    
    // Prepare for transform saving.
    pImage->qform_code = 1;
    float qb(0.0f), qc(0.0f), qd(0.0f);
    float qx(0.0f), qy(0.0f), qz(0.0f);
    float dx(0.0f), dy(0.0f), dz(0.0f);
    float qfac(0.0f);
    
    FMatrix &system_transform = DatasetManager::getInstance()->getNiftiTransform();
    mat44 tempTransfo;
    
    // Create a temp transform in the type that nifti functions expect,
    // and set the qto_xyz transform at the same time.
    for( int i(0); i < 4; ++i)
    {
        for( int j(0); j < 4; ++j)
        {
            tempTransfo.m[i][j] = system_transform(i, j);
            pImage->qto_xyz.m[i][j] = tempTransfo.m[i][j];
        }
    }
    
    // The nifti library uses the quatern_x, qoffset_x and qfac field to 
    // decide the orientation and transform, when saving with a qform_code >= 1.
    // We get the quaternion params from the transformation matrix.
    nifti_mat44_to_quatern(tempTransfo, &qb, &qc, &qd, &qx, &qy, &qz, &dx, &dy, &dz, &qfac);
    
    pImage->quatern_b = qb;
    pImage->quatern_c = qc;
    pImage->quatern_d = qd;
    pImage->qoffset_x = qx;
    pImage->qoffset_y = qy;
    pImage->qoffset_z = qz;
    pImage->qfac = qfac;

    vector<float> tmp( pDataset->size() );
    int datasetSize = m_columns * m_rows * m_frames;
    
    for( int i( 0 ); i < datasetSize; ++i )
    {
        for( int j( 0 ); j < m_bands; ++j )
        {
            tmp[j * datasetSize + i] = l_fileFloatData[i * m_bands + j];
        }
    }
    
    // Do not move the call to nifti_image_write out of the 
    // if, because it will crash, since the temp vector will
    // not exist anymore, and pImage->data will point to garbage.
    pImage->data = &tmp[0];
    nifti_image_write( pImage );

}

//////////////////////////////////////////////////////////////////////////
bool Maximas::createStructure  ( std::vector< float > &i_fileFloatData )
{
    m_nbGlyphs         = DatasetManager::getInstance()->getColumns() * DatasetManager::getInstance()->getRows() * DatasetManager::getInstance()->getFrames();
    m_mainDirections.resize( m_nbGlyphs );

    vector< float >::iterator it;
    int i = 0;

	FMatrix transform = FMatrix( DatasetManager::getInstance()->getNiftiTransform() );
	FMatrix rotMat( 3, 3 );
	transform.getSubMatrix( rotMat, 0, 0 );
    
	storedRot = rotMat*(1.0f/abs(rotMat(0,0))); //Divide by identity values for scaling

	//std::cout << storedRot( 0, 0 ) << " " << storedRot( 0, 1 ) << " " << storedRot( 0, 2 ) << std::endl;
	//std::cout << storedRot( 1, 0 ) << " " << storedRot( 1, 1 ) << " " << storedRot( 1, 2 ) << std::endl;
	//std::cout << storedRot( 2, 0 ) << " " << storedRot( 2, 1 ) << " " << storedRot( 2, 2 ) << std::endl;
	//
    //Fetching the directions
    for( it = i_fileFloatData.begin(), i = 0; it != i_fileFloatData.end(); it += m_bands, ++i )
    { 
        m_mainDirections[i].insert( m_mainDirections[i].end(), it, it + m_bands );	
    }

	for( int i = 0; i < m_mainDirections.size(); i++ )
    { 

		FMatrix p1( 3, 1 );
		FMatrix p2( 3, 1 );
		FMatrix p3( 3, 1 );
        p1( 0, 0 ) = m_mainDirections[i][0];
        p1( 1, 0 ) = m_mainDirections[i][1];
        p1( 2, 0 ) = m_mainDirections[i][2];

		p2( 0, 0 ) = m_mainDirections[i][3];
        p2( 1, 0 ) = m_mainDirections[i][4];
        p2( 2, 0 ) = m_mainDirections[i][5];

		p3( 0, 0 ) = m_mainDirections[i][6];
        p3( 1, 0 ) = m_mainDirections[i][7];
        p3( 2, 0 ) = m_mainDirections[i][8];

        FMatrix rotP1 = invert(storedRot)  * p1;
		FMatrix rotP2 = invert(storedRot)  * p2;
		FMatrix rotP3 = invert(storedRot)  * p3;

        m_mainDirections[i][0] = rotP1( 0, 0 );
        m_mainDirections[i][1] = rotP1( 1, 0 );
        m_mainDirections[i][2] = rotP1( 2, 0 );

		m_mainDirections[i][3] = rotP2( 0, 0 );
        m_mainDirections[i][4] = rotP2( 1, 0 );
        m_mainDirections[i][5] = rotP2( 2, 0 );

		m_mainDirections[i][6] = rotP3( 0, 0 );
        m_mainDirections[i][7] = rotP3( 1, 0 );
        m_mainDirections[i][8] = rotP3( 2, 0 );

    }

    getSlidersPositions( m_currentSliderPos );

    return true;
}


void Maximas::rotatePeaks()
{
	m_doRotate = !m_doRotate;
	float voxelX = DatasetManager::getInstance()->getVoxelX();
    float voxelY = DatasetManager::getInstance()->getVoxelY();
    float voxelZ = DatasetManager::getInstance()->getVoxelZ();

	FMatrix rot;
	
	if(m_doRotate)
		rot = storedRot;
	else
		rot = invert(storedRot);

	//std::cout << rot( 0, 0 ) << " " << rot( 0, 1 ) << " " << rot( 0, 2 ) << std::endl;
	//std::cout << rot( 1, 0 ) << " " << rot( 1, 1 ) << " " << rot( 1, 2 ) << std::endl;
	//std::cout << rot( 2, 0 ) << " " << rot( 2, 1 ) << " " << rot( 2, 2 ) << std::endl;

	for( int i = 0; i < m_mainDirections.size(); i++ )
    { 

		FMatrix p1( 3, 1 );
		FMatrix p2( 3, 1 );
		FMatrix p3( 3, 1 );
        p1( 0, 0 ) = m_mainDirections[i][0];
        p1( 1, 0 ) = m_mainDirections[i][1];
        p1( 2, 0 ) = m_mainDirections[i][2];

		p2( 0, 0 ) = m_mainDirections[i][3];
        p2( 1, 0 ) = m_mainDirections[i][4];
        p2( 2, 0 ) = m_mainDirections[i][5];

		p3( 0, 0 ) = m_mainDirections[i][6];
        p3( 1, 0 ) = m_mainDirections[i][7];
        p3( 2, 0 ) = m_mainDirections[i][8];

        FMatrix rotP1 = rot  * p1;
		FMatrix rotP2 = rot  * p2;
		FMatrix rotP3 = rot  * p3;

        m_mainDirections[i][0] = rotP1( 0, 0 );
        m_mainDirections[i][1] = rotP1( 1, 0 );
        m_mainDirections[i][2] = rotP1( 2, 0 );

		m_mainDirections[i][3] = rotP2( 0, 0 );
        m_mainDirections[i][4] = rotP2( 1, 0 );
        m_mainDirections[i][5] = rotP2( 2, 0 );

		m_mainDirections[i][6] = rotP3( 0, 0 );
        m_mainDirections[i][7] = rotP3( 1, 0 );
        m_mainDirections[i][8] = rotP3( 2, 0 );

    }
}

//////////////////////////////////////////////////////////////////////////
// NOTE: This currently only supports 3 maximas, because when we extract 
// Maximas from ODFs, only 3 are extracted.
bool Maximas::createMaximas( std::vector<std::vector<Vector> > &mainDirections)
{
    m_columns = DatasetManager::getInstance()->getColumns(); //XSlice
    m_rows    = DatasetManager::getInstance()->getRows(); //YSlice
    m_frames  = DatasetManager::getInstance()->getFrames(); //ZSlice
    m_bands = 9;
    m_dataType = 16;

    m_voxelSizeX = DatasetManager::getInstance()->getVoxelX();
    m_voxelSizeY = DatasetManager::getInstance()->getVoxelY();
    m_voxelSizeZ = DatasetManager::getInstance()->getVoxelZ();

    m_type = MAXIMAS;

    int datasetSize = m_columns * m_rows * m_frames;
    
    l_fileFloatData.assign( datasetSize * m_bands, std::numeric_limits<float>::max() );

    for( int i( 0 ); i < datasetSize; ++i )
    {
        for( unsigned int j( 0 ); j < mainDirections[i].size(); ++j )
        {
			l_fileFloatData[i * m_bands + j*3] = mainDirections[i][j].x;
            l_fileFloatData[i * m_bands + j*3+1] = mainDirections[i][j].y;
            l_fileFloatData[i * m_bands + j*3+2] = mainDirections[i][j].z;
        }
    }
    
    createStructure( l_fileFloatData );

    m_isLoaded = true;
    return true;
}

void Maximas::flipAnat( AxisType axe )
{
    int curIndex;
    int flipIndex;

    int row(DatasetManager::getInstance()->getRows());
    int col(DatasetManager::getInstance()->getColumns());
    int frames(DatasetManager::getInstance()->getFrames());

    int m_rows = DatasetManager::getInstance()->getRows();
    int m_columns = DatasetManager::getInstance()->getColumns();
    int m_frames = DatasetManager::getInstance()->getFrames();
    int m_bands = DatasetManager::getInstance()->getBands();

    switch (axe)
    {
        case X_AXIS:
            col /= 2;
            break;
        case Y_AXIS:
            row /= 2;
            break;
        case Z_AXIS:
            frames /= 2;
            break;
        default:
            Logger::getInstance()->print( wxT( "Cannot flip axis. The given axis is undefined." ), LOGLEVEL_ERROR );
            return;
    }

    for( int f(0); f < frames; ++f )
    {
        for( int r(0); r < row; ++r )
        {
            for( int c(0); c < col; ++c )
            {
                curIndex = (c + r * m_columns + f * m_columns * m_rows);

                //Compute the index of the value that will be replaced by the one defined by our current index
                switch (axe)
                {
                    case X_AXIS:
                        flipIndex = ((m_columns - 1 - c) + r * m_columns + f * m_columns * m_rows);
                        break;
                    case Y_AXIS:
                        flipIndex = (c + (m_rows - 1 - r) * m_columns + f * m_columns * m_rows);
                        break;
                    case Z_AXIS:
                        flipIndex = (c + r * m_columns + (m_frames - 1 - f) * m_columns * m_rows);
                        break;
                    default:
                        break;
                }

                std::vector<float> tmp = m_mainDirections[curIndex];
                m_mainDirections[curIndex] = m_mainDirections[flipIndex];
                m_mainDirections[flipIndex] = tmp;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void Maximas::draw()
{
    // Enable the shader.
    ShaderHelper::getInstance()->getOdfsShader()->bind();
    glBindTexture( GL_TEXTURE_1D, m_textureId );

    // This is the color look up table texture.
    ShaderHelper::getInstance()->getOdfsShader()->setUniSampler( "clut", 0 );
    
    // This is the brightness level of the odf.
    ShaderHelper::getInstance()->getOdfsShader()->setUniFloat( "brightness", DatasetInfo::m_brightness );

    // This is the alpha level of the odf.
    ShaderHelper::getInstance()->getOdfsShader()->setUniFloat( "alpha", DatasetInfo::m_alpha );

    // If m_colorWithPosition is true then the glyph will be colored with the position of the vertex.
    ShaderHelper::getInstance()->getOdfsShader()->setUniInt( "colorWithPos", ( GLint ) m_colorWithPosition );
    
    if(isDisplay(SLICES))
    {
        Glyph::draw();
    }
    else
    {
        Glyph::drawSemiAll();
    }
    // Disable the tensor color shader.
    ShaderHelper::getInstance()->getOdfsShader()->release();
}

//////////////////////////////////////////////////////////////////////////
void Maximas::setScalingFactor( float i_scalingFactor )
{
    m_scalingFactor = i_scalingFactor;
    generateSpherePoints( m_scalingFactor/5 );   
    //loadBuffer();
}

//////////////////////////////////////////////////////////////////////////
void Maximas::drawGlyph( int i_zVoxel, int i_yVoxel, int i_xVoxel, AxisType i_axis )
{
    if( ! boxInFrustum( Vector( ( i_xVoxel + 0.5f ) * m_voxelSizeX,
                                ( i_yVoxel + 0.5f ) * m_voxelSizeY,
                                ( i_zVoxel + 0.5f ) * m_voxelSizeZ ),
                        Vector( m_voxelSizeX * 0.5f,
                                m_voxelSizeY * 0.5f,
                                m_voxelSizeZ * 0.5f ) ) )
        return;

    // Get the current maxima index in the coeffs's buffer
    int  currentIdx = getGlyphIndex( i_zVoxel, i_yVoxel, i_xVoxel );   
 
    // Maxima offset..
    GLfloat l_offset[3];
    getVoxelOffset( i_zVoxel, i_yVoxel, i_xVoxel, l_offset, i_axis );
    ShaderHelper::getInstance()->getOdfsShader()->setUni3Float( "offset", l_offset );

    GLfloat l_flippedAxes[3];
    m_flippedAxes[0] ? l_flippedAxes[0] = -1.0f : l_flippedAxes[0] = 1.0f;
    m_flippedAxes[1] ? l_flippedAxes[1] = -1.0f : l_flippedAxes[1] = 1.0f;
    m_flippedAxes[2] ? l_flippedAxes[2] = -1.0f : l_flippedAxes[2] = 1.0f;

    ShaderHelper::getInstance()->getOdfsShader()->setUni3Float(   "axisFlip",    l_flippedAxes );

    ShaderHelper::getInstance()->getOdfsShader()->setUniInt( "showAxis", 1 );

    if(m_mainDirections[currentIdx].size() != 0)
    { 
        for(unsigned int i =0; i < m_mainDirections[currentIdx].size()/3; i++)
        {
            GLfloat l_coloring[3];
            l_coloring[0] = m_mainDirections[currentIdx][i*3];
            l_coloring[1] = m_mainDirections[currentIdx][i*3+1];
            l_coloring[2] = m_mainDirections[currentIdx][i*3+2];

            ShaderHelper::getInstance()->getOdfsShader()->setUni3Float( "coloring", l_coloring );
            
            float scale = m_scalingFactor / 5.0f;
			float norm = sqrt(l_coloring[0] * l_coloring[0] + l_coloring[1] * l_coloring[1] + l_coloring[2] * l_coloring[2]);
			float halfScale = norm * scale;

            GLfloat stickPos[3];
            stickPos[0] = halfScale*m_mainDirections[currentIdx][i*3];
            stickPos[1] = halfScale*m_mainDirections[currentIdx][i*3+1];
            stickPos[2] = halfScale*m_mainDirections[currentIdx][i*3+2];

            glLineWidth(2);
            glBegin(GL_LINES);  
                glVertex3f(-stickPos[0],-stickPos[1],-stickPos[2]);
                glVertex3f(stickPos[0],stickPos[1],stickPos[2]);       
            glEnd();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void Maximas::createPropertiesSizer( PropertiesWindow *pParent )
{
#if !_USE_ZOOM_GUI
int zoomS = 100;
#else
int zoomS = 300;

#endif

    Glyph::createPropertiesSizer( pParent );

    wxBoxSizer *pBoxMain = new wxBoxSizer( wxVERTICAL );

	wxToggleButton *m_pToggleRotPeaks = new wxToggleButton( pParent, wxID_ANY,wxT("Un-rotate peaks with header"), wxDefaultPosition, wxSize(zoomS*2, -1) );
    pParent->Connect( m_pToggleRotPeaks->GetId(), wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler(PropertiesWindow::OnToggleRotatePeaks) );
	pBoxMain->Add( m_pToggleRotPeaks, 0, wxFIXED_MINSIZE | wxEXPAND, 0 );

    wxRadioButton *pRadSlices = new wxRadioButton( pParent, wxID_ANY, wxT( "Slices" ), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
    wxRadioButton *pRadWhole   = new wxRadioButton( pParent, wxID_ANY, wxT( "Whole" ) );

    wxBoxSizer *pBoxDisplay = new wxBoxSizer( wxVERTICAL );
    pBoxDisplay->Add( new wxStaticText( pParent, wxID_ANY, wxT( "Sticks display:" ) ), 0, wxALIGN_LEFT | wxALL, 1 );

    wxBoxSizer *pBoxButton = new wxBoxSizer( wxVERTICAL );
    pBoxButton->Add( pRadSlices, 0, wxALIGN_LEFT | wxALL, 1 );
    pBoxButton->Add( pRadWhole,   0, wxALIGN_LEFT | wxALL, 1 );

    pBoxDisplay->Add( pBoxButton, 0, wxALIGN_LEFT | wxLEFT, 32 );
    pBoxMain->Add( pBoxDisplay, 0, wxFIXED_MINSIZE | wxEXPAND, 0 );

    //////////////////////////////////////////////////////////////////////////
    pParent->Connect( pRadSlices->GetId(), wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PropertiesWindow::OnMaximasDisplaySlice ) );
    pParent->Connect( pRadWhole->GetId(),   wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PropertiesWindow::OnMaximasDisplayWhole ) );

    pRadSlices->SetValue( isDisplay(SLICES) );
    pRadWhole->SetValue(  isDisplay(WHOLE) );


    m_pPropertiesSizer->Add( pBoxMain, 0, wxFIXED_MINSIZE | wxEXPAND, 0 );
}

//////////////////////////////////////////////////////////////////////////
void Maximas::updatePropertiesSizer()
{
    Glyph::updatePropertiesSizer();
    
    /*m_pSliderLightAttenuation->Enable( false );
    m_pSliderLightXPosition->Enable( false );
    m_pSliderLightYPosition->Enable( false );
    m_pSliderLightZPosition->Enable( false );

    m_pSliderMinHue->Enable( false );
    m_pSliderMaxHue->Enable( false );
    m_pSliderSaturation->Enable( false );
    m_pSliderLuminance->Enable( false );*/

    m_pLabelLOD->Hide();
    m_pSliderLOD->Hide();
    m_pLabelDisplay->Hide();
    m_pSliderDisplay->Hide();
    
#if !_USE_LIGHT_GUI
    m_pToggleColorWithPosition->Hide();
    m_pToggleColorWithPosition->Enable( false );
#endif
    
    m_pSliderScalingFactor->SetValue( getScalingFactor() * 10.0f );

    m_pToggleAxisFlipX->SetValue( isAxisFlipped( X_AXIS ) );
    m_pToggleAxisFlipY->SetValue( isAxisFlipped( Y_AXIS ) );
    m_pToggleAxisFlipZ->SetValue( isAxisFlipped( Z_AXIS ) );

    // Hiding, since it is not supported.
    m_pLabelDisplayRadioType->Hide();
    m_pRadNormal->Hide();
    m_pRadMapOnSphere->Hide();
    m_pRadMainAxis->Hide();
}

bool Maximas::save( wxXmlNode *pNode, const wxString &rootPath ) const
{
    assert( pNode != NULL );

    pNode->SetName( wxT( "dataset" ) );
    DatasetInfo::save( pNode, rootPath );

    return true;
}