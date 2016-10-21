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
m_nodeAlpha(1.0f),
m_nodeSize(2),
m_edgeSize(2.0f),
m_edgeAlpha(1.0f),
m_isFlashyEdges(false),
m_isOrientationDep(false),
m_Edgemax(0),
m_Edgemin(std::numeric_limits<int>::max()),
m_NodeDegreeMax(1),
m_Edgethreshold(0.0f),
m_savedNodeColor(1.0f,0.0f,0.0f),
m_NbOfPickedNodes(0)
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
    m_labelHist.clear();
    Nodes.clear();
    m_fiberMatrix.clear();

    m_Globalstats.m_NbNodes = 0;
    m_Globalstats.m_NbEdges = 0;
    m_Globalstats.m_Density = 0;

    ConnectomeHelper::getInstance()->setEdgesReady(false);
    ConnectomeHelper::getInstance()->setLabelsReady(false);
      
}
void Connectome::setLabels(Anatomy *labels)
{
    m_labels = labels;
    std::cout << "Nb labels: " << m_NbLabels << std::endl;
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

    labelMapping.resize(m_NbLabels);
    int count=0;
    for(size_t i = 0; i<m_labelHist.size(); i++)
    {
        if(m_labelHist[i].empty())
        {
            m_NbLabels--;
        }
        else
        {
            labelMapping[i] = count;
            count++;
        }
    }

    Nodes.resize( m_NbLabels );

    int l = 0;
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
            Nodes[l].center = Vector(posX, posY, posZ);
            l++;

        }
    }

    m_Globalstats.m_NbNodes = m_NbLabels;
    ConnectomeHelper::getInstance()->setLabelsReady(true);
}

