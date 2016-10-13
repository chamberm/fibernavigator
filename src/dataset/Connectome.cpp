/////////////////////////////////////////////////////////////////////////////
// Name:            Connectome.cpp
// Author:          Maxime Chamberland
/////////////////////////////////////////////////////////////////////////////

#include "Connectome.h"
#include "../gui/MainFrame.h"

#include "DatasetManager.h"
#include "AnatomyHelper.h"
#include "ConnectomeHelper.h"
#include "../Logger.h"
#include "../gfx/ShaderHelper.h"
#include "../gfx/TheScene.h"
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
#include <math.h>

#include "../main.h"

//////////////////////////////////////////
//Constructor
//////////////////////////////////////////
Connectome::Connectome():
m_labels(NULL),
m_NbLabels(161),
m_nodeAlpha(0.5f),
m_nodeSize(2),
m_edgeSize(2.0f),
m_edgeAlpha(1.0f),
m_isFlashyEdges(false),
m_isOrientationDep(false),
m_Edgemax(0),
m_Edgemin(std::numeric_limits<int>::max()),
m_NodeDegreeMax(1)
{
	std::cout<<"Connectome constructor"<<std::endl;

    m_columns = DatasetManager::getInstance()->getColumns();
    m_rows = DatasetManager::getInstance()->getRows();
	m_frames =  DatasetManager::getInstance()->getFrames();

    m_voxelSizeX = DatasetManager::getInstance()->getVoxelX();
	m_voxelSizeY = DatasetManager::getInstance()->getVoxelY();
	m_voxelSizeZ =  DatasetManager::getInstance()->getVoxelZ();

	m_datasetSize = m_rows * m_columns * m_frames;
}

//////////////////////////////////////////
//Destructor
//////////////////////////////////////////
Connectome::~Connectome()
{
    Logger::getInstance()->print( wxT( "Connectome destructor called but nothing to do." ), LOGLEVEL_DEBUG );
}

void Connectome::clearConnectome()
{
    Edges.clear();
    nodeDegree.clear();
    m_labelHist.clear();
    Nodes.clear();
    ConnectomeHelper::getInstance()->setEdgesReady(false);
    ConnectomeHelper::getInstance()->setLabelsReady(false);
      
}
void Connectome::setLabels(Anatomy *labels)
{

    std::cout << "Nb labels: " << m_NbLabels << std::endl;
    m_labels = labels;
    m_labelHist.resize( m_NbLabels );

    //Read labels, prepare for barycenter
    if( m_labels != NULL )
	{
        for( int f = 0; f < m_frames; ++f )
		{
			for( int r = 0; r < m_rows; ++r )
			{
				for( int c = 0; c < m_columns; ++c )
				{
                    int idx = f * m_rows * m_columns + r * m_columns + c;
                    float value = m_labels->atNonNorm(idx);
					if ( value > 0.0f)
					{
						m_labelHist[value-1].push_back( Vector(c, r, f) );
					}
				}
			}
		}
	}

    Nodes.resize( m_NbLabels );
    //Compute barycenter to display node
    for(unsigned int i=0; i<m_labelHist.size(); i++)
    {
        if(!m_labelHist[i].empty())
        {
            int sumX = 0;
            int sumY = 0;
            int sumZ = 0;

            for(unsigned int j=0; j<m_labelHist[i].size(); j++)
            {
                sumX += m_labelHist[i][j].x;
                sumY += m_labelHist[i][j].y;
                sumZ += m_labelHist[i][j].z;
            }
            float posX = sumX/m_labelHist[i].size()*m_voxelSizeX;
            float posY = sumY/m_labelHist[i].size()*m_voxelSizeY;
            float posZ = sumZ/m_labelHist[i].size()*m_voxelSizeZ;
            Nodes[i] = Vector(posX, posY, posZ);
        }
    }

    ConnectomeHelper::getInstance()->setLabelsReady(true);
    //renderGraph();
}

void Connectome::setEdges(Fibers *edges)
{

    m_fibers = edges;
    int nbFibers = m_fibers->getFibersCount();

    Edges.resize(m_NbLabels);
    for (unsigned int i=0; i<Edges.size();i++)
    {
        Edges[i].resize(m_NbLabels);
        for (unsigned int j=0; j<Edges[i].size(); j++)
        {
            Edges[i][j] = 0.0f;
        }
    }
    
    for( int i = 0; i < nbFibers; ++i )
    {        
        vector< Vector > currentFiberPoints;
        m_fibers->getFiberCoordValues(i, currentFiberPoints);
        Vector A =  currentFiberPoints[0];
        Vector B = currentFiberPoints.back();
        
        //look in labels to populate adjacency matrix
        //Get the 3D voxel
        int Ax = (int)( floor(A.x / m_voxelSizeX) );
        int Ay = (int)( floor(A.y / m_voxelSizeY) );
        int Az = (int)( floor(A.z / m_voxelSizeZ) );

        int Bx = (int)( floor(B.x / m_voxelSizeX) );
        int By = (int)( floor(B.y / m_voxelSizeY) );
        int Bz = (int)( floor(B.z / m_voxelSizeZ) );

        //Corresponding stick number
        unsigned int IdA = Az * m_columns * m_rows + Ay *m_columns + Ax;
        unsigned int IdB = Bz * m_columns * m_rows + By *m_columns + Bx;

        int Ex = m_labels->atNonNorm(IdA);
        int Ey = m_labels->atNonNorm(IdB);
        if(Ey != 0 && Ex != 0)
        {
            Edges[Ex-1][Ey-1] +=1;
            Edges[Ey-1][Ex-1] +=1;
        }
    }

    nodeDegree.resize(m_NbLabels);

    //Normalize
    for (unsigned int i=0; i<Edges.size();i++)
    {
        int deg = 0;
        for (unsigned int j=0; j<Edges[i].size(); j++)
        {
            if(Edges[i][j] !=0 && i!=j)
            {
                deg+=1;
                int tmpMax = Edges[i][j];
                int tmpMin = Edges[i][j];
                if(tmpMax > m_Edgemax)
                {
                    m_Edgemax = tmpMax;
                }
                if(tmpMin < m_Edgemin)
                {
                    m_Edgemin = tmpMin;
                }
            }
         nodeDegree[i] =  deg;
        }
    }

    for (unsigned int i=0; i<Edges.size();i++)
    {
        //normalize edge for visualization
        for (unsigned int j=0; j<Edges[i].size(); j++)
        {
            if(Edges[i][j] !=0 && i!=j)
            {
                Edges[i][j] = (Edges[i][j] - m_Edgemin)/(m_Edgemax-m_Edgemin);
            }
        }

        //Normalize node degree for visualization
        int tmpMax = nodeDegree[i];
        if(tmpMax > m_NodeDegreeMax)
        {
            m_NodeDegreeMax = tmpMax;
        }
    }
    std::cout << "Edge min.: " << m_Edgemin << " " << "Edge max.: " << m_Edgemax << std::endl;
    ConnectomeHelper::getInstance()->setEdgesReady(true);
    //renderGraph();
}

