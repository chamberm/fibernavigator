/////////////////////////////////////////////////////////////////////////////
//
// Description: SelectionObject class.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef SELECTIONOBJECT_H_
#define SELECTIONOBJECT_H_

#include "BoundingBox.h"
#include "SceneObject.h"

#include "../dataset/DatasetIndex.h"
#include "../misc/Algorithms/Face3D.h"
#include "../misc/IsoSurface/Vector.h"

#include <GL/glew.h>

#include <wx/xml/xml.h>
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/button.h>
#include <wx/grid.h>
#include <wx/string.h>
#include <wx/tglbtn.h>

#include <list>
#include <map>
#include <vector>
using std::vector;

class Anatomy;
class Fibers;
class CIsoSurface;
class MainCanvas;
class PropertiesWindow;

/****************************************************************************/
// Description : This is the base class for any Selection Object.
/****************************************************************************/

///////////////////////////////////////////////////////////////////////////
// Structure containing the information that are displayed in the fibers info grid window.
///////////////////////////////////////////////////////////////////////////
struct FibersInfoGridParams 
{
    FibersInfoGridParams() : m_count            ( 0    ),
                             m_meanValue        ( 0.0f ),
                             m_meanLength       ( 0.0f ),
                             m_minLength        ( 0.0f ),
                             m_maxLength        ( 0.0f ),
                             m_meanCrossSection ( 0.0f ),
                             m_minCrossSection  ( 0.0f ),
                             m_maxCrossSection  ( 0.0f ),
                             m_dispersion       ( 0.0f )
    {
    }
    
    int   m_count;
    float m_meanValue;
    float m_meanLength;
    float m_minLength;
    float m_maxLength;
    float m_meanCrossSection;
    float m_minCrossSection;
    float m_maxCrossSection;
    float m_dispersion;
};

class SelectionObject : public SceneObject, public wxTreeItemData
{
public :
    // TODO selection should be pure virtual
    SelectionObject( Vector i_center, Vector i_size );
    SelectionObject( const wxXmlNode selObjNode );
    virtual ~SelectionObject();

    virtual hitResult hitTest( Ray* i_ray ) = 0;

    virtual void objectUpdate();

    void draw();

    void moveBack();
    void moveDown();
    void moveForward();
    void moveLeft();
    void moveRight();
    void moveUp();
    void processDrag( wxPoint i_click, wxPoint i_lastPos, GLdouble i_projection[16], GLint i_viewport[4], GLdouble i_modelview[16] );
    void resizeBack();
    void resizeDown();
    void resizeForward();
    void resizeLeft();
    void resizeRight();    
    void resizeUp();
    void update();
    virtual void createPropertiesSizer( PropertiesWindow *pParent );
    virtual void updatePropertiesSizer();
    
    
    // Set/get and there affiliated functions
    bool       toggleIsActive();
    void       setIsActive( bool isActive );
    bool       getIsActive() const                    { return m_isActive;                     };

    ObjectType getSelectionType()                     { return m_objectType;                   };

    void       setCenter( float i_x, float i_y, float i_z );
    void       setCenter( Vector i_center );
    Vector     getCenter()                            { return m_center;                       };

    void       setColor( wxColour i_color );
    wxColour   getColor()                             { return m_color;                        };

    int        getIcon();

    void       setName( wxString i_name )             { m_name = i_name;                       };
    wxString   getName()                              { return m_name;                         };
    
    bool       toggleIsNOT();
    void       setIsNOT( bool i_isNOT );
    bool       getIsNOT()                             { return m_isNOT;                        };

    bool       togglePruneRemove();
    void       setPruneRemove( bool m_isRemove );
    bool       getIsRemove()                          { return m_isRemove;                     };

    void       setPicked( int i_picked )              { m_hitResult.picked = i_picked;         };

