/////////////////////////////////////////////////////////////////////////////
// Name:            selecitonObject.cpp
// Author:          Imagicien ->LAMIRANDE-NADEAU Julien & NAZRATI R�da<-
// Creation Date:   10/26/2009
//
// Description: This is the implementation file for SelectionObject class.
//
// Last modifications:
//      by : GGirard - 17/01/2011
/////////////////////////////////////////////////////////////////////////////

#include "SelectionObject.h"

#include "SceneHelper.h"
#include "SceneManager.h"
#include "SelectionTree.h"
#include "../main.h"
#include "../dataset/Anatomy.h"
#include "../dataset/DatasetManager.h"
#include "../dataset/Fibers.h"
#include "../dataset/RTTrackingHelper.h"
#include "../gui/MainFrame.h"
#include "../misc/Algorithms/ConvexGrahamHull.h"
#include "../misc/Algorithms/ConvexHullIncremental.h"
#include "../gfx/ShaderHelper.h"
#include "../misc/Algorithms/Helper.h"
// TODO selection remove.
#include "../misc/IsoSurface/CIsoSurface.h"
#include "../misc/IsoSurface/TriangleMesh.h"
#include "../misc/XmlHelper.h"
#include "../misc/Fantom/FBSpline.h"

#include <wx/textctrl.h>
#include <wx/tglbtn.h>

#include <fstream>
#include <iostream>
#include <list>
using std::list;

#include <limits>
using std::numeric_limits;

#include <vector>
using std::vector;

#define DEF_POS   wxDefaultPosition
#define DEF_SIZE  wxDefaultSize

// Protected. Should only be used to create an empty object when loading a scene.
// Should only be called by children object.
SelectionObject::SelectionObject()
{
    doBasicInit();
}

void SelectionObject::doBasicInit()
{
    hitResult hr = { false, 0.0f, 0, NULL };
    m_hitResult = hr;
    
    m_name = wxT("object");
    m_objectType = DEFAULT_TYPE;   // TODO selection to remove
    m_center = Vector( 0.0f, 0.0f, 0.0f );
    m_size = Vector( 0.0f, 0.0f, 0.0f );
    m_isActive = true;
    m_isNOT = false;
    m_isRemove = false;
    m_isSelected = false;
    m_isVisible = true;
    m_isMagnet = false;
    m_stepSize = 9;
    m_color = wxColour( 0, 0, 0 );
    m_treeId = NULL;
    m_pLabelAnatomy = NULL;
    m_pCBSelectDataSet = NULL;
	m_pSaveTractometry = NULL;
	m_displayCrossSections = CS_NOTHING;
	m_displayDispersionCone = DC_NOTHING;
	m_noOfMeanFiberPts = 20;
	m_meanFiberColor = wxColour( 255.0f, 255.0f, 255.0f);
	m_pRefAnatInfo = NULL;
    
    m_maxCrossSectionIndex  = 0;
    m_minCrossSectionIndex  = 0;
    m_Q = 0.25;
    
    m_statsNeedUpdating     = true;
	m_showStartPoint = false;
	m_flipStartingPoint = false;
    m_statsAreBeingComputed = false;
    m_meanFiberIsBeingDisplayed = false;
    m_boxMoved              = false;
    m_boxResized            = false;
    m_mustUpdateConvexHull  = true;
    
    //Distance coloring
    m_DistColoring          = false;

}

SelectionObject::SelectionObject( Vector center, Vector size )
{
    doBasicInit();
    
    m_center = center;
    m_size = size;
}

SelectionObject::SelectionObject( const wxXmlNode selObjNode )
{   
    doBasicInit();
    
    wxXmlNode *pChildNode = selObjNode.GetChildren();
    
    while( pChildNode != NULL )
    {
        wxString nodeName = pChildNode->GetName();
        wxString propVal;
        
        if( nodeName == wxT("state") )
        {
            pChildNode->GetAttribute( wxT("name"), &m_name );
            pChildNode->GetAttribute( wxT("active"), &propVal );
            m_isActive = parseXmlBoolString( propVal );
            pChildNode->GetAttribute( wxT("visible"), &propVal );
            m_isVisible = parseXmlBoolString( propVal );
            pChildNode->GetAttribute( wxT("isNOT"), &propVal );
            m_isNOT = parseXmlBoolString( propVal );
            pChildNode->GetAttribute( wxT("isMagnet"), &propVal );
            m_isMagnet = parseXmlBoolString( propVal );

            double x, y, z;
            pChildNode->GetAttribute( wxT("fieldX"), &propVal );
            propVal.ToDouble( &x );
            m_magnetField.x = x;
            pChildNode->GetAttribute( wxT("fieldY"), &propVal );
            propVal.ToDouble( &y );
            m_magnetField.y = y;
            pChildNode->GetAttribute( wxT("fieldZ"), &propVal );
            propVal.ToDouble( &z );
            m_magnetField.z = z;
        }
        else if( nodeName == wxT("center") )
        {
            double x, y, z;
            pChildNode->GetAttribute( wxT("posX"), &propVal );
            propVal.ToDouble( &x );
            pChildNode->GetAttribute( wxT("posY"), &propVal );
            propVal.ToDouble( &y );
            pChildNode->GetAttribute( wxT("posZ"), &propVal );
            propVal.ToDouble( &z );
            m_center.x = x;
            m_center.y = y;
            m_center.z = z;
        }
        else if( nodeName == wxT("size") )
        {
            double x, y, z;
            pChildNode->GetAttribute( wxT("sizeX"), &propVal );
            propVal.ToDouble( &x );
            pChildNode->GetAttribute( wxT("sizeY"), &propVal );
            propVal.ToDouble( &y );
            pChildNode->GetAttribute( wxT("sizeZ"), &propVal );
            propVal.ToDouble( &z );
            m_size.x = x;
            m_size.y = y;
            m_size.z = z;
        }
        else if( nodeName == wxT("color") )
        {
            pChildNode->GetAttribute( wxT("colorHTML"), &propVal );
            m_color.Set( propVal );

            long alpha;
            pChildNode->GetAttribute( wxT("colorAlpha"), &propVal );
            propVal.ToLong( &alpha );

            // Need to repeat it, there is no Alpha setter in wxWidgets.
            m_color.Set( m_color.Red(), m_color.Green(), m_color.Blue(), alpha );
        }
        else if( nodeName == wxT("distance_coloring_state") )
        {
            pChildNode->GetAttribute( wxT("used"), &propVal );
            m_DistColoring = parseXmlBoolString( propVal );
        }
        else if( nodeName == wxT("mean_fiber_options") )
        {
            pChildNode->GetAttribute( wxT("colorHTML"), &propVal );
            m_meanFiberColor.Set( propVal );
            
            double opacity;
            pChildNode->GetAttribute( wxT("opacity"), &propVal );
            propVal.ToDouble( &opacity );
            m_meanFiberOpacity = opacity;
            
            pChildNode->GetAttribute( wxT("colorationMode"), &propVal );
            m_meanFiberColorationMode = Helper::getColorationModeFromString( propVal );
        }
        
        pChildNode = pChildNode->GetNext();
    }
}

SelectionObject::~SelectionObject( )
{
}

///////////////////////////////////////////////////////////////////////////
// Moves the object back.
///////////////////////////////////////////////////////////////////////////
void SelectionObject::moveBack()
{
    if( m_center.y > DatasetManager::getInstance()->getRows() * DatasetManager::getInstance()->getVoxelY() ) 
        return;

    m_center.y = (int)m_center.y + 1.0;

    if( wxGetKeyState( WXK_SHIFT ) )
        m_center.y += m_stepSize;
    
    m_boxMoved = true;
    update();
    notifyInBoxNeedsUpdating();
}

///////////////////////////////////////////////////////////////////////////
// Moves the object down.
///////////////////////////////////////////////////////////////////////////
void SelectionObject::moveDown()
{
    if( m_center.z > DatasetManager::getInstance()->getFrames() * DatasetManager::getInstance()->getVoxelZ() )
        return;

    m_center.z = (int)m_center.z + 1.0;

    if( wxGetKeyState( WXK_SHIFT ) )
        m_center.z += m_stepSize;

    m_boxMoved = true;
    update();
    notifyInBoxNeedsUpdating();
}

///////////////////////////////////////////////////////////////////////////
// Moves the object forward.
///////////////////////////////////////////////////////////////////////////
void SelectionObject::moveForward()
{
    if( m_center.y < 0 ) 
        return;

    m_center.y = (int)m_center.y - 1.0;

    if( wxGetKeyState( WXK_SHIFT ) ) 
        m_center.y -= m_stepSize;

    m_boxMoved = true;
    update();
    notifyInBoxNeedsUpdating();
}

///////////////////////////////////////////////////////////////////////////
// Moves the object left.
///////////////////////////////////////////////////////////////////////////
void SelectionObject::moveLeft()
{
    if( m_center.x < 0 ) 
        return;

    m_center.x = (int)m_center.x - 1.0;

    if( wxGetKeyState( WXK_SHIFT ) )
        m_center.x -= m_stepSize;

    m_boxMoved = true;
    update();
    notifyInBoxNeedsUpdating();
}

///////////////////////////////////////////////////////////////////////////
// Moves the object right.
///////////////////////////////////////////////////////////////////////////
void SelectionObject::moveRight()
{
    if( m_center.x > DatasetManager::getInstance()->getColumns() * DatasetManager::getInstance()->getVoxelX() ) 
        return;

    m_center.x = (int)m_center.x + 1.0;

    if( wxGetKeyState( WXK_SHIFT ) )
        m_center.x += m_stepSize;

    m_boxMoved = true;
    update();
    notifyInBoxNeedsUpdating();
}

///////////////////////////////////////////////////////////////////////////
// Moves the object up.
///////////////////////////////////////////////////////////////////////////
void SelectionObject::moveUp()
{
    if( m_center.z < 0 ) 
        return;

    m_center.z = (int)m_center.z - 1.0;

    if( wxGetKeyState( WXK_SHIFT ) )
        m_center.z -= m_stepSize;

    m_boxMoved = true;
    update();
    notifyInBoxNeedsUpdating();
}

void SelectionObject::processDrag( wxPoint i_click, wxPoint i_lastPos, GLdouble i_projection[16], GLint i_viewport[4], GLdouble i_modelview[16] )
{
    if( m_hitResult.picked == 10 ) 
        drag( i_click, i_lastPos, i_projection, i_viewport, i_modelview );
    else 
        resize( i_click, i_lastPos, i_projection, i_viewport, i_modelview );
}

///////////////////////////////////////////////////////////////////////////
// Resize the object making it smaller in y.
///////////////////////////////////////////////////////////////////////////
void SelectionObject::resizeBack()
{
    if( m_size.y < 2 ) 
        return;

    m_size.y -= 1.0;

    if( wxGetKeyState( WXK_SHIFT ) ) 
        m_size.y = wxMax( 1, m_size.y - m_stepSize );

    m_boxResized = true;
    update();
    notifyInBoxNeedsUpdating();
}

///////////////////////////////////////////////////////////////////////////
// Resize the object making it smaller in z.
///////////////////////////////////////////////////////////////////////////
void SelectionObject::resizeDown()
{
    if( m_size.z < 2 ) 
        return;

    m_size.z -= 1.0;

    if( wxGetKeyState( WXK_SHIFT ) ) 
        m_size.z = wxMax( 1, m_size.z - m_stepSize );

    m_boxResized = true;
    update();
    notifyInBoxNeedsUpdating();
}

///////////////////////////////////////////////////////////////////////////
// Resize the object making it bigger in y.
///////////////////////////////////////////////////////////////////////////
void SelectionObject::resizeForward()
{
    if( m_size.y > DatasetManager::getInstance()->getRows() ) 
        return;

    m_size.y += 1.0;

    if( wxGetKeyState ( WXK_SHIFT ) )
        m_size.y += m_stepSize;

    m_boxResized = true;
    update();
    notifyInBoxNeedsUpdating();
}

///////////////////////////////////////////////////////////////////////////
// Resize the object making it smaller in x.
///////////////////////////////////////////////////////////////////////////
void SelectionObject::resizeLeft()
{
    if( m_size.x < 2 ) 
        return;

    m_size.x -= 1.0;

    if( wxGetKeyState( WXK_SHIFT ) )
        m_size.x = wxMax( 1 ,m_size.x - m_stepSize );

    m_boxResized = true;
    update();
    notifyInBoxNeedsUpdating();
}

///////////////////////////////////////////////////////////////////////////
// Resize the object making it bigger in x.
///////////////////////////////////////////////////////////////////////////
void SelectionObject::resizeRight()
{
    if( m_size.x > DatasetManager::getInstance()->getColumns() )
        return;

    m_size.x += 1.0;

    if( wxGetKeyState( WXK_SHIFT ) )
        m_size.x += m_stepSize;

    m_boxResized = true;
    update();
    notifyInBoxNeedsUpdating();
}

///////////////////////////////////////////////////////////////////////////
// Resize the object making it bigger in z.
///////////////////////////////////////////////////////////////////////////
void SelectionObject::resizeUp()
{
    if( m_size.z > DatasetManager::getInstance()->getFrames() )
        return;

    m_size.z += 1.0;

    if( wxGetKeyState( WXK_SHIFT ) )
        m_size.z += m_stepSize;

    m_boxResized = true;
    update();
    notifyInBoxNeedsUpdating();
}

///////////////////////////////////////////////////////////////////////////
// Select or unselect the object.
//
// i_flag       : Indicate if we want to select or unselect the object.
///////////////////////////////////////////////////////////////////////////
void SelectionObject::select()
{
    m_isSelected = true;
    updateStatusBar();
}

///////////////////////////////////////////////////////////////////////////
// Updates the object.
///////////////////////////////////////////////////////////////////////////
void SelectionObject::update()
{
    updateStatusBar();
    SceneManager::getInstance()->setSelBoxChanged( true );
    //MyApp::frame->refreshAllGLWidgets();

    float voxelX = DatasetManager::getInstance()->getVoxelX();
    float voxelY = DatasetManager::getInstance()->getVoxelY();
    float voxelZ = DatasetManager::getInstance()->getVoxelZ();

    // Update the min/max position in x, y and z of the object.
    m_minX = m_center.x - ( m_size.x * 0.5f * voxelX );
    m_maxX = m_center.x + ( m_size.x * 0.5f * voxelX );
    m_minY = m_center.y - ( m_size.y * 0.5f * voxelY );
    m_maxY = m_center.y + ( m_size.y * 0.5f * voxelY );
    m_minZ = m_center.z - ( m_size.z * 0.5f * voxelZ );
    m_maxZ = m_center.z + ( m_size.z * 0.5f * voxelZ );

    objectUpdate();
}

///////////////////////////////////////////////////////////////////////////
// Updates the object. If a daughter class of SelectionObject needs a specific update call, 
// then they should implement this virtual method (like SelectionEllipsoid is doing).
///////////////////////////////////////////////////////////////////////////
void SelectionObject::objectUpdate()
{
    // Nothing for the mother class!
}

///////////////////////////////////////////////////////////////////////////
// Toggles the m_isActive field.
///////////////////////////////////////////////////////////////////////////
bool SelectionObject::toggleIsActive()
{
    setIsActive( !getIsActive() );
    RTTrackingHelper::getInstance()->setRTTDirty(true);
    return getIsActive();
}

void SelectionObject::setIsActive( bool isActive )
{
    SceneManager::getInstance()->setSelBoxChanged( true );
    SceneManager::getInstance()->getSelectionTree().notifyStatsNeedUpdating( this );
    m_isActive = isActive;
}

///////////////////////////////////////////////////////////////////////////
// Set the center of the object.
//
// i_x          : The new x center value.
// i_y          : The new y center value.
// i_z          : The new z center value.
///////////////////////////////////////////////////////////////////////////
void SelectionObject::setCenter( float i_x, float i_y, float i_z )
{
    setCenter( Vector( i_x,i_y, i_z ) );
}

///////////////////////////////////////////////////////////////////////////
// Set the center of the object.
//
// i_center     : The new center vector;
///////////////////////////////////////////////////////////////////////////
void SelectionObject::setCenter( Vector i_center )
{
    m_center  = i_center; 
    update();
    notifyInBoxNeedsUpdating();
}

///////////////////////////////////////////////////////////////////////////
// Sets the color of the object.
//
// i_color      : The new color of the object.
///////////////////////////////////////////////////////////////////////////
void SelectionObject::setColor( wxColour i_color )
{
    m_color = i_color;
    update();
}

int SelectionObject::getIcon()
{
    if( m_isActive && m_isVisible ) 
        return 1;
    else if( m_isActive && ! m_isVisible )
        return -1;
    else
        return 0;
}

bool SelectionObject::toggleIsNOT()
{
    setIsNOT( !getIsNOT() ); 
    RTTrackingHelper::getInstance()->setRTTDirty(true);
    return getIsNOT();
}

bool SelectionObject::togglePruneRemove()
{
    setPruneRemove( !getIsRemove() ); 
    RTTrackingHelper::getInstance()->setRTTDirty(true);
    return getIsRemove();
}

void SelectionObject::setIsNOT( bool i_isNOT )
{
    m_isNOT = i_isNOT;
    SceneManager::getInstance()->getSelectionTree().notifyStatsNeedUpdating( this );
    SceneManager::getInstance()->setSelBoxChanged( true );
}

