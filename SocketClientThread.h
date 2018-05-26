//
// Created by ruiy on 18-5-25.
//

#ifndef ASIOBASEDCS_SOCKETCLIENTTHREAD_H
#define ASIOBASEDCS_SOCKETCLIENTTHREAD_H

#include <queue>
#include "command_type.h"
#include <boost/asio.hpp>
#include <boost/core/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>

class SocketClientThread: public boost::enable_shared_from_this<SocketClientThread>, boost::noncopyable {
    typedef SocketClientThread self_type;
    typedef boost::system::error_code error_code;
private:
    bool start_status;

    boost::asio::io_service service; // io_service provide asynchronous operations
    boost::shared_ptr<boost::asio::io_service> work; // construct a work to keep io service available(working always)
    std::thread io_service_thread; // the thread to run io_service

    std::thread socket_thread; // a thread that socket accept, receive, send data
    boost::asio::ip::tcp::socket sock; // a socket channel

    std::queue<Command> order_queue; // the outside program ask the channel to do something
    std::queue<Command> reply_queue; // when the channel finished something, the channel would reply the result to outside program

private:
//    void io_service_run () { service.run();}

    void on_async_connect(const error_code& err);
    void on_async_read();
    void on_async_write();

public:
    SocketClientThread():
            service(),
            work(new boost::asio::io_service::work(service)),
            sock(service),
            start_status(false){
    }
    void run();
    void start();
    void stop();

    void do_async_send_file(std::string ip_addr, int port, std::string send_file_path);


    bool started(){ return start_status;}
};

#endif //ASIOBASEDCS_SOCKETCLIENTTHREAD_H