void Connectome::setEdges(Fibers *edges)
{
    m_fibers = edges;
    int nbFibers = m_fibers->getFibersCount();
    m_selectedFibers.assign(m_fibers->getFibersCount(), false);

    Edges.resize(m_NbLabels);
    m_fiberMatrix.resize(m_NbLabels);
    for (unsigned int i=0; i<Edges.size();i++)
    {
        Edges[i].resize(m_NbLabels);
        m_fiberMatrix[i].resize(m_NbLabels);
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
            Edges[labelMapping[Ex-1]][labelMapping[Ey-1]] +=1;
            Edges[labelMapping[Ey-1]][labelMapping[Ex-1]] +=1;
            m_fiberMatrix[labelMapping[Ex-1]][labelMapping[Ey-1]].push_back(i);
        }
    }

    //Normalize
    for (unsigned int i=0; i<Edges.size();i++)
    {
        for (unsigned int j=0; j<Edges[i].size(); j++)
        {
            if(Edges[i][j] !=0 && i!=j)
            {
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
    }

    std::cout << "Edge min.: " << m_Edgemin << " " << "Edge max.: " << m_Edgemax << std::endl;
    
    computeNodeDegreeAndStrength();
    computeGlobalMetrics();
    ConnectomeHelper::getInstance()->setEdgesReady(true);
}
void Connectome::setLabelNames(std::vector<wxString> names)
{
    for(size_t i =0; i< Nodes.size(); i++)
    {
         Nodes[i].name = names[i];
    }
}
void Connectome::computeNodeDegreeAndStrength()
{
    float sumDegree = 0;
    int nbNodesActive = 0;
    FMatrix T(Edges.size(),Edges.size());
    std::vector<int> pickedNodes;
    
    for (unsigned int i=0; i<Edges.size();i++)
    {
        Nodes[i].degree = 0;
        Nodes[i].strength = 0;
        for (unsigned int j=0; j<Edges[i].size(); j++)
        {
            if(Edges[i][j] > m_Edgethreshold && i!=j)
            {
                Nodes[i].degree++; //Node degree 
                Nodes[i].strength+=Edges[i][j]; //Node strength
                T(i,j) = Edges[i][j];
            }
        }

        if(Nodes[i].degree > 0)
        {
            sumDegree += Nodes[i].degree;
            nbNodesActive++;
        }
        if(Nodes[i].picked)
        {
            pickedNodes.push_back(i);
            ConnectomeHelper::getInstance()->m_pGridNodeInfo->SetCellValue(0,0,Nodes[i].name);
            ConnectomeHelper::getInstance()->m_pGridNodeInfo->SetCellValue(1,0,wxString::Format( wxT( "%i" ), i));
            ConnectomeHelper::getInstance()->m_pGridNodeInfo->SetCellValue(2,0,wxString::Format( wxT( "%i" ), Nodes[i].degree));
            ConnectomeHelper::getInstance()->m_pGridNodeInfo->SetCellValue(3,0,wxString::Format( wxT( "%.3f" ), Nodes[i].strength));
        }
    }

    //Eigen Centrality
    std::vector< FArray > evecs;
    FArray evals;
    T.getEigenSystem( evals, evecs);

    //find Max eval
    float max = 0;
    int maxEvalID;
    for(size_t i=0; i<evals.size(); i++)
    {
        if(evals(i)>max)
        {
            max = evals(i);
            maxEvalID = i;
        }
    }
    /*for(int i = 0; i < evals.size();i++)
        std::cout << evals(i) << std::endl;

    std::cout<<maxEvalID<< " " << evals(maxEvalID);*/

    //std::cout<<"EVECS";
    //for(int i = 0; i < evecs[evals.size()-1].size();i++)
    //    std::cout << evecs[evals.size()-1][i] << std::endl;
        
    for(size_t i=0; i<Nodes.size();i++)
    {
        Nodes[i].eigen_centrality = evecs[maxEvalID][i];
        if(Nodes[i].picked)
        {
            ConnectomeHelper::getInstance()->m_pGridNodeInfo->SetCellValue(4,0,wxString::Format( wxT( "%.3f" ), Nodes[i].eigen_centrality));
        }
    }


    m_Globalstats.m_meanDegree = sumDegree/nbNodesActive; //Graph mean degree

    m_NodeDegreeMax = 1;
    for(unsigned int i=0; i<Edges.size();i++)
    {
        //Normalize node degree for visualization
        int tmpMax = Nodes[i].degree;
        if(tmpMax > m_NodeDegreeMax)
        {
            m_NodeDegreeMax = tmpMax;
        }
    }
}
void Connectome::setNodeColor( wxColour color )
{
    m_savedNodeColor = Vector( (float)color.Red() / 255.0f, (float)color.Green() / 255.0f, (float)color.Blue() / 255.0f);;
    for (unsigned int i=0; i<Nodes.size(); i++)
    {
        Nodes[i].color = m_savedNodeColor;
    }
}

namespace
{
template< class T > struct IndirectComp
{
    IndirectComp( const T &zvals ) :
        zvals( zvals )
    {
    }

    // Watch out: operator less, but we are sorting in descending z-order, i.e.,
    // highest z value will be first in array and painted first as well
    template< class I > bool operator()( const I &i1, const I &i2 ) const
    {
        return zvals[i1] > zvals[i2];
    }

private:
    const T &zvals;
};
}

void Connectome::renderNodes()
{
    float zMax = -999.0f;
    float zMin = 999.0f;
    size_t siz = Nodes.size();
    vector< float > zVals( siz );
    if(m_isOrientationDep)
    {
        GLfloat projMatrix[16];
        glGetFloatv( GL_PROJECTION_MATRIX, projMatrix );
        

        // Compute z values of lines (in our case: starting points only).
        

        unsigned int *pSnippletSort = NULL;
        pSnippletSort = new unsigned int[siz + 1];
     
        for(unsigned int i = 0; i < siz; ++i )
        {
            zVals[i] = ( Nodes[i].center.x * projMatrix[2] + Nodes[i].center.y * projMatrix[6]
                            + Nodes[i].center.z * projMatrix[10] + projMatrix[14] ) / ( Nodes[i].center.x * projMatrix[3]
                                    + Nodes[i].center.y * projMatrix[7] + Nodes[i].center.z * projMatrix[11] + projMatrix[15] );
            pSnippletSort[i] = i;
        }

        sort( &pSnippletSort[0], &pSnippletSort[siz], IndirectComp< vector< float > > ( zVals ) );

        
        for(size_t i =0; i<zVals.size(); i++)
        {
            if(zVals[i] > zMax)
            {
                zMax = zVals[i];
            }
            if(zVals[i] < zMin)
            {
                zMin = zVals[i];
            }
        }
    }

    for (unsigned int i=0; i<Nodes.size(); i++)
    {
        /*glDepthMask(GL_FALSE);
        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ); */

        if(Nodes[i].center.getSquaredLength() != 0)
        {
            if(m_nodeAlpha != 1.0f)
		    {
                glEnable( GL_BLEND );
                glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
                glDepthMask( GL_FALSE );
            }
            //nodes

            float depth = 1.0f;
            if(m_isOrientationDep)
                depth = 1 - ((zVals[i] - zMin) / (zMax - zMin));

            glColor4f(Nodes[i].color.x*depth, Nodes[i].color.y*depth, Nodes[i].color.z*depth, m_nodeAlpha);

            Nodes[i].size = m_nodeSize;
            if(ConnectomeHelper::getInstance()->isEdgesReady())
            {
                Nodes[i].size *= (float(Nodes[i].degree)/m_NodeDegreeMax);
            }
    
            glPushMatrix();
                glTranslatef( Nodes[i].center.x, Nodes[i].center.y, Nodes[i].center.z );
                GLUquadricObj* l_quadric = gluNewQuadric();
                gluQuadricNormals( l_quadric, GLU_SMOOTH );
                glScalef( Nodes[i].size, Nodes[i].size, Nodes[i].size);
                gluSphere( l_quadric, 1.0f, 16, 16 );
            glPopMatrix();

            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE);
            
        }
    }
}

