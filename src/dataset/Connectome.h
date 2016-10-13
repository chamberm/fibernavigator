/////////////////////////////////////////////////////////////////////////////
// Name:            Connectome.h
// Author:          Maxime Chamberland
/////////////////////////////////////////////////////////////////////////////
#ifndef CONNECTOME_H_
#define CONNECTOME_H_

#include "Anatomy.h"
#include "Fibers.h"
#include "DatasetInfo.h"
#include "DatasetIndex.h"
#include "../misc/nifti/nifti1_io.h"

class Connectome
{
public:

    // Constructor/Destructor
    Connectome();
    virtual ~Connectome();

	void setLabels( Anatomy* labels );
    void setEdges( Fibers* edges );
    void setNbLabels( double nbLabels ) {m_NbLabels = int(nbLabels); }
    void setNodeAlpha( float value ) {m_nodeAlpha = value;}
    void setNodeSize( float value ) {m_nodeSize = value;}
    void setEdgeSize( float value) {m_edgeSize = value;}
    void setEdgeAlpha(float value) {m_edgeAlpha = value;}
    
    bool toggleFlashyEdges()           { return m_isFlashyEdges = !m_isFlashyEdges; }
    bool isFlashyEdges() const { return m_isFlashyEdges; }
    bool toggleOrientationDep()           { return m_isOrientationDep = !m_isOrientationDep; }
    bool isOrientationDep() const { return m_isOrientationDep; }

    void renderGraph();
    void renderNodes();
    void renderEdges();

    void clearConnectome();

	
private:
    
    Anatomy *m_labels;
    Fibers  *m_fibers;
    int m_rows;
	int m_columns;
	int m_frames;
    int m_datasetSize;
	float m_voxelSizeX;
    float m_voxelSizeY;
    float m_voxelSizeZ;
    float m_nodeAlpha;
    float m_nodeSize;
    float m_edgeSize;
    float m_edgeAlpha;
    bool m_isFlashyEdges;
    bool m_isOrientationDep;
    int m_Edgemax;
    int m_Edgemin;
    int m_NodeDegreeMax;

    int m_NbLabels;
    std::vector<std::vector<Vector> > m_labelHist;

    std::vector<Vector> Nodes;
    std::vector<std::vector<float> > Edges;

    //metrics
    std::vector<float> nodeDegree;

};

#endif /* CONNECTOME_H_ */
