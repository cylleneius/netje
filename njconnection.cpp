#include "njconnection.h"

using namespace std;

void njSocket::open_socket( int d, int t, int p ) throw( nj_error )
{
    if( ( fd = socket( d, t, p ) ) == -1 )
    {
        throw nj_error( );
    }
}

void njSocket::set_socket_address( int p, int a ) throw( )
{
    address.sin_family      = PF_INET;
    address.sin_port        = htons( p );
    address.sin_addr.s_addr = a;
}
void njTCPListener::close( ) throw( nj_error )
{
    
}
void njTCPListener::listenOnPort( int port ) throw( nj_error )
{
    if( fd == -1 ) open_socket(PF_INET, SOCK_STREAM, 0);
    
    set_socket_address( port, INADDR_ANY );
    try
    {
        int yes = 1;
        //FIXME: Perhaps, something like a local variable to set for this, eh?
        int r = setsockopt( fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof( int ) );
        //FIXME: check r value...
        r = 0;
        //
        if( bind( fd, (struct sockaddr *)&address, sizeof( sockaddr ) ) == -1 )
        {
            throw nj_error( );
        }
        if(listen(fd, backlog) == -1)
        {
            throw nj_error( );
        }
        
    }
    catch( nj_error &e )
    {
        if( !e.isIgnorable( ) ) throw;
        //else listener = true;
    }
    //FIXME: Option to turn this shit off! Maybe throw an (ignorable) exception?
    cout << getpid() << ": njTCPListener::listenOn() : Now Listening on port "
         << port << endl;
}


njConnection * const njTCPListener::acceptIncoming( ) throw( nj_error )
{
        sockaddr_in their_addr;
        int len = sizeof(sockaddr_in);
        njConnection *cc = new njTCPConnection( );
        if( (*cc = accept( (*this), (sockaddr*)&their_addr, (socklen_t*)&len ) )
            == -1 )
        {
            throw nj_error( );
        }
        int id = *cc;
        int v1 = 10;
        int v2 = 3;
        int y = 1;
        setsockopt( id, SOL_SOCKET, SO_KEEPALIVE, &y, sizeof(int) );
        setsockopt( id, SOL_TCP, TCP_KEEPIDLE, &v2, sizeof(int) );
        setsockopt( id, SOL_TCP, TCP_KEEPCNT, &v1, sizeof(int) );
        setsockopt( id, SOL_TCP, TCP_KEEPINTVL, &v2, sizeof(int) );
        return cc;
}

void njUDPListener::close( ) throw( nj_error )
{
    
}

void njUDPListener::listenOnPort( int port ) throw( nj_error )
{
    if( fd == -1 ) open_socket(PF_INET, SOCK_DGRAM, 0);
    
    set_socket_address( port, INADDR_ANY );
    try
    {
        int yes = 1;
        //FIXME: Perhaps, something like a local variable to set for this, eh?
        int r = setsockopt( fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof( int ) );
        //FIXME: check r value...
        r = 0;
        //
        if( bind( fd, (struct sockaddr *)&address, sizeof( sockaddr ) ) == -1 )
        {
            throw nj_error( );
        }
    }
    catch( nj_error &e )
    {
        if( !e.isIgnorable( ) ) throw;
        //else listener = true;
    }
    //FIXME: Option to turn this shit off! Maybe throw an (ignorable) exception?
    cout << getpid() << ": njUDPListener::listenOn() : Now Listening on port "
         << port << endl;
}


njConnection * const njUDPListener::acceptIncoming( ) throw( nj_error )
{
        sockaddr_in their_addr;
        socklen_t len = sizeof(sockaddr_in);
        njConnection *cc = new njUDPConnection( );
        //
        // We'll use one byte
        //
        char buf;
        recvfrom( fd, &buf, 1, 0, (sockaddr*)&their_addr, &len );
        cc->connectTo( inet_ntoa(their_addr.sin_addr), their_addr.sin_port );

        return cc;
}

const struct in_addr *njConnection::resolve_host( char *name )
throw( nj_error )
{
    struct hostent *rt = NULL;
    //
    //POSIX Compliant code would probably have this:
    //
    //int error_num;
    //getipnodebyname("www.google.com", PF_INET, 0, &error_num);
    //
    //However, I don't have the headers for it.
    //
    if( ( rt = gethostbyname( name ) ) == NULL )
    {
        string err;
        string n = name;
        switch(h_errno)
        {
            case HOST_NOT_FOUND:
                err = "Host \"" + n + "\" was not found.";
                throw nj_error( err, true );
            break;
            case NO_ADDRESS:
                err = "Valid hostname \"" + n + "\", but no IP data.";
                throw nj_error( err, true );
            break;
            case NO_RECOVERY:
                err = "Non-recoverable name server error.";
                throw nj_error( err );
            break;
            case TRY_AGAIN:
                err = "Temporary error occurred, try again.";
                throw nj_error( err, true );
            break;
         }
         //Not really needed but whatever.
         return NULL;
    }
	
    // FIXME: Need to put in some handling for multiple IP addresses.
    memcpy( &paddress.sin_addr, rt->h_addr_list[0], sizeof( struct in_addr ) );
    //rr = new in_addr(*(struct in_addr*)rt->h_addr_list[0]);
    //if( rt != NULL ) delete rt;
    return &paddress.sin_addr;
}

