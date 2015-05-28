#ifndef __NJ_CONNECTION_H__
#define __NJ_CONNECTION_H__
#include "net.h"
#include <netinet/tcp.h>

#include <stdexcept>
#include <typeinfo>
#include <iostream>
#include <vector>

// nj_error for no good reason. ( actually, just to wrap errno )
class nj_error : public std::exception
{
public:
    //Just print errno default error message!
    nj_error( ) : err( strerror( errno ) )
    {
        if( errno == EBADF || errno == ENOTSOCK ) ignorable = false;
        if( errno == EPIPE ) ignorable = true;
        ignorable = true;
    };
    nj_error( const std::string &s ) : err( s ), ignorable( false ) { };
    nj_error( const std::string &s, bool i ) : err( s ), ignorable( i ) { };
   ~nj_error( ) throw( ) { };
    virtual const char * what( ) const throw( ) { return err.c_str( ); };
    bool isIgnorable( void ) { return ignorable; };
private:
    const std::string err;
    bool ignorable;
};

// Buffer type for internal tracking
// FIXME: Add >> and << operators for streaming?
// FIXME: Need to change the internals or something.
class njBuffer
{
public:
    njBuffer( ) : l(0), d(NULL) { };
    njBuffer( const std::string s ) : l(0), d(NULL) { fill( s ); };
    njBuffer( const njBuffer &b ) : l(0), d(NULL) { fill( b.l, b.d ); };
    njBuffer( const unsigned int Ll, const unsigned char* Dd ) : l(0), d(NULL)
    {
        fill( Ll, Dd );
    };
    ~njBuffer( )
    {
        if( d != NULL )
        {
            if( d != NULL ) delete[ ] d;
            d = NULL;
        }
    };
    void fill( const std::string s )
    {
        fill( s.length( ), (unsigned char*)s.c_str( ) );
    };
    void fill( const unsigned int Ll, const unsigned char* Dd )
    {
        if( l == 0 && d == NULL )
        {
            l = Ll;
            if( l > 0 && Dd != NULL )
            {
                d = new unsigned char[ l ];
                memcpy( d, Dd, l );
            }
        }
        else
        {
            if( d != NULL ) delete[ ] d;
            d = NULL;
            l = 0;
            fill( Ll, Dd );
        }
    };
    const std::string empty( )
    {
        if( l != 0 && d != NULL )
        {
            std::string s = std::string( (char*)d, l );
            if( d != NULL ) delete[ ] d;
            d = NULL;
            l = 0;
            return s;
        }
        else
        {
            std::string s;
            return s;
        }
    };
    bool isEmpty( ) { return l == 0 ? true : false; };
    
    inline unsigned int length( ) { return l; };
    inline unsigned char *data( ) { return d; };
    
    njBuffer& operator=( const njBuffer &b )
    {
        if( this == &b ) return *this;
        
        fill( b.l, b.d );
        return *this;
    }
private:
    unsigned int l;
    unsigned char *d;
};

/*******************************************************************************
 *
 *    Base for everything. Just a socket. Can't do much with it though.
 *
 ******************************************************************************/
class njSocket
{
public:
    njSocket( void ) : fd( -1 )
    {
        memset( address.sin_zero, '\0', sizeof address.sin_zero );
    };
    njSocket( const njSocket &s ) { fd = s.fd; };
    virtual ~njSocket( void )
    {
        fd = -1;
    };
    
    virtual void close( ) throw( nj_error ) = 0;
    
    //
    // Operators to make life easy with the function calls.
    //
    operator int() throw( ) { return fd; };
    
    njSocket& operator=(int t) throw( )
    {
        fd = t;
        return *this;
    };
    //
    // Assignment operator. Just in case. NOTE: Might need to deconstruct c...
    //
    virtual njSocket& operator=( const njSocket &c ) throw( )
    {
        if( &c != this ) fd = c.fd;
        return *this;
    };
    inline void setNonBlocking( bool y ) throw( nj_error )
    {
        int flags;
        flags = fcntl_( fd, F_GETFL, 0 );
        
        if( y ) flags |=  O_NONBLOCK;
        else    flags &= ~O_NONBLOCK;
        
        flags = fcntl_( fd, F_SETFL, flags );
    };
protected:
    void open_socket( int, int, int ) throw( nj_error );
    void set_socket_address( int, int ) throw( ); 
    inline int fcntl_( int f, int fl, int fll ) throw( nj_error )
    {
        fll = fcntl( f, fl, fll );
        if( fll == -1 ) throw nj_error( );
        return fll;
    };
    
