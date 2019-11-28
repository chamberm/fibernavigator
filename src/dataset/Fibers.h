#ifndef FIBERS_H_
#define FIBERS_H_

#include "DatasetInfo.h"
#include "Octree.h"
#include "../gui/SelectionObject.h"
#include "../misc/Fantom/FVector.h"

#include <GL/glew.h>
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <fstream>
#include <string>
#include <vector>

enum FiberFileType
{
    ASCII_FIBER = 0,
    ASCII_VTK   = 1,
    BINARY_VTK  = 2,
};

const int FIBERS_SUBSAMPLING_RANGE_MIN(0);
const int FIBERS_SUBSAMPLING_RANGE_MAX(99);
const int FIBERS_SUBSAMPLING_RANGE_START(0);

/**
 * This class represents a set of fibers.
 * It supports loading different fibers file types.
 * It holds the fibers data, and can also be used to
 * compute statistics and selected fibers data.
 */
class Fibers : public DatasetInfo
{
public:
    Fibers();
    virtual ~Fibers();

    // Fibers loading methods
    bool    load( const wxString &filename );
    bool    createFrom( const vector<Fibers*>& fibers, wxString name=wxT("Merged"));
	bool    createFrom( const vector<float*>& pointArray, const vector<int>& linePointers, const vector<float*>& colorArray, wxString name=wxT("Generated.trk"));
	void    addCentroidToRender(vector<float> pts);
	void    toggleShowCentroid();
	bool    isShowCentroids() {return m_showCentroids;}

    void    updateFibersColors();

    Anatomy* generateFiberVolume();

    void    getFibersInfoToSave( std::vector<float> &pointsToSave, std::vector<int> &linesToSave, std::vector<int> &colorsToSave, int &countLines );
	void    getFibersInfoToSaveTCK( std::vector<float> &pointsToSave, int &countLines );
    void    getNbLines( int &nbLines );
    void    loadDMRIFibersInFile( std::ofstream &myfile );

    void    save( wxString filename, int format );
    bool    save( wxXmlNode *pNode, const wxString &rootPath ) const;
    void    saveDMRI( wxString filename );

    int     getPointsPerLine(     const int lineId );
    int     getStartIndexForLine( const int lineId );

    int     getLineForPoint( const int pointIdx );

    void    resetColorArray();
    
    void     setFiberColor( const int fiberIdx, const wxColour& col );
    wxColour getFiberPointColor( const int fiberIdx, const int ptIdx );

    void    updateLinesShown();

    void    initializeBuffer();

    void    draw();
    void    switchNormals( bool positive );

    float   getPointValue( int  ptIndex );
    int     getLineCount();
    int     getPointCount();
    bool    isSelected( int  fiberId );

    float    getLocalizedAlpha( int index );

    void    setFibersLength();
	vector<float> getCentroidPts() { return m_centroidPts;}
    
    float   getFiberLength( const int fiberId ) const
    {
        if( fiberId < 0 || static_cast< unsigned int >( fiberId ) >= m_length.size() )
        {
            return 0.0f;
        }
        
        return m_length[ fiberId ];
    }
    
    bool    getFiberCoordValues( int fiberIndex, std::vector< Vector > &fiberPoints );

    void    updateFibersFilters();
    void    updateFibersFilters(int minLength, int maxLength, int minSubsampling, int maxSubsampling);
    std::vector< bool >  getFilteredFibers();

    void    updateAlpha();
    void    setAxisView(bool value);
    void    setModeOpac(bool value);
    void    setRenderFunc(bool value);
    void    setLocalGlobal(bool value);
    void    setUsingEndpts(bool value);

    void    flipAxis( AxisType i_axe);
    void    fitToAnat( bool saving);
    
    int     getFibersCount() const { return m_countLines; }
    
    // TODO check if we can set const
    Octree* getOctree() const { return m_pOctree; }
    
    vector< int > getReverseIdx() const { return m_reverse; }

    virtual void createPropertiesSizer( PropertiesWindow *pParent );
    virtual void updatePropertiesSizer();

