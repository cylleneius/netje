#ifndef __NJ_CPROCESSOR_H__
#define __NJ_CPROCESSOR_H__

class njConnectionProcessor
{
protected:
    virtual void outgoing( njConnection *, njBuffer * ) throw( nj_error ) =0;
    virtual void incoming( njConnection *, njBuffer * ) throw( nj_error ) =0;
    virtual void except( ) throw( nj_error ) =0;
    
    virtual njConnection* connectionBirth( njConnection * const c )
            throw( nj_error ) { return c; };
    virtual void connectionDeath( njConnection * const )
            throw( nj_error ) =0;
    friend class njMultiplex;
};

#endif //__NJ_CPROCESSOR_H__

