//
// Created by ruiy on 18-5-25.
//

#include "SocketClientThread.h"
#include <iostream>
#include <fstream>

void SocketClientThread::start() {

    if (started()) return;
    start_status = true;

    socket_thread = std::thread(std::bind(&self_type::run, shared_from_this()));
    io_service_thread = std::thread(std::bind(&self_type::io_service_run), shared_from_this()); // Now just one thread to run io_service. Maybe it will possess more thread in later development;
}

/**
 * poll order queue to execute corresponding command
 */
//void SocketClientThread::run() {
//    for(;;){
//        usleep(1e5);
//        if (order_queue.empty()) continue;
//        Command cmd = order_queue.back();
//
//        if (cmd.type == ClientOrder::ASYNC_SEND_FILE) {
//            // if send file asynchronously, the endpoint(ip, port) should be aquired from cmd
//            do_async_send_file(cmd);
//        } else {
//
//        }
//
//        queue_.pop();
//
//    }
//}

void SocketClientThread::stop(){
    if (!started()) {return;}
    start_status = false;
    std::cout << "stopping" << std::endl;
    sock.close();
    std::cout << "stopped" << std::endl;
}

void SocketClientThread::do_async_send_file(std::string ip_addr, int port, std::string send_file_path) {
    sock.async_connect(
            boost::asio::ip::tcp::endpoint(
                    boost::asio::ip::address::from_string(ip_addr),
                    port
            ),
            std::bind(&self_type::on_async_connect, enable_shared_from_this(), send_file_path, _1)
    );
}

void SocketClientThread::on_async_connect(const std::string &send_file_path, const error_code &err) {
    if (!err) stop();
    if (!started()) return;

    std::ifstream read_handle(send_file_path);
    read_handle.seekg(0, ios::end);
    int file_size = read_handle.tellg();
    read_handle.seekg();

    boost::asio::streambuf write_stream_buf;
    std::ostream out(&write_stream_buf);

    out.write((char*)&filesize, sizeof(int)); // write file length
    out << read_handle; // write file content
    read_handle.close();

    sock.async_write_some(boost::asio::buffer(write_stream_buf, boost::asio::transfer_exactly(write_stream_buf.size())), std::bind(&self_type::on_async_write, _1, _2));

}

void SocketClientThread::on_async_write(const error_code &err, size_t bytes) {
    boost::asio::streambuf read_stream_buf;
    // read result
    boost::asio::async_read(sock, read_stream_buf, '\n');
}