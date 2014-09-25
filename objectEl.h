#ifndef _PWR_OBJECTEL_H 
#define _PWR_OBJECTEL_H

#include <map>

#include "tinyxml2.h"
#include "object.h"

struct _ObjEl : public _Obj
{
  public:
    _ObjEl( _Cntxt* ctx, _Obj* parent, tinyxml2::XMLElement* el );
    ~_ObjEl();
    PWR_ObjType type() { return m_type; }
    _Grp* children();

    int attrGetNumber() { return m_attrVector.size(); }
    _Attr* attributeGet( int index ) { return m_attrVector[index]; }
    int attrIsValid( PWR_AttrName type ) { return 0; }

    _Obj* findChild( const std::string name );
    GraphNode* findDev( const std::string name );

    int attrGetValue( PWR_AttrName, void*, size_t, PWR_Time* );
    int attrSetValue( PWR_AttrName, void*, size_t );

    int attrGetValues( const std::vector<PWR_AttrName>& types,
            void* ptr, std::vector<PWR_Time>& ts, std::vector<int>& status );
    int attrSetValues(  const std::vector<PWR_AttrName>& types,
            void* ptr, std::vector<int>& status );
  private:
    PWR_ObjType m_type;
    _Grp*       m_children;

    std::map< std::string, GraphNode* >       m_devices;

    tinyxml2::XMLElement*   m_xmlElement;
    std::vector< _Attr* >   m_attrVector;
};

inline _Obj* createObj( _Cntxt* ctx, _Obj* parent,
										tinyxml2::XMLElement* el )
{
	return rosebud( new _ObjEl( ctx, parent, el ) );
}

#endif
