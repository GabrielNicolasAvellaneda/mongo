// message_server_asio.cpp

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

#include <iostream>
#include <vector>

#include "message.h"
#include "message_server.h"

using namespace boost;
using namespace boost::asio;
using namespace boost::asio::ip;
using namespace std;

namespace mongo {

    class MessageServerSession : public enable_shared_from_this<MessageServerSession> {
    public:
        MessageServerSession( io_service& ioservice ) : _socket( ioservice ){
            
        }

        tcp::socket& socket(){
            return _socket;
        }

        void start(){
            cout << "MessageServerSession start from:" << _socket.remote_endpoint() << endl;
            async_read( _socket , 
                        buffer( &_inHeader , sizeof( _inHeader ) ) ,
                        bind( &MessageServerSession::handleReadHeader , shared_from_this() , placeholders::error ) );
        }
        
        void handleReadHeader( const boost::system::error_code& error ){
            cout << "got header\n" 
                 << " len: " << _inHeader.len << "\n"
                 << " id : " << _inHeader.id << "\n"
                 << " op : " << _inHeader._operation << "\n";
            
            char * raw = (char*)malloc( _inHeader.len );

            MsgData * data = (MsgData*)raw;
            memcpy( data , &_inHeader , sizeof( _inHeader ) );
            assert( data->len == _inHeader.len );
            
            _cur.setData( data , true );
            async_read( _socket , 
                        buffer( raw + sizeof( _inHeader ) , _inHeader.len - sizeof( _inHeader ) ) ,
                        bind( &MessageServerSession::handleReadBody , shared_from_this() , placeholders::error ) );
        }
        
        void handleReadBody( const boost::system::error_code& error ){
            cout << "got whole message!" << endl;
        }

    private:
        tcp::socket _socket;
        MsgData _inHeader;
        Message _cur;
    };


    class AsyncMessageServer : public MessageServer {
    public:
        AsyncMessageServer( int port ) : MessageServer( port ) , 
                                    _endpoint( tcp::v4() , port ) , 
                                    _acceptor( _ioservice , _endpoint ){
            _accept();
        }
        virtual ~AsyncMessageServer(){
            
        }

        void run(){
            cout << "AsyncMessageServer starting to listen on: " << _port << endl;
            _ioservice.run();
            cout << "AsyncMessageServer done listening on: " << _port << endl;
        }
        
        void handleAccept( shared_ptr<MessageServerSession> session , 
                           const boost::system::error_code& error ){
            if ( error ){
                cerr << "handleAccept error!" << endl;
                return;
            }
            session->start();
            _accept();
        }
        
    private:

        void _accept(){
            shared_ptr<MessageServerSession> session( new MessageServerSession( _ioservice ) );
            _acceptor.async_accept( session->socket() ,
                                    bind( &AsyncMessageServer::handleAccept,
                                          this, 
                                          session,
                                          boost::asio::placeholders::error )
                                    );            
        }

        io_service _ioservice;
        tcp::endpoint _endpoint;
        tcp::acceptor _acceptor;
        
    };


    // --temp hacks--
    void dbexit( int rc , const char * why ){
        cerr << "dbserver.cpp::dbexit" << endl;
        ::exit(rc);
    }
    
    const char * curNs = "";
    
    string getDbContext(){
        return "getDbContext bad";
    }

}

using namespace mongo;

int main(){
    mongo::AsyncMessageServer s(9999);
    s.run();

    return 0;
}
