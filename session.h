#ifndef SESSION_H_INCLUDED
#define SESSION_H_INCLUDED

#include"common.h"

typedef struct session{
    ///控制连接
    uid_t uid;//用户id
    int ctrl_fd;//数据连接的socket
    char cmdline[MAX_COMMAND];
    char cmd[MAX_COMMAND];
    char arg[MAX_ARG];

    //数据连接
    struct sockaddr_in *port_addr;
    int pasv_listen_fd; //监听的
    int data_fd; //用来readwrite的
    int data_process;

    //限速的
    unsigned bw_upload_rate_max;
    unsigned bw_download_rate_max;
    long bw_transfer_start_sec;
    long bw_transfer_start_usec;

    //内部的父子进程通讯的socket
    int parent_fd;
    int child_fd;

    ///ftp协议状态
    int is_ascii;
    long long restart_pos;
    char* rnfr_name;
    int abor_received;

    //连接数限制
    unsigned num_clients;
    unsigned num_this_ip;
} session_t;

void begin_session(session_t* sess);


#endif // SESSION_H_INCLUDED
