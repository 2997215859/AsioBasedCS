//
// Created by ruiy on 18-5-23.
//
//#define BOOST_ASIO_ENABLE_HANDLER_TRACKING
#include "client.h"

#include <boost/enable_shared_from_this.hpp>
#include <boost/core/noncopyable.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <iostream>

namespace ba = boost::asio;
ba::io_service service;

class talk_to_svr: public boost::enable_shared_from_this<talk_to_svr>, boost::noncopyable{

    typedef talk_to_svr self_type;
    typedef boost::system::error_code error_code;
    typedef boost::shared_ptr<talk_to_svr> ptr;

public:
    /**
     * a static method to start to connect a server endpoint
     * @param ep server endpoint
     * @param username client name
     * @return the ptr to the client
     */
    static ptr start(ba::ip::tcp::endpoint ep, const std::string & username){
        ptr new_(new talk_to_svr(username));
        new_->start(ep);
        return new_;
    }

    /**
     * start a channel by connecting the server endpoint
     * @param ep server endpoint
     */
    void start(ba::ip::tcp::endpoint ep) {
        sock_.async_connect(ep, boost::bind(&self_type::on_connect, shared_from_this(), _1));
    }

    void stop(){
        if (!started()) return;
        std::cout << "stopping " << username_ << std::endl;
        started_ = false;
        sock_.close();
    }

    bool started() {return started_;}

private:
    talk_to_svr(const std::string &username): sock_(service), started_(true), username_(username), timer_(service) {}

    void on_connect(const error_code &err){
        if (!err) {
            do_write("login " + username_ + "\n");
        } else {
            std::cout << err.message() << std::endl;
            stop();
        }
    }

    void on_write(const error_code& err, size_t bytes){
        do_read();
    }

    void on_read(const error_code& err, size_t bytes){
        if (err) stop();
        if (!started()) return;

        std::string msg(read_buffer_, bytes);
        std::cout << msg << std::endl;
        if (msg.find("login ") == 0) on_login();
        else if (msg.find("ping ") == 0) on_ping(msg);
        else if (msg.find("clients ") == 0) on_clients(msg);
        else std::cerr << "invalid msg " << msg << std::endl;
    }

    void on_clients(std::string msg){
        std::string clients = msg.substr(8);
        std::cout << username_ << ", new client list:" << clients;
        postpone_ping();
    }
    void on_login(){
        std::cout << username_ << " logined in" << std::endl;
        do_ask_clients();
    }

    void on_ping(std::string msg) {
        std::istringstream in(msg);
        std::string answer;
        in >> answer >> answer;
        if (answer == "client_list_changed") do_ask_clients();
        else postpone_ping();
    }

    void postpone_ping(){
        int millis = rand() % 7000;
        std::cout << username_ << " postponing ping" << millis << " millis" << std::endl;
        timer_.expires_from_now(boost::posix_time::millisec(millis));
        timer_.async_wait(boost::bind(&self_type::do_ping, shared_from_this()));
    }

    void do_ping(){
        do_write("ping\n");
    };

    void do_ask_clients(){
        do_write("ask_clients\n");
    }

    void do_read(){
        ba::async_read(sock_, ba::buffer(read_buffer_),
                       boost::bind(&self_type::read_complete, shared_from_this(), _1, _2),
                       boost::bind(&self_type::on_read, shared_from_this(),  _1, _2));
    }

    void do_write(std::string msg) {
        if (!started()) return;
        std::copy(msg.begin(), msg.end(), write_buffer_);
        sock_.async_write_some(ba::buffer(write_buffer_, msg.size()),
                               boost::bind(&self_type::on_write, shared_from_this(), _1, _2));
    };

    size_t read_complete(const error_code &err, size_t bytes){
        if (err) return 0;
        bool found = std::find(read_buffer_, read_buffer_ + bytes, '\n') < read_buffer_ + bytes;
        return found? 0: 1;
    }
private:
    boost::asio::ip::tcp::socket sock_;
    std::string username_;
    bool started_;
    ptr new_;
    ba::deadline_timer timer_;

    enum {max_msg = 1024};
    char write_buffer_[max_msg];
    char read_buffer_[max_msg];
};

int main () {
    ba::ip::tcp::endpoint ep(ba::ip::address::from_string("127.0.0.1"), 8001);
    std::vector<std::string> names = {"John", "James", "Lucy", "Tracy", "Frank", "Abby"};
    for (const auto &name: names) {
        talk_to_svr::start(ep, name);
        boost::this_thread::sleep(boost::posix_time::millisec(100));
    }

    service.run();
}