void SelectionObject::setPruneRemove( bool i_isRemove )
{
    m_isRemove = i_isRemove;
    SceneManager::getInstance()->getSelectionTree().notifyStatsNeedUpdating( this );
    SceneManager::getInstance()->setSelBoxChanged( true );
}

void SelectionObject::drag( wxPoint i_click, wxPoint i_lastPos, GLdouble i_projection[16], GLint i_viewport[4], GLdouble i_modelview[16] )
{
    Vector l_vs = mapMouse2World( i_click.x, i_click.y, i_projection, i_viewport, i_modelview );
    Vector l_ve = mapMouse2WorldBack( i_click.x, i_click.y, i_projection, i_viewport, i_modelview );
    Vector l_dir( l_ve.x - l_vs.x, l_ve.y - l_vs.y, l_ve.z - l_vs.z );

    Vector l_vs2 = mapMouse2World( i_lastPos.x, i_lastPos.y, i_projection, i_viewport, i_modelview );
    Vector l_ve2 = mapMouse2WorldBack( i_lastPos.x, i_lastPos.y, i_projection, i_viewport, i_modelview );
    Vector l_dir2( l_ve2.x - l_vs2.x, l_ve2.y - l_vs2.y, l_ve2.z - l_vs2.z );

    Vector l_change( ( l_vs.x + l_dir.x * m_hitResult.tmin ) - ( l_vs2.x + l_dir2.x * m_hitResult.tmin ),
                     ( l_vs.y + l_dir.y * m_hitResult.tmin ) - ( l_vs2.y + l_dir2.y * m_hitResult.tmin ),
                     ( l_vs.z + l_dir.z * m_hitResult.tmin ) - ( l_vs2.z + l_dir2.z * m_hitResult.tmin ) );

    m_center.x += l_change.x;
    m_center.y += l_change.y;
    m_center.z += l_change.z;
    
    m_boxMoved = true;
    update();
    notifyInBoxNeedsUpdating();
}

void SelectionObject::resize( wxPoint i_click, wxPoint i_lastPos, GLdouble i_projection[16], GLint i_viewport[4], GLdouble i_modelview[16] )
{
    Vector  l_n( 0, 0, 0 );
    float l_delta = 0;

    switch( m_hitResult.picked )
    {
        case 11: l_n.x = -1.0; break;
        case 12: l_n.x =  1.0; break;
        case 13: l_n.y = -1.0; break;
        case 14: l_n.y =  1.0; break;
        case 15: l_n.z = -1.0; break;
        case 16: l_n.z =  1.0; break;
        default: break;
    }

    l_delta = wxMax( wxMin(getAxisParallelMovement( i_lastPos.x, i_lastPos.y, i_click.x, i_click.y, l_n, i_projection, i_viewport, i_modelview ), 1 ), -1 );

    if( m_hitResult.picked == 11 || m_hitResult.picked == 12 ) 
    {
        float newX = m_size.x + l_delta;
        m_size.x = wxMin( wxMax( newX, 1 ), DatasetManager::getInstance()->getColumns() );
    }
    if( m_hitResult.picked == 13 || m_hitResult.picked == 14 ) 
    {
        float newY = m_size.y + l_delta;
        m_size.y = wxMin(wxMax(newY, 1), DatasetManager::getInstance()->getRows() );
    }
    if( m_hitResult.picked == 15 || m_hitResult.picked == 16 ) 
    {
        float newZ = m_size.z + l_delta;
        m_size.z = wxMin( wxMax( newZ, 1 ), DatasetManager::getInstance()->getFrames() );
    }
    
    m_boxResized = true;
    update();
    notifyInBoxNeedsUpdating();
}

float SelectionObject::getAxisParallelMovement( int i_x1, int i_y1, int i_x2, int i_y2, Vector i_n, GLdouble i_projection[16], GLint i_viewport[4], GLdouble i_modelview[16] )
{
    Vector l_vs = mapMouse2World( i_x1, i_y1, i_projection, i_viewport, i_modelview);
    Vector l_ve = mapMouse2World( i_x2, i_y2, i_projection, i_viewport, i_modelview );
    Vector l_dir( l_ve.x - l_vs.x, l_ve.y - l_vs.y, l_ve.z - l_vs.z );

    float l_bb = ( ( l_dir.x * l_dir.x ) + ( l_dir.y * l_dir.y ) + ( l_dir.z * l_dir.z ) );
    float l_nb = ( ( l_dir.x * i_n.x )   + ( l_dir.y * i_n.y )   + ( l_dir.z * i_n.z ) );

    return l_bb / l_nb;
}

///////////////////////////////////////////////////////////////////////////
// Draws a thick fiber on the screen.
//
// i_fiberPoints    : The points of the fiber that will be draw.
// i_thickness      : The thickness of the fiber to draw.
// i_nmTubeEdge     : This param will determine the number of edge the fake tube will have
//                    ( example -> if i_nmTubeEdge =3, then the tube will be shaped like a
//                    triangle, if i_nmTubeEdge = 4 then the tube will be shaped like a square.
///////////////////////////////////////////////////////////////////////////
void SelectionObject::drawThickFiber( const vector< Vector > &i_fiberPoints, float i_thickness, int i_nmTubeEdge )
{
    if( i_fiberPoints.size() < 2 || !m_pToggleDisplayMeanFiber->GetValue() )
        return;

    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    
    //Get the mean fiber color
    m_meanFiberColorVector.clear();
    switch ( m_meanFiberColorationMode ){
        case NORMAL_COLOR:
            setNormalColorArray( i_fiberPoints );
            break;
        case CUSTOM_COLOR:
            //Use custom color
            glColor4f( (float)m_meanFiberColor.Red() / 255.0f, (float)m_meanFiberColor.Green() / 255.0f, (float)m_meanFiberColor.Blue() / 255.0f, m_meanFiberOpacity );
            break;
        default:
            glColor4f( 0.25f, 0.25f, 0.25f, m_meanFiberOpacity ); // Grayish
    }

    Vector l_normal;

    // This vector will countain all the points forming the circles around the fiber.
    vector< vector< Vector > > l_circlesPoints;
    l_circlesPoints.resize( i_fiberPoints.size() );

    for( unsigned int i = 0; i < i_fiberPoints.size(); ++i )
    {
        // Lets get a unit-length normal for the current point.
        if( i == i_fiberPoints.size() - 1 ) // if we are on the last point of this fiber, we calculate the normal using the point before the current one.
            l_normal = i_fiberPoints[i] - i_fiberPoints[i-1];
        else // else we calculate the normal using the point after the current one.
            l_normal = i_fiberPoints[i+1] - i_fiberPoints[i];

		l_normal.normalize();
        Helper::getCirclePoints( i_fiberPoints[i], l_normal, i_thickness, i_nmTubeEdge, l_circlesPoints[i] );
    }

    // Now with our circles points calculated, we link them using quads strip.
    for( int i = 0; i < i_nmTubeEdge; ++i )
    {
        glBegin( GL_QUAD_STRIP );
            for( unsigned int j = 0; j < l_circlesPoints.size(); ++j )
            {
                if ( m_meanFiberColorVector.size() != 0 && m_meanFiberColorVector.size() == l_circlesPoints.size() )
                    glColor4f( m_meanFiberColorVector[j][0], m_meanFiberColorVector[j][1], m_meanFiberColorVector[j][2], m_meanFiberOpacity );

                glVertex3f( l_circlesPoints[j][i].x, l_circlesPoints[j][i].y, l_circlesPoints[j][i].z );
                if( i+1 == i_nmTubeEdge )
                    glVertex3f( l_circlesPoints[j][0].x, l_circlesPoints[j][0].y, l_circlesPoints[j][0].z );
                else
                    glVertex3f( l_circlesPoints[j][i+1].x, l_circlesPoints[j][i+1].y, l_circlesPoints[j][i+1].z );
            }
        glEnd();
    }

    glDisable( GL_BLEND );

}


void SelectionObject::computeConvexHull()
{

	int activeFiberSetCount( 0 );
    
    vector< Fibers * > pFibersSet = DatasetManager::getInstance()->getFibers();
    
    for( size_t fiberSetIdx(0); fiberSetIdx < pFibersSet.size(); ++fiberSetIdx )
    {
        Fibers *pCurFibers = pFibersSet[ fiberSetIdx ];
        if( pCurFibers->getShow() )
        {
            vector< int > selectedFibersIdx = getSelectedFibersIndexes( pCurFibers );
            
            if( selectedFibersIdx.empty() )
            {
                // Do not want to do any processing in this case.
                continue;
            }
            
            vector< int > nbPointsBySelectedFiber;
            vector< vector < Vector > > pointsBySelectedFiber;
            
            getSelectedFibersInfo( selectedFibersIdx, pCurFibers, 
                                  nbPointsBySelectedFiber, pointsBySelectedFiber );
            
		     m_mustUpdateConvexHull = true;
			 if ( m_pToggleDisplayConvexHull->GetValue() && m_convexHullOpacity != 0)
			 {
					 vector< Vector > pts;
					 vector< vector< Vector > > l_selectedFibersPoints = pointsBySelectedFiber;
					 for (unsigned int i(0); i< l_selectedFibersPoints.size(); i++)
						 pts.insert(pts.end(), l_selectedFibersPoints[i].begin(), l_selectedFibersPoints[i].end());
 
					 m_hullTriangles.clear();
					 ConvexHullIncremental hull(pts);
					 hull.buildHull();
					 hull.getHullTriangles( m_hullTriangles );
					 m_mustUpdateConvexHull = false;
			 }
		}
	}
}

void SelectionObject::drawConvexHull()
{
     if ( m_pToggleDisplayConvexHull->GetValue() && m_pToggleDisplayConvexHull->IsEnabled() && m_convexHullOpacity != 0)
     {
         if (m_mustUpdateConvexHull)
             computeConvexHull();
 
         glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
         glEnable( GL_BLEND );
         glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
         glColor4f( (float)m_convexHullColor.Red() / 255.0f, (float)m_convexHullColor.Green() / 255.0f, (float)m_convexHullColor.Blue() / 255.0f, m_convexHullOpacity );
 
         Vector normal;
         glBegin( GL_TRIANGLES );
         list< Face3D >::iterator it;
             for( it = m_hullTriangles.begin(); it != m_hullTriangles.end(); it++ )
             {
                 //Vector normal;
                 //normal = (p[i+1] - p[i]).Cross(p[i+2]-p[i]);
                 //normal.normalize();
                 //glNormal3f(normal[0], normal[1], normal[2]);
                 glVertex3f( it->getPt1().x, it->getPt1().y, it->getPt1().z );
 
                 //normal = (p[i] - p[i+1]).Cross(p[i+2]-p[i+1]);
                 //normal.normalize();
                 //glNormal3f(normal[0], normal[1], normal[2]);
                 glVertex3f( it->getPt2().x, it->getPt2().y, it->getPt2().z );
 
                 //normal = (p[i] - p[i+2]).Cross(p[i+1]-p[i+2]);
                 //normal.normalize();
                 //glNormal3f(normal[0], normal[1], normal[2]);
                 glVertex3f( it->getPt3().x, it->getPt3().y, it->getPt3().z );
             }
         glEnd();
 
         glDisable( GL_BLEND );
     }
}

void SelectionObject::updateConvexHullOpacity()
{
     setConvexHullOpacity( ( m_pSliderConvexHullOpacity->GetValue() + (float)m_pSliderConvexHullOpacity->GetMin() ) / (float)m_pSliderConvexHullOpacity->GetMax() );
}

///////////////////////////////////////////////////////////////////////////
// Fill the color array use to display the mean fiber base on the mean fiber direction
//      i_fiberPoints : All points of the mean fiber
///////////////////////////////////////////////////////////////////////////
void SelectionObject::setNormalColorArray(const vector< Vector > &i_fiberPoints)
{
    m_meanFiberColorVector.clear();
    Vector color;

    if ( i_fiberPoints.size() < 2 )
        return;
    for ( unsigned int i(0); i < i_fiberPoints.size(); i++ )
    {
        if ( i==0 )
            color = i_fiberPoints[i+1] - i_fiberPoints[i];
        else
            color = i_fiberPoints[i] - i_fiberPoints[i-1];

		color.x = std::abs(color.x);
		color.y = std::abs(color.y);
		color.z = std::abs(color.z);
        color.normalize();
        m_meanFiberColorVector.push_back( color );
            
    }
}


///////////////////////////////////////////////////////////////////////////
// Draws all the cross sections.
///////////////////////////////////////////////////////////////////////////
void SelectionObject::drawCrossSections()
{
    if( m_crossSectionsPoints.size() == 0 || m_displayCrossSections == CS_NOTHING )
        return;

    // We draw the cross sections first with only a simple line around them 
    // and then fill the inside with an alpha layer.

    // Cross section line.
    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    glDisable( GL_BLEND );
    drawCrossSectionsPolygons();

    // Cross section fill.
    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    drawCrossSectionsPolygons();
    glDisable( GL_BLEND );
}

///////////////////////////////////////////////////////////////////////////
// Draws all the cross sections.
///////////////////////////////////////////////////////////////////////////
void SelectionObject::drawCrossSectionsPolygons()
{
    for( unsigned int i = 0; i < m_crossSectionsPoints.size(); ++i )
    {
        if( m_displayCrossSections == CS_MIN_MAX_ONLY )
            if( i != m_minCrossSectionIndex && i != m_maxCrossSectionIndex )
                continue;

        getCrossSectionAreaColor( i );
        drawPolygon( m_crossSectionsPoints[i] );
    }
}

///////////////////////////////////////////////////////////////////////////
// Draws a polygon represented by a set of points. 
//
// i_polygonPoints      : The vector containing the polygon points.
///////////////////////////////////////////////////////////////////////////
void SelectionObject::drawPolygon( const vector< Vector > &i_polygonPoints )
{
    if( i_polygonPoints.size() < 3)
        return;

	Vector mean(0,0,0);
    glBegin( GL_POLYGON );

    for( unsigned int i = 0; i < i_polygonPoints.size(); ++i )
	{
        glVertex3f( i_polygonPoints[i].x, i_polygonPoints[i].y, i_polygonPoints[i].z );
		mean+=Vector(i_polygonPoints[i].x, i_polygonPoints[i].y, i_polygonPoints[i].z);
	}

	mean.x/=i_polygonPoints.size();
	mean.y/=i_polygonPoints.size();
	mean.z/=i_polygonPoints.size();
    glEnd();

	/*glColor3f(1,1,1);
	glPointSize(15);
	glBegin(GL_POINTS);
		glVertex3f(mean.x, mean.y, mean.z);
	glEnd();*/
}

void SelectionObject::updateStats()
{
    if( !m_statsNeedUpdating )
    {
        return;
    }

    FibersGroup *pFiberGroup = DatasetManager::getInstance()->getFibersGroup();
    
    if( pFiberGroup == NULL )
    {
        Logger::getInstance()->print( wxT( "In SelectionObject::updateStats, pFiberGroup is NULL" ), LOGLEVEL_DEBUG );
        return;
    }
    
    if( m_statsAreBeingComputed )
    {
        m_stats.m_count = 0;
        m_stats.m_meanLength = 0.0f;
        m_stats.m_maxLength  = 0.0f;
        m_stats.m_minLength  = std::numeric_limits< float >::max();
        m_stats.m_meanValue  = 0.0f;
    }
    
	m_meanFiberPoints.assign(m_noOfMeanFiberPts, Vector( 0.0, 0.0, 0.0 ));
	VecMean.clear();

    int activeFiberSetCount( 0 );
    
    vector< Fibers * > pFibersSet = DatasetManager::getInstance()->getFibers();
	vector< vector < Vector > > l_fibersPtns;
    
    for( size_t fiberSetIdx(0); fiberSetIdx < pFibersSet.size(); ++fiberSetIdx )
    {
        Fibers *pCurFibers = pFibersSet[ fiberSetIdx ];
        if( pCurFibers->getShow() )
        {
            vector< int > selectedFibersIdx = getSelectedFibersIndexes( pCurFibers );
            
            if( selectedFibersIdx.empty() )
            {
                // Do not want to do any processing in this case.
                continue;
            }
            
            vector< int > nbPointsBySelectedFiber;
            vector< vector < Vector > > pointsBySelectedFiber;
            
            getSelectedFibersInfo( selectedFibersIdx, pCurFibers, 
                                  nbPointsBySelectedFiber, pointsBySelectedFiber );
			l_fibersPtns = pointsBySelectedFiber;
            ++activeFiberSetCount;

			//get longest streamline
			getLongestStreamline(selectedFibersIdx, pCurFibers);
            
            if( m_statsAreBeingComputed )
            {
                m_stats.m_count += selectedFibersIdx.size();
                
                float localMeanLength( 0.0f );
                float localMaxLength(  0.0f );
                float localMinLength(  0.0f );
                
                getMeanMaxMinFiberLength( selectedFibersIdx, pCurFibers, 
                                            localMeanLength, 
                                            localMaxLength, 
                                            localMinLength);
                
                m_stats.m_meanLength += localMeanLength;
                
                m_stats.m_maxLength = std::max( m_stats.m_maxLength, localMaxLength );
                m_stats.m_minLength = std::min( m_stats.m_minLength, localMinLength );
                
                float localMeanValue( 0.0f );
                
                getMeanFiberValue( pointsBySelectedFiber, localMeanValue );
                
                m_stats.m_meanValue += localMeanValue;

            }


            //vector< Vector > meanFiberPoint;
            //getMeanFiber(pointsBySelectedFiber, m_noOfMeanFiberPts, m_meanFiberPoints);
                
            //for( int meanPtIdx( 0 ); meanPtIdx < MEAN_FIBER_NB_POINTS; ++meanPtIdx )
            //{
            //    m_meanFiberPoints[ meanPtIdx ] += meanFiberPoint[ meanPtIdx ];
            //}

        }
    }
    
    if( activeFiberSetCount > 0 )
    {
        if( m_statsAreBeingComputed )
        {
            m_stats.m_meanLength    /= activeFiberSetCount;
            m_stats.m_meanValue     /= activeFiberSetCount;
        }
        
        //for( int meanPtIdx( 0 ); meanPtIdx < MEAN_FIBER_NB_POINTS; ++meanPtIdx )
        //{
        //    m_meanFiberPoints[ meanPtIdx ] /= activeFiberSetCount;
        //}
    }
    
    if( m_stats.m_minLength == std::numeric_limits< float >::max() )
    {
        m_stats.m_minLength = 0.0f;
    }
    
    m_statsNeedUpdating = false;

	getMeanMaxMinFiberCrossSection  ( l_fibersPtns,
                           m_meanFiberPoints,
                            m_stats.m_meanCrossSection, 
                            m_stats.m_maxCrossSection,
                            m_stats.m_minCrossSection   );

	if(m_meanFiberMode == MEAN)
	{
		getMeanFiber(l_fibersPtns, 50, m_meanFiberPoints);
	}
	else if(m_meanFiberMode == CROSS)
	{
		getSpline();
	}
	

	getFiberDispersion              ( m_stats.m_dispersion        );
    
    updateStatsGrid();
  
}