    void       setSize( float sizeX, float sizeY, float sizeZ ) 
                                                      { setSize( Vector( sizeX, sizeY, sizeZ ) ); }
    void       setSize( Vector i_size )               { m_size = i_size; update(); notifyInBoxNeedsUpdating(); m_boxResized = true; };
    Vector     getSize()                              { return m_size;};

    void       setTreeId( wxTreeItemId i_treeId )     { m_treeId = i_treeId;                   };
    wxTreeItemId getTreeId()                          { return m_treeId;                       };
    
    void       select();
    void       unselect()                             { m_isSelected = false;                  };
    
    bool       toggleIsVisible()                      { setIsVisible( !getIsVisible() ); return getIsVisible(); };
    void       setIsVisible( bool i_isVisible )       { m_isVisible = i_isVisible;             };
    bool       getIsVisible()                         { return m_isVisible;                    };

    void       setIsMagnet( bool i_isMagnet )       { m_isMagnet = i_isMagnet;             };
    bool       isMagnet()                           { return m_isMagnet;};
    void       setStrength( float str )             { m_Q = str;             };
    float      getStrength()                        { return m_Q;              };
    Vector     getMagnetField()                     { return m_magnetField; };
    void       setMagnetField(Vector field)         { m_magnetField = field; };
	
    bool meanStreamlineDisplayed() {return m_pToggleDisplayMeanFiber->GetValue();}
	vector<Vector> getMeanFiberPts() {return m_meanFiberPoints;   }


    void       setConvexHullColor( wxColour i_color ) { m_convexHullColor = i_color;            }; 
    wxColour   getConvexHullColor()                   { return m_convexHullColor;               };

    void       setConvexHullOpacity( float i_opacity) { m_convexHullOpacity = i_opacity;         };
    float      getConvexHullOpacity()                 { return m_convexHullOpacity;              };

    void       setMeanFiberColor( wxColour i_color )  { m_meanFiberColor = i_color;            }; 
    wxColour   getMeanFiberColor()                    { return m_meanFiberColor;               };

    void       setMeanFiberOpacity( float i_opacity) { m_meanFiberOpacity = i_opacity;         };
    float      getMeanFiberOpacity()                 { return m_meanFiberOpacity;              };

	void       setCSThreshold( float i_CSthresh) { m_CSThreshold = i_CSthresh;         };
    float      getCSThreshold()                 { return m_CSThreshold;              };

	void       setNoOfCS( float i_noOfCS) { m_noOfMeanFiberPts = i_noOfCS;         };
    float      getNoOfCS()                 { return m_noOfMeanFiberPts;              };

	void       setRefAnat(Anatomy * refAnat);

    void       setMeanFiberColorMode( FibersColorationMode i_mode ) { m_meanFiberColorationMode = i_mode; };
    FibersColorationMode getMeanFiberColorMode()     { return m_meanFiberColorationMode;        };

	void       setMeanMethodMode( MeanMethods i_mode ) { m_meanFiberMode = i_mode; };
    MeanMethods getMeanMethodMode()     { return m_meanFiberMode;        };
    
    // Methods related to the different fiber bundles selection.
    typedef DatasetIndex FiberIdType;
    struct SelectionState
    {
        public: 
            SelectionState()
            : m_inBoxNeedsUpdating( true )
            {};
            
            vector< bool > m_inBranch;
            vector< bool > m_inBox;
            bool           m_inBoxNeedsUpdating;
    };
    
    bool            addFiberDataset(    const FiberIdType &fiberId, const int fiberCount );
    void            removeFiberDataset( const FiberIdType &fiberId );
    SelectionState& getState(           const FiberIdType &fiberId );
    
    // Methods related to saving and loading.
    virtual bool populateXMLNode( wxXmlNode *pCurNode, const wxString &rootPath );
    
    virtual wxString getTypeTag() const;

    //Distance coloring setup
    bool        IsUsedForDistanceColoring() const;
    void        UseForDistanceColoring(bool aUse);

    //Normal flips
    // Do not flip for generic selection objects.
    virtual void flipNormals() {};

