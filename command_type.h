//
// Created by ruiy on 18-5-25.
//

#ifndef ASIOBASEDCS_COMMAND_TYPE_H
#define ASIOBASEDCS_COMMAND_TYPE_H

struct Command {
    int type; // 命令类型, 对应的是ClientOrder和ClientReply的值
    int port; // 端口
    std::string ip_addr; // ip 地址
    std::string send_file_path; // 发送文件地址
    std::string recv_file_path; // 接收文件地址
};

enum ClientOrder {ASYNC_SEND_FILE, SEND, RECEIVE, CLOSE};
enum ClientReply {ERROR, SUCCESS};

#endif //ASIOBASEDCS_COMMAND_TYPE_H
