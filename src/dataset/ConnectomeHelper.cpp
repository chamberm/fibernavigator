#include "ConnectomeHelper.h"

#include <stdlib.h>

ConnectomeHelper * ConnectomeHelper::m_pInstance = NULL;

ConnectomeHelper::ConnectomeHelper():   
m_isLabelsSelected( false ),
m_isEdgesSelected( false ),
m_isLabelsReady( false ),
m_isEdgesReady( false ),
m_isDirty( true ),
m_Nodecolor( wxColour( 255, 0, 0 ) )
{
    m_Connectome = new Connectome();
}

ConnectomeHelper * ConnectomeHelper::getInstance()
{
    if( NULL == m_pInstance )
    {
        m_pInstance = new ConnectomeHelper();
    }
    return m_pInstance;
}


ConnectomeHelper::~ConnectomeHelper()
{
    m_pInstance = NULL;
    delete m_Connectome;
}