    // Variables
    bool                m_boxMoved;
    bool                m_boxResized;


protected :
    virtual void drawObject( GLfloat* i_color ) = 0;
    
    void  drag  ( wxPoint i_click, wxPoint i_lastPos, GLdouble i_projection[16], GLint i_viewport[4], GLdouble i_modelview[16] );
    void  resize( wxPoint i_click, wxPoint i_lastPos, GLdouble i_projection[16], GLint i_viewport[4], GLdouble i_modelview[16] );
    void  updateStatusBar();
    float getAxisParallelMovement( int i_x1, int i_y1, int i_x2, int i_y2, Vector i_n, GLdouble i_projection[16], GLint i_viewport[4], GLdouble i_modelview[16] );
    
    wxString        m_name;
    ObjectType      m_objectType;
    Vector          m_center;
    Vector          m_size;
    bool            m_isActive;
    bool            m_isNOT;
    bool            m_isRemove;
    bool            m_isSelected;
    bool            m_isVisible;
    bool            m_isMagnet;
    int             m_stepSize;
    float           m_Q;
    Vector          m_magnetField;
	int             m_noOfMeanFiberPts;

    wxColour        m_color;         // Used for coloring the isosurface.
    
    hitResult       m_hitResult;
    
    wxTreeItemId    m_treeId;
    
    bool            m_statsAreBeingComputed;
    bool            m_meanFiberIsBeingDisplayed;

    //Distance coloring switch
    bool            m_DistColoring;

    wxColour m_convexHullColor;
    float    m_convexHullOpacity; //Between 0 and 1
    bool     m_mustUpdateConvexHull;
    std::list< Face3D >  m_hullTriangles;

    
    //Mean fiber coloring variables
    wxColour m_meanFiberColor; //Custom color chose by the user
    std::vector< Vector > m_meanFiberColorVector; //Vector of colour compute by the program
    float m_meanFiberOpacity; //Between 0 and 1
	float m_CSThreshold;
    FibersColorationMode m_meanFiberColorationMode;
	MeanMethods m_meanFiberMode;

    // Those variables represent the min/max value in pixel of the object.
    float m_minX;
    float m_minY;
    float m_minZ;
    float m_maxX;
    float m_maxY;
    float m_maxZ;
    Anatomy *m_pRefAnatInfo;

    std::map< FiberIdType, SelectionState > m_selectionStates;
    
    void notifyInBoxNeedsUpdating();
    void notifyInBranchNeedsUpdating();
    
    SelectionObject();

private:
    void doBasicInit();

    /******************************************************************************************
    * Functions/variables related to the fiber info calculation.
    ******************************************************************************************/
public:
    // Functions
    void   updateStats                       ();
    void   notifyStatsNeedUpdating           ();

    void   computeConvexHull                 ();
    
    void   updateMeanFiberOpacity             ();
	void   updateCSThreshold                  ();
	void   updateNoOfCS                       ();
    void   UpdateMeanValueTypeBox             ();
    void   updateConvexHullOpacity            ();
	bool   saveTractometry(wxString filename);
	void   flipStartingPoint();
	void   setShowStartingPoint();
protected:
    void   drawCrossSections                 ();
    void   drawCrossSectionsPolygons         ();
    void   drawDispersionCone                ();
    void   drawFibersInfo                    ();
    void   setNormalColorArray               ( const std::vector< Vector > &i_fiberPoints);
    void   setShowMeanFiberOption            ( bool i_val );
    void   drawPolygon                       ( const std::vector< Vector >           &i_crossSectionPoints      );
    void   drawSimpleCircles                 ( const std::vector< std::vector< Vector > > &i_allCirclesPoints        );
    void   drawThickFiber                    ( const std::vector< Vector >           &i_fiberPoints,
                                                     float                           i_thickness, 
                                                     int                             i_nmTubeEdge               );
	void   drawStartingPoint();
    void   drawConvexHull                    ();
    void   setShowConvexHullOption           (bool i_val);
    void   drawTube                          ( const std::vector< std::vector< Vector > > &i_allCirclesPoints,
                                                     GLenum                          i_tubeType               );
    void   getCrossSectionAreaColor          (       unsigned int                    i_index                   );
    void   getDispersionCircle               ( const std::vector< Vector >           &i_crossSectionPoints, 
                                               const Vector                          &i_crossSectionNormal, 
                                                     std::vector< Vector >           &o_circlePoints            );
    
