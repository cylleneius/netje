#ifndef __NJ_MULTIPLEX_H__
#define __NJ_MULTIPLEX_H__

#include "net.h"
#include "njconnection.h"
#include "njcprocessor.h"
#include <poll.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <utility>
using namespace std;

class njMultiplex
{
public:
   ~njMultiplex();
   
    pthread_t  loop();
    void pump(timeval*); 
    
    void      writeTo( njConnection *, njBuffer );
    void writeToAll( njBuffer );
    njBuffer readFrom( njConnection * );
    
    void except( );
    
    
    //void  openListener( );
    //void closeListener( );
    void registerListener( njListener * );
    void unregisterListener( njListener * );
    
    void registerConnection( njConnection * );
    void unregisterConnection( njConnection * );
    
    void registerConnectionProcessor( njConnectionProcessor* cp )
    {
        conpro = cp;
    };
private:
    enum so_type { LIST=0, CONN=1 };
    //FIXME: These are kind of crappy. Above you have a so_type, which controls
    // how the njSocket will be cast. Not very elegant, but it works.
    void registerSocket( njSocket *, so_type );
    void unregisterSocket( njSocket * );

    void handleLowWrite( int );
    void handleLowRead( int );
    void handleLowExcept( int );
    void handleLowDeath( int );
    

    map< int, pair<njSocket *,so_type> > sockets;
    
    //FIXME: Maybe in the future I'll put multiple processors in...
    //map< int, njConnectionProcessor * > conpros;
    njConnectionProcessor *conpro;
    pollfd *fds;
    int nfds;
    fd_set tread, twrite, texcept;
    struct epoll_event *events;
    int high;
    
    int epfd;

public:
    static njMultiplex* instance();
	static void* loop_( void * );
    
protected:
    njMultiplex();
    static njMultiplex* instance_;    
};

#endif //__NJ_MULTIPLEX_H__