hitResult Connectome::hitTest(Ray* i_ray)
{
    hitResult hr = { false, 0.0f, 0, NULL, 0 };

    for(unsigned int i=0; i < Nodes.size(); i++)
    {
        if(Nodes[i].size > 0)
        {
            int   picked  = 0;
            float tpicked = 0;
            float cx = Nodes[i].center.x;
            float cy = Nodes[i].center.y;
            float cz = Nodes[i].center.z;
            float size = Nodes[i].size * m_voxelSizeX;

            BoundingBox *bb = new BoundingBox( cx, cy, cz, size, size, size );
            hr = bb->hitTest( i_ray );
            if( hr.hit )
            {
                hr.picked = 100;
                hr.obj = i;
                
                break;
            }
        }     
    }

    return hr;
}

void Connectome::displayPickedNodeMetrics(hitResult hr)
{
    int id = hr.obj;
    Nodes[id].picked = !Nodes[id].picked;

    if(Nodes[id].picked)
    {
        Nodes[id].color = Vector(1.0f, 1.0f, 1.0f);

        m_NbOfPickedNodes++;

        ConnectomeHelper::getInstance()->m_pGridNodeInfo->SetCellValue(0,0,Nodes[id].name);
        ConnectomeHelper::getInstance()->m_pGridNodeInfo->SetCellValue(1,0,wxString::Format( wxT( "%i" ), id));
        ConnectomeHelper::getInstance()->m_pGridNodeInfo->SetCellValue(2,0,wxString::Format( wxT( "%i" ), Nodes[id].degree));
        ConnectomeHelper::getInstance()->m_pGridNodeInfo->SetCellValue(3,0,wxString::Format( wxT( "%.3f" ), Nodes[id].strength));
        ConnectomeHelper::getInstance()->m_pGridNodeInfo->SetCellValue(4,0,wxString::Format( wxT( "%.3f" ), Nodes[id].eigen_centrality));
    }
    else
    {
        Nodes[id].color = m_savedNodeColor;

        m_NbOfPickedNodes--;

        ConnectomeHelper::getInstance()->m_pGridNodeInfo->SetCellValue(0,0,wxString::Format( wxT( "" ), wxT( "" )));
        ConnectomeHelper::getInstance()->m_pGridNodeInfo->SetCellValue(1,0,wxString::Format( wxT( "" ), wxT( "" )));
        ConnectomeHelper::getInstance()->m_pGridNodeInfo->SetCellValue(2,0,wxString::Format( wxT( "" ), wxT( "" )));
        ConnectomeHelper::getInstance()->m_pGridNodeInfo->SetCellValue(3,0,wxString::Format( wxT( "" ), wxT( "" )));
        ConnectomeHelper::getInstance()->m_pGridNodeInfo->SetCellValue(4,0,wxString::Format( wxT( "" ), wxT( "" )));
    }

    if(ConnectomeHelper::getInstance()->isEdgesReady())
        setSelectedStreamlines();
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
            if(Edges[i][j] > m_Edgethreshold)
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
                Vector normalVector = Vector(Nodes[i].center.x-Nodes[j].center.x,Nodes[i].center.y-Nodes[j].center.y,Nodes[i].center.z-Nodes[j].center.z);
                normalVector.normalize();

                
        //        if(m_isOrientationDep)
        //        {
        //            //View vector
        //            Matrix4fT transform = SceneManager::getInstance()->getTransform();
        //            float dots[8];
        //            Vector3fT v1 = { { 0, 0, 1 } };
        //            Vector3fT v2 = { { 1, 1, 1 } };
        //            Vector3fT view;

        //            Vector3fMultMat4( &view, &v1, &transform );
        //            dots[0] = Vector3fDot( &v2, &view );
        //    
			     //   Vector zVector;
				    //zVector = Vector(view.s.X, view.s.Y, view.s.Z); 
				    //zVector.normalize();
			     //       
				    //alphaValue = 1-std::abs(normalVector.Dot(zVector)); 
        //            alphaValue = std::pow(alphaValue,3.0f);
        //        }

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
                    glVertex3f( Nodes[i].center.x, Nodes[i].center.y, Nodes[i].center.z );
                    glVertex3f( Nodes[j].center.x, Nodes[j].center.y, Nodes[j].center.z );
                glEnd();
                glDisable(GL_BLEND);
                glDepthMask(GL_TRUE);
                glPopAttrib();
                glLineWidth(1.0f);
            }
        }
    }
}