    int fd;
    struct sockaddr_in address;
    struct sockaddr_in paddress;
};

/*******************************************************************************
 *
 *            njListener - Abstract Base for TCP and UDP Listeners
 *                    To accept incoming njConnections
 *
 ******************************************************************************/
class njConnection;
class njListener : public njSocket
{
public:
    njListener( void ) { };
    virtual ~njListener( void ) { };
    virtual void listenOnPort( int ) throw( nj_error ) = 0;
    virtual njConnection * const acceptIncoming( ) throw( nj_error ) = 0;
    void setBackLogSize( int n ) throw( ) { backlog = n; };
protected:
    int backlog;
};
/*******************************************************************************
 *
 *          njTCPListener -
 *
 ******************************************************************************/
class njTCPListener : public njListener
{
public:
    njTCPListener( void ) { };
    virtual ~njTCPListener( void ) { };
    virtual void close( ) throw( nj_error );
    virtual void listenOnPort( int ) throw( nj_error );
    virtual njConnection * const acceptIncoming( ) throw( nj_error );
};

class njUDPListener : public njListener
{
public:
    njUDPListener( void ) { };
    virtual ~njUDPListener( void ) { };
    virtual void close( ) throw( nj_error );
    virtual void listenOnPort( int ) throw( nj_error );
    virtual njConnection * const acceptIncoming( ) throw( nj_error );
};

/*******************************************************************************
 *
 *            njConnection - Abstract Base for TCP and UDP Connections
 *
 ******************************************************************************/
class njConnection : public njSocket
{
public:
    njConnection( void ) : pl( 0 ) { };
    virtual ~njConnection( void ) { };
//
// The methods!
//
public:

    //
    // Methods to create connections
    //
    virtual void connectTo( std::string, int ) throw( nj_error ) = 0;
    
    //
    // Send and Recieve data
    //
    virtual njBuffer* const Recv( void ) throw( nj_error ) = 0;
    virtual void Send( njBuffer* ) throw( nj_error ) = 0;
    
    //
    // So you can check the buffers
    //
    njBuffer readSendBuffer( ) throw( ) { return sendbuffer; };
    njBuffer readRecvBuffer( ) throw( ) { return recvbuffer; };
	
    //
    // Set up for customizing the internals
    //
    void setSendBuffer( njBuffer b ) throw( nj_error ){ sendbuffer = b; };
         // Why you would want to set the recv buffer, I don't know.
         // Just here for consistancy's sake.
    void setRecvBuffer( njBuffer b ) throw( nj_error ){ recvbuffer = b; };
	void setPacketLength( int n ) throw( ) { pl = n; };

    using njSocket::operator=;
//
// Low level wraps
//
protected:

    inline void recv_( int s, void*b, size_t l, int f ) throw( nj_error )
    {
        int i = recv( s, b, l, f );
        if(i == 0)
        {
            throw nj_error( );
        }
        else if(i < 0)
        {
            throw nj_error( );
        }
    };
    inline void send_( int s, const void* b, size_t l, int f ) throw( nj_error )
    {
        int i = send( s, b, l, f );
        if( i < 0 )
        {
            throw nj_error( );
        }
    };
    const struct in_addr *resolve_host(char *) throw( nj_error );
    
//
// All the bones.
//
protected:

    unsigned int pl;
    
    njBuffer recvbuffer;
    njBuffer sendbuffer;
    
};



class njTCPConnection : public njConnection
{
public:
    njTCPConnection( );
    ~njTCPConnection( );
    virtual void close( ) throw( nj_error );
    
    virtual void connectTo( std::string, int ) throw( nj_error );
    virtual njBuffer* const Recv( void ) throw( nj_error );
    virtual void Send( njBuffer* ) throw( nj_error );
};

class njUDPConnection : public njConnection
{
public:
    njUDPConnection( );
    ~njUDPConnection( );
    virtual void close( ) throw( nj_error );
    
    virtual void connectTo( std::string, int ) throw( nj_error );
    virtual njBuffer* const Recv( void ) throw( nj_error );
    virtual void Send( njBuffer* ) throw( nj_error );
};

#endif //__NJ_CONNECTION_H__

