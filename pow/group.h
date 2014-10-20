
#ifndef _PWR_GROUP_H
#define _PWR_GROUP_H

#include <vector>
#include <string>

#include "status.h"
#include "types.h"
#include "debug.h"
#include "objTreeNode.h"

namespace PowerAPI {

class Cntxt;

class Grp {
  public:
    Grp( Cntxt* ctx, const std::string name ="" ) 
		: m_ctx(ctx), m_name(name) {}

    long size() { return m_list.size(); }
    bool empty() { return m_list.empty(); }

    ObjTreeNode* getObj( int i ) { return m_list[i]; }

    int add( ObjTreeNode* obj ) {
        m_list.push_back( obj );
        return PWR_RET_SUCCESS; 
    }

    const std::string& name() {
        return m_name;
    }
    
    int attrSetValue( PWR_AttrName type, void* ptr, size_t len, Status* status )
	{
        for ( unsigned int i = 0; i < m_list.size(); i++ ) {
            int ret = m_list[i]->attrSetValue( type, ptr, len );
            if ( PWR_RET_SUCCESS != ret ) {
                status->add( m_list[i], type, ret );
            }
        }
        if ( !status->empty() ) {
            return PWR_RET_FAILURE;
        } else {
            return PWR_RET_SUCCESS;
        }
    }

    int attrSetValues( int num, PWR_AttrName attr[], void* buf, Status* status )
    {
        for ( unsigned int i = 0; i < m_list.size(); i++ ) {

            std::vector<PWR_AttrName> attrsV(num);
            std::vector<int>          statusV(num);
        
            if ( PWR_RET_SUCCESS !=
						m_list[i]->attrSetValues( attrsV, buf, statusV ) ) 
			{
                for ( int j = 0; j < num; j++ ) {
                    if ( PWR_RET_SUCCESS != statusV[j] ) {
                        status->add( m_list[i], attrsV[j], statusV[j] );
                    }
                }
            }
        }
        if ( !status->empty() ) {
            return PWR_RET_FAILURE;
        } else {
            return PWR_RET_SUCCESS;
        }
    }

    int attrGetValues( int num, PWR_AttrName attr[], void* buf,
                                   		PWR_Time ts[], Status* status)
    {
        uint64_t* ptr = (uint64_t*) buf;
        for ( unsigned int i = 0; i < m_list.size(); i++ ) {

            std::vector<PWR_AttrName> attrsV(num);
            std::vector<PWR_Time> tsV(num);
            std::vector<int>          statusV(num);

            for ( int j = 0; j < num; j++ ) {
                attrsV[j] =  attr[ i * num + j ];
            }

            if ( PWR_RET_SUCCESS != m_list[i]->attrGetValues( attrsV, 
                        				ptr + i * num, tsV, statusV ) ) 
			{
                for ( int j = 0; j < num; j++ ) {
                    if ( PWR_RET_SUCCESS != statusV[j] ) {
                        status->add( m_list[i], attrsV[j], statusV[j] );
                    }
                }
            }
            for ( int j = 0; j < num; j++ ) {
                ts[ i * num + j ] = tsV[j];
            }
        }

        if ( !status->empty() ) {
            return PWR_RET_FAILURE;
        } else {
            return PWR_RET_SUCCESS;
        }
    }

    int remove( ObjTreeNode* obj ) {
        std::vector<ObjTreeNode*>::iterator iter = m_list.begin();
        for ( ; iter != m_list.end(); ++iter ) {
            if ( *iter == obj ) {
                m_list.erase( iter );
                break;
            }
        }
        return PWR_RET_SUCCESS;
    }

    ObjTreeNode* find( std::string name ) {
        DBGX("%s\n", name.c_str());
        
        for ( unsigned int i = 0; i < m_list.size(); i++ ) {
            ObjTreeNode* obj = m_list[i];
            if ( obj->parent() ) {
                std::string tmp = m_list[i]->parent()->name() + "." + name;
                DBGX("%s %s\n",tmp.c_str(), obj->name().c_str() );
                if ( 0 == tmp.compare( obj->name() ) ) { 
                    return obj;
                }
            }
        }
        return NULL;
    }

    Cntxt* getCtx() { return m_ctx; }

  private:
    Cntxt*   					m_ctx;
    std::string 				m_name;
    std::vector<ObjTreeNode*> 	m_list;
};

}

#endif