void Connectome::renderNodes()
{
    
    for (unsigned int i=0; i<Nodes.size(); i++)
    {
        /*glDepthMask(GL_FALSE);
        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ); */

        if(Nodes[i].getSquaredLength() != 0)
        {
            if(m_nodeAlpha != 1.0f)
		    {
                glEnable( GL_BLEND );
                glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
                glDepthMask( GL_FALSE );
            }
            //nodes
            wxColor color = ConnectomeHelper::getInstance()->getNodeColor();
            glColor4f( (float)color.Red() / 255.0f, (float)color.Green() / 255.0f, (float)color.Blue() / 255.0f, m_nodeAlpha );

            float size = m_nodeSize;
            if(ConnectomeHelper::getInstance()->isEdgesReady())
            {
                size *= (nodeDegree[i]/m_NodeDegreeMax);
            }
    
            glPushMatrix();
                glTranslatef( Nodes[i].x, Nodes[i].y, Nodes[i].z );
                GLUquadricObj* l_quadric = gluNewQuadric();
                gluQuadricNormals( l_quadric, GLU_SMOOTH );
                glScalef( size, size, size );
                gluSphere( l_quadric, 1.0f, 16, 16 );
            glPopMatrix();

            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE);
            
        }
    }
}

void Connectome::renderEdges()
{
    //Edges
    for (unsigned int i=0; i<Edges.size(); i++)
    {    
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
        for(unsigned int j=i+1; j<Edges[i].size();j++)
        {
            if(Edges[i][j] !=0)
            {
                float alphaValue = m_edgeAlpha;
			    float R,G,B;
                float v = Edges[i][j];
                float edgeSize;
			    if(v < 0.33f)
			    {
                    R = 0.0f;
                    G = (v/0.25f);
                    B = 1.0f;
                    edgeSize = 0;
                    alphaValue = 0.1f;
			    }
                else if(v > 0.33f && v < 0.66f)
                {
				    R = (v-0.33f)/(0.66f-0.33f);
				    G = 1.0f - (v-0.33f)/(0.66f-0.33f);
				    B = 1.0f;
                    edgeSize = 1;
                    alphaValue = 0.8f;
			    }
                else
                {
                    R = 1.0f;
				    G = 1.0f-((v-0.66f)/(1.0f-0.66f));
				    B = 1.0f-((v-0.66f)/(1.0f-0.66f));
                    edgeSize = 2;
                    alphaValue = 1.0f;
                }
                
                //Local vector
                Vector normalVector = Vector(Nodes[i].x-Nodes[j].x,Nodes[i].y-Nodes[j].y,Nodes[i].z-Nodes[j].z);
                normalVector.normalize();

                
                if(m_isOrientationDep)
                {
                    //View vector
                    Matrix4fT transform = SceneManager::getInstance()->getTransform();
                    float dots[8];
                    Vector3fT v1 = { { 0, 0, 1 } };
                    Vector3fT v2 = { { 1, 1, 1 } };
                    Vector3fT view;

                    Vector3fMultMat4( &view, &v1, &transform );
                    dots[0] = Vector3fDot( &v2, &view );
            
			        Vector zVector;
				    zVector = Vector(view.s.X, view.s.Y, view.s.Z); 
				    zVector.normalize();
			            
				    alphaValue = 1-std::abs(normalVector.Dot(zVector)); 
                    alphaValue = std::pow(alphaValue,3.0f);
                }

                if(alphaValue != 1.0f)
		        {
                    glEnable( GL_BLEND );
                    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
                    if(m_isFlashyEdges)
                        glBlendFunc( GL_ONE, GL_ONE );
                    glDepthMask( GL_FALSE );
                }

                glPushAttrib(GL_LINE_WIDTH);
                glLineWidth(edgeSize+m_edgeSize); 
                    
                glColor4f( R,G,B, alphaValue*m_edgeAlpha);
                glBegin( GL_LINES );
                    glVertex3f( Nodes[i].x, Nodes[i].y, Nodes[i].z );
                    glVertex3f( Nodes[j].x, Nodes[j].y, Nodes[j].z );
                glEnd();
                glDisable(GL_BLEND);
                glDepthMask(GL_TRUE);
                glPopAttrib();
                glLineWidth(1.0f);
            }
        }
    }
}

void Connectome::renderGraph()
{
    renderNodes();
    renderEdges();
}