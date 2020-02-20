#include "common.h"
#include "sysutil.h"
#include "session.h"
#include "str.h"
#include "tunable.h"
#include "parseconf.h"
#include "ftpproto.h"
#include "ftpcodes.h"
#include "hash.h"

extern session_t *p_sess;
static unsigned s_children;
static hash_t *s_ip_count_hash;
static hash_t *s_pid_ip_hash;

void isValid(session_t* sess);
void handle_sigchld(int sig);
unsigned hash_func(unsigned buckets,void *key);
unsigned handle_ip_count(void* ip);
void drop_ip_count(void* ip);


int main(){

    parseconf_load_file(MYFTP_CONF);
    //daemon(0,0); ///真正启动的时候在弄守护进程

    printf("tunable_pasv_enable=%d\n", tunable_pasv_enable);
	printf("tunable_port_enable=%d\n", tunable_port_enable);

	printf("tunable_listen_port=%u\n", tunable_listen_port);
	printf("tunable_max_clients=%u\n", tunable_max_clients);
	printf("tunable_max_per_ip=%u\n", tunable_max_per_ip);
	printf("tunable_accept_timeout=%u\n", tunable_accept_timeout);
	printf("tunable_connect_timeout=%u\n", tunable_connect_timeout);
	printf("tunable_idle_session_timeout=%u\n", tunable_idle_session_timeout);
	printf("tunable_data_connection_timeout=%u\n", tunable_data_connection_timeout);
	printf("tunable_local_umask=0%o\n", tunable_local_umask);
	printf("tunable_upload_max_rate=%u\n", tunable_upload_max_rate);
	printf("tunable_download_max_rate=%u\n", tunable_download_max_rate);

	if (tunable_listen_address == NULL)
		printf("tunable_listen_address=NULL\n");
	else
		printf("tunable_listen_address=%s\n", tunable_listen_address);


    if(getuid()!=0){
        fprintf(stderr,"myftpd: must be started as root.\n");
        exit(EXIT_FAILURE);
    }

    session_t sess={
        0,-1,"","","",
        NULL,-1,-1,0,
        0,0,0,0,
        -1,-1,
        0,0,NULL,0,
        0,0
    };
    p_sess=&sess;

    sess.bw_upload_rate_max=tunable_upload_max_rate;
    sess.bw_download_rate_max=tunable_download_max_rate;

    s_ip_count_hash=hash_alloc(256,hash_func);
    s_pid_ip_hash=hash_alloc(256,hash_func);

    signal(SIGCHLD,handle_sigchld);
    ///监听cmd连接的socket
    int listenfd=tcp_server(tunable_listen_address,tunable_listen_port);
    int conn;
    pid_t pid;
    struct sockaddr_in addr;

    for(;;){
            ///0估计就是没有等待时间,直接阻塞?
            conn=accept_timeout(listenfd,&addr,0);
            if(conn==-1)ERR_EXIT("accept_timeout");
            unsigned ip=addr.sin_addr.s_addr;
            s_children++;
            sess.num_clients=s_children;
            sess.num_this_ip=handle_ip_count(&ip);
            pid=fork();
            if(pid==-1){
                s_children--;
                ERR_EXIT("fork");
            }
            if(pid==0){
                close(listenfd);
                sess.ctrl_fd=conn;
                isValid(&sess);
                signal(SIGCHLD,SIG_IGN);
                begin_session(&sess);
            }else{
                hash_add_entry(s_pid_ip_hash,&pid,sizeof(pid),&ip,sizeof(unsigned));
                close(conn);
            }

    }
    return 0;
}

void isValid(session_t* sess){
    if(tunable_max_clients>0&&sess->num_clients>tunable_max_clients){
        ftp_reply(sess,FTP_TOO_MANY_USERS,"There are too many users. Try later.");
        exit(EXIT_FAILURE);
    }
    if(tunable_max_per_ip>0&&sess->num_this_ip>tunable_max_per_ip){
        ftp_reply(sess,FTP_IP_LIMIT,"There are too many connections from your IP. Try later.");
        exit(EXIT_FAILURE);
    }
}

void handle_sigchld(int sig){
    pid_t pid;
    while((pid=waitpid(-1,NULL,WNOHANG))>0){
        s_children--;
        unsigned *ip=(unsigned*)hash_lookup_entry(s_pid_ip_hash,&pid,sizeof(pid));
        if(ip==NULL)continue;
        drop_ip_count(ip);
        hash_free_entry(s_pid_ip_hash,&pid,sizeof(pid));
    }
}


unsigned hash_func(unsigned buckets,void* key){
    return *(unsigned*)key%buckets;
}

unsigned handle_ip_count(void* ip){
    unsigned cnt;
    unsigned *pcnt=(unsigned*)hash_lookup_entry(s_ip_count_hash,ip,sizeof(unsigned));
    if(pcnt==NULL){
        cnt=1;
        hash_add_entry(s_ip_count_hash,ip,sizeof(unsigned),&cnt,sizeof(unsigned));
    }else{
        cnt=*pcnt;
        ++cnt;
        *pcnt=cnt;
    }
    return cnt;
}

void drop_ip_count(void* ip){
    unsigned ret;
    unsigned *pcnt=(unsigned*)hash_lookup_entry(s_ip_count_hash,(void*)ip,sizeof(unsigned));
    if(pcnt==NULL)return;
    ret=*pcnt;
    ///感觉这种情况是没有可能发生的，因为这个函数是在中断处理程序中调用的
    ///但是中断处理程序没用奇技淫巧，所以它不会被中断，所以这个函数也不会被中断
    ///即减成0和删除entry是一气呵成的
    if(ret<=0)return;
    --ret;
    *pcnt=ret;
    if(ret==0)hash_free_entry(s_ip_count_hash,(void*)ip,sizeof(unsigned));
}
