void SelectionObject::getSpline()
{
	if(VecMean.size() !=0)
	{
		FBSpline spline(4, VecMean);
		std::vector<std::vector<double> > pts;
		spline.samplePoints(pts,50);
		m_meanFiberPoints.clear();
		for(int i =0; i< pts.size(); i++)
		{
			m_meanFiberPoints.push_back(Vector(pts[i][0],pts[i][1],pts[i][2]));
		}
	}
}

void SelectionObject::notifyStatsNeedUpdating()
{
    m_statsNeedUpdating = true;
}

void SelectionObject::setShowStartingPoint()
{
	m_showStartPoint = !m_showStartPoint; 
}

void SelectionObject::flipStartingPoint()
{
	m_flipStartingPoint = !m_flipStartingPoint;

	vector<Vector> temp;
	for(int i =m_meanFiberPoints.size()-1; i > -1; i--)
	{
		temp.push_back(m_meanFiberPoints[i]);
	}
	m_meanFiberPoints = temp;
}

vector< int > SelectionObject::getSelectedFibersIndexes( Fibers *pFibers )
{
    vector< bool > filteredFiber = pFibers->getFilteredFibers();
    
    SelectionState &curState = getState( pFibers->getDatasetIndex() );
    
    vector< bool > branchToUse;
    SelectionTree &selTree( SceneManager::getInstance()->getSelectionTree() );
    
    if( selTree.getActiveChildrenObjectsCount( this ) > 0 )
    {
        branchToUse = curState.m_inBranch;
    }
    else // No child.
    {
        // If it has a parent
        SelectionObject *pParentObj = selTree.getParentObject( this );
        
        if( pParentObj != NULL )
        {
            // OPTIM: this could be optimized
            SelectionState &parentState = pParentObj->getState( pFibers->getDatasetIndex() );
            branchToUse.assign( curState.m_inBranch.size(), false );
            
            bool parentIsNot( pParentObj->getIsNOT() );
            bool currentIsNot( getIsNOT() );
            
            for( unsigned int fiberIdx( 0 ); fiberIdx < curState.m_inBox.size(); ++fiberIdx )
            {
                if( !parentIsNot && !currentIsNot )
                {
                    branchToUse[ fiberIdx ] = parentState.m_inBox[ fiberIdx ] & curState.m_inBox[ fiberIdx ];
                }
                else if( !parentIsNot && currentIsNot )
                {
                    branchToUse[ fiberIdx ] = parentState.m_inBox[ fiberIdx ] & !curState.m_inBox[ fiberIdx ];
                }
                else if( parentIsNot && !currentIsNot )
                {
                    branchToUse[ fiberIdx ] = !parentState.m_inBox[ fiberIdx ] & curState.m_inBox[ fiberIdx ];
                }
                else // parentIsNot && currentIsNot
                {
                    branchToUse[ fiberIdx ] = !parentState.m_inBox[ fiberIdx ] & !curState.m_inBox[ fiberIdx ];
                }
            }
        }
        else // No parent, so this is a root object with no child.
        {
            branchToUse = curState.m_inBox;
        }
    }
    
    vector< int > selectedIndexes;
    
    for( unsigned int fiberIdx = 0; fiberIdx < branchToUse.size(); ++fiberIdx )
    {
        if( branchToUse[fiberIdx] && !filteredFiber[fiberIdx] )
        {
            selectedIndexes.push_back( fiberIdx );
        }
    }
    
    return selectedIndexes;
}

bool SelectionObject::getSelectedFibersInfo( const vector< int > &selectedFibersIdx, 
                                            Fibers *pFibers, 
                                            vector< int > &pointsCount, 
                                            vector< vector< Vector > > &fibersPoints )

{
    pointsCount.assign( selectedFibersIdx.size(), 0 );
    fibersPoints.assign( selectedFibersIdx.size(), vector< Vector >() );
    
    int curItem( 0 );
    
    for( vector< int >::const_iterator idxIt( selectedFibersIdx.begin() ); idxIt != selectedFibersIdx.end(); ++idxIt, ++curItem )
    {
        pointsCount[ curItem ] = pFibers->getPointsPerLine( *idxIt );
        pFibers->getFiberCoordValues( *idxIt, fibersPoints[ curItem ] );
    }
    
    return true;
}

///////////////////////////////////////////////////////////////////////////
//Return all the visible fibers that pass through the selection object
//
///////////////////////////////////////////////////////////////////////////
vector< vector< Vector > > SelectionObject::getSelectedFibersPoints()
{
    vector< Vector >           l_currentFiberPoints;
    vector< Vector >           l_currentSwappedFiberPoints;
    vector< vector< Vector > > l_selectedFibersPoints;
    Vector l_meanStart( 0.0f, 0.0f, 0.0f );
    
    Fibers* pFibers( NULL );

    long index = MyApp::frame->getCurrentListIndex();
    if( -1 != index )
    {
        pFibers = DatasetManager::getInstance()->getSelectedFibers( MyApp::frame->m_pListCtrl->GetItem( index ) );
    }
    vector< bool > filteredFiber = pFibers->getFilteredFibers();
    
    vector< bool > selectedInBranch = SceneManager::getInstance()->getSelectionTree().getSelectedFibersInBranch( pFibers, this);
    
    for( unsigned int i = 0; i < selectedInBranch.size(); ++i )
    {
        if( selectedInBranch[i] && !filteredFiber[i] )
        {
            // OPTIM: could avoid searching for the fiber every time.
            getFiberCoordValues( i, l_currentFiberPoints );
            
            // Because the direction of the fibers is not all the same, for example 2 fibers side to side on the screen
            // could have there coordinated in the data completly inversed ( first fiber 0,0,0 - 1,1,1 ),  
            // second fiber 1,1,1 - 0,0,0) we need to make sure that all the fibers are in the same order so we do a
            // verification and if the order of the fiber points are wrong we switch them.
            if( l_selectedFibersPoints.size() > 0 )
            {
                l_meanStart.zero();
                for( unsigned int j = 0; j < l_selectedFibersPoints.size(); ++j )
                    l_meanStart += l_selectedFibersPoints[j][0];
                l_meanStart /= l_selectedFibersPoints.size();
            }
            
            // If the starting point of the current fiber is closer to the mean starting point of the rest of the fibers
            // then the order of the points of this fiber are ok, otherwise we need to flip them.
            if( ( l_meanStart - l_currentFiberPoints[0] ).getLength() <
               ( l_meanStart - l_currentFiberPoints[l_currentFiberPoints.size() - 1] ).getLength() )
            {
                l_selectedFibersPoints.push_back( l_currentFiberPoints );
            }
            else
            {
                for( int k = (int)l_currentFiberPoints.size() - 1; k >= 0; --k )
                    l_currentSwappedFiberPoints.push_back( l_currentFiberPoints[k] );
                
                l_selectedFibersPoints.push_back( l_currentSwappedFiberPoints );
                l_currentSwappedFiberPoints.clear();
            }
            
            l_currentFiberPoints.clear();
        }
    }
    
    //std::cout << l_selectedFibersPoints.size() << std::endl;
    return l_selectedFibersPoints;
}