njTCPConnection::njTCPConnection( ) { }
njTCPConnection::~njTCPConnection( ) { }
void njTCPConnection::close( ) throw( nj_error )
{
    //FIXME: make this close connection
}
void njTCPConnection::connectTo( string add, int port ) throw( nj_error )
{
    if( fd == -1 ) open_socket( PF_INET, SOCK_STREAM, 0 );
    
    set_socket_address( port, resolve_host( (char*)add.c_str( ) )->s_addr );
    
    if( connect( fd, (struct sockaddr *)&address, sizeof( sockaddr ) ) == -1 )
    {
        throw nj_error( );
    }
}

njBuffer* const njTCPConnection::Recv( void ) throw( nj_error )
{
    //Convenience union for converting packet length to two bytes.
    union
    {
        unsigned short int l;
        unsigned char cc[2];
    } len;
    
    len.l = pl;
    unsigned char *buf;
    // Recieving Packets, if we don't know the length ( pl = 0 ) then
    // we have to get the first two bytes to find it out.
    
    //FIXME: need better handling here... Like getting rid of this temp work-
    // around.
    if( recv( fd, len.cc, 1, MSG_PEEK ) > 0)
    {
        if( len.l == 0 )
        {
            recv_( fd, len.cc, 2, 0 );
            //Convert recieved length to host byte order!
            len.l = ntohs( len.l );
        }
        //Our temporary buffer
        buf = new unsigned char[ len.l ];
        //FIXME: need to check if entire length was read.
        recv_(fd, buf, len.l, 0 );
        
        //Fill in our connection buffer with our temporary buffer.
        recvbuffer.fill( len.l, buf );
        //Get rid of our temporary buffer.
        delete[ ] buf;
        //Return connection buffer.
        return &recvbuffer;
    }
    return NULL;
}

void njTCPConnection::Send( njBuffer *b ) throw( nj_error )
{
    //Just so we can set the sendbuffer for later sending.
    if( b != NULL ) sendbuffer = *b;
    
    if( sendbuffer.length( ) != 0 )
    {
        //Convenience union for converting packet length to two bytes.
        union
        {
            unsigned short int l;
            unsigned char cc[2];
        } len;
        //More convenience variables...
        unsigned int length = sendbuffer.length( );
        unsigned char* data = sendbuffer.data( );
        //Temp packet length, so we can have variable sized packets.
        unsigned int tpl = pl;
        int pos = 0;
        //Convert outgoing length to network byte order.
        len.l = htons(length);
        if( tpl == 0 )
        {
            send_( fd, len.cc, 2, 0 );
            tpl = length;
        }
        //This is our send loop, needed if we have set packet sizes.
        do
        {
            send_( fd, (data + pos), tpl, 0 );
            length -= tpl;
            pos += tpl;
        } while( length > tpl );
        //Get what's left over...
        if( length != 0 )
        {
            send_( fd, data+pos, length, 0 );
        }
        //
        sendbuffer.empty( );
    }
}

njUDPConnection::njUDPConnection( ) { };
njUDPConnection::~njUDPConnection( ) { };
void njUDPConnection::close( ) throw( nj_error )
{
    //FIXME: make this close connection
}
void njUDPConnection::connectTo( string add, int port ) throw( nj_error )
{
    if( fd == -1 ) open_socket( PF_INET, SOCK_DGRAM, 0 );

    set_socket_address( port, resolve_host( (char*)add.c_str( ) )->s_addr );
    
    if( connect( fd, (struct sockaddr *)&address, sizeof( sockaddr ) ) == -1 )
    {
        throw nj_error( );
    }
}

njBuffer* const njUDPConnection::Recv( void ) throw( nj_error )
{
    //Convenience union for converting packet length to two bytes.
    union
    {
        unsigned short int l;
        unsigned char cc[2];
    } len;
    
    len.l = pl;
    unsigned char *buf;
    // Recieving Packets, if we don't know the length ( pl = 0 ) then
    // we have to get the first two bytes to find it out.
    
    //FIXME: need better handling here... Like getting rid of this temp work-
    // around.
    if( recv( fd, len.cc, 1, MSG_PEEK ) > 0)
    {
        if( len.l == 0 )
        {
            recv_( fd, len.cc, 2, 0 );
            //Convert recieved length to host byte order!
            len.l = ntohs( len.l );
        }
        //Our temporary buffer
        buf = new unsigned char[ len.l ];
        //FIXME: need to check if entire length was read.
        recv_(fd, buf, len.l, 0 );
        
        //Fill in our connection buffer with our temporary buffer.
        recvbuffer.fill( len.l, buf );
        //Get rid of our temporary buffer.
        delete[ ] buf;
        //Return connection buffer.
        return &recvbuffer;
    }
    return NULL;
}

void njUDPConnection::Send( njBuffer *b ) throw( nj_error )
{
    //Just so we can set the sendbuffer for later sending.
    if( b != NULL ) sendbuffer = *b;
    
    if( sendbuffer.length( ) != 0 )
    {
        //Convenience union for converting packet length to two bytes.
        union
        {
            unsigned short int l;
            unsigned char cc[2];
        } len;
        //More convenience variables...
        unsigned int length = sendbuffer.length( );
        unsigned char* data = sendbuffer.data( );
        //Temp packet length, so we can have variable sized packets.
        unsigned int tpl = pl;
        int pos = 0;
        //Convert outgoing length to network byte order.
        len.l = htons(length);
        if( tpl == 0 )
        {
            send_( fd, len.cc, 2, 0 );
            tpl = length;
        }
        //This is our send loop, needed if we have set packet sizes.
        do
        {
            send_( fd, (data + pos), tpl, 0 );
            length -= tpl;
            pos += tpl;
        } while( length > tpl );
        //Get what's left over...
        if( length != 0 )
        {
            send_( fd, data+pos, length, 0 );
        }
        //
        sendbuffer.empty( );
    }
}
