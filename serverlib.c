#include "serverlib.h"
int port_valid_chk(char ** argv){
    int port = atoi(argv[1]);
    if(port == 0){
        if(argv[1][0] == '\x30'){
            return port;
        }
        return -1;
    }
    else if(port >65535){
        return -1;
    }
    else{
        return port;
    }
}
void error_hander(const char * buf){
    fputs(buf,stderr);
    fputc('\x0a',stderr);
    exit(-1);
}
// 47 45 54 20 2F 61 72 74 69 63 6C 65 73 2F 61 75 2D 65 6E 64 69 61 6E 63 2F 20 48 54 54 50 2F 31 2E 31 0D 0A -> CRLF
// GET /articles/au-endianc/ HTTP/1.1
// Host: developer.ibm.com
// Sec-Ch-Ua: "Chromium";v="107", "Not=A?Brand";v="24"
// Sec-Ch-Ua-Mobile: ?0
// Sec-Ch-Ua-Platform: "Windows"
// Upgrade-Insecure-Requests: 1
// User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/107.0.5304.63 Safari/537.36
// Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9
// Sec-Fetch-Site: none
// Sec-Fetch-Mode: navigate
// Sec-Fetch-User: ?1
// Sec-Fetch-Dest: document
// Accept-Encoding: gzip, deflate
// Accept-Language: ko-KR,ko;q=0.9,en-US;q=0.8,en;q=0.7
// Connection: close

char * get_uri(const char * buf){
    int buf_len = strlen(buf);
    char * local_buf = malloc(MAX_URI_LEN+1);
    int flag = 0;
    int idx = 0;
    memset(local_buf,0,MAX_URI_LEN+1);
    for (int i=0; i<buf_len;i++){
        if(!flag && buf[i] == '/'){
            if(buf[i-1] != 0x20){ // filtering invalid format
                free(local_buf);
                return NULL; 
            }
            flag = 1;
            local_buf[idx++] = buf[i];
        }
        else if(flag){                         
            if(buf[i] == 0x20 &&idx <= MAX_URI_LEN){
                local_buf[idx++] = '\x00';     
                return local_buf;
            } 
            else if (buf[i] > 0x20 && buf[i] <0x7f &&idx <= MAX_URI_LEN)
                local_buf[idx++] = buf[i];
            else{
                free(local_buf);
                return NULL;
            }
        }
    }
    return local_buf;
}
char * get_method(const char * buf){
    char * local_buf = malloc(0x10);
    memset(local_buf, 0, 0x10);
    for (int i=0;i<0x10;i++){
        if(buf[i] == 0x20){
            local_buf[i] = '\0';
            return local_buf;
        }
        else if(buf[i] > 0x20 && buf[i]<0x7f)
            local_buf[i] = buf[i];
        else{
            free(local_buf);
            return NULL;
        }
            
    }
    free(local_buf);
    return NULL;
}

int is_method_valid(const char * method){
    char * methods[] = {"GET","POST","HEAD","PUT","DELETE","CONNECT","OPTIONS","TRACE","PATCH",NULL};
    int is_method_valid =0,i=0;
    while(methods[i]){
        if(!strcmp(method, methods[i++])){
            is_method_valid = 1;
        }
    }
    return is_method_valid;
}
int is_method_allowed(const char* method, char ** allow_list){
    int i=0,is_method_allowed=0;
    while(allow_list[i]){
        if(!strcmp(method, allow_list[i++])){
            is_method_allowed = 1;
        }
    }
    return is_method_allowed;
}
char * reply(unsigned int code, char * reply_buf){
    int idx=0;
    char t[0x50];
    time_t timer = time(NULL);
    struct tm * tp = gmtime(&timer);
    strftime(t, 0x50, "%a, %d %b %Y %H:%M:%S GMT\r\n",tp);
    struct http * HTTP = (struct http *)malloc(sizeof(struct http));
    strcpy(HTTP -> ver, "HTTP/1.1 ");
    strcpy(HTTP -> date, t);
    switch (code){
        case 200:
            strcpy(HTTP -> code_str, "200 OK\r\n");
            strcpy(HTTP -> content, reply_buf);
            break;
        case 400:
            strcpy(HTTP -> code_str, "400 Bad Request\r\n");
            strcpy(HTTP -> content, "<h1>400 Bad Request</h1>");
            break;
        case 403:
            strcpy(HTTP -> code_str, "403 Forbidden\r\n");
            strcpy(HTTP -> content, "<h1>403 Forbidden</h1>");
            break;
        case 404:
            strcpy(HTTP -> code_str, "404 Not Found\r\n");
            strcpy(HTTP -> content, "<h1>404 Not Found</h1>");
            break;
        case 405:
            strcpy(HTTP -> code_str, "405 Method Not Allowed\r\n");
            strcpy(HTTP -> content, "<h1>405 Method Not Allowed</h1>");
            break;
        case 413:
            strcpy(HTTP -> code_str, "413 Payload Too Large\r\n");
            strcpy(HTTP -> content, "<h1>413 Payload Too Large</h1>");
            break;
        case 414:
            strcpy(HTTP -> code_str, "414 URI Too Long\r\n");
            strcpy(HTTP -> content, "<h1>414 URI Too Long</h1>");
            break;
        
    }
    strcpy(HTTP->content_length,"Content-Length: ");
    memset(t,0,0x50);
    sprintf(t,"%ld",strlen(HTTP -> content));
    strcat(HTTP->content_length,t);
    strcat(HTTP->content_length,"\r\n");
    strcpy(HTTP -> ser_name, "Server: msh/1.5.2\r\n");
    strcpy(HTTP -> content_type, "Content-Type: text/html\r\n");
    strcpy(HTTP -> connect,"Connection: Close\r\n\r\n");

    memset(reply_buf,0,MAX_REP_LEN);
    strcat(reply_buf,HTTP->ver);
    strcat(reply_buf,HTTP->code_str);
    strcat(reply_buf,HTTP->content_type);
    strcat(reply_buf,HTTP->date);
    strcat(reply_buf,HTTP->ser_name);
    strcat(reply_buf,HTTP->content_length);
    strcat(reply_buf,HTTP->connect);
    strcat(reply_buf,HTTP->content);
    free(HTTP);
}

char * get_conf(){
    FILE * f = fopen("./conf","r"); 
    char c;
    int len = 0,idx = 0;  
    char * buf = NULL;
    fseek(f,0,SEEK_END);
    len = ftell(f);
    fseek(f,0,SEEK_SET);
    buf = malloc(len);
    if(!buf)
        error_hander("allocation failed");
    
    while ((c = fgetc(f)) && !feof(f)) 
        buf[idx++] = c;
    fclose(f);
    return buf;
}