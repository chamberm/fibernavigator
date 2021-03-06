#ifndef DATASETINFO_H_
#define DATASETINFO_H_

#include "DatasetIndex.h"

#include "../misc/Algorithms/Helper.h"
#include "../gui/SceneObject.h"

#include <GL/glew.h>
#include <wx/colour.h>
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

union converterByteINT16 
{
    wxUint8 b[2];
    wxUint16 i;
};

union converterByteINT32 
{
    wxUint8 b[4];
    wxUint32 i;
};

union converterByteFloat 
{
    wxUint8 b[4];
    float f;
};

class MySlider;
class PropertiesWindow;
class TriangleMesh;
class wxBitmapButton;
class wxButton;
class wxStaticText;
class wxTextCtrl;
class wxToggleButton;
class wxXmlNode;
class wxXmlProperty;

class DatasetInfo : public SceneObject
{
public:
    DatasetInfo();
    virtual ~DatasetInfo();

    // Pure Virtual functions
    virtual void   draw()                       = 0;
    virtual void   smooth()                     = 0;
    virtual void   flipAxis( AxisType i_axe )   = 0;
    virtual GLuint getGLuint()                  = 0;

    // Virtual functions
    virtual void createPropertiesSizer(PropertiesWindow *parent); 
    virtual void updatePropertiesSizer();
    virtual bool save( wxString filename ) const { return false; }
    virtual bool save( wxXmlNode *node, const wxString &rootPath ) const;

    // Functions
    wxString getName() const                     { return m_name;               };
    wxString getPath() const                     { return m_fullPath;           };
    void     setName(wxString name)              { m_name = name;               };
    int      getType() const                     { return m_type;               };
    void     setType(int type)                   { m_type = type;               };
    float    getHighestValue() const             { return m_highest_value;      };
    void     setHighestValue(float value)        { m_highest_value = value;     };
    float    getThreshold() const                { return m_threshold;          };
    float    getOldMax() const                   { return m_oldMax;             };
    void     setOldMax(float v)                  { m_oldMax = v;                };
	float    getMin() const                      { return m_dataMin;             };
    void     setMin(float v)                     { m_dataMin = v;                };
    void     setNewMax(float v)                  { m_newMax = v;                };
    float    getNewMax() const                   { return m_newMax;             };
    void     setThreshold(float value)           { m_threshold = value;         };
    float    getAlpha() const                    { return m_alpha;              };
    void     setAlpha(float v)                   { m_alpha = v;                 };
	float    getDotThresh() const                { return m_dot;              };
    void     setDotThresh(float v)               { m_dot = v;                 };
	float    getEdgeOpThresh() const                { return m_edgeOp;              };
    void     setEdgeOpThresh(float v)               { m_edgeOp = v;                 };
    float    getBrightness() const               { return m_brightness;         };
    void     setBrightness( float i_brightness ) { m_brightness = i_brightness; };

    int      getLength() const                   { return m_length;   };
    int      getBands() const                    { return m_bands;    };
    int      getFrames() const                   { return m_frames;   };
    int      getRows() const                     { return m_rows;     };
    int      getColumns() const                  { return m_columns;  };

    wxString getRpn() const                      { return m_repn; };

    virtual bool     toggleShow()                        { m_show        = !m_show;          return m_show;        };
    bool     toggleShowFS()                      { m_showFS      = !m_showFS;        return m_showFS;      };
    bool     toggleUseTex()                      { m_useTex      = !m_useTex;        return m_useTex;      };
	bool     toggleShowHalo()					 { m_showHalo    = !m_showHalo;      return m_showHalo;      };

    void     setShow       ( bool i_show   )     { m_show        = i_show;   };
    void     setShowFS     ( bool i_show   )     { m_showFS      = i_show;   };
    void     setUseTex     ( bool i_useTex )     { m_useTex      = i_useTex; };
    bool     getShow() const                     { return m_show;            };
    bool     getShowFS() const                   { return m_showFS;          };
	bool     getShowHalo() const                 { return m_showHalo;          };
    bool     getUseTex() const                   { return m_useTex;          };
    bool     getIsGlyph() const                  { return m_isGlyph;         };

    void     setGLuint( GLuint value )           { m_GLuint = value; };
    void     setColor ( wxColour color )         { m_color  = color; };
    wxColour getColor() const                    { return m_color;   };

    float getVoxelSizeX() const                    { return m_voxelSizeX; }
    float getVoxelSizeY() const                    { return m_voxelSizeY; }
    float getVoxelSizeZ() const                    { return m_voxelSizeZ; }
    
    void setDatasetIndex(const DatasetIndex &dsIndex) { m_dsIndex = dsIndex; }
    DatasetIndex getDatasetIndex() const              { return m_dsIndex; }

public:
    // Trianglemesh
    TriangleMesh    *m_tMesh;

    wxTextCtrl      *m_pTxtName;
    MySlider        *m_pSliderThresholdIntensity;
    MySlider        *m_pSliderOpacity;
    wxToggleButton  *m_pToggleVisibility;
    wxToggleButton  *m_pToggleFiltering;
    wxBitmapButton  *m_pBtnDelete;
    wxBitmapButton  *m_pBtnUp;
    wxBitmapButton  *m_pBtnDown;
    wxButton        *m_pbtnSmoothLoop;
    wxButton        *m_pbtnClean;
    wxButton        *m_pBtnRename;
    wxToggleButton  *m_pBtnFlipX;
    wxToggleButton  *m_pBtnFlipY;
    wxToggleButton  *m_pBtnFlipZ;
    wxStaticText    *m_pIntensityText;
    wxStaticText    *m_pOpacityText;

protected:
    virtual void generateTexture()  = 0;
    virtual void generateGeometry() = 0;
    virtual void initializeBuffer() = 0;
    void swap( DatasetInfo &d );

protected:
    int         m_length;
    int         m_bands;
    int         m_frames;
    int         m_rows;
    int         m_columns;
    int         m_type;
    wxString    m_repn;
    bool        m_isLoaded;
    float       m_highest_value;
    wxString    m_name;
    wxString    m_fullPath;
    float       m_threshold;
    float       m_alpha;
	float       m_dot;
	float       m_edgeOp;
    float       m_brightness;
    float       m_oldMax;
	float       m_dataMin;
    float       m_newMax;
	float       m_eqMax;
	float       m_eqMin;

    wxColour    m_color;
    GLuint      m_GLuint;

    bool        m_show;
    bool        m_showFS;       // Show front sector for meshs.
    bool        m_useTex;        // Color mesh with textures loaded.
	bool        m_showHalo;
    
    // If false use colormap on threshold value.
    bool        m_isGlyph;
    GLuint*     m_bufferObjects;

    float       m_voxelSizeX;
    float       m_voxelSizeY;
    float       m_voxelSizeZ;
    
private:
    DatasetIndex m_dsIndex;
};

#endif /*DATASETINFO_H_*/
