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

struct GlobalGraphMetrics 
{
    GlobalGraphMetrics() : 
        m_NbNodes( 0 ),
        m_NbEdges ( 0 ),
        m_Density ( 0.0f ),
        m_meanDegree ( 0.0f )
    {
    }
    
    int   m_NbNodes;
    int   m_NbEdges;
    float m_Density;
    float m_meanDegree;
    float m_globalEfficiency;
};

struct Node
{
    Node():
    name(wxT("")),
    center(0,0,0),
    size(1),
    color(1.0f, 0.0f, 0.0f),
    picked(false),
    degree(0),
    strength(0.0f),
    eigen_centrality(0.0f),
    local_efficiency(0.0f)
{
}
    Vector center;
    float size;
    Vector color;
    bool picked;

    wxString name;
    int degree;
    float strength;
    float eigen_centrality;
    float closeness_centrality;
    float betweenness_centrality;
    float local_efficiency;
    std::vector<float> min_dist;
};

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
    void setEdgeThreshold(float value) {m_Edgethreshold = value;}
    
    bool toggleFlashyEdges()           { return m_isFlashyEdges = !m_isFlashyEdges; }
    bool isFlashyEdges() const { return m_isFlashyEdges; }
    bool toggleOrientationDep()           { return m_isOrientationDep = !m_isOrientationDep; }
    bool isOrientationDep() const { return m_isOrientationDep; }

    void renderGraph();
    void renderNodes();
    void renderEdges();
    void setNodeColor( wxColour color );
    void displayPickedNodeMetrics( hitResult hr);
    std::vector<bool> getSelectedFibers() { return m_selectedFibers;}
    void setSelectedStreamlines();
    void setLabelNames(std::vector<wxString> names);

    void computeNodeDegreeAndStrength();
    void computeGlobalMetrics();

    hitResult hitTest( Ray* i_ray );

    void clearConnectome();
    GlobalGraphMetrics getGlobalStats() {return m_Globalstats;}

    void dijkstra(int src, int nbNodes, vector<vector<float> > graph, vector<float>& min_dist);
    int minDistance(int nbNodes, std::vector<float> dist, std::vector<bool> sptSet);
    float closenessCentrality(int nodeID);
    float localEfficiency(int id);


protected:
    GlobalGraphMetrics        m_Globalstats;

	
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
    float m_Edgethreshold;
    Vector m_savedNodeColor;
    int m_NbOfPickedNodes;
    int m_nbNodesActive;

    int m_NbLabels;
    std::vector<std::vector<Vector> > m_labelHist;
    std::vector<int> labelMapping;
    std::vector<std::vector<std::vector<int> > > m_fiberMatrix;
    std::vector<bool> m_selectedFibers;
    std::vector<Node> Nodes;
    std::vector<std::vector<float> > Edges;

    //metrics

};

#endif /* CONNECTOME_H_ */
