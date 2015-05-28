#include "njmultiplex.h"

njMultiplex::~njMultiplex()
{
    close( epfd );
}

njMultiplex::njMultiplex()
{
    nfds = 0;
    //FIXME: need size changeable
    epfd = epoll_create( 10 );
    if( epfd == -1 ) throw nj_error( );
}

njMultiplex* njMultiplex::instance_ = NULL;
njMultiplex* njMultiplex::instance()
{
    if(instance_ == NULL)
    {
        instance_ = new njMultiplex();
    }
    return instance_;
}

pthread_t njMultiplex::loop()
{
	pthread_t loopt;
    cout << getpid() << ": njMultiplex - Setting up loop..." << endl;
	if ( pthread_create( &loopt, NULL, njMultiplex::instance()->loop_, NULL) )
	{
		cout << "Oops!" << endl;
		abort();
	}
    cout << getpid() << ": njMultiplex - Set loop up..." << endl;
	return loopt;
}

void* njMultiplex::loop_(void *arg)
{
	njMultiplex *us = njMultiplex::instance();
    cout << getpid() << ": njMultiplex - Entering loop..." << endl;
    for(;;)
    {
        us->pump(NULL);
    }
	return NULL;
}

void njMultiplex::pump(timeval *timeout)
{
    //FIXME: Put in some timeout.
    int r = epoll_wait( epfd, events, nfds, 10 );
    if( r == -1 )
    {
        switch( errno )
        {
            case EBADF:
                throw nj_error( "epfd is not a valid file descriptor." );
                break;
            case EFAULT:
                throw nj_error( "The memory area pointed to by events is not \
                                 accessible with write permissions." );
                break;
            case EINTR:
                //FIXME: Try again.
                throw nj_error( "epoll was interrupted", true );
                break;
            case EINVAL:
                throw nj_error( "epfd is not an epoll file descriptor, \
                                 or maxevents is less than or equal to zero.");
                break;
        }
    }
    else if( r == 0 )
    {
        //
        // No data
        //
        return;
    }
    else
    {
        //
        // Cycle through available connection events.
        //
        for(int n = 0; n < r; ++n)
        {
            int id = events[n].data.fd;
            if( events[n].events & EPOLLIN )
            {
                handleLowRead( id );
            }
            if( events[n].events & EPOLLOUT )
            {
                handleLowWrite( id );
            }
            if( events[n].events & EPOLLERR )
            {
                handleLowExcept( id );
            }
            if( events[n].events & EPOLLHUP )
            {
                if( epoll_ctl( epfd, EPOLL_CTL_DEL, id, &events[n] ) < 0)
                    throw nj_error( );
                handleLowDeath( id );
            }
        }
    }
}

void njMultiplex::writeTo( njConnection *c, njBuffer b )
{
    conpro->outgoing( c, &b );
    c->setSendBuffer( b );
}

void njMultiplex::writeToAll( njBuffer b )
{
    map< int, pair<njSocket*, so_type> >::iterator pos;
    for (pos = sockets.begin(); pos != sockets.end(); pos++)
    {
        if( (*pos).second.second == CONN )
        {
            njConnection *s =dynamic_cast< njConnection* >((*pos).second.first);
            conpro->outgoing( s, &b );
            s->setSendBuffer( b );
        }
    }
}

njBuffer njMultiplex::readFrom( njConnection *c )
{
    njBuffer b;
    b = c->readRecvBuffer( );
    conpro->incoming( c, &b );
    return b;
}

//
// "Low level" stuff... These are called in each pump( );
//
void njMultiplex::handleLowWrite( int id )
{
    if( id == -1 ) return;
    if( sockets.find( id ) == sockets.end( ) ) return;
    
    njSocket *s = sockets[ id ].first;
    if( sockets[ id ].second == CONN )
    {
        try
        {
            dynamic_cast< njConnection* >(s)-> \
            Send( &dynamic_cast< njConnection* >(s)->readSendBuffer( ) );
        }
        catch( nj_error e )
        {
            if( e.isIgnorable( ) ) return;
        }
    }
    //
    // else can't do anything. ( listeners are read only )
    //
}