    bool isUsingFakeTubes()    const       { return m_useFakeTubes; }
    bool isUsingTransparency() const       { return m_useTransparency; }
    bool isFibersInverted()    const       { return m_fibersInverted; }

    void updateColorationMode()              { m_isColorationUpdated = true; }
    FibersColorationMode getColorationMode() { return m_fiberColorationMode; }
    void setColorationMode(FibersColorationMode val) { m_fiberColorationMode = val; }
    
    void setConstantColor( const wxColor &col ) { m_constantColor = col; }

    void useFakeTubes();
    void useTransparency();
    bool invertFibers()        { return m_fibersInverted = !m_fibersInverted; }

    void findCrossingFibers();

    GLuint  getGLuint() { return 0; }

    float   getMaxFibersLength() { return m_maxLength; }
    float   getMinFibersLength() { return m_minLength; }
    void    updateSliderMinLength( int val )        { m_pSliderFibersFilterMin->SetValue( val ); }
    void    updateSliderMaxLength( int val )        { m_pSliderFibersFilterMax->SetValue( val ); }
    void    updateSliderSubsampling( int val )      { m_pSliderFibersSampling->SetValue( val ); }
    void    updateToggleLocalColoring( bool val )   { m_pToggleLocalColoring->SetValue( val ); }
    void    updateToggleNormalColoring( bool val )  { m_pToggleNormalColoring->SetValue( val ); }
    void    updateSliderThickness( int val )        { m_pSliderInterFibersThickness->SetValue( val ); }

    // Empty derived methods
    void    drawVectors()      {};
    void    generateTexture()  {};
    void    generateGeometry() {};
    void    smooth()           {};

    void    toggleCrossingFibers();
    void    toggleSliceFibers();
    bool    isIntersectFibers() {return m_useIntersectedFibers;}
    bool    isSliceFibers() {return m_useSliceFibers;}

    void    updateCrossingFibersThickness();
    void    updateTubeRadius();

	void    convertFromRTT();

    // Inherited from DatasetInfo
    bool    toggleShow();
    bool    getViewRadValue() { return m_pToggleAxisView->GetValue(); }
	bool    getModeOpacValue() { return m_pToggleModeOpac->GetValue(); }
    bool    getRenderFuncValue() { return m_pToggleRenderFunc->GetValue(); }
    bool    getLocalGlobalValue() { return m_pToggleLocalGlobal->GetValue(); }
    bool    getEndPtsValue() { return m_pToggleEndpts->GetValue(); }

private:
    Fibers( const Fibers & );
    Fibers &operator=( const Fibers & );

private:
    bool            loadTRK(    const wxString &filename );
    bool            loadCamino( const wxString &filename );
    bool            loadMRtrix( const wxString &filename );
    bool            loadPTK(    const wxString &filename );
    bool            loadVTK(    const wxString &filename );
    bool            loadDmri(   const wxString &filename );
    void            loadTestFibers();

    void            colorWithTorsion(       float *pColorData );
    void            colorWithCurvature(     float *pColorData );
    void            colorWithDistance(      float *pColorData );
    void            colorWithMinDistance(   float *pColorData );
    void            colorWithConstantColor( float *pColorData );
    

    void            toggleEndianess();
    std::string     intToString( const int number );

    void            calculateLinePointers();
    void            createColorArray( const bool colorsLoadedFromFile );

    void            resetLinesShown();

    void            drawFakeTubes();
    void            drawSortedLines();
    void            drawCrossingFibers();

    void            freeArrays();

    void            setShader();
    void            releaseShader();