    void   updateStatsGrid                   ();
    bool   getFiberCoordValues               (       int                             fiberIndex, 
                                                     std::vector< Vector >           &o_fiberPoints             );
    bool   getFiberLength                    ( const std::vector< Vector >           &i_fiberPoints,
                                                     float                           &o_length                  );
   
    bool   getFiberPlaneIntersectionPoint    ( const std::vector< Vector >           &i_fiberPoints, 
                                               const Vector                          &i_pointOnPlane,
                                               const Vector                          &i_planeNormal,
                                                     std::vector< Vector >           &o_intersectionPoints,
													 int id);
    
    bool   getFiberDispersion                (       float                           &o_dispersion              );
    
    float  getMaxDistanceBetweenPoints       ( const std::vector< Vector >           &i_points, 
                                                     int*                            o_firstPointIndex = NULL, 
                                                     int*                            o_secondPointIndex = NULL );
    bool   getMeanFiber                      ( const std::vector< std::vector< Vector > > &i_fibersPoints,
                                                     unsigned int                    i_nbPoints,
                                                     std::vector< Vector >           &o_meanFiber               );
	
    bool   getMeanFiberValue                 ( const std::vector< std::vector< Vector > > &fibersPoints, 
                                                     float                           &computedMeanValue         );
	bool   performTractometry                 ( const std::vector< std::vector< Vector > > &fibersPoints);
    
    bool   getMeanMaxMinFiberCrossSection    ( const std::vector< std::vector< Vector > > &i_fibersPoints,
                                               const std::vector< Vector >           &i_meanFiberPoints,
                                                     float                           &o_meanCrossSection,
                                                     float                           &o_maxCrossSection,
                                                     float                           &o_minCrossSection         );
	void getSpline();
    
    bool   getMeanMaxMinFiberLength( const vector< int > &selectedFibersIndexes,
                                           Fibers        *pCurFibers,
                                           float         &o_meanLength,
                                           float         &o_maxLength,
                                           float         &o_minLength);

	bool   getLongestStreamline( const vector< int > &selectedFibersIndexes,
                                           Fibers        *pCurFibers);

    std::vector< std::vector< Vector > >   getSelectedFibersPoints ();
    
    vector< int > getSelectedFibersIndexes( Fibers *pFibers );
    bool          getSelectedFibersInfo( const vector< int > &selectedFibersIdx, 
                                        Fibers *pFibers,
                                        vector< int > &pointsCount, 
                                        vector< vector< Vector > > &fibersPoints );

    
    std::vector< float >        m_crossSectionsAreas;   // All the cross sections areas value.
    std::vector< Vector >       m_crossSectionsNormals; // All the cross sections normals value.
    std::vector< std::vector < Vector > > m_crossSectionsPoints;  // All the cross sections hull points in 3D.
    unsigned int                m_maxCrossSectionIndex; // Index of the max cross section of m_crossSectionsPoints.
    std::vector< Vector >       m_meanFiberPoints;      // The points representing the mean fiber.
    unsigned int                m_minCrossSectionIndex; // Index of the min cross section of m_crossSectionsPoints.
	std::vector <float>         m_tractometrics;
	std::vector< std::vector<double> > VecMean;
    
