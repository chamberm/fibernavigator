#ifndef CONNECTOMEHELPER_H_
#define CONNECTOMEHELPER_H_

#include "Connectome.h"
#include <wx/grid.h>

class ConnectomeHelper
{
public:
    ~ConnectomeHelper();

    static ConnectomeHelper * getInstance();
    bool isLabelsSelected() const { return m_isLabelsSelected; }
    void setLabelsSelected( bool selected )   { m_isLabelsSelected = selected; }
    bool isEdgesSelected() const { return m_isEdgesSelected; }
    void setEdgesSelected( bool selected )   { m_isEdgesSelected = selected; }
  
    void setLabelsReady( bool ready )   { m_isLabelsReady = ready; }
    void setEdgesReady( bool ready )   { m_isEdgesReady = ready; }
    bool isLabelsReady() const { return m_isLabelsReady;}
    bool isEdgesReady() const { return m_isEdgesReady;}
    bool isReady() const { return m_isLabelsReady && m_isEdgesReady; }
    
    bool isDirty() const     { return m_isDirty; }
    void setDirty( bool dirty )          { m_isDirty = dirty; }
    
    Connectome* getConnectome() { return m_Connectome; }
    void createConnectome(); 
    void deleteConnectome();

    wxGrid              *m_pGridNodeInfo;
    
    
protected:
    ConnectomeHelper(void);

private:
    ConnectomeHelper( const ConnectomeHelper & );
    ConnectomeHelper &operator=( const ConnectomeHelper &);
    

private:
    static ConnectomeHelper * m_pInstance;
    Connectome *m_Connectome;
    bool m_isLabelsSelected;
    bool m_isEdgesSelected;
    bool m_isLabelsReady;
    bool m_isEdgesReady;
    bool m_isDirty;

    

};

#endif //CONNECTOMEHELPER_H_