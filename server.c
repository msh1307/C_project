#include <sys/socket.h>
#include <netinet/ip.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include "serverlib.h"

#define send_reply(fd, buf) write(fd, buf,strlen(buf));

void clnt_main(int fd,struct config * conf);

void * mem_list[0x20];
int j;
unsigned long long file_sz;
size_t header_sz;
int main(int argc,char ** argv){
    int serv_sock, cl_sock;
    int clnt;
    struct sockaddr_in serv_addr;
    struct sockaddr_in cl_addr;
    struct config * conf = NULL;
    socklen_t cl_addrlen;
    char * config = NULL;
    if(argc == 2){
        if(port_valid_chk(argv) == -1)
            error_hander("invalid port number (0<= port <= 65535)");
    }
    else
        error_hander("usage : ./server [port]");

    config = get_file("./conf");
    if(config == NULL)
        error_hander("there is no file named conf");
    conf = parse_conf(config);
    if(!conf -> server_name || strlen(conf -> server_name) == 0)
        error_hander("conf error : SERVER_NAME / empty server name");
    if(!conf -> version || strlen(conf -> version) == 0)
        error_hander("conf error : VERSION / empty server version");
    if(!conf -> base_path || strlen(conf -> base_path) == 0)
        error_hander("conf error : BASE_PATH / empty base path");
    if(!conf -> default_ || strlen(conf -> default_) == 0)
        error_hander("conf error : DEFAULT / empty default");
    add_ele(conf);
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
            clnt_main(cl_sock,conf);
            close(cl_sock);
            exit(0);
        }
        if(clnt < 0)
            error_hander("fork() error");
        close(cl_sock);
    }
}

void clnt_main(int fd,struct config * conf){
    int i=0;
    char local_name[0x20];
    char local_uri[0x20];
    unsigned int len=0;
    char * uri= NULL;
    char *req_buf = malloc(MAX_PAY_LEN);
    char * method = NULL;
    char * reply_buf = malloc(MAX_REP_LEN);
    char * file_content = NULL;
    if(!req_buf)
        error_hander("allocation failed");
    add_ele(req_buf);
    if(!reply_buf)
        error_hander("allocation failed");
    add_ele(reply_buf);

    printf("connected fd: %d\n",fd);
    len = read(fd,req_buf,MAX_PAY_LEN);
    if(len > MAX_PAY_LEN){ 
        reply(413, reply_buf,conf -> server_name,conf -> version,"text/html");
        puts("413 Payload Too Large");
        goto ABORT;
    }

    method = get_method(req_buf);
    if(!method){ 
        reply(400, reply_buf,conf -> server_name,conf -> version,"text/html");
        puts("400 Bad Request");
        goto ABORT;
    }
    add_ele(method);
    if(is_method_valid(method)){
        if(!is_method_allowed(method,conf -> method_list)){ 
            reply(405, reply_buf,conf -> server_name,conf -> version,"text/html");
            write(fd, reply_buf, MAX_REP_LEN);
            puts("405 Method Not Allowed");
            goto ABORT;
        }
    }
    else{ 
        reply(400, reply_buf,conf -> server_name,conf -> version,"text/html");
        write(fd, reply_buf, MAX_REP_LEN);
        puts("400 Bad Request");
        goto ABORT;
    } 
    uri = get_uri(req_buf);
    if(!uri){
        reply(400, reply_buf,conf -> server_name,conf -> version,"text/html");
        write(fd, reply_buf, MAX_REP_LEN);
        puts("400 Bad Request");
        goto ABORT;
    }
    add_ele(uri); 
    if(strlen(uri) > MAX_URI_LEN){
        reply(414, reply_buf,conf -> server_name,conf -> version,"text/html");
        write(fd, reply_buf, MAX_REP_LEN);
        puts("414 URI Too Long");
        goto ABORT;
    }
    
    if(!strcmp(uri, "/")){
        file_content = get_file(conf -> default_);
        if(file_content == NULL){
            reply(404, reply_buf, conf -> server_name, conf -> version,"text/html");
            puts("404 Not Found");
            write(fd, reply_buf, MAX_REP_LEN);
            goto ABORT;
        }
        add_ele(file_content);
        ncpy(reply_buf, file_content,file_sz);
        reply(200, reply_buf, conf -> server_name, conf -> version,"text/html");
        if(file_sz+header_sz > MAX_REP_LEN)
            error_hander("reply buf overflow");
        puts("200 OK");
        write(fd, reply_buf, file_sz+header_sz);
    }
    else{
        i=0;
        memset(local_uri, 0, sizeof(local_uri));
        memset(local_name, 0, sizeof(local_name));
        while(uri[i]=='/')
            i++;
        strcpy(local_name,uri+i);
        strcpy(local_uri,conf -> base_path);
        strcat(local_uri,uri+i);
        puts("permchk");
        if(perm_chk(local_name, conf -> file_list)){
            puts("permchk passsed");
            file_content = get_file(local_uri);
            puts("read");
            if(file_content == NULL){
                reply(404, reply_buf, conf -> server_name, conf -> version,"text/html");
                puts("404 Not Found");
                write(fd, reply_buf, MAX_REP_LEN);
                goto ABORT;
            }
            add_ele(file_content);
            ncpy(reply_buf, file_content,file_sz);
            for(int j =0;j<0x100;j++){
                printf("%c",reply_buf[j]);
            }
            if(strcmp(get_file_extension(local_uri),".html"))
                reply(200, reply_buf, conf -> server_name, conf -> version,"text/html");
            else if(strcmp(get_file_extension(local_uri),".jpg"))
                reply(200, reply_buf, conf -> server_name, conf -> version,"image/jpeg");
            else{
                error_hander("unsupported file extentsion");
            }
            if(file_sz+header_sz > MAX_REP_LEN)
                error_hander("reply buf overflow");
            write(1, reply_buf, file_sz+header_sz);
            printf("%llu",file_sz);
            puts("");
            write(fd, reply_buf, file_sz+header_sz);
        }
        else{
            reply(403, reply_buf, conf -> server_name, conf -> version,"text/html");
            puts("403 Forbidden");
            write(fd, reply_buf, file_sz+header_sz);
        }
    }

    ABORT:
    close(fd);
    printf("closed fd: %d\n",fd);
    for (int i=0;mem_list[i];i++){
        if(i > 0x20)
            break;
        free(mem_list[i]);
    }

}