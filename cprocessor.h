#ifndef __NJ_CONNECTIONPROCESSOR_H__
#define __NJ_CONNECTIONPROCESSOR_H__
#include "njmultiplex.h"

class njConnectionProcessor
{
public:
    njConnectionProcessor( ) throw( nj_error ) : m( njMultiplex::instance( ) )
    { m->registerConnectionProcessor( &this ); };
   ~njConnectionProcessor( ) { };
    virtual void outgoing( njConnection *, njBuffer * ) throw( nj_error ) =0;
    virtual void incoming( njConnection *, njBuffer * ) throw( nj_error ) =0;
    virtual void except( ) throw( nj_error ) =0;
    
    virtual njConnection* connectionBirth( njConnection * const c )
            throw( nj_error ) { return c; };
    virtual void connectionDeath( const njConnection &c )
            throw( nj_error ) =0;
    njMultiplex *m;
};

#endif //__NJ_CONNECTIONPROCESSOR_H__