    void            computeGLobalProperties();

private:
    // Variables
    bool                  m_isSpecialFiberDisplay;
    Vector                m_barycenter;
    std::vector< float >  m_boxMax;
    std::vector< float >  m_boxMin;
    std::vector< float >  m_colorArray;
    int                   m_count;
    int                   m_countLines;
    int                   m_countPoints;
    bool                  m_isInitialized;
    std::vector< int >    m_lineArray;
    std::vector< int >    m_linePointers;
    std::vector< float >  m_pointArray;
    std::vector< float >  m_normalArray;
    bool                  m_normalsPositive;
    std::vector< int >    m_reverse;
    std::vector< bool >   m_selected;
    std::vector< bool >   m_filtered;
    std::vector< float >  m_length;
    int                   m_subsampledLines;
    float                 m_maxLength;
    float                 m_minLength;
    std::vector< float  > m_localizedAlpha;
    float                 m_cachedThreshold;
    bool                  m_fibersInverted;
    bool                  m_useFakeTubes;
    bool                  m_useTransparency;
    std::vector< float >  m_tractDirection;
    std::vector< float >  m_dispFactors;
    std::vector< float >  m_endPointsVector;

    bool                  m_isColorationUpdated;
    FibersColorationMode  m_fiberColorationMode;

    Octree                *m_pOctree;

    bool            m_cfDrawDirty;
    float           m_exponent;
	float           m_xAngle;
	float           m_yAngle;
	float           m_zAngle;
    float           m_lina;
    float           m_linb;
    float           m_cl;
	bool            m_axisView;
	bool			m_ModeOpac;
    bool            m_isAlphaFunc;
    bool            m_isLocalRendering;
    bool            m_usingEndpts;
    bool            m_axialShown;
    bool            m_coronalShown;
    bool            m_sagittalShown;
    bool            m_useIntersectedFibers;
    bool            m_useSliceFibers;
    float           m_thickness;
    float           m_tubeRadius;
    float           m_xDrawn;
    float           m_yDrawn;
    float           m_zDrawn;
    std::vector< unsigned int > m_cfStartOfLine;
    std::vector< unsigned int > m_cfPointsPerLine;
	vector<float> m_centroidPts;
	bool			m_showCentroids;
    
    wxColor         m_constantColor;

    // GUI members
    wxSlider       *m_pSliderFibersFilterMin;
    wxSlider       *m_pSliderFibersFilterMax;
    wxSlider       *m_pSliderFibersSampling;
    wxSlider       *m_pSliderInterFibersThickness;
    wxSlider       *m_pTubeRadius;

    wxSlider       *m_pSliderFibersAlpha;
    wxSlider       *m_pSliderFibersLina;
    wxSlider       *m_pSliderFibersLinb;
    wxSlider       *m_pSliderFibersPhi;
    wxSlider       *m_pSliderFibersTheta;
    wxSlider       *m_pSliderFiberscl;
    wxTextCtrl     *m_pTxtSamplingBox;
    wxTextCtrl     *m_pTxtAlphaBox;
    wxTextCtrl     *m_pTxtThetaBox;
    wxTextCtrl     *m_pTxtPhiBox;
    wxTextCtrl     *m_pTxtlina;
    wxTextCtrl     *m_pTxtlinb;
    wxTextCtrl     *m_pTxtclBox;

    wxButton       *m_pFitToAnat;
    wxToggleButton *m_pToggleLocalColoring;
    wxToggleButton *m_pToggleNormalColoring;
    wxButton       *m_pSelectConstantFibersColor;
	wxButton       *m_pShowCentroidPts;
    wxToggleButton *m_pToggleCrossingFibers;
    wxToggleButton *m_pToggleSliceFibers;
    wxRadioButton  *m_pRadNormalColoring;
    wxRadioButton  *m_pRadDistanceAnchoring;
    wxRadioButton  *m_pRadMinDistanceAnchoring;
    wxRadioButton  *m_pRadCurvature;
    wxRadioButton  *m_pRadTorsion;
    wxRadioButton  *m_pRadConstant;
    wxToggleButton  *m_pToggleAxisView;
	wxToggleButton  *m_pToggleModeOpac;
    wxToggleButton  *m_pToggleRenderFunc;
    wxToggleButton  *m_pToggleLocalGlobal;
    wxToggleButton  *m_pToggleEndpts;

	friend class QuickBundles;
};

#endif /* FIBERS_H_ */