void njMultiplex::handleLowRead( int id )
{
    if( id == -1 ) return;
    if( sockets.find( id ) == sockets.end( ) ) return;
    
    njSocket *s = sockets[ id ].first;
    if( sockets[ id ].second == CONN )
    {
        njBuffer *b = dynamic_cast< njConnection* >(s)->Recv( );
        if( b != NULL )
        {
            conpro->incoming( dynamic_cast< njConnection* >(s), b );
        }
    }
    else
    {
        try
        {
            njConnection *c = dynamic_cast< njListener* >(s)->acceptIncoming( );
            registerConnection( conpro->connectionBirth( c ) );
        }
        catch( nj_error e )
        {
            cout << e.what( ) << endl;
        }
        return;
    }
}

void njMultiplex::handleLowExcept( int id )
{
    cout << getpid() << ": Caught Exception! " << id << endl;
}

void njMultiplex::handleLowDeath( int id )
{

    if( id == -1 ) return;
    if( sockets.find( id ) == sockets.end( ) ) return;
    
    njSocket *s = sockets[ id ].first;
    njConnection *c = dynamic_cast< njConnection* >(s);

    conpro->connectionDeath( c );
    unregisterSocket( s );

}

void njMultiplex::registerListener( njListener *l )
{
    registerSocket( l, LIST );
}

void njMultiplex::registerConnection( njConnection *c )
{
    registerSocket( c, CONN );
    //FIXME: Might want to make this changeable?
    c->setNonBlocking( true );
}

void njMultiplex::unregisterConnection( njConnection *c )
{
    unregisterSocket( c );
}

void njMultiplex::registerSocket( njSocket *s, so_type t )
{
    int id = (*s);
    if( id != -1 )
    {
        //
        // Add new connection.
        //
        sockets.insert( make_pair(id, make_pair(s, t) ) );
        
        epoll_event *ev = new epoll_event;
        if( t == CONN ) ev->events = EPOLLIN | EPOLLOUT;
        else ev->events = EPOLLIN;
        ev->data.fd = id;
        if( epoll_ctl( epfd, EPOLL_CTL_ADD, id, ev ) < 0) throw nj_error( );
        
        //
        // Create our new epoll_event array.
        //
        int ns = sockets.size( );
        
        epoll_event *tmpevnts = new epoll_event[ ns ];
        for( int x = 0; x < nfds; x++ )
        {
            //
            //Copy over our existing connections to new epoll_event array
            //
            memcpy( &tmpevnts[x], &events[x], sizeof(epoll_event) );
        }
        //
        // And then our newest one.
        //
        memcpy( &tmpevnts[ns-1], ev, sizeof(epoll_event) );
        //
        // Remove old array.
        //
        delete[ ] events;
        //
        // Our old is now new.
        //
        events = tmpevnts;
        nfds = ns;
        
        //FIXME: This part could probably go.
        if( t == LIST )
        {
            cout << getpid() << ": Registered (" << id << ") for listening." << endl;
        }
        else
        {
            cout << getpid() << ": Registered (" << id << ")" << endl;
        }
    }
    else
    {
        throw nj_error( "Tried to register a dead connection!", true );
    }
}

void njMultiplex::unregisterSocket( njSocket *s )
{
    int id = (*s);
    
    //
    // Remove socket from collection
    //
    map< int, pair<njSocket*, so_type> >::iterator old;
    old = sockets.find( id );
    sockets.erase( old );
    //
    // Close the socket...
    //
    s->close( );
    delete s;
    
    //
    // Recalculate our epoll_event array...
    //
    int ns = sockets.size( );
    epoll_event *tmpevnts = new epoll_event[ ns ];
    //
    // Loop through socket collection, and copy only the corresponding events
    //
    int x = 0;
    map< int, pair<njSocket*, so_type> >::iterator pos;
    for (pos = sockets.begin(); pos != sockets.end(); pos++)
    {
        for( int y = 0; y < nfds; y++ )
        {
            if( (*pos).first == events[y].data.fd )
            {
                memcpy( &tmpevnts[x], &events[y], sizeof(epoll_event) );
            }
        }
        x++;
    }
    nfds = ns;
    //
    // Remove old array, and reasign new...
    //
    delete[ ] events;
    events = tmpevnts;
    //FIXME: This part can probably go.
    cout << getpid() << ": Unregistered (" << id << ")" << endl;
}

