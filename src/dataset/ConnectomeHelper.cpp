#include "ConnectomeHelper.h"

#include <stdlib.h>

ConnectomeHelper * ConnectomeHelper::m_pInstance = NULL;

ConnectomeHelper::ConnectomeHelper():   
m_isLabelsSelected( false ),
m_isEdgesSelected( false ),
m_isLabelsReady( false ),
m_isEdgesReady( false ),
m_isDirty( true ),
m_isShowStreamlines(false),
m_isLabelNamesLoaded(false),
m_Connectome( NULL )
{

}

ConnectomeHelper * ConnectomeHelper::getInstance()
{
    if( NULL == m_pInstance )
    {
        m_pInstance = new ConnectomeHelper();
    }
    return m_pInstance;
}

void ConnectomeHelper::createConnectome()
{
    if(m_Connectome == NULL)
        m_Connectome = new Connectome();
}

void ConnectomeHelper::deleteConnectome()
{
    m_Connectome->clearConnectome();
    delete m_Connectome;
}
ConnectomeHelper::~ConnectomeHelper()
{
    m_pInstance = NULL;
    delete m_Connectome;
}
