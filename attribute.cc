#include <assert.h>
#include "./attribute.h"
#include "./util.h"
#include "./object.h"
#include "./attributeSrc.h"
#include "./debug.h"

using namespace tinyxml2;


_Attr::_Attr( _Obj* obj, tinyxml2::XMLElement* el  ) :
    m_obj( obj ),
    m_xml( el ),
    m_srcList( NULL )
{
    m_type = attrTypeStrToInt( el->Attribute("name") );
    assert( PWR_ATTR_INVALID != m_type );

    DBGX("name=`%s` op=`%s`\n",
                    m_xml->Attribute("name"),m_xml->Attribute("op"));

    m_name = m_xml->Attribute("name");
    m_dataType = attrTypeToDataType( m_type );

    switch ( m_dataType ) {
      case PWR_ATTR_DATA_FLOAT:
        m_len = sizeof(float);
        break;
      case PWR_ATTR_DATA_INT:
        m_len = sizeof(int);
        break;
      case PWR_ATTR_DATA_STRING:
        m_len = 1024;
        break;
      case PWR_ATTR_DATA_INVALID:
        break;
    }

    if ( 0 != strcmp( m_xml->Attribute("op"), "INVALID" ) ) {
        m_srcList = initSrcList( m_xml );
    }
}

int _Attr::getValue( void* ptr, size_t len ) 
{
    DBGX("%s %s\n",m_obj->name().c_str(), m_name.c_str());

    if ( ! m_srcList ) {
        return PWR_ERR_INVALID;
    } 

    srcList_t::iterator iter = m_srcList->begin();

    for ( ; iter != m_srcList->end(); ++iter ) {
        //DBGX("\n");
        (*iter)->get( ptr, len ); 
    }

    return PWR_ERR_SUCCESS;
}

int _Attr::setValue( void* ptr, size_t len )
{
    if ( ! m_srcList ) {
        return PWR_ERR_INVALID;
    } 

    return PWR_ERR_SUCCESS;
}

_Attr::srcList_t* _Attr::initSrcList( tinyxml2::XMLElement* el )
{
    DBGX("%s name=`%s` op=`%s`\n", m_obj->name().c_str(),
                    m_xml->Attribute("name"),m_xml->Attribute("op"));
    XMLNode* tmp = el->FirstChild();

    srcList_t* list = new srcList_t;

    // find the attributes element
    while ( tmp ) {
        el = static_cast<XMLElement*>(tmp);

        DBGX("attr src type=`%s` name=`%s`\n",
                el->Attribute("type"), el->Attribute("name"));

        if ( 0 == strcmp( "child", el->Attribute("type" ) ) ) {

            _Obj* obj = m_obj->findChild( el->Attribute("name") );
            
            DBGX("%s %p %s\n",m_obj->name().c_str(),obj,obj->name().c_str());
            assert( obj );
            list->push_back( new ChildSrc( m_type, obj ) ); 
                                

        } else if ( 0 == strcmp( "plugin", el->Attribute("type" ) ) )  {
            DBGX("\n");
            list->push_back( new DevSrc( m_type,  NULL, "" ) );
        }
        tmp = tmp->NextSibling();
    }
    DBGX("return\n");

    return list;
}
