//
// Created by ruiy on 18-5-24.
//
//#define BOOST_ASIO_ENABLE_HANDLER_TRACKING

#include "server.h"

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <iostream>
#include <boost/thread/recursive_mutex.hpp>

using namespace boost::asio;
using namespace boost::posix_time;

io_service service;

class talk_to_client;
typedef boost::shared_ptr<talk_to_client> client_ptr;
typedef std::vector<client_ptr> array;
array clients;
boost::recursive_mutex clients_cs;

void update_clients_changed();

class talk_to_client: public boost::enable_shared_from_this<talk_to_client>, boost::noncopyable {
    typedef talk_to_client self_type;
    typedef boost::system::error_code error_code;
    talk_to_client() :sock_(service), started_(false), timer_(service), clients_changed_(false){}

public:
    typedef boost::shared_ptr<talk_to_client> ptr;

    static ptr new_() {
        ptr new_(new talk_to_client);
        return new_;
    }

    void start(){
        started_  = true;
        clients.push_back(shared_from_this());
        last_ping = boost::posix_time::microsec_clock::local_time();
        do_read();
    }

    void stop() {
        if (!started_) return;
        started_ = false;
        sock_.close();
        ptr self = shared_from_this();
        array::iterator it = std::find(clients.begin(), clients.end(), self);
        clients.erase(it);
        update_clients_changed();
    }

    void set_clients_changed () {clients_changed_ = true;}
    std::string username() {return username_;}

    ip::tcp::socket & sock() {return sock_;}
private:


    void on_read(const error_code &err, size_t bytes){
        if (err) stop();
        if (!started_) return;
        std::string msg(read_buffer_, bytes);
        if (msg.find("login ") == 0) on_login(msg);
        else if (msg.find("ping") == 0) on_ping();
        else if (msg.find("ask_clients") == 0) on_clients();
        else std::cerr << "invalide msg" << msg << std::endl;
    }

    void on_login(const std::string & msg){
        std::istringstream in(msg);
        in >> username_ >> username_;
        std::cout << username_ << " logged in " << std::endl;
        do_write("login ok\n");
        update_clients_changed();
    }

    void on_ping() {
        do_write(clients_changed_ ? "ping client_list_changed\n": "ping ok\n");
        clients_changed_ = false;
    }

    void on_clients () {
        std::string msg;
        for (array::const_iterator b=clients.begin(); b!=clients.end();b++) {
            msg += (*b)->username() + " ";
        }
        do_write("clients " + msg + "\n");
    }

    void on_write(const error_code &err, size_t bytes){
        do_read();
    }

    void do_write(const std::string msg){
        if (!started_) return;
        std::copy(msg.begin(), msg.end(), write_buffer_);
        sock_.async_write_some(buffer(write_buffer_, msg.size()), boost::bind(&self_type::on_write, shared_from_this(), _1, _2));
    }
    void do_read(){
        async_read(sock_, buffer(read_buffer_),
                   boost::bind(&self_type::read_complete, shared_from_this(), _1, _2),
                   boost::bind(&self_type::on_read, shared_from_this(), _1, _2));
        post_check_ping();
    }

    void on_check_ping(){
        ptime now = microsec_clock::local_time();
        if ((now - last_ping).total_milliseconds() > 5000) {
            std::cout << "stopping " << username_ << " - no ping in time" << std::endl;
            stop();
        }
        last_ping = microsec_clock::local_time();
    }

    void post_check_ping () {
        timer_.expires_from_now(boost::posix_time::millisec(5000));
        timer_.async_wait(boost::bind(&self_type::on_check_ping, shared_from_this()));
    }

    size_t read_complete(const boost::system::error_code &err, size_t bytes){
        if (err) return 0;
        bool found = std::find(read_buffer_, read_buffer_ + bytes, '\n') < read_buffer_ + bytes;
        return found? 0: 1;
    }
private:
    bool started_;
    ip::tcp::socket sock_;
    deadline_timer timer_;
    bool clients_changed_;
    ptime last_ping;
    std::string username_;

    enum {max_msg=1024};
    char read_buffer_[max_msg];
    char write_buffer_[max_msg];
};


void update_clients_changed() {
    array copy;
    {
        boost::recursive_mutex::scoped_lock lk(clients_cs);
        copy = clients;
    }
    for (array::iterator b = clients.begin(); b!= clients.end(); b++) {
        (*b)->set_clients_changed();
    }
}

ip::tcp::acceptor acceptor(service, ip::tcp::endpoint(ip::tcp::v4(), 8001));

void handle_accept(talk_to_client::ptr client, const boost::system::error_code &err) {
    client->start();
    talk_to_client::ptr new_client = talk_to_client::new_();
    acceptor.async_accept(new_client->sock(), boost::bind(handle_accept, new_client, _1));
}



int main () {
    talk_to_client::ptr client = talk_to_client::new_();
    acceptor.async_accept(client->sock(), boost::bind(&handle_accept, client, _1));
    service.run();
}