///////////////////////////////////////////////////////////////////////////
// Computes the length of a given fiber.
//
// i_fiberPoints            : The fiber to compute the length.
// o_length                 : The output length.
//
// Returns true if successful, false otherwise.
///////////////////////////////////////////////////////////////////////////
bool SelectionObject::getFiberLength( const vector< Vector > &i_fiberPoints, float &o_length )
{
    if( i_fiberPoints.size() == 0 )
        return false;

    float l_dx,l_dy, l_dz;
    o_length = 0.0;
    
    float voxelX = DatasetManager::getInstance()->getVoxelX();
    float voxelY = DatasetManager::getInstance()->getVoxelY();
    float voxelZ = DatasetManager::getInstance()->getVoxelZ();

    for( unsigned int i = 1; i < i_fiberPoints.size(); ++i )
    {
        // The values are in pixel, we need to set them in millimeters using the spacing 
        // specified in the anatomy file ( m_datasetHelper->xVoxel... ).
        l_dx = ( i_fiberPoints[i].x - i_fiberPoints[i-1].x ) * voxelX;
        l_dy = ( i_fiberPoints[i].y - i_fiberPoints[i-1].y ) * voxelY;
        l_dz = ( i_fiberPoints[i].z - i_fiberPoints[i-1].z ) * voxelZ;

        FArray currentVector( l_dx, l_dy, l_dz );
        o_length += (float)currentVector.norm();
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////
// Will calculate the mean fiber of a set of fibers.
//
// i_fibersPoints          : Set of fibers points we will use to calculate the mean fiber.
// i_nbPoints              : Number of points the mean fiber will have.
// o_meanFiberPoints       : The resulting points are store in o_meanFiberPoints.
//
// Returns true if successful, false otherwise.
///////////////////////////////////////////////////////////////////////////
bool SelectionObject::getMeanFiber( const vector< vector< Vector > > &i_fibersPoints, 
                                          unsigned int                i_nbPoints,
                                          vector< Vector >           &o_meanFiberPoints )
{
    o_meanFiberPoints.assign(i_nbPoints, Vector( 0.0, 0.0, 0.0 ));
	std::vector<Vector> firstStrmline;
	firstStrmline.resize(i_nbPoints);

	//For each streamline, resample to nbPoints
    for( unsigned int i = 0; i < i_fibersPoints.size(); ++i )
    {
        unsigned int l_currentFiberNbPoints = i_fibersPoints[i].size();
        std::vector<Vector> currentStrmResampled;
		
		currentStrmResampled.resize(i_nbPoints);

        // The -1 is because we dont take into consideration the number of points but 
        // the number of vector between the points.      
        float        l_currentFiberRatio    = (float)( i_fibersPoints[i].size() ) / ( i_nbPoints );
        
        currentStrmResampled[0]              += i_fibersPoints[i][0];
        currentStrmResampled[i_nbPoints - 1] += i_fibersPoints[i][l_currentFiberNbPoints - 1] ;

		//For each point of current streamline, resample the fibers to inbPoints
        for( unsigned int j = 1; j < i_nbPoints - 1; ++j )
        {
            float l_currentPointInterpolationRatio = l_currentFiberRatio * j;
             
            // Calculating the ratio of the interpolation.
            int l_pointBelow = (int)l_currentPointInterpolationRatio;
            l_currentPointInterpolationRatio -= l_pointBelow;
            // Simple interpolation.
            currentStrmResampled[j] += ( ( i_fibersPoints[i][l_pointBelow]     * ( 1.0 - l_currentPointInterpolationRatio ) ) + 
                                    ( i_fibersPoints[i][l_pointBelow + 1] *         l_currentPointInterpolationRatio ) );  	
		}

		if(i==0) //keep first streamline order
		{
			o_meanFiberPoints = currentStrmResampled;
		}
		else
		{
			float dDirect = 0.0;
			float dFlipped = 0.0;
			float dx, dy, dz;
			float dxf, dyf, dzf;

			//Average the mean points
			vector<Vector> centroid;
			centroid.assign(i_nbPoints, Vector( 0.0, 0.0, 0.0 ));
			for( unsigned int it = 0; it < o_meanFiberPoints.size(); ++it )
				centroid[it] = o_meanFiberPoints[it] / i;

			for( unsigned int j=0; j < i_nbPoints; j++ )
			{
				dx = centroid[j].x - currentStrmResampled[j].x;
				dy = centroid[j].y - currentStrmResampled[j].y;
				dz = centroid[j].z - currentStrmResampled[j].z;
				dDirect += sqrt(dx * dx + dy * dy + dz * dz);

				dxf = centroid[j].x - currentStrmResampled[i_nbPoints-1-j].x;
				dyf = centroid[j].y - currentStrmResampled[i_nbPoints-1-j].y;
				dzf = centroid[j].z - currentStrmResampled[i_nbPoints-1-j].z;
				dFlipped += sqrt(dxf * dxf + dyf * dyf + dzf * dzf);
			}

			dDirect /= i_nbPoints;
			dFlipped /= i_nbPoints;
			int idx;
			for( unsigned int j = 0; j < i_nbPoints; ++j )
			{
				idx = (dDirect > dFlipped)? i_nbPoints-1-j : j; 
				o_meanFiberPoints[idx] += currentStrmResampled[j];
			}
		}
	}

	//Average the mean points
	for( unsigned int i = 0; i < o_meanFiberPoints.size(); ++i )
		o_meanFiberPoints[i] /= i_fibersPoints.size();

    return true;
}

///////////////////////////////////////////////////////////////////////////
// Fills the o_fiberPoints vector with the points that compose a given fiber.
//
// i_fiberIndex             : The index of the given fiber.
// o_fiberPoints            : The vector that will be filled with the points of the given fiber.
//
// Returns true if successful, false otherwise.
///////////////////////////////////////////////////////////////////////////
bool SelectionObject::getFiberCoordValues( int i_fiberIndex, vector< Vector > &o_fiberPoints )
{
    long index = MyApp::frame->getCurrentListIndex();
    if( -1 != index )
    {
        Fibers* pFibers = DatasetManager::getInstance()->getSelectedFibers( MyApp::frame->m_pListCtrl->GetItem( index ) );

        if( pFibers == NULL || i_fiberIndex < 0 || i_fiberIndex >= pFibers->getFibersCount() )
            return false;

        int l_index = pFibers->getStartIndexForLine( i_fiberIndex ) * 3;
        Vector l_point3D;

        for( int i = 0; i < pFibers->getPointsPerLine( i_fiberIndex ); ++i )
        {
            l_point3D.x = pFibers->getPointValue( l_index );
            l_point3D.y = pFibers->getPointValue( l_index + 1);
            l_point3D.z = pFibers->getPointValue( l_index + 2 );
            o_fiberPoints.push_back( l_point3D );

            l_index += 3;
        }

        return true;
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////
// Computes the mean value of a given set of fibers. This value is calculated 
// from the anatomy that is already loaded.
//
// i_fibersPoints           : The given set of fibers.
// o_meanValue              : The output mean value.
//
// Returns true if successful, false otherwise.
///////////////////////////////////////////////////////////////////////////
bool SelectionObject::getMeanFiberValue( const vector< vector< Vector > > &fibersPoints, float &computedMeanValue )
{
    computedMeanValue = 0.0f;

    if( fibersPoints.size() == 0 )
    {
        return false;
    }
    
    Anatomy *pCurrentAnatomy( NULL );

    vector< Anatomy* > datasets = DatasetManager::getInstance()->getAnatomies();
    
    if( m_pCBSelectDataSet == NULL && datasets.size() > 0 && m_pRefAnatInfo != NULL )
    {
        // Select the first dataset in the list
        pCurrentAnatomy = m_pRefAnatInfo;
    }
    else if( m_pCBSelectDataSet != NULL && 
             m_pCBSelectDataSet->GetCount() > 0 && 
             m_pCBSelectDataSet->GetCurrentSelection() != -1 )
    {
        // TODO: Verify if the selection is the right index
        // Get the currently selected dataset.
        pCurrentAnatomy = datasets[ m_pCBSelectDataSet->GetSelection() ];
    }
    else
    {
        return false;
    }

    unsigned int pointsCount( 0 );
    unsigned int datasetPos ( 0 );

    float voxelX = DatasetManager::getInstance()->getVoxelX();
    float voxelY = DatasetManager::getInstance()->getVoxelY();
    float voxelZ = DatasetManager::getInstance()->getVoxelZ();

    for( unsigned int i = 0; i < fibersPoints.size(); ++i )
    {
        for( unsigned int j = 0; j < fibersPoints[i].size(); ++j )
        {
            datasetPos = ( static_cast<int>( fibersPoints[i][j].x / voxelX ) +
                           static_cast<int>( fibersPoints[i][j].y / voxelY ) * pCurrentAnatomy->getColumns() +
                           static_cast<int>( fibersPoints[i][j].z / voxelZ ) * pCurrentAnatomy->getColumns() * pCurrentAnatomy->getRows() ) * pCurrentAnatomy->getBands();
            
            for (int i(0); i < pCurrentAnatomy->getBands(); i++)
            {
                computedMeanValue += (* ( pCurrentAnatomy->getFloatDataset() ) )[datasetPos + i];
            }

            ++pointsCount;
        }
    }
    
    computedMeanValue /= pointsCount;

	float maxValue( 1.0f );
    switch( pCurrentAnatomy->getType() )
    {
        case HEAD_BYTE:
        {
            maxValue = 255.0;
            break;
        }
        case HEAD_SHORT:
        {
            maxValue = pCurrentAnatomy->getNewMax();
            break;
        }
        case OVERLAY:
        {
            maxValue = pCurrentAnatomy->getOldMax();
            break;
        }
		case RGB:
        {
            computedMeanValue = 0;
            break;
        }
    }
    //Denormalize
    computedMeanValue *= maxValue;

    return true;
}

void SelectionObject::setRefAnat(Anatomy * refAnat)
{
	m_pRefAnatInfo = refAnat; 
	m_pBtnRefAnat->SetLabel(refAnat->getName());
}

bool SelectionObject::performTractometry( const vector< vector< Vector > > &fibersPoints)
{
    if( fibersPoints.size() == 0 )
    {
        return false;
    }

	int noOfPts = m_meanFiberPoints.size();
    
    Anatomy *pCurrentAnatomy( NULL );
	std::vector<std::vector <float> > localMetrics;
	std::vector<std::vector <float> > localDists;
	m_tractometrics.resize(noOfPts);
	localMetrics.resize(noOfPts);
	localDists.resize(noOfPts);
    vector< Anatomy* > datasets = DatasetManager::getInstance()->getAnatomies();
    
	if( m_pCBSelectDataSet == NULL && datasets.size() > 0 && m_pRefAnatInfo != NULL)
    {
        // Select the first dataset in the list
        pCurrentAnatomy = m_pRefAnatInfo;
    }
    else if( m_pCBSelectDataSet != NULL && 
             m_pCBSelectDataSet->GetCount() > 0 && 
             m_pCBSelectDataSet->GetCurrentSelection() != -1 )
    {
        // TODO: Verify if the selection is the right index
        // Get the currently selected dataset.
        pCurrentAnatomy = datasets[ m_pCBSelectDataSet->GetSelection() ];
    }
    else
    {
        return false;
    }

    unsigned int pointsCount( 0 );
    unsigned int datasetPos ( 0 );
	float value = 0;

    float voxelX = DatasetManager::getInstance()->getVoxelX();
    float voxelY = DatasetManager::getInstance()->getVoxelY();
    float voxelZ = DatasetManager::getInstance()->getVoxelZ();

    for( unsigned int i = 0; i < fibersPoints.size(); ++i )
    {
        for( unsigned int j = 0; j < fibersPoints[i].size(); ++j )
        {
            datasetPos = ( static_cast<int>( fibersPoints[i][j].x / voxelX ) +
                           static_cast<int>( fibersPoints[i][j].y / voxelY ) * pCurrentAnatomy->getColumns() +
                           static_cast<int>( fibersPoints[i][j].z / voxelZ ) * pCurrentAnatomy->getColumns() * pCurrentAnatomy->getRows() ) * pCurrentAnatomy->getBands();
            
            for (int k(0); k < pCurrentAnatomy->getBands(); k++)
            {
                value= (* ( pCurrentAnatomy->getFloatDataset() ) )[datasetPos + k];
            }

			float dist = std::numeric_limits<float>::infinity();
			int assignTo;
			//assign Value to closest mean streamline pt;
			for(int l=0; l<noOfPts;l++)
			{
				float  l_distance = ( fibersPoints[i][j] - m_meanFiberPoints[l] ).getLength();

				if( l_distance < dist )
				{
					dist = l_distance;
					assignTo         = l;

					//update fiber color vector to match assigned point?
					//from 1 to noOfPts; (or within fibers color with distance from mean streamline ...)
				}
			}
			localMetrics[assignTo].push_back(value);
			localDists[assignTo].push_back(dist);

            ++pointsCount;
        }
    }
    
	float maxValue( 1.0f );
    switch( pCurrentAnatomy->getType() )
    {
        case HEAD_BYTE:
        {
            maxValue = 255.0;
            break;
        }
        case HEAD_SHORT:
        {
            maxValue = pCurrentAnatomy->getNewMax();
            break;
        }
        case OVERLAY:
        {
            maxValue = pCurrentAnatomy->getOldMax();
            break;
        }
    }
	vector<float> distMins;
	vector<float> distMaxs;
	//find min max dist for each point along the core to minmax norm their values when assigned
	for(int l=0; l<noOfPts;l++)
	{
		float min = std::numeric_limits<float>::infinity();
		float max = 0;
		for(int m=0; m<localDists[l].size();m++)
		{
			float v = localDists[l][m];
			if(v < min)
			{
				min = v;
			}
			if(v > max)
			{
				max = v;
			}
		}
		distMins.push_back(min);
		distMaxs.push_back(max);
	}

	for(int l=0; l<noOfPts;l++)
	{
		float sum = 0;
		for(int m=0; m<localMetrics[l].size();m++)
		{
			sum += localMetrics[l][m] * (1 - ((localDists[l][m] - distMins[l]) / (distMaxs[l] - distMins[l])));
		}
		m_tractometrics[l] = (sum/localMetrics[l].size())*maxValue;
	}

    return true;
}

///////////////////////////////////////////////////////////////////////////
// Computes the mean, max and min length for a given set of fibers.
//
// selectedFibersIndexes : The indexes of the selected fibers.
// pCurFibers            : A pointer to the current fiber dataset.
// meanLength            : The output mean length.
// maxLength             : The output max length.
// minLength             : The output min length.
//
// Returns true if successful, false otherwise.
///////////////////////////////////////////////////////////////////////////
bool SelectionObject::getMeanMaxMinFiberLength( const vector< int > &selectedFibersIndexes,
                                                  Fibers        *pCurFibers,
                                                  float         &meanLength,
                                                  float         &maxLength,
                                                  float         &minLength)
{
    meanLength = 0.0f;
    maxLength  = 0.0f;
    minLength  = numeric_limits<float>::max();
    
    if( selectedFibersIndexes.empty() )
    {
        minLength = 0.0f;
        return false;
    }
    
    float curFiberLength( 0.0f );
    
    for( vector< int >::const_iterator idxIt( selectedFibersIndexes.begin() );
        idxIt != selectedFibersIndexes.end(); ++idxIt )
    {
        curFiberLength = pCurFibers->getFiberLength( *idxIt );
        
        meanLength += curFiberLength;
        
        maxLength = std::max( maxLength, curFiberLength );
        minLength = std::min( minLength, curFiberLength );

    }
    
    meanLength /= selectedFibersIndexes.size();
    
    return true;
}

bool SelectionObject::saveTractometry(wxString filename)
{
	//Compute part
	FibersGroup *pFiberGroup = DatasetManager::getInstance()->getFibersGroup();
    
    if( pFiberGroup == NULL )
    {
        Logger::getInstance()->print( wxT( "In SelectionObject::updateStats, pFiberGroup is NULL" ), LOGLEVEL_DEBUG );
        return false;
    }
    
    int activeFiberSetCount( 0 );
    
    vector< Fibers * > pFibersSet = DatasetManager::getInstance()->getFibers();
    
    for( size_t fiberSetIdx(0); fiberSetIdx < pFibersSet.size(); ++fiberSetIdx )
    {
        Fibers *pCurFibers = pFibersSet[ fiberSetIdx ];
        if( pCurFibers->getShow() )
        {
            vector< int > selectedFibersIdx = getSelectedFibersIndexes( pCurFibers );
            
            if( selectedFibersIdx.empty() )
            {
                // Do not want to do any processing in this case.
                continue;
            }
            
            vector< int > nbPointsBySelectedFiber;
            vector< vector < Vector > > pointsBySelectedFiber;
            
            getSelectedFibersInfo( selectedFibersIdx, pCurFibers, 
                                  nbPointsBySelectedFiber, pointsBySelectedFiber );
            ++activeFiberSetCount;
                
            performTractometry( pointsBySelectedFiber );
		}
	}

	//Save part
	std::ofstream myfile;
    char *pFn;
    float dist = 0.5;

    if( filename.AfterLast( '.' ) != _T( "txt" ) )
    {
        filename += _T( ".txt" );
    }

    pFn = ( char * ) malloc( filename.length() );
    strcpy( pFn, ( const char * ) filename.mb_str( wxConvUTF8 ) );
    myfile.open( pFn, std::ios::out );

	for (int i = 0; i < m_tractometrics.size(); i++)
	{
		myfile << m_tractometrics[i];
		if(i != m_tractometrics.size()-1)
			myfile << ", ";
	}

    myfile.close();

	return true;
}

bool SelectionObject::getLongestStreamline( const vector< int > &selectedFibersIndexes,
                                                  Fibers        *pCurFibers)
{
	float maxLength  = 0.0f;
	std::vector<std::pair<int,int> > mapLength;
	std::vector<Vector> firstStrmline;
	firstStrmline.resize(m_noOfMeanFiberPts);
	if( selectedFibersIndexes.empty() )
    {
        return false;
    }

	//Top 5% longest streamlines
	int topFive = selectedFibersIndexes.size() * 0.05;
    
    float curFiberLength( 0.0f );
    
    for( vector< int >::const_iterator idxIt( selectedFibersIndexes.begin() );
        idxIt != selectedFibersIndexes.end(); ++idxIt )
    {
		mapLength.push_back(std::pair<int,int>(pCurFibers->getFiberLength( *idxIt ),*idxIt));
    }

	std::sort(mapLength.begin(), mapLength.end());

	//Resample to X nb point
	for(int i=0; i< topFive; i++)
	{
		int reverseid = selectedFibersIndexes.size()-1-i;
		int id = mapLength[reverseid].second;
		vector<Vector> currFiber;
		std::vector<Vector> currentStrmResampled;
		currentStrmResampled.resize(m_noOfMeanFiberPts);
		pCurFibers->getFiberCoordValues(id, currFiber);
		unsigned int l_currentFiberNbPoints = currFiber.size();
        
		// The -1 is because we dont take into consideration the number of points but 
		// the number of vector between the points.      
		float        l_currentFiberRatio    = (float)( currFiber.size() - 1 ) / ( m_noOfMeanFiberPts - 1 );
        
		currentStrmResampled[0]              += currFiber[1];
		currentStrmResampled[m_noOfMeanFiberPts - 1] += currFiber[l_currentFiberNbPoints - 2] ;

		//For each point of current streamline, resample the fibers to inbPoints
		for(int j = 1; j < m_noOfMeanFiberPts - 1; ++j )
		{
			float l_currentPointInterpolationRatio = l_currentFiberRatio * j;
             
			// Calculating the ratio of the interpolation.
			int l_pointBelow = (int)l_currentPointInterpolationRatio;
			l_currentPointInterpolationRatio -= l_pointBelow;
			// Simple interpolation.
			currentStrmResampled[j] += ( ( currFiber[l_pointBelow]     * ( 1.0 - l_currentPointInterpolationRatio ) ) + 
									( currFiber[l_pointBelow + 1] *         l_currentPointInterpolationRatio ) );  	
		}

		if(i==0) //keep first streamline order
		{
			m_meanFiberPoints = currentStrmResampled;
			firstStrmline = currentStrmResampled;
		}
		else
		{
			//Before insterting it in the o_meanfiberPoints, check MDF for flips
			float dDirect = 0;
			float dFlipped = 0;
			for(int j = 0; j < m_noOfMeanFiberPts; ++j )
			{	
				dDirect += abs(sqrt((firstStrmline[j].x - currentStrmResampled[j].x)*(firstStrmline[j].x - currentStrmResampled[j].x)
					+(firstStrmline[j].y - currentStrmResampled[j].y)*(firstStrmline[j].y - currentStrmResampled[j].y)
					+(firstStrmline[j].z - currentStrmResampled[j].z)*(firstStrmline[j].z - currentStrmResampled[j].z)));

				int k = (m_noOfMeanFiberPts-1)-j;

				dFlipped += abs(sqrt((firstStrmline[j].x - currentStrmResampled[k].x)*(firstStrmline[j].x - currentStrmResampled[k].x)
					+(firstStrmline[j].y - currentStrmResampled[k].y)*(firstStrmline[j].y - currentStrmResampled[k].y)
					+(firstStrmline[j].z - currentStrmResampled[k].z)*(firstStrmline[j].z - currentStrmResampled[k].z)));
			}

			dDirect /= m_noOfMeanFiberPts;
			dFlipped /= m_noOfMeanFiberPts;

			if(dDirect < dFlipped)
			{
				for(int j = 0; j < m_noOfMeanFiberPts; ++j )
				{
					m_meanFiberPoints[j] += currentStrmResampled[j];
				}
			}
			else
			{
				int l = 0;
				for( int j = m_noOfMeanFiberPts - 1; j > -1; --j )
				{				
					m_meanFiberPoints[l] += currentStrmResampled[j];
					l++;
				}
			}
		}
	}
	
	for(int j = 0; j < m_noOfMeanFiberPts; ++j )
	{
		m_meanFiberPoints[j] /= topFive;
	}
   
    return true;
}

///////////////////////////////////////////////////////////////////////////
// Computes the mean, max and the min cross section for a given set of fibers.
//
// i_fibersPoints           : The given set of fibers we are going to calculate the cross section for.
// i_meanFiberPoints        : The mean fiber we are going to use to generate the plane for the cross section calculation.
// o_meanCrossSection       : The output mean cross section.
// o_maxCrossSection        : The output max cross section.
// o_minCrossSection        : The output min cross section.
//
// Returns true if successful, false otherwise.
///////////////////////////////////////////////////////////////////////////
bool SelectionObject::getMeanMaxMinFiberCrossSection( const vector< vector< Vector > > &i_fibersPoints,
                                                      const vector< Vector >           &i_meanFiberPoints, 
                                                            float                      &o_meanCrossSection,
                                                            float                      &o_maxCrossSection,
                                                            float                      &o_minCrossSection )
{
    m_crossSectionsPoints.clear();
    m_crossSectionsAreas.clear();
    m_crossSectionsNormals.clear();
    m_crossSectionsPoints.resize ( i_meanFiberPoints.size() );
    m_crossSectionsAreas.resize  ( i_meanFiberPoints.size() );
    m_crossSectionsNormals.resize( i_meanFiberPoints.size() );

	vector<Vector> currentStrmResampled;
	currentStrmResampled.resize(i_meanFiberPoints.size() );

    o_meanCrossSection = 0.0f;
    o_maxCrossSection  = 0.0f;

    // We need at least 3 fibers to get 3 points to be able to calculate a convex hull!
    if( i_fibersPoints.size() < 3 )
    {
        o_minCrossSection  = 0.0f;
        return false;
    }

    o_minCrossSection  = numeric_limits<float>::max();

    vector< Vector > l_maxHullPts, l_maxOriginalPts, l_minHullPts, l_minOriginalPts;

    // For each points on the mean fiber we calculate a plane perpendicular with it.
    for( unsigned int i = 0; i < i_meanFiberPoints.size(); ++i )
    {        
        vector < Vector > l_intersectionPoints;
        Vector l_pointOnPlane, l_planeNormal;
        l_pointOnPlane = i_meanFiberPoints[i];

        // When we are at the last point of the mean fiber, since i_meanFiberPoints[i + 1] 
        // will not exist, we calculate the vector with the previous point.
        if( i == i_meanFiberPoints.size() - 1 )
            l_planeNormal = (i_meanFiberPoints[i] - i_meanFiberPoints[i - 1])*-1;
        else
            l_planeNormal = i_meanFiberPoints[i + 1] - i_meanFiberPoints[i];

        l_planeNormal.normalize();

        // We get the points intersecting the current plane for all the selectedFibers.
        for( unsigned int j = 0; j < i_fibersPoints.size(); ++j )
            getFiberPlaneIntersectionPoint( i_fibersPoints[j], l_pointOnPlane, l_planeNormal, l_intersectionPoints, i );

        // We need at least 3 points to get a valid cross section.

        if( l_intersectionPoints.size() < 3 )
            continue;

        vector < Vector > original3Dpts = l_intersectionPoints;

        // Once we have those 3d points, we need to map them to 2d so we can calculate the convex hull.
        if( ! ( Helper::convert3DPlanePointsTo2D( l_planeNormal, l_intersectionPoints ) ) )
            continue;

        ConvexGrahamHull hull( l_intersectionPoints );
       
        // Let's build the convex hull.
        vector < Vector > l_hullPoints;
        if( ! ( hull.buildHull() ) )
            continue;
        hull.getHullPoints( l_hullPoints );
        // Computing the surface area of the hull.
        double l_hullArea = 0.0f;
        if( ! ( hull.area( l_hullArea ) ) )
            continue;

        o_meanCrossSection += l_hullArea;

        // To be able to see the cross section on the screen we save the points in m_crossSectionsPoints
        // and the area value in m_crossSectionsAreas, we also save the normals of the planes inside
        // m_crossSectionsNormals to be able to draw the dispersion cone.
        vector< Vector > tmp;
        Helper::convert2DPlanePointsTo3D( original3Dpts, l_hullPoints, tmp );
        m_crossSectionsPoints[i]  = tmp;
        m_crossSectionsAreas[i]   = l_hullArea;
        m_crossSectionsNormals[i] = l_planeNormal;

		////update mean streamline from mid point of cross sections
		Vector mean (0,0,0);
		for(int cs = 0; cs < m_crossSectionsPoints[i].size(); cs++)
			mean+=Vector(m_crossSectionsPoints[i][cs].x, m_crossSectionsPoints[i][cs].y, m_crossSectionsPoints[i][cs].z);

		mean/=m_crossSectionsPoints[i].size();
		//except first and last plane, since sometime this may be jagged with spurious ends
		
	/*	if(i != 0 && i != m_crossSectionsPoints.size()-1) 
		{*/
			//m_meanFiberPoints[i] = mean;
			vector<double> v;
			v.push_back(mean.x);
			v.push_back(mean.y);
			v.push_back(mean.z);
			VecMean.push_back(v);

		//}
		//if((i % 2) == 0)
		//{

		//}


        // Maximum Cross Section.
        if( l_hullArea > o_maxCrossSection )
        {
            o_maxCrossSection = (float)l_hullArea;
            m_maxCrossSectionIndex = i;
        }

        // Minimum Cross Section.
        if( l_hullArea < o_minCrossSection )
        {
            o_minCrossSection = (float)l_hullArea;
            m_minCrossSectionIndex = i;
        }
    }
     
    o_meanCrossSection /= i_meanFiberPoints.size();

    float voxelX = DatasetManager::getInstance()->getVoxelX();
    float voxelY = DatasetManager::getInstance()->getVoxelY();
    float voxelZ = DatasetManager::getInstance()->getVoxelZ();

    // We want to return the values in millimeters so we need to multiply them by the spacing in the anatomy file.
    float l_spacing = voxelX * voxelY * voxelZ;

    o_maxCrossSection  *= l_spacing;
    o_minCrossSection  *= l_spacing;
    o_meanCrossSection *= l_spacing;

    return true;
}

///////////////////////////////////////////////////////////////////////////
// Will return the intersection point between a fiber and a plane.
//
// i_fiberPoints            : Containt the points information of the fiber.
// i_pointOnPlane           : A point on the plane.
// i_planeNormal            : The normal of the plane.
// o_intersectionPoints     : Output vector that will containt the intersection point.
//
// Returns true if successful, false otherwise.
///////////////////////////////////////////////////////////////////////////
bool SelectionObject::getFiberPlaneIntersectionPoint( const vector< Vector > &i_fiberPoints,
                                                      const Vector           &i_pointOnPlane,
                                                      const Vector           &i_planeNormal,
                                                            vector< Vector > &o_intersectionPoints,
															int id)
{
    float l_minDistance = m_CSThreshold;
	//if(id == 0 || id == m_meanFiberPoints.size()-1)
		//	l_minDistance /= 2;

    Vector l_tmp;
    bool l_intersected = false;
    
    for( unsigned int i = 0; i < i_fiberPoints.size() - 1; ++i )
    {
        Vector l_intersectionPoint;
        // Check if there is an intersection point between the plane and the line.
        if( Helper::getIntersectionPoint( i_fiberPoints[i], i_fiberPoints[i + 1], i_pointOnPlane, i_planeNormal, l_intersectionPoint ) )
        {
            // If there was more that 1 point of the current fiber that was in contact with the plane,
            // we only keep the one closest to the mean fiber and since i_pointOnPlane is actually the
            // mean fiber point used to generate the plane, we use it.
            float  l_distance = ( l_intersectionPoint - i_pointOnPlane ).getLength();

            if( l_distance < l_minDistance )
            {
                l_minDistance = l_distance;
                l_tmp         = l_intersectionPoint;
                l_intersected = true;
            }
        }
    }

    // Only push back a point if there was at least 1 intersection.
    if( l_intersected )
        o_intersectionPoints.push_back( l_tmp );

    return true;
}

///////////////////////////////////////////////////////////////////////////
// We compute the dispersion in the following manner:
// We create the two tightest circles we can fit around the min and max cross sections.
// ( m_crossSectionsPoints[m_minCrossSectionIndex], m_crossSectionsPoints[m_maxCrossSectionIndex] ).
// We then create a cone using these two circles.
// A portion of the cone's height is equal to the length of the mean fiber ( m_meanFiberPoints )
// between the min and max cross section planes.
//
// o_dispersion         : The output mean dipersion of the given fiber.
//
// Returns true if successful, false otherwise.
///////////////////////////////////////////////////////////////////////////
bool SelectionObject::getFiberDispersion( float &o_dispersion )
{
    o_dispersion = 0.0f;

    if( m_meanFiberPoints.size()                             == 0 ||  
        m_crossSectionsPoints.size()                         == 0 ||
        m_crossSectionsPoints[m_maxCrossSectionIndex].size() == 0 ||
        m_crossSectionsPoints[m_minCrossSectionIndex].size() == 0 )
    {
        return false;
    }

    vector< Vector > l_fiberPoints;

    if( m_minCrossSectionIndex < m_maxCrossSectionIndex )
    {
        for( unsigned int i = m_minCrossSectionIndex; i <= m_maxCrossSectionIndex; ++i )
            l_fiberPoints.push_back( m_meanFiberPoints[i] ) ;
    }
    else if( m_minCrossSectionIndex > m_maxCrossSectionIndex )
    {
        for( unsigned int i = m_maxCrossSectionIndex; i <= m_minCrossSectionIndex; ++i )
            l_fiberPoints.push_back( m_meanFiberPoints[i] ) ;
    }
    else
        return false;
     
    float l_coneLength = 0.0f;
    getFiberLength( l_fiberPoints, l_coneLength );

    // Calculating the max and min cross section circle radius.
    float l_maxRadius = getMaxDistanceBetweenPoints( m_crossSectionsPoints[m_maxCrossSectionIndex] ) / 2.0f;
    float l_minRadius = getMaxDistanceBetweenPoints( m_crossSectionsPoints[m_minCrossSectionIndex] ) / 2.0f;

    l_coneLength = l_coneLength / ( 1.0f - l_minRadius / l_maxRadius );

    float hypotenuse = sqrt( l_coneLength*l_coneLength + l_maxRadius*l_maxRadius );

    // We calculate the dispersion angle in degrees.
    o_dispersion = asin( l_maxRadius / hypotenuse ) * 180.0f / M_PI;

    return true;
}

///////////////////////////////////////////////////////////////////////////
// This fonction will calculate the distance between all the points in i_points
// and return the biggest one, it will also return the index of the 2 points that
// gave this distance.
//
// i_points             : The vector that contains the points we will calculate the distance from.
// o_firstPointIndex    : The first point index forming the pair of points with the biggest distance.
// o_secondPointIndex   : The second point index forming the pair of points with the biggest distance.
//
// Returns the max distance found.
///////////////////////////////////////////////////////////////////////////
float SelectionObject::getMaxDistanceBetweenPoints( const vector< Vector > &i_points, int* o_firstPointIndex, int* o_secondPointIndex )
{
    float l_currentDistance, l_maxDistance = 0.0f;
    
    for( unsigned int i = 0; i < i_points.size(); ++i )
        for( unsigned int j = 0; j < i_points.size(); ++j )
            if( i > j )
            {
                l_currentDistance = ( i_points[i] - i_points[j] ).getLength();
                
                if( l_currentDistance > l_maxDistance )
                {
                    l_maxDistance = l_currentDistance;
                    if( o_firstPointIndex  ) *o_firstPointIndex  = i;
                    if( o_secondPointIndex ) *o_secondPointIndex = j;
                }
            }

    return l_maxDistance;
}

///////////////////////////////////////////////////////////////////////////
// This is the basic draw function that all the selection object share.
// After doing some basic calculation this function will call drawObject()
// to let its subclass do there specific draw.
///////////////////////////////////////////////////////////////////////////
void SelectionObject::draw()
{
	if( m_meanFiberIsBeingDisplayed || m_displayCrossSections != CS_NOTHING || m_displayDispersionCone != DC_NOTHING )
    {
		if(m_objectType == VOI_TYPE)
		{
			SceneManager::getInstance()->getScene()->lightsOff();
			drawFibersInfo();
            SceneManager::getInstance()->getScene()->lightsOn();
		}
		else
			drawFibersInfo();
    }
    
    if( ! m_isVisible )
        return;

    GLfloat l_color[] = { 0.5f, 0.5f, 0.5f, 0.5f };

    if ( ! m_isNOT )
    {
        l_color[0] = 138.0f / 255.0f; // Red
        l_color[1] = 43.0f / 255.0f; // Green
        l_color[2] = 228.0f / 255.0f; // Blue
    }
    else
    {
        l_color[0] = 1.0f; // Red
        l_color[1] = 0.0f; // Green
        l_color[2] = 0.0f; // Blue
    }

    if( m_isSelected )
        l_color[3] = 0.4f; // Alpha
    else
        l_color[3] = 0.2f; // Alpha

    if(m_isMagnet)
    {
        l_color[0] = std::abs(m_magnetField.x);
        l_color[1] = std::abs(m_magnetField.y);
        l_color[2] = std::abs(m_magnetField.z);
    }

	if(m_objectType == VOI_TYPE)
	{

		ShaderProgram *pMeshShader = ShaderHelper::getInstance()->getMeshShader();
		pMeshShader->bind();
		ShaderHelper::getInstance()->setMeshShaderVars();

		pMeshShader->setUniInt  ( "showFS", true );
		pMeshShader->setUniInt  ( "useTex", false );
		pMeshShader->setUniFloat( "alpha_", 1.0 );

		drawObject( l_color );

		pMeshShader->release();
	}
	else
		drawObject( l_color );
}

///////////////////////////////////////////////////////////////////////////
// Updates the status bar at the button of the application window.
///////////////////////////////////////////////////////////////////////////
void SelectionObject::updateStatusBar()
{
    float voxelX = DatasetManager::getInstance()->getVoxelX();
    float voxelY = DatasetManager::getInstance()->getVoxelY();
    float voxelZ = DatasetManager::getInstance()->getVoxelZ();

    MyApp::frame->GetStatusBar()->SetStatusText( m_name, 1 );
    MyApp::frame->GetStatusBar()->SetStatusText( wxString::Format( wxT( "Position: %.2f, %.2f, %.2f  Size: %.2f, %.2f, %.2f" ),
                                                                 m_center.x, m_center.y, m_center.z,
                                                                 m_size.x * voxelX, m_size.y * voxelY, m_size.z * voxelZ ),
                                                                 2 );
}

///////////////////////////////////////////////////////////////////////////
// Will draw the fibers info (the mean fiber, the cross sections and the dispersion cone)
// if this selection object is of type MASTER_OBJECT, if not then its of type CHILD_OBJECT, 
// we get its master object because the master object is the one coitaining the fibers that are
// in branch between itself and its child object.
///////////////////////////////////////////////////////////////////////////
void SelectionObject::drawFibersInfo()
{
    updateStats();

    glDisable( GL_DEPTH_TEST);
    // Draw the mean fiber.
    drawThickFiber( m_meanFiberPoints, (float)THICK_FIBER_THICKNESS/100.0f, THICK_FIBER_NB_TUBE_EDGE );

	if(m_showStartPoint)
	{
		glColor3f(1.0f, 0.0f, 0.0f);
		SceneManager::getInstance()->getScene()->drawSphere( m_meanFiberPoints[0].x, m_meanFiberPoints[0].y, m_meanFiberPoints[0].z, 5);
	}
	glEnable( GL_DEPTH_TEST);

    // TODO selection convex hull
    drawConvexHull();
    drawCrossSections();
}

///////////////////////////////////////////////////////////////////////////
// This function will draw the dispersion cone formed by the selected fibers.
///////////////////////////////////////////////////////////////////////////
void SelectionObject::drawDispersionCone()
{
    // This vector will contain all the dispersion circle to draw depending
    // on the m_displayDispersionCone currently selected.
    vector< vector< Vector > > l_allCirclesPoints;
    
    switch( m_displayDispersionCone )
    {
        case DC_NOTHING : return;
        case DC_MIN_MAX_ONLY : 
        {
            l_allCirclesPoints.resize( m_crossSectionsPoints.size() );
            getDispersionCircle( m_crossSectionsPoints[m_minCrossSectionIndex],
                                 m_crossSectionsNormals[m_minCrossSectionIndex],
                                 l_allCirclesPoints[m_minCrossSectionIndex] );
            getDispersionCircle( m_crossSectionsPoints[m_maxCrossSectionIndex],
                                 m_crossSectionsNormals[m_maxCrossSectionIndex],
                                 l_allCirclesPoints[m_maxCrossSectionIndex] );
            drawSimpleCircles( l_allCirclesPoints );
            break;
        }
        case DC_EVERYTHING :
        case DC_FULL_LINE_CONE :
        case DC_FULL_CONE :
        {
            l_allCirclesPoints.resize( m_crossSectionsPoints.size() );
            for( unsigned int i = 0; i < m_crossSectionsPoints.size(); ++i )
                getDispersionCircle( m_crossSectionsPoints[i], m_crossSectionsNormals[i], l_allCirclesPoints[i] );
            
            switch( m_displayDispersionCone )
            {
                case DC_EVERYTHING     : drawSimpleCircles( l_allCirclesPoints );       break;
                case DC_FULL_LINE_CONE : drawTube( l_allCirclesPoints, GL_LINE_LOOP );  break;
                case DC_FULL_CONE      : drawTube( l_allCirclesPoints, GL_QUAD_STRIP ); break;
                default : return;
            }
        }
        default : return;
    }
}

///////////////////////////////////////////////////////////////////////////
// This function will draw multiple circles formed by the points inside i_allCirclesPoints. 
//
// i_circlesPoints          : The points forming the circles we need to draw.
///////////////////////////////////////////////////////////////////////////
void SelectionObject::drawSimpleCircles( const vector< vector< Vector > > &i_circlesPoints )
{
    // For all the emelement that should be drawn.
    for( unsigned int i = 0; i < i_circlesPoints.size(); ++i )
    {
        if( i_circlesPoints[i].size() <= 0 )
            continue;

        getCrossSectionAreaColor( i );

        // Form simple circle around all the cross sections.
        glBegin( GL_LINE_LOOP );
            for( unsigned int j = 0; j < i_circlesPoints[i].size(); ++j )
                glVertex3f( i_circlesPoints[i][j].x, i_circlesPoints[i][j].y, i_circlesPoints[i][j].z );

            // This is to actually have a full circle the first point need to be the last point.
            glVertex3f( i_circlesPoints[i][0].x, i_circlesPoints[i][0].y, i_circlesPoints[i][0].z );
        glEnd();
    }
}

///////////////////////////////////////////////////////////////////////////
// This function will draw a cone formed by a series of circles. 
// The circles all need to have the same amount of points.
//
// i_allCirclesPoints           : The points forming the circles we will draw the cone with.
// i_tubeType                   : The type of primitive that we will use to render the tube.
///////////////////////////////////////////////////////////////////////////
void SelectionObject::drawTube( const vector< vector< Vector > > &i_allCirclesPoints, GLenum i_tubeType )
{           
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
                 
    unsigned int l_secondCircleIndex;

    for( unsigned int i = 0; i < i_allCirclesPoints.size(); ++i )
    {
        // Make sure that the circle at index i actually has some points.
        if( i_allCirclesPoints[i].size() <= 0 )
            continue;
        else // Find the closest circle from the circle at index i that also has some points.
        {
            for( l_secondCircleIndex = i+1; l_secondCircleIndex < i_allCirclesPoints.size(); ++l_secondCircleIndex )
            {
                if( i_allCirclesPoints[l_secondCircleIndex].size() <= 0 )
                    continue;
                else
                    break;
            }
            // make sure that we did found a second circle with some points, 
            if( ! ( l_secondCircleIndex < i_allCirclesPoints.size() ) )
                break;
        }

        // Make the tube by linking two circles together.
        glBegin( i_tubeType );
            for( unsigned int k = 0; k < i_allCirclesPoints[i].size(); ++k )
            {                
                getCrossSectionAreaColor( i );
                glVertex3f( i_allCirclesPoints[i][k].x, i_allCirclesPoints[i][k].y, i_allCirclesPoints[i][k].z );

                getCrossSectionAreaColor( l_secondCircleIndex );
                glVertex3f( i_allCirclesPoints[l_secondCircleIndex][k].x,
                            i_allCirclesPoints[l_secondCircleIndex][k].y,
                            i_allCirclesPoints[l_secondCircleIndex][k].z );
            } 

            // This is to actually have a full tube the first point need to be the last point.
            getCrossSectionAreaColor( i );
            glVertex3f( i_allCirclesPoints[i][0].x, i_allCirclesPoints[i][0].y, i_allCirclesPoints[i][0].z );

            getCrossSectionAreaColor( l_secondCircleIndex );
            glVertex3f( i_allCirclesPoints[l_secondCircleIndex][0].x,
                        i_allCirclesPoints[l_secondCircleIndex][0].y,
                        i_allCirclesPoints[l_secondCircleIndex][0].z );
        glEnd();
    }
    
    glDisable( GL_BLEND );
}

///////////////////////////////////////////////////////////////////////////
// This function will calculate the points forming a circle formed by the arguments.
// The circle will be the smallest circle possible to fit around the points inside i_crossSectionPoints.
//
// i_crossSectionPoints         : The points representing the cross section we want to draw a circle around.
// i_crossSectionNormal         : The value of the normal of this cross section.
// o_circlePoints               : This vector will contain all the points generated for this circle.
///////////////////////////////////////////////////////////////////////////
void SelectionObject::getDispersionCircle( const vector< Vector > &i_crossSectionPoints, 
                                           const Vector           &i_crossSectionNormal, 
                                                 vector< Vector > &o_circlePoints )
{
    if( i_crossSectionPoints.size() < 3 )
        return;
    
    int l_firstPointIndex, l_secondPointIndex;

    // The radius of the circle is the half of the distance between the 2 points forming the max distance.
    float l_radius = getMaxDistanceBetweenPoints( i_crossSectionPoints, &l_firstPointIndex, &l_secondPointIndex ) / 2.0f;

    // The center of the circle is the center of the vector formed by the the 2 points forming the max distance.
    Vector l_center = ( ( i_crossSectionPoints[l_secondPointIndex] - i_crossSectionPoints[l_firstPointIndex] ) / 2.0f )
                      + i_crossSectionPoints[l_firstPointIndex];


    Helper::getCirclePoints( l_center, i_crossSectionNormal, l_radius, DISPERSION_CONE_NB_TUBE_EDGE, o_circlePoints );
}

///////////////////////////////////////////////////////////////////////////
// This function will set a color depending on the cross section index passed in argument.
// The cross section with the maximum area will be pure blue, and the cross section with 
// the minimum area will be pure red, all the other will be a mix of red and blue.
//
// i_index          : The index representing the position of the cross section we
//                    want to use to set the color.
///////////////////////////////////////////////////////////////////////////
void SelectionObject::getCrossSectionAreaColor( unsigned int i_index )
{
    float l_u = ( m_crossSectionsAreas[i_index] - m_crossSectionsAreas[m_minCrossSectionIndex] ) /
                ( m_crossSectionsAreas[m_maxCrossSectionIndex] - m_crossSectionsAreas[m_minCrossSectionIndex] );

    glColor4f( 1.0f - l_u, 0.0f, l_u, 0.35f );
}

bool SelectionObject::addFiberDataset( const FiberIdType &fiberId, const int fiberCount )
{
    using std::map;
    using std::pair;
    
    pair< map< FiberIdType, SelectionState >::iterator, bool > insertResult = m_selectionStates.insert( pair< FiberIdType, SelectionState >( fiberId, SelectionState( ) ) );
    
    notifyInBoxNeedsUpdating();
    notifyStatsNeedUpdating();
    
    return insertResult.second;
}

void SelectionObject::removeFiberDataset( const FiberIdType &fiberId )
{
    m_selectionStates.erase( fiberId );
    notifyInBoxNeedsUpdating();
    notifyStatsNeedUpdating();
}

SelectionObject::SelectionState& SelectionObject::getState( const FiberIdType &fiberId )
{
    return m_selectionStates[ fiberId ];
}

bool SelectionObject::populateXMLNode( wxXmlNode *pCurNode, const wxString &rootPath )
{
    wxString floatPrecision = wxT( ".8" );
    
    pCurNode->AddAttribute( new wxXmlAttribute( wxT( "type" ), getTypeTag() ) );
    
    wxXmlNode *pState = new wxXmlNode( NULL, wxXML_ELEMENT_NODE, wxT( "state" ) );
    pCurNode->AddChild( pState );
    
    pState->AddAttribute( new wxXmlAttribute( wxT( "name" ), m_name) );
    pState->AddAttribute( new wxXmlAttribute( wxT( "active" ), m_isActive? wxT( "yes" ) : wxT( "no" ) ) );
    pState->AddAttribute( new wxXmlAttribute( wxT( "visible" ), m_isVisible? wxT( "yes" ) : wxT( "no" ) ) );
    pState->AddAttribute( new wxXmlAttribute( wxT( "isNOT" ), m_isNOT? wxT( "yes" ) : wxT( "no" ) ) );
    pState->AddAttribute( new wxXmlAttribute( wxT( "isMagnet" ), m_isMagnet? wxT( "yes" ) : wxT( "no" ) ) );
    pState->AddAttribute( new wxXmlAttribute( wxT( "fieldX" ), wxStrFormat( m_magnetField.x, floatPrecision ) ) );
    pState->AddAttribute( new wxXmlAttribute( wxT( "fieldY" ), wxStrFormat( m_magnetField.y, floatPrecision ) ) );
    pState->AddAttribute( new wxXmlAttribute( wxT( "fieldZ" ), wxStrFormat( m_magnetField.z, floatPrecision ) ) );
    
    
    wxXmlNode *pCenter = new wxXmlNode( NULL, wxXML_ELEMENT_NODE, wxT( "center" ) );
    pCenter->AddAttribute( new wxXmlAttribute( wxT( "posX" ), wxStrFormat( m_center.x, floatPrecision ) ) );
    pCenter->AddAttribute( new wxXmlAttribute( wxT( "posY" ), wxStrFormat( m_center.y, floatPrecision ) ) );
    pCenter->AddAttribute( new wxXmlAttribute( wxT( "posZ" ), wxStrFormat( m_center.z, floatPrecision ) ) );
    pCurNode->AddChild( pCenter );
    
    wxXmlNode *pSize = new wxXmlNode( NULL, wxXML_ELEMENT_NODE, wxT( "size" ) );
    pSize->AddAttribute( new wxXmlAttribute( wxT( "sizeX" ), wxStrFormat( m_size.x, floatPrecision ) ) );
    pSize->AddAttribute( new wxXmlAttribute( wxT( "sizeY" ), wxStrFormat( m_size.y, floatPrecision ) ) );
    pSize->AddAttribute( new wxXmlAttribute( wxT( "sizeZ" ), wxStrFormat( m_size.z, floatPrecision ) ) );
    pCurNode->AddChild( pSize );
    
    // Color is currently only used by VOI, but let's save it anyway. We want to support it later on.
    wxXmlNode *pColor = new wxXmlNode( NULL, wxXML_ELEMENT_NODE, wxT( "color" ) );
    pColor->AddAttribute( new wxXmlAttribute( wxT( "colorHTML" ), m_color.GetAsString(wxC2S_HTML_SYNTAX) ) );
    pColor->AddAttribute( new wxXmlAttribute( wxT( "colorAlpha" ), wxStrFormat( m_color.Alpha() ) ) );
    pCurNode->AddChild( pColor );
    
    wxXmlNode *pDistanceColoring = new wxXmlNode( NULL, wxXML_ELEMENT_NODE, wxT( "distance_coloring_state" ) );
    pDistanceColoring->AddAttribute( new wxXmlAttribute( wxT( "used" ), m_DistColoring ? wxT( "yes") : wxT( "no" ) ) );
    pCurNode->AddChild( pDistanceColoring );

    //wxXmlNode *pMeanFiberOptions = new wxXmlNode( NULL, wxXML_ELEMENT_NODE, wxT( "mean_fiber_options" ) );
    //pMeanFiberOptions->AddAttribute( new wxXmlAttribute( wxT( "colorHTML" ), m_meanFiberColor.GetAsString(wxC2S_HTML_SYNTAX) ) );
    //pMeanFiberOptions->AddAttribute( new wxXmlAttribute( wxT( "opacity" ), wxStrFormat( m_meanFiberOpacity, floatPrecision ) ) );
    //pMeanFiberOptions->AddAttribute( new wxXmlAttribute( wxT( "colorationMode" ), Helper::getColorationModeString( m_meanFiberColorationMode ) ) );
    //pCurNode->AddChild( pMeanFiberOptions );

    return true;
}

wxString SelectionObject::getTypeTag() const
{
    return wxT( "base" );
}

void SelectionObject::notifyInBoxNeedsUpdating()
{
    for( map< FiberIdType, SelectionState >::iterator stateIt( m_selectionStates.begin() );
        stateIt != m_selectionStates.end(); ++stateIt )
    {
        // Always update in branch at the same time, since an
        // update to the inBox will always influence the inBranch.
        stateIt->second.m_inBoxNeedsUpdating = true;
    }

    SceneManager::getInstance()->getSelectionTree().notifyStatsNeedUpdating( this );    
}

void SelectionObject::notifyInBranchNeedsUpdating()
{
}

///////////////////////////////////////////////////////////////////////////
// This function will set the correct information in the fiber info grid.
///////////////////////////////////////////////////////////////////////////
void SelectionObject::updateStatsGrid()
{
    if( m_pToggleCalculatesFibersInfo->GetValue() )
    {
        m_pGridFibersInfo->SetCellValue( 0,  0, wxString::Format( wxT( "%d" ),   m_stats.m_count            ) );
        m_pGridFibersInfo->SetCellValue( 1,  0, wxString::Format( wxT( "%.2f" ), m_stats.m_meanValue        ) );
        m_pGridFibersInfo->SetCellValue( 2,  0, wxString::Format( wxT( "%.2f" ), m_stats.m_meanLength       ) );
        m_pGridFibersInfo->SetCellValue( 3,  0, wxString::Format( wxT( "%.2f" ), m_stats.m_minLength        ) );
        m_pGridFibersInfo->SetCellValue( 4,  0, wxString::Format( wxT( "%.2f" ), m_stats.m_maxLength        ) );
		m_pGridFibersInfo->SetCellValue( 5,  0, wxString::Format( wxT( "%.2f" ), m_stats.m_meanCrossSection       ) );
		m_pGridFibersInfo->SetCellValue( 6,  0, wxString::Format( wxT( "%.2f" ), m_stats.m_minCrossSection        ) );
		m_pGridFibersInfo->SetCellValue( 7,  0, wxString::Format( wxT( "%.2f" ), m_stats.m_maxCrossSection        ) );
    }
}

void SelectionObject::setShowConvexHullOption( bool i_val )
{
     m_pLblConvexHullOpacity->Show( i_val );
     m_pSliderConvexHullOpacity->Show( i_val);
     m_pBtnSelectConvexHullColor->Enable( i_val );
}

void SelectionObject::UpdateMeanValueTypeBox()
{
    vector< Anatomy* > dataSets = DatasetManager::getInstance()->getAnatomies();
    
    if( dataSets.size() != m_pCBSelectDataSet->GetCount() )
    {
        int oldIndex = -1;
        wxString oldName = m_pCBSelectDataSet->GetStringSelection();
        
        m_pCBSelectDataSet->Clear();
        
        for( unsigned int i( 0 ); i < dataSets.size(); ++i )
        {
            m_pCBSelectDataSet->Insert( dataSets[i]->getName().BeforeFirst('.'), m_pCBSelectDataSet->GetCount() );
            if( oldName == dataSets[i]->getName().BeforeFirst('.') )
            {
                oldIndex = i;
            }
        }
        
        if( oldIndex < 0 )
        {
            oldIndex = 0;
        }
        else if( static_cast<unsigned int>( oldIndex ) >= m_pCBSelectDataSet->GetCount() )
        {
            oldIndex = m_pCBSelectDataSet->GetCount() - 1;
        }
        
        m_pCBSelectDataSet->SetSelection(oldIndex);
    }
}

void SelectionObject::setShowMeanFiberOption( bool i_val )
{
    m_pLblColoring->Show( i_val );
    m_pRadCustomColoring->Show( i_val );
    m_pRadNormalColoring->Show( i_val );
	m_pLblMethod->Show( i_val );
    m_pRadMeanMean->Show( i_val );
    m_pRadMeanCross->Show( i_val );
    m_pLblMeanFiberOpacity->Show( i_val );
    m_pSliderMeanFiberOpacity->Show( i_val );
	m_pSliderCSthreshold->Show( i_val );
	m_pSliderNoOfCS->Show( i_val );
	m_pLblCrossSectionThreshold->Show( i_val );
	m_pLblNoOfCS->Show( i_val );
	m_pTxtBoxNbOfPlanes->Show( i_val );
	m_pToggleShowStartingPoint->Show(i_val);
	m_pToggleFlipStartingPoint->Show(i_val);
}

void SelectionObject::updateMeanFiberOpacity()
{
    setMeanFiberOpacity( ( m_pSliderMeanFiberOpacity->GetValue() + (float)m_pSliderMeanFiberOpacity->GetMin() ) / (float)m_pSliderMeanFiberOpacity->GetMax() );
}

void SelectionObject::updateCSThreshold()
{
	setCSThreshold( m_pSliderCSthreshold->GetValue() );
	notifyStatsNeedUpdating();
}

void SelectionObject::updateNoOfCS()
{
	float v = m_pSliderNoOfCS->GetValue();
	setNoOfCS( v );
	m_pTxtBoxNbOfPlanes->SetValue(wxString::Format( wxT( "%.1f" ), v ));
	notifyStatsNeedUpdating();
}

void SelectionObject::createPropertiesSizer( PropertiesWindow *pParent )
{
    SceneObject::createPropertiesSizer( pParent );
    float voxelX = DatasetManager::getInstance()->getVoxelX();
    float voxelY = DatasetManager::getInstance()->getVoxelY();
    float voxelZ = DatasetManager::getInstance()->getVoxelZ();

    wxBoxSizer *pBoxMain = new wxBoxSizer( wxVERTICAL );
    if(!m_isMagnet)
    {
        m_meanFiberOpacity = 0.50f;
		m_CSThreshold = 20;
        m_convexHullOpacity = 0.35f;
        m_meanFiberColorationMode = NORMAL_COLOR;
		m_meanFiberMode = MEAN;

        //////////////////////////////////////////////////////////////////////////
#if _USE_ZOOM_GUI
        wxImage bmpDelete(          MyApp::iconsPath + wxT( "delete64.png" ),      wxBITMAP_TYPE_PNG );
        wxImage bmpMeanFiberColor(  MyApp::iconsPath + wxT( "colorSelect64.png" ), wxBITMAP_TYPE_PNG );
        wxImage bmpConvexHullColor( MyApp::iconsPath + wxT( "colorSelect64.png" ), wxBITMAP_TYPE_PNG );
#else
		wxImage bmpDelete(          MyApp::iconsPath + wxT( "delete.png" ),      wxBITMAP_TYPE_PNG );
        wxImage bmpMeanFiberColor(  MyApp::iconsPath + wxT( "colorSelect.png" ), wxBITMAP_TYPE_PNG );
        wxImage bmpConvexHullColor( MyApp::iconsPath + wxT( "colorSelect.png" ), wxBITMAP_TYPE_PNG );
#endif

        wxButton *pBtnChangeName          = new wxButton( pParent, wxID_ANY, wxT( "Rename" ), DEF_POS, wxSize( 20, -1 ) );
        wxButton *pBtnSelectColorFibers   = new wxButton( pParent, wxID_ANY, wxT( "Select Fibers Color" ) );

    #if !_USE_LIGHT_GUI
        wxButton *pBtnNewColorVolume      = new wxButton( pParent, wxID_ANY, wxT( "New Color map" ) );
        wxButton *pBtnNewDensityVolume    = new wxButton( pParent, wxID_ANY, wxT( "New Density map" ) );
        wxButton *pBtnSetAsDistanceAnchor = new wxButton( pParent, wxID_ANY, wxT( "Set As Anchor" ) );
    #endif
    
        m_pbtnDisplayCrossSections      = new wxButton( pParent, wxID_ANY, wxT( "Display Cross Section (C.S.)" ) );
        m_pbtnDisplayDispersionTube     = new wxButton( pParent, wxID_ANY, wxT( "Display Dispersion Tube" ) );
        wxBitmapButton *pBtnDelete      = new wxBitmapButton( pParent, wxID_ANY, bmpDelete, DEF_POS, wxSize( 20, -1 ) );
        m_pBtnSelectMeanFiberColor      = new wxBitmapButton( pParent, wxID_ANY, bmpMeanFiberColor );
         m_pBtnSelectConvexHullColor     = new wxBitmapButton( pParent, wxID_ANY, bmpConvexHullColor );
        m_pToggleVisibility           = new wxToggleButton( pParent, wxID_ANY, wxT( "Visible" ), DEF_POS, wxSize( 20, -1 ) );
        m_pToggleActivate             = new wxToggleButton( pParent, wxID_ANY, wxT( "Activate" ), DEF_POS, wxSize( 20, -1 ) );
        wxToggleButton *pToggleAndNot = new wxToggleButton( pParent, wxID_ANY, wxT( "And / Not" ), DEF_POS, wxSize( 20, -1 ) );
        m_pTogglePruneRemove = new wxToggleButton( pParent, wxID_ANY, wxT( "Prune" ), DEF_POS, wxSize( 20, -1 ) );
        m_pToggleCalculatesFibersInfo = new wxToggleButton( pParent, wxID_ANY, wxT( "Calculate Fibers Stats" ) );
		m_pLblRefAnat          = new wxStaticText( pParent, wxID_ANY, wxT( "Ref. Anat:" ) );
		m_pBtnRefAnat    = new wxButton( pParent, wxID_ANY, wxT( "Choose Anat" ) );


        m_pToggleDisplayMeanFiber     = new wxToggleButton( pParent, wxID_ANY, wxT( "Display Mean Fiber" ) );
		m_pToggleShowStartingPoint     = new wxToggleButton( pParent, wxID_ANY, wxT( "Show starting point" ) );
		m_pToggleFlipStartingPoint     = new wxToggleButton( pParent, wxID_ANY, wxT( "Flip" ), DEF_POS, wxSize( 20, -1 ) );
		m_pSaveTractometry    = new wxButton( pParent, wxID_ANY, wxT( "Export values" ) );
         m_pToggleDisplayConvexHull    = new wxToggleButton( pParent, wxID_ANY, wxT( "Display convex hull" ) );
        m_pLblColoring          = new wxStaticText( pParent, wxID_ANY, wxT( "Coloring" ) );
		m_pLblMethod          = new wxStaticText( pParent, wxID_ANY, wxT( "Method" ) );
        m_pLblMeanFiberOpacity  = new wxStaticText( pParent, wxID_ANY, wxT( "Opacity" ) );
		m_pLblCrossSectionThreshold  = new wxStaticText( pParent, wxID_ANY, wxT( "C.S. threshold" ) );
		m_pLblNoOfCS  = new wxStaticText( pParent, wxID_ANY, wxT( "No. of planes" ) );
		m_pTxtBoxNbOfPlanes  = new wxTextCtrl( pParent, wxID_ANY, wxString::Format( wxT( "%d" ), m_noOfMeanFiberPts ), DEF_POS, wxSize( 10, -1 ) );
        m_pLblConvexHullOpacity = new wxStaticText( pParent, wxID_ANY, wxT( "Opacity" ) );
        m_pRadCustomColoring = new wxRadioButton( pParent, wxID_ANY, _T( "Custom" ), DEF_POS, DEF_SIZE, wxRB_GROUP );
        m_pRadNormalColoring = new wxRadioButton( pParent, wxID_ANY, _T( "Normal" ) );
		m_pRadMeanMean = new wxRadioButton( pParent, wxID_ANY, _T( "Mean" ), DEF_POS, DEF_SIZE, wxRB_GROUP );
        m_pRadMeanCross = new wxRadioButton( pParent, wxID_ANY, _T( "Cross-sections" ) );
		//m_pRadMeanCross->SetValue(true);
        m_pSliderMeanFiberOpacity  = new wxSlider( pParent, wxID_ANY, 35, 0, 100, DEF_POS, wxSize( 40, -1 ), wxSL_HORIZONTAL | wxSL_AUTOTICKS );
		m_pSliderCSthreshold  = new wxSlider( pParent, wxID_ANY, 20, 1, 100, DEF_POS, wxSize( 40, -1 ), wxSL_HORIZONTAL | wxSL_AUTOTICKS );
		m_pSliderNoOfCS  = new wxSlider( pParent, wxID_ANY, 20, 10, 30, DEF_POS, wxSize( 60, -1 ), wxSL_HORIZONTAL | wxSL_AUTOTICKS );
        m_pSliderConvexHullOpacity = new wxSlider( pParent, wxID_ANY, 35, 0, 100, DEF_POS, wxSize( 40, -1 ), wxSL_HORIZONTAL | wxSL_AUTOTICKS );
        m_pTxtName  = new wxTextCtrl( pParent, wxID_ANY, getName(), DEF_POS, DEF_SIZE, wxTE_CENTRE | wxTE_READONLY );
        m_pTxtBoxX  = new wxTextCtrl( pParent, wxID_ANY, wxString::Format( wxT( "%.2f" ), m_center.x ), DEF_POS, wxSize( 10, -1 ) );
        m_pTxtBoxY  = new wxTextCtrl( pParent, wxID_ANY, wxString::Format( wxT( "%.2f" ), m_center.y ), DEF_POS, wxSize( 10, -1 ) );
        m_pTxtBoxZ  = new wxTextCtrl( pParent, wxID_ANY, wxString::Format( wxT( "%.2f" ), m_center.z ), DEF_POS, wxSize( 10, -1 ) );
        m_pTxtSizeX = new wxTextCtrl( pParent, wxID_ANY, wxString::Format( wxT( "%.2f" ), m_size.x * voxelX ), DEF_POS, wxSize( 10, -1 ) );
        m_pTxtSizeY = new wxTextCtrl( pParent, wxID_ANY, wxString::Format( wxT( "%.2f" ), m_size.y * voxelY ), DEF_POS, wxSize( 10, -1 ) );
        m_pTxtSizeZ = new wxTextCtrl( pParent, wxID_ANY, wxString::Format( wxT( "%.2f" ), m_size.z * voxelZ ), DEF_POS, wxSize( 10, -1 ) );

        //////////////////////////////////////////////////////////////////////////

        wxFont font = m_pTxtName->GetFont();
        font.SetPointSize(10);
        font.SetWeight( wxFONTWEIGHT_BOLD );
        m_pTxtName->SetFont( font );
        m_pTxtName->SetBackgroundColour( *wxLIGHT_GREY );

        //////////////////////////////////////////////////////////////////////////

        pBoxMain->Add( m_pTxtName, 0, wxALIGN_CENTER | wxEXPAND | wxALL, 1 );

        wxBoxSizer *pBoxSizer = new wxBoxSizer( wxHORIZONTAL );
        pBoxSizer->Add( m_pToggleVisibility, 1, wxEXPAND | wxALL, 1 );
        pBoxSizer->Add( m_pToggleActivate,   1, wxEXPAND | wxALL, 1 );
        pBoxSizer->Add( pBtnDelete,          1, wxEXPAND | wxALL, 1 );
        pBoxMain->Add( pBoxSizer, 0, wxFIXED_MINSIZE | wxEXPAND, 0 );

        //////////////////////////////////////////////////////////////////////////

        wxBoxSizer *pBoxSizer1 = new wxBoxSizer( wxHORIZONTAL );
        pBoxSizer1->Add( pToggleAndNot,  1, wxEXPAND | wxALL, 1 );
        pBoxSizer1->Add( m_pTogglePruneRemove, 1, wxEXPAND | wxALL, 1 );
        pBoxSizer1->Add( pBtnChangeName, 1, wxEXPAND | wxALL, 1 );
        pBoxMain->Add( pBoxSizer1, 0, wxFIXED_MINSIZE | wxEXPAND, 0 );

        //////////////////////////////////////////////////////////////////////////

        pBoxMain->Add( pBtnSelectColorFibers,   0, wxEXPAND | wxLEFT | wxRIGHT, 24 );
    
    #if !_USE_LIGHT_GUI
        pBoxMain->Add( pBtnNewColorVolume,      0, wxEXPAND | wxLEFT | wxRIGHT, 24 );
        pBoxMain->Add( pBtnNewDensityVolume,    0, wxEXPAND | wxLEFT | wxRIGHT, 24 );
        pBoxMain->Add( pBtnSetAsDistanceAnchor, 0, wxEXPAND | wxLEFT | wxRIGHT, 24 );
    #endif

        pBoxMain->AddSpacer( 8 );

        pBoxMain->Add( m_pToggleCalculatesFibersInfo, 0, wxEXPAND | wxLEFT | wxRIGHT, 24 );

		#ifdef __WXMSW__ 
		wxBoxSizer *pBoxSizerRef = new wxBoxSizer( wxHORIZONTAL );
        pBoxSizerRef->Add( m_pLblRefAnat,    0, wxEXPAND, 0 );
        pBoxSizerRef->Add( m_pBtnRefAnat, 1, wxEXPAND, 0 );
        pBoxMain->Add( pBoxSizerRef, 0, wxEXPAND | wxALL, 1 );
		#endif

        pBoxMain->AddSpacer( 2 );

        //////////////////////////////////////////////////////////////////////////

		m_pGridFibersInfo = new wxGrid( pParent, wxID_ANY);
        m_pGridFibersInfo->SetRowLabelAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
        font = m_pGridFibersInfo->GetFont();
        font.SetPointSize( 8 );
        font.SetWeight( wxFONTWEIGHT_BOLD );
        m_pGridFibersInfo->SetFont( font );
        m_pGridFibersInfo->SetColLabelSize( 2 );
        m_pGridFibersInfo->CreateGrid( 8, 1, wxGrid::wxGridSelectCells );
        m_pGridFibersInfo->SetColLabelValue( 0, wxT( "" ) );
        m_pGridFibersInfo->SetRowLabelValue( 0, wxT( "Count" ) );
        m_pGridFibersInfo->SetRowLabelValue( 1, wxT( "Mean Value" ) );
        m_pGridFibersInfo->SetRowLabelValue( 2, wxT( "Mean Length (mm)" ) );
        m_pGridFibersInfo->SetRowLabelValue( 3, wxT( "Min Length (mm)" ) );
        m_pGridFibersInfo->SetRowLabelValue( 4, wxT( "Max Length (mm)" ) );
        m_pGridFibersInfo->SetRowLabelValue( 5, wxT( "Mean C. S. (mm sq.)" ) );
        m_pGridFibersInfo->SetRowLabelValue( 6, wxT( "Min C. S. (mm sq.)" ) );
        m_pGridFibersInfo->SetRowLabelValue( 7, wxT( "Max C. S. (mm sq.)" ) );
        //m_pGridFibersInfo->SetRowLabelValue( 10, wxT( "Dispersion" ) );

#if !_USE_ZOOM_GUI
		int lab = 120;
#else
		int lab = 300;
#endif
        m_pGridFibersInfo->SetRowLabelSize( lab );
		m_pGridFibersInfo->SetColSize(0, lab*0.7);



        pBoxMain->Add( m_pGridFibersInfo, 0, wxALIGN_CENTER | wxALL, 0 );

        pBoxMain->AddSpacer( 2 );

        //////////////////////////////////////////////////////////////////////////

    // Because of a bug on the Windows version of this, we currently do not use this wxChoice on Windows.
    // Will have to be fixed.
    #ifndef __WXMSW__    
        m_pCBSelectDataSet = new wxChoice( pParent, wxID_ANY, wxDefaultPosition, wxSize( 140, -1 ) );
        m_pLabelAnatomy = new wxStaticText( pParent, wxID_ANY, wxT("Anatomy file : ") );
        pBoxSizer = new wxBoxSizer( wxHORIZONTAL );
        pBoxSizer->Add( m_pLabelAnatomy, 0, wxEXPAND, 0 );
        pBoxSizer->Add( m_pCBSelectDataSet, 0, wxEXPAND, 0 );

        pBoxMain->Add( pBoxSizer, 0, wxEXPAND, 0 );
        pParent->Connect( m_pCBSelectDataSet->GetId(), wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PropertiesWindow::OnMeanComboBoxSelectionChange ) );
        pBoxMain->AddSpacer( 2 );
    #endif

        //////////////////////////////////////////////////////////////////////////

        wxBoxSizer *pBoxSizer2 = new wxBoxSizer( wxHORIZONTAL );
        pBoxSizer2->Add( m_pToggleDisplayMeanFiber,  3, wxEXPAND, 0 );
        pBoxSizer2->Add( m_pBtnSelectMeanFiberColor, 1, wxEXPAND, 0 );
        pBoxMain->Add( pBoxSizer2, 0, wxEXPAND, 0 );

        pBoxMain->AddSpacer( 8 );

        //////////////////////////////////////////////////////////////////////////

        wxBoxSizer *pBoxColoring = new wxBoxSizer( wxVERTICAL );
        pBoxColoring->Add( m_pLblColoring, 0, wxALIGN_LEFT | wxALL, 1 );

        wxBoxSizer *pBoxColoringRadios = new wxBoxSizer( wxVERTICAL );
        pBoxColoringRadios->Add( m_pRadCustomColoring, 0, wxALIGN_LEFT | wxALL, 1 );
        pBoxColoringRadios->Add( m_pRadNormalColoring, 0, wxALIGN_LEFT | wxALL, 1 );
        pBoxColoring->Add( pBoxColoringRadios, 0, wxALIGN_LEFT | wxLEFT, 32 );

        pBoxMain->Add( pBoxColoring, 0, wxFIXED_MINSIZE | wxEXPAND | wxTOP | wxBOTTOM, 8 );

		wxBoxSizer *pBoxMethod = new wxBoxSizer( wxVERTICAL );
        pBoxMethod->Add( m_pLblMethod, 0, wxALIGN_LEFT | wxALL, 1 );

        wxBoxSizer *pBoxMethodRadios = new wxBoxSizer( wxVERTICAL );
        pBoxMethodRadios->Add( m_pRadMeanMean, 0, wxALIGN_LEFT | wxALL, 1 );
        pBoxMethodRadios->Add( m_pRadMeanCross, 0, wxALIGN_LEFT | wxALL, 1 );
        pBoxMethod->Add( pBoxMethodRadios, 0, wxALIGN_LEFT | wxLEFT, 32 );

        pBoxMain->Add( pBoxMethod, 0, wxFIXED_MINSIZE | wxEXPAND | wxTOP | wxBOTTOM, 8 );

        //////////////////////////////////////////////////////////////////////////

        wxBoxSizer *pBoxSizer3 = new wxBoxSizer( wxHORIZONTAL );
        pBoxSizer3->Add( m_pLblMeanFiberOpacity,    0, wxEXPAND, 0 );
        pBoxSizer3->Add( m_pSliderMeanFiberOpacity, 1, wxEXPAND, 0 );
        pBoxMain->Add( pBoxSizer3, 0, wxEXPAND | wxALL, 1 );

		wxBoxSizer *pBoxSizer4 = new wxBoxSizer( wxHORIZONTAL );
		pBoxSizer4->Add( m_pLblCrossSectionThreshold,    0, wxEXPAND, 0 );
        pBoxSizer4->Add( m_pSliderCSthreshold, 1, wxEXPAND, 0 );
        pBoxMain->Add( pBoxSizer4, 0, wxEXPAND | wxALL, 1 );

		wxBoxSizer *pBoxSizer5 = new wxBoxSizer( wxHORIZONTAL );
		pBoxSizer5->Add( m_pLblNoOfCS,    0, wxEXPAND, 0 );
        pBoxSizer5->Add( m_pSliderNoOfCS, 1, wxEXPAND, 0 );
		pBoxSizer5->Add( m_pTxtBoxNbOfPlanes, 1, wxEXPAND, 0 );
        pBoxMain->Add( pBoxSizer5, 0, wxEXPAND | wxALL, 1 );

		wxBoxSizer *pBoxSizerflip = new wxBoxSizer( wxHORIZONTAL );
        pBoxSizerflip->Add( m_pToggleShowStartingPoint,  0, wxEXPAND, 0 );
        pBoxSizerflip->Add( m_pToggleFlipStartingPoint, 1, wxEXPAND, 0 );
        pBoxMain->Add( pBoxSizerflip, 0, wxEXPAND | wxALL, 1);

        //////////////////////////////////////////////////////////////////////////
		wxBoxSizer *pBoxSizercs = new wxBoxSizer( wxVERTICAL );
        pBoxSizercs->Add( m_pbtnDisplayCrossSections,  0,wxEXPAND );
        pBoxSizercs->Add( m_pbtnDisplayDispersionTube, 0,wxEXPAND );
		pBoxSizercs->Add( m_pSaveTractometry,0,wxEXPAND );
		pBoxMain->Add( pBoxSizercs, 0, wxEXPAND | wxALL, 1 );

        //////////////////////////////////////////////////////////////////////////

         wxBoxSizer *pBoxSizer6 = new wxBoxSizer( wxHORIZONTAL );
         pBoxSizer6->Add( m_pToggleDisplayConvexHull,  3, wxALIGN_CENTER | wxEXPAND | wxALL, 1 );
         pBoxSizer6->Add( m_pBtnSelectConvexHullColor, 1, wxALIGN_CENTER | wxEXPAND | wxALL, 1 );
         pBoxMain->Add( pBoxSizer6, 0, wxEXPAND | wxALL, 1 );

        //////////////////////////////////////////////////////////////////////////

         wxBoxSizer *pBoxSizer7 = new wxBoxSizer( wxHORIZONTAL );
         pBoxSizer7->Add( m_pLblConvexHullOpacity, 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxALL, 1 );
         pBoxSizer7->Add( m_pSliderConvexHullOpacity, 1, wxALIGN_LEFT | wxEXPAND | wxALL, 1 );
         pBoxMain->Add( pBoxSizer7, 0, wxEXPAND | wxALL, 1 );

        //////////////////////////////////////////////////////////////////////////

        pBoxMain->Add( new wxStaticText( pParent, wxID_ANY, wxT( "Position" ) ), 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 1 );
        pBoxSizer = new wxBoxSizer( wxHORIZONTAL );
        pBoxSizer->Add( new wxStaticText( pParent, wxID_ANY, wxT( "x:" ) ), 0, wxALL, 1 );
        pBoxSizer->Add( m_pTxtBoxX, 1, wxEXPAND | wxALL, 1 );
        pBoxSizer->Add( new wxStaticText( pParent, wxID_ANY, wxT( "y:" ) ), 0, wxALL, 1 );
        pBoxSizer->Add( m_pTxtBoxY, 1, wxEXPAND | wxALL, 1 );
        pBoxSizer->Add( new wxStaticText( pParent, wxID_ANY, wxT( "z:" ) ), 0, wxALL, 1 );
        pBoxSizer->Add( m_pTxtBoxZ, 1, wxEXPAND | wxALL, 1 );
        pBoxMain->Add( pBoxSizer, 0, wxEXPAND, 0 );

        pBoxMain->AddSpacer( 8 );

        //////////////////////////////////////////////////////////////////////////

        pBoxMain->Add( new wxStaticText( pParent, wxID_ANY, wxT( "Size" ) ), 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 1 );
        pBoxSizer = new wxBoxSizer( wxHORIZONTAL );
        pBoxSizer->Add( new wxStaticText( pParent, wxID_ANY, wxT( "x:" ) ), 0, wxALL, 1 );
        pBoxSizer->Add( m_pTxtSizeX, 1, wxEXPAND | wxALL, 1 );
        pBoxSizer->Add( new wxStaticText( pParent, wxID_ANY, wxT( "y:" ) ), 0, wxALL, 1 );
        pBoxSizer->Add( m_pTxtSizeY, 1, wxEXPAND | wxALL, 1 );
        pBoxSizer->Add( new wxStaticText( pParent, wxID_ANY, wxT( "z:" ) ), 0, wxALL, 1 );
        pBoxSizer->Add( m_pTxtSizeZ, 1, wxEXPAND | wxALL, 1 );
        pBoxMain->Add( pBoxSizer, 0, wxEXPAND, 0 );

        //////////////////////////////////////////////////////////////////////////

        m_pRadNormalColoring->SetValue( true );

    #if !_USE_LIGHT_GUI
        bool isFirstLevel = SceneManager::getInstance()->getSelectionTree().isFirstLevel( this );
        pBtnNewColorVolume->Enable( isFirstLevel );
        pBtnNewDensityVolume->Enable( isFirstLevel );
        // TODO selection remove object type
        pBtnSetAsDistanceAnchor->Enable( m_objectType == VOI_TYPE );
    #endif
    
        // TODO selection
        //pToggleAndNot->Enable( !getIsFirstLevel() );

        

        //////////////////////////////////////////////////////////////////////////
        pParent->Connect( pBtnChangeName->GetId(),          wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PropertiesWindow::OnRenameBox ) );
        pParent->Connect( pBtnSelectColorFibers->GetId(),   wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PropertiesWindow::OnAssignColor ) );
    
    #if !_USE_LIGHT_GUI
        pParent->Connect( pBtnNewColorVolume->GetId(),      wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PropertiesWindow::OnCreateFibersColorTexture ) );
        pParent->Connect( pBtnNewDensityVolume->GetId(),    wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PropertiesWindow::OnCreateFibersDensityTexture ) );
        pParent->Connect( pBtnSetAsDistanceAnchor->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PropertiesWindow::OnDistanceAnchorSet ) );
    #endif
    
        pParent->Connect( pBtnDelete->GetId(),              wxEVT_COMMAND_BUTTON_CLICKED, wxTreeEventHandler(    PropertiesWindow::OnDeleteTreeItem ) );
        pParent->Connect( m_pBtnSelectMeanFiberColor->GetId(),  wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PropertiesWindow::OnMeanFiberColorChange ) );
        pParent->Connect( m_pBtnSelectConvexHullColor->GetId(), wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PropertiesWindow::OnConvexHullColorChange ) );
        pParent->Connect( m_pbtnDisplayCrossSections->GetId(),wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(PropertiesWindow::OnDisplayCrossSections ) );
        pParent->Connect( m_pbtnDisplayDispersionTube->GetId(),wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(PropertiesWindow::OnDisplayDispersionTube ) );
		pParent->Connect( m_pSaveTractometry->GetId(),    wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PropertiesWindow::onSaveTractometry ) );
        pParent->Connect( m_pToggleVisibility->GetId(), wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( PropertiesWindow::OnToggleShowSelectionObject ) );
        pParent->Connect( m_pToggleActivate->GetId(),   wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxTreeEventHandler(    PropertiesWindow::OnActivateTreeItem ) );
        pParent->Connect( pToggleAndNot->GetId(),       wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( PropertiesWindow::OnToggleAndNot ) );
        pParent->Connect( m_pTogglePruneRemove->GetId(),       wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( PropertiesWindow::OnTogglePruneRemove ) );
        pParent->Connect( m_pToggleCalculatesFibersInfo->GetId(), wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( PropertiesWindow::OnDisplayFibersInfo ) );
		#ifdef __WXMSW__ 
		pParent->Connect( m_pBtnRefAnat->GetId(),    wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PropertiesWindow::OnSelectRefAnat ) );
		#endif
        pParent->Connect( m_pToggleDisplayMeanFiber->GetId(),     wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( PropertiesWindow::OnDisplayMeanFiber ) );
		pParent->Connect( m_pToggleShowStartingPoint->GetId(),     wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( PropertiesWindow::OnShowStartingPoint ) );
		pParent->Connect( m_pToggleFlipStartingPoint->GetId(),     wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( PropertiesWindow::OnFlipStartingPoint ) );
        pParent->Connect( m_pToggleDisplayConvexHull->GetId(),    wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( PropertiesWindow::OnDisplayConvexHull ) );
        pParent->Connect( m_pRadCustomColoring->GetId(), wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PropertiesWindow::OnCustomMeanFiberColoring ) );
        pParent->Connect( m_pRadNormalColoring->GetId(), wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PropertiesWindow::OnNormalMeanFiberColoring ) );
		pParent->Connect( m_pRadMeanMean->GetId(), wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PropertiesWindow::OnMeanMean ) );
        pParent->Connect( m_pRadMeanCross->GetId(), wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PropertiesWindow::OnMeanCross ) );
        pParent->Connect( m_pSliderMeanFiberOpacity->GetId(),  wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler( PropertiesWindow::OnMeanFiberOpacityChange ) );
		pParent->Connect( m_pSliderCSthreshold->GetId(),  wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler( PropertiesWindow::OnCSThresholdChange ) );
		pParent->Connect( m_pSliderNoOfCS->GetId(),  wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler( PropertiesWindow::OnNoofCSchange ) );
        pParent->Connect( m_pSliderConvexHullOpacity->GetId(), wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler( PropertiesWindow::OnConvexHullOpacityChange ) );
        pParent->Connect( m_pTxtBoxX->GetId(),  wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PropertiesWindow::OnBoxPositionX ) );
        pParent->Connect( m_pTxtBoxY->GetId(),  wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PropertiesWindow::OnBoxPositionY ) );
        pParent->Connect( m_pTxtBoxZ->GetId(),  wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PropertiesWindow::OnBoxPositionZ ) );
        pParent->Connect( m_pTxtSizeX->GetId(), wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PropertiesWindow::OnBoxSizeX ) );
        pParent->Connect( m_pTxtSizeY->GetId(), wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PropertiesWindow::OnBoxSizeY ) );
        pParent->Connect( m_pTxtSizeZ->GetId(), wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PropertiesWindow::OnBoxSizeZ ) );                                
    }
    else
    {

        //////////////////////////////////////////////////////////////////////////

        wxImage bmpDelete(          MyApp::iconsPath + wxT( "delete64.png" ),      wxBITMAP_TYPE_PNG );
        wxButton *pBtnChangeName          = new wxButton( pParent, wxID_ANY, wxT( "Rename" ), DEF_POS, wxSize( 20, -1 ) );
        wxBitmapButton *pBtnDelete      = new wxBitmapButton( pParent, wxID_ANY, bmpDelete, DEF_POS, wxSize( 20, -1 ) );
        m_pToggleVisibility           = new wxToggleButton( pParent, wxID_ANY, wxT( "Visible" ), DEF_POS, wxSize( 20, -1 ) );

        m_pTxtName  = new wxTextCtrl( pParent, wxID_ANY, getName(), DEF_POS, DEF_SIZE, wxTE_CENTRE | wxTE_READONLY );
        m_pTxtBoxX  = new wxTextCtrl( pParent, wxID_ANY, wxString::Format( wxT( "%.2f" ), m_center.x ), DEF_POS, wxSize( 10, -1 ) );
        m_pTxtBoxY  = new wxTextCtrl( pParent, wxID_ANY, wxString::Format( wxT( "%.2f" ), m_center.y ), DEF_POS, wxSize( 10, -1 ) );
        m_pTxtBoxZ  = new wxTextCtrl( pParent, wxID_ANY, wxString::Format( wxT( "%.2f" ), m_center.z ), DEF_POS, wxSize( 10, -1 ) );
        m_pTxtSizeX = new wxTextCtrl( pParent, wxID_ANY, wxString::Format( wxT( "%.2f" ), m_size.x * voxelX ), DEF_POS, wxSize( 10, -1 ) );
        m_pTxtSizeY = new wxTextCtrl( pParent, wxID_ANY, wxString::Format( wxT( "%.2f" ), m_size.y * voxelY ), DEF_POS, wxSize( 10, -1 ) );
        m_pTxtSizeZ = new wxTextCtrl( pParent, wxID_ANY, wxString::Format( wxT( "%.2f" ), m_size.z * voxelZ ), DEF_POS, wxSize( 10, -1 ) ); 

        //////////////////////////////////////////////////////////////////////////

        wxFont font = m_pTxtName->GetFont();
        font.SetPointSize(10);
        font.SetWeight( wxFONTWEIGHT_BOLD );
        m_pTxtName->SetFont( font );
        m_pTxtName->SetBackgroundColour( *wxLIGHT_GREY );

        //////////////////////////////////////////////////////////////////////////

        pBoxMain->Add( m_pTxtName, 0, wxALIGN_CENTER | wxEXPAND | wxALL, 1 );

        wxBoxSizer *pBoxSizer = new wxBoxSizer( wxHORIZONTAL );
        pBoxSizer->Add( pBtnChangeName, 1, wxEXPAND | wxALL, 1 );
        pBoxSizer->Add( m_pToggleVisibility, 1, wxEXPAND | wxALL, 1 );
        pBoxSizer->Add( pBtnDelete,          1, wxEXPAND | wxALL, 1 );
        pBoxMain->Add( pBoxSizer, 0, wxFIXED_MINSIZE | wxEXPAND, 0 );

        pBoxMain->Add( new wxStaticText( pParent, wxID_ANY, wxT( "Position" ) ), 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 1 );
        pBoxSizer = new wxBoxSizer( wxHORIZONTAL );
        pBoxSizer->Add( new wxStaticText( pParent, wxID_ANY, wxT( "x:" ) ), 0, wxALL, 1 );
        pBoxSizer->Add( m_pTxtBoxX, 1, wxEXPAND | wxALL, 1 );
        pBoxSizer->Add( new wxStaticText( pParent, wxID_ANY, wxT( "y:" ) ), 0, wxALL, 1 );
        pBoxSizer->Add( m_pTxtBoxY, 1, wxEXPAND | wxALL, 1 );
        pBoxSizer->Add( new wxStaticText( pParent, wxID_ANY, wxT( "z:" ) ), 0, wxALL, 1 );
        pBoxSizer->Add( m_pTxtBoxZ, 1, wxEXPAND | wxALL, 1 );
        pBoxMain->Add( pBoxSizer, 0, wxEXPAND, 0 );

        pBoxMain->AddSpacer( 8 );

        //////////////////////////////////////////////////////////////////////////

        pBoxMain->Add( new wxStaticText( pParent, wxID_ANY, wxT( "Size" ) ), 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 1 );
        pBoxSizer = new wxBoxSizer( wxHORIZONTAL );
        pBoxSizer->Add( new wxStaticText( pParent, wxID_ANY, wxT( "x:" ) ), 0, wxALL, 1 );
        pBoxSizer->Add( m_pTxtSizeX, 1, wxEXPAND | wxALL, 1 );
        pBoxSizer->Add( new wxStaticText( pParent, wxID_ANY, wxT( "y:" ) ), 0, wxALL, 1 );
        pBoxSizer->Add( m_pTxtSizeY, 1, wxEXPAND | wxALL, 1 );
        pBoxSizer->Add( new wxStaticText( pParent, wxID_ANY, wxT( "z:" ) ), 0, wxALL, 1 );
        pBoxSizer->Add( m_pTxtSizeZ, 1, wxEXPAND | wxALL, 1 );
        pBoxMain->Add( pBoxSizer, 0, wxEXPAND, 0 );
        
        pParent->Connect( pBtnChangeName->GetId(),          wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PropertiesWindow::OnRenameBox ) );
        pParent->Connect( pBtnDelete->GetId(),              wxEVT_COMMAND_BUTTON_CLICKED, wxTreeEventHandler(    PropertiesWindow::OnDeleteTreeItem ) );
        pParent->Connect( m_pToggleVisibility->GetId(), wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( PropertiesWindow::OnToggleShowSelectionObject ) );
        pParent->Connect( m_pTxtBoxX->GetId(),  wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PropertiesWindow::OnBoxPositionX ) );
        pParent->Connect( m_pTxtBoxY->GetId(),  wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PropertiesWindow::OnBoxPositionY ) );
        pParent->Connect( m_pTxtBoxZ->GetId(),  wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PropertiesWindow::OnBoxPositionZ ) );
        pParent->Connect( m_pTxtSizeX->GetId(), wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PropertiesWindow::OnBoxSizeX ) );
        pParent->Connect( m_pTxtSizeY->GetId(), wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PropertiesWindow::OnBoxSizeY ) );
        pParent->Connect( m_pTxtSizeZ->GetId(), wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PropertiesWindow::OnBoxSizeZ ) ); 

        m_pSliderQ = new MySlider( pParent, wxID_ANY, 25, 0, 25, wxPoint(60,120), wxSize(100, -1), wxSL_HORIZONTAL | wxSL_AUTOTICKS );
	    wxBoxSizer *pBoxQ = new wxBoxSizer( wxHORIZONTAL );
        pBoxQ->Add( new wxStaticText( pParent, wxID_ANY, wxT("Strength (Q)"), wxPoint(0,120), wxSize(70, -1), wxALIGN_CENTER ), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxALL, 1 );
        pBoxQ->Add( m_pSliderQ,   0, wxALIGN_LEFT | wxEXPAND | wxALL, 1);
        m_pBoxQ = new wxTextCtrl( pParent, wxID_ANY, wxT("0.25"), wxPoint(190,120), wxSize(55, -1), wxTE_CENTRE | wxTE_READONLY );
	    pBoxQ->Add( m_pBoxQ,   0, wxALIGN_LEFT | wxALL, 1);
	    pBoxMain->Add( pBoxQ, 0, wxFIXED_MINSIZE | wxEXPAND, 0 );
        pParent->Connect( m_pSliderQ->GetId(), wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler(PropertiesWindow::OnSliderQMoved) );

        m_pToggleFieldDirection           = new wxToggleButton( pParent, wxID_ANY, wxT( "Flip magnetisation" ));
        pBoxMain->Add(m_pToggleFieldDirection, 0, wxFIXED_MINSIZE | wxEXPAND, 0 );
        pParent->Connect( m_pToggleFieldDirection->GetId(), wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( PropertiesWindow::OnToggleFlipMagnetisation ) );
    }

    m_pPropertiesSizer->Add( pBoxMain, 1, wxFIXED_MINSIZE | wxEXPAND, 0 );
}

