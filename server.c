#include <sys/socket.h>
#include <netinet/ip.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include "serverlib.h"

#define add_ele(ele) mem_list[j++] = ele;
#define send_reply(fd, buf) write(fd, buf,strlen(buf));

void clnt_main(int fd);

int main(int argc,char ** argv){
    int serv_sock, cl_sock;
    int clnt;
    struct sockaddr_in serv_addr;
    struct sockaddr_in cl_addr;
    socklen_t cl_addrlen;
    if(argc == 2){
        if(port_valid_chk(argv) == -1)
            error_hander("invalid port number (0<= port <= 65535)");
    }
    else
        error_hander("usage : ./server [port]");
    serv_sock = socket(AF_INET,SOCK_STREAM,0);
    if(serv_sock == -1)
        error_hander("socket() error");

    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);// htonl -> host to network long
    serv_addr.sin_port = htons(atoi(argv[1]));  // htons -> host to network short

    // struct sockaddr_in {
	// short    sin_family;          // AF_INET
	// u_short  sin_port;            // 16 bit port, network byte order 
	// struct   in_addr  sin_addr;   // 32 bit IP
	// char     sin_zero[8];         // dummy
    // };

    // struct  in_addr {
	//     u_long  s_addr;     // 32 bit ip struct, network byte order
    // };

    if(bind(serv_sock, (struct sockaddr*)&serv_addr,sizeof(serv_addr))==1)
        error_hander("bind() error");
    if(listen(serv_sock,50)<0) //backlog -> queue 
        error_hander("listen() error");
    puts("server listening");
    while(1){
        while(1){
            cl_sock = accept(serv_sock,(struct sockaddr*)&cl_addr,&cl_addrlen);
            if(cl_sock >= 0)
                break;
        }
        clnt = fork();
        if(!clnt){
            close(serv_sock);
            clnt_main(cl_sock);
            close(cl_sock);
            exit(0);
        }
        if(clnt < 0)
            error_hander("fork() error");
        close(cl_sock);
    }
}

void clnt_main(int fd){
    int i=0,j=0;
    unsigned int len=0;
    char * allowed_req_methods[] = {"GET","POST",NULL}; //config req_list
    char * uri= NULL;
    char *req_buf = malloc(MAX_PAY_LEN);
    char * method = NULL;
    void * mem_list[10] = {0};
    char * reply_buf = malloc(MAX_REP_LEN);
    if(!req_buf)
        error_hander("allocation failed");
    add_ele(req_buf);
    if(!reply_buf)
        error_hander("allocation failed");
    add_ele(reply_buf);

    printf("connected fd: %d\n",fd);
    len = read(fd,req_buf,MAX_PAY_LEN);
    if(len > MAX_PAY_LEN){ 
        reply(413, reply_buf);
        puts("413 Payload Too Large");
        goto ABORT;
    }

    method = get_method(req_buf);
    if(!method){ 
        reply(400, reply_buf);
        puts("400 Bad Request");
        goto ABORT;
    }
    add_ele(method); // DFB avoid
    if(is_method_valid(method)){
        if(!is_method_allowed(method,allowed_req_methods)){ 
            reply(405, reply_buf);
            puts("405 Method Not Allowed");
            goto ABORT;
        }
    }
    else{ 
        reply(400, reply_buf);
        puts("400 Bad Request");
        goto ABORT;
    } 
    uri = get_uri(req_buf);
    if(!uri){
        reply(400, reply_buf);
        puts("400 Bad Request");
        goto ABORT;
    }
    add_ele(uri); // DFB avoid
    if(strlen(uri) > MAX_URI_LEN){
        reply(414, reply_buf);
        puts("414 URI Too Long");
        goto ABORT;
    }
    


    ABORT:
    close(fd);
    printf("closed fd: %d\n",fd);
    for (int i=0;mem_list[i];i++){
        if(i > 9)
            break;
        free(mem_list[i]);
    }


}