void Connectome::computeGlobalMetrics()
{
    int nbNodes = 0;
    int totalEdges = 0;
    for (unsigned int i=0; i<Edges.size(); i++)
    {
        float sumEdge = 0.0f;
        for(unsigned int j=0; j<Edges[i].size();j++)
        {
            if(Edges[i][j] > m_Edgethreshold && j!=i)
            {
                sumEdge++;
            }
        }
        if(sumEdge > 0.0f)
        {
            nbNodes++;
        }
        totalEdges+=sumEdge;
    }

    m_Globalstats.m_NbNodes = nbNodes;
    m_Globalstats.m_NbEdges = totalEdges/2;
    m_Globalstats.m_Density = float(totalEdges) / float((nbNodes)*(nbNodes-1));

}

void Connectome::renderGraph()
{
    renderNodes();
    renderEdges();
}

void Connectome::setSelectedStreamlines()
{
    if(ConnectomeHelper::getInstance()->isShowStreamlines() && m_NbOfPickedNodes == 2)
    {
        m_selectedFibers.assign(m_fibers->getFibersCount(), false);
        std::vector<int> id;
        for(size_t i =0; i < Nodes.size(); i++)
        {
            if(Nodes[i].picked)
            {
                id.push_back(i);
            }
        }

        for(size_t f=0; f<m_fiberMatrix[id[0]][id[1]].size(); f++)
        {
            int selectedFibersID = m_fiberMatrix[id[0]][id[1]][f];
            m_selectedFibers[selectedFibersID] = true;
        }
        
    }
    else
    {
        m_selectedFibers.assign(m_fibers->getFibersCount(), false);
    }
}