//////////////////////////////////////////////////////////////////////////

void SelectionObject::updatePropertiesSizer()
{
    SceneObject::updatePropertiesSizer();

    m_pToggleVisibility->SetValue( getIsVisible() );
    m_pTxtName->SetValue( getName() );

    if(!m_isMagnet)
    {
        bool fibersLoaded( DatasetManager::getInstance()->getFibersCount() > 0 );

        m_pToggleCalculatesFibersInfo->Enable( fibersLoaded );
        m_pGridFibersInfo->Enable( fibersLoaded && m_pToggleCalculatesFibersInfo->GetValue() );
    
        m_pToggleDisplayMeanFiber->Enable( fibersLoaded );
        m_pBtnSelectMeanFiberColor->Enable( m_pToggleDisplayMeanFiber->GetValue() );
        setShowMeanFiberOption( m_pToggleDisplayMeanFiber->GetValue() );
    
        m_statsAreBeingComputed = m_pToggleCalculatesFibersInfo->GetValue();
        m_meanFiberIsBeingDisplayed = m_pToggleDisplayMeanFiber->GetValue();
    
        if( m_statsAreBeingComputed || m_meanFiberIsBeingDisplayed )
        {
            updateStats();
        }
    
        // TODO selection convex hull
         m_pToggleDisplayConvexHull->Enable( fibersLoaded );
         setShowConvexHullOption( m_pToggleDisplayConvexHull->GetValue() );

    // Because of a bug on the Windows version of this, we currently do not use this wxChoice on Windows.
    // Will have to be fixed.
    #ifndef __WXMSW__
        m_pCBSelectDataSet->Show( m_pToggleCalculatesFibersInfo->GetValue() );
        m_pLabelAnatomy->Show( m_pToggleCalculatesFibersInfo->GetValue() );
        if( m_pToggleCalculatesFibersInfo->GetValue() )
        {
            UpdateMeanValueTypeBox();
        }
		m_pBtnRefAnat->Show(false);
		m_pLblRefAnat->Show(false);
    #endif		


        m_pbtnDisplayDispersionTube->Enable(m_pToggleCalculatesFibersInfo->GetValue());
        m_pbtnDisplayCrossSections->Enable(m_pToggleCalculatesFibersInfo->GetValue());
		m_pSaveTractometry->Enable(m_pToggleCalculatesFibersInfo->GetValue());
		
    }

    if( m_boxMoved )
    {
        m_pTxtBoxX->ChangeValue(wxString::Format( wxT( "%.2f" ), m_center.x));
        m_pTxtBoxY->ChangeValue(wxString::Format( wxT( "%.2f" ), m_center.y));
        m_pTxtBoxZ->ChangeValue(wxString::Format( wxT( "%.2f" ), m_center.z));
        m_boxMoved = false;
    }

    if( m_boxResized )
    {
        float voxelX = DatasetManager::getInstance()->getVoxelX();
        float voxelY = DatasetManager::getInstance()->getVoxelY();
        float voxelZ = DatasetManager::getInstance()->getVoxelZ();

        m_pTxtSizeX->ChangeValue( wxString::Format( wxT( "%.2f"), m_size.x * voxelX ) );
        m_pTxtSizeY->ChangeValue( wxString::Format( wxT( "%.2f"), m_size.y * voxelY ) );
        m_pTxtSizeZ->ChangeValue( wxString::Format( wxT( "%.2f"), m_size.z * voxelZ ) );
        m_boxResized = false;
    }
}