    FibersInfoGridParams        m_stats;                // The stats for this box.
    bool                        m_statsNeedUpdating;    // Will be used to check if the stats<
	bool                        m_showStartPoint;
	bool						m_flipStartingPoint;
    /******************************************************************************************
    * END of the functions/variables related to the fiber info calculation.
    *****************************************************************************************/

protected:
    wxBitmapButton  *m_pbtnSelectColor;

private:
    wxTextCtrl      *m_pTxtName;
    wxToggleButton  *m_pToggleVisibility;
    wxToggleButton  *m_pToggleActivate;
    wxToggleButton  *m_pToggleCalculatesFibersInfo;
    wxGrid          *m_pGridFibersInfo;
    wxToggleButton  *m_pToggleDisplayMeanFiber;
	wxToggleButton  *m_pToggleShowStartingPoint;
	wxToggleButton  *m_pToggleFlipStartingPoint;
     wxToggleButton  *m_pToggleDisplayConvexHull;
     wxBitmapButton  *m_pBtnSelectConvexHullColor;
     wxStaticText    *m_pLblConvexHullOpacity;
     wxSlider        *m_pSliderConvexHullOpacity;
    wxBitmapButton  *m_pBtnSelectMeanFiberColor;
    wxStaticText    *m_pLblColoring;
    wxRadioButton   *m_pRadCustomColoring;
    wxRadioButton   *m_pRadNormalColoring;
	wxStaticText    *m_pLblMethod;
    wxRadioButton   *m_pRadMeanMean;
    wxRadioButton   *m_pRadMeanCross;
    wxStaticText    *m_pLblMeanFiberOpacity;
	wxStaticText    *m_pLblCrossSectionThreshold;
	wxStaticText    *m_pLblNoOfCS;
    wxSlider        *m_pSliderMeanFiberOpacity;
	wxSlider		*m_pSliderCSthreshold;
	wxSlider        *m_pSliderNoOfCS;
    wxButton        *m_pbtnDisplayCrossSections;
    wxButton        *m_pbtnDisplayDispersionTube;
	wxButton        *m_pSaveTractometry;
	
    wxStaticText    *m_pLabelAnatomy;
    wxChoice        *m_pCBSelectDataSet;
    wxToggleButton  *m_pToggleFieldDirection;
	wxStaticText    *m_pLblRefAnat;
    

public:
    wxTextCtrl      *m_pTxtBoxX;
    wxTextCtrl      *m_pTxtBoxY;
    wxTextCtrl      *m_pTxtBoxZ;
    wxTextCtrl      *m_pTxtSizeX;
    wxTextCtrl      *m_pTxtSizeY;
    wxTextCtrl      *m_pTxtSizeZ;
	wxTextCtrl      *m_pTxtBoxNbOfPlanes;
    wxSlider        *m_pSliderQ;
    wxTextCtrl      *m_pBoxQ;
    wxToggleButton  *m_pTogglePruneRemove;
	wxButton        *m_pBtnRefAnat;
    
    static const int    DISPERSION_CONE_NB_TUBE_EDGE=25; // This value represent the number of edge the dispersion cone will have.
    //static const int    MEAN_FIBER_NB_POINTS=50;         // This value represent the number of points we want the mean fiber to have.
    static const int    THICK_FIBER_NB_TUBE_EDGE=10;     // This value represent the number of edge the tube of the thick fiber will have.
    static const int    THICK_FIBER_THICKNESS=33;        // This value represent the size of the tube the thick fiber will have (*1/100).

public:
    CrossSectionsDisplay   m_displayCrossSections;
    DispersionConeDisplay  m_displayDispersionCone;
};

//////////////////////////////////////////////////////////////////////////

inline bool SelectionObject::IsUsedForDistanceColoring() const
{
    return m_DistColoring;
}

//////////////////////////////////////////////////////////////////////////

inline void SelectionObject::UseForDistanceColoring(bool aUse)
{
    m_DistColoring = aUse;
}

//////////////////////////////////////////////////////////////////////////


#endif /*SELECTIONOBJECT_H_*/
