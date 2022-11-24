#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#define MAX_REP_LEN 0x200000+0xe0
unsigned long long file_sz=0;
size_t header_sz=0;
struct http{
    char ver[0x10];
    char code_str[0x20];// crlf
    char content_type[0x20]; // crlf
    char date[0x20]; // crlf
    char ser_name[0x20]; // crlf
    char content_length[0x20]; // crlf
    char connect[0x20]; // crlf*2
};
struct config {
    char server_name[0x20];
    char version[0x20];
    char * method_list[0x10];
    char base_path[0x20];
    char * file_list[0x10];
    char default_[0x20];
};
enum last {
    NONE = 1, CONF_ST, SER_NAME, VER, METH_LIST,FILE_LIST,BASE_PATH, DEFAULT_
};
int ncpy(char * dest, char * src, unsigned long long int cnt){
    for(int i =0;i<cnt;i++){
        dest[i] = src[i];
    }
}
void error_hander(const char * buf){
    fputs(buf,stderr);
    fputc('\x0a',stderr);
    exit(-1);
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
void reply(unsigned int code, char * reply_buf, char * server_name, char * server_version,char * content_type){
    int idx=0;
    char t[0x50];
    char * content = NULL;
    time_t timer = time(NULL);
    struct tm * tp = gmtime(&timer);
    strftime(t, 0x50, "%a, %d %b %Y %H:%M:%S GMT\r\n",tp);
    struct http * HTTP = (struct http *)malloc(sizeof(struct http));
    if(code == 200)
        content = malloc(file_sz);
    else
        content = malloc(0x50);
    strcpy(HTTP -> ver, "HTTP/1.1 ");
    strcpy(HTTP -> date, t);
    switch (code){
        case 200:
            strcpy(HTTP -> code_str, "200 OK\r\n");
            ncpy(content, reply_buf,file_sz);
            break;
        case 400:
            strcpy(HTTP -> code_str, "400 Bad Request\r\n");
            strcpy(content, "<h1>400 Bad Request</h1>");
            break;
        case 403:
            strcpy(HTTP -> code_str, "403 Forbidden\r\n");
            strcpy(content, "<h1>403 Forbidden</h1>");
            break;
        case 404:
            strcpy(HTTP -> code_str, "404 Not Found\r\n");
            strcpy(content, "<h1>404 Not Found</h1>");
            break;
        case 405:
            strcpy(HTTP -> code_str, "405 Method Not Allowed\r\n");
            strcpy(content, "<h1>405 Method Not Allowed</h1>");
            break;
        case 413:
            strcpy(HTTP -> code_str, "413 Payload Too Large\r\n");
            strcpy(content, "<h1>413 Payload Too Large</h1>");
            break;
        case 414:
            strcpy(HTTP -> code_str, "414 URI Too Long\r\n");
            strcpy(content, "<h1>414 URI Too Long</h1>");
            break;
        
    }
    strcpy(HTTP->content_length,"Content-Length: ");
    memset(t,0,0x50);
    if(code == 200){
        sprintf(t,"%llu",file_sz);
    }
    else{
        sprintf(t,"%ld",strlen(content));
    }
    strcat(HTTP->content_length,t);
    strcat(HTTP->content_length,"\r\n");
    strcpy(HTTP -> ser_name, "Server: ");
    strcat(HTTP -> ser_name, server_name);
    strcat(HTTP -> ser_name, "/");
    strcat(HTTP -> ser_name, server_version);
    strcat(HTTP -> ser_name, "\r\n");
    strcpy(HTTP -> content_type, "Content-Type: ");
    strcat(HTTP -> content_type,content_type);
    strcat(HTTP -> content_type,"\r\n");
    strcpy(HTTP -> connect,"Connection: Close\r\n\r\n");

    memset(reply_buf,0,MAX_REP_LEN);
    strcat(reply_buf,HTTP->ver);
    strcat(reply_buf,HTTP->code_str);
    strcat(reply_buf,HTTP->content_type);
    strcat(reply_buf,HTTP->date);
    strcat(reply_buf,HTTP->ser_name);
    strcat(reply_buf,HTTP->content_length);
    strcat(reply_buf,HTTP->connect);
    header_sz = strlen(reply_buf);
    ncpy(reply_buf+strlen(reply_buf),content,file_sz);

    free(content);
    free(HTTP); 
}

int dif(char * str1, const char * str2){ // cmp except null
    size_t len = strlen(str2);
    int flag=0;
    for(int i=0;i<len;i++){
        if(str1[i] != str2[i]) 
            flag = 1;
    }
    return flag;
}

int word_len(char * str){ 
    int i =0;
    while(str[i] != '\x0a' && str[i] != '\x20' && str[i] != '\x09'&& str[i] != '\x0d'){
        if(i > 0x20-1)
            return -1;
        i++;
    }
    return i;
}
int list_len(char * str){ 
    int i =0;
    while(str[i] != '\x0a' && str[i] != '\x20' && str[i] != '\x09'&& str[i] != '\x0d'){
        if(i > 0x60-1)
            return -1;
        i++;
    }
    return i;
}
struct config * parse_conf(char * buf){
    struct config * conf = malloc(sizeof(struct config));
    enum last last = NONE;
    int len = strlen(buf);
    int cnt,idx;
    int i =0, delim=0;
    int on = 0;
    int wlen = 0;
    char * method_list = malloc(0xA0);
    char * file_list = malloc(0x200);
    memset(conf, 0, sizeof(struct config));
    memset(method_list, 0, sizeof(method_list));
    memset(file_list,0,sizeof(file_list));
    while(i < len){
        if(buf[i] == '\x20' | buf[i] == '\x0a' | buf[i] == '\x09'| buf[i] == '\x0d'){
            i++;
            continue;
        }
        switch (last){
            case NONE:
                if(buf[i] == 'c'){
                    if(!dif(buf + i,"conf")){
                        last = CONF_ST;
                        i += 4;
                        continue;
                    }  
                }
                i++;
                break;
            case CONF_ST:
                if(buf[i] == 'S'){
                    if(!dif(buf + i,"SERVER_NAME")){
                        last = SER_NAME;
                        i += 11;
                        continue;
                    }  
                }
                else if(buf[i] == 'V'){
                    if(!dif(buf + i,"VERSION")){
                        last = VER;
                        i += 7;
                        continue;
                    }  
                }
                else if(buf[i] == 'M'){
                    if(!dif(buf + i,"METHOD_LIST")){
                        last = METH_LIST;
                        i += 11;
                        continue;
                    }  
                }
                else if(buf[i] == 'B'){
                    if(!dif(buf + i,"BASE_PATH")){
                        last = BASE_PATH;
                        i += 9;
                        continue;
                    }  
                }
                else if(buf[i] == 'F'){
                    if(!dif(buf + i,"FILE_LIST")){
                        last = FILE_LIST;
                        i += 9;
                        continue;
                    }  
                }
                else if(buf[i] == 'D'){
                    if(!dif(buf + i,"DEFAULT")){
                        last = DEFAULT_;
                        i += 7;
                        continue;
                    }  
                }
                i++;
                break;
            case SER_NAME:
                if(delim){
                    wlen = word_len(buf+i);
                    if(wlen == -1)
                        error_hander("conf error : SERVER_NAME / Maximum length is 31");
                    strncpy(conf -> server_name,buf+i, wlen);
                    conf -> server_name[wlen] = '\x00';
                    last = CONF_ST;
                    delim = 0;
                    i += wlen;
                    wlen = 0;
                    continue;
                }
                else{
                    if(buf[i] == ':'){
                        delim = 1;
                    }
                }
                i++;
                break;
            case VER:
                if(delim){
                    wlen = word_len(buf+i);
                    if(wlen == -1)
                        error_hander("conf error : VERSION / Maximum length is 31");
                    strncpy(conf -> version,buf+i, wlen);
                    conf -> version[wlen] = '\x00';
                    last = CONF_ST;
                    delim = 0;
                    i += wlen;
                    wlen = 0;
                    continue;
                }
                else{
                    if(buf[i] == ':'){
                        delim = 1;
                    }
                }
                i++;
                break;
            case METH_LIST:
                if(delim){
                    wlen = list_len(buf+i);
                    if(wlen == -1)
                        error_hander("conf error : METHOD_LIST / Maximum length is 95");
                    cnt = 0;
                    idx = 0;
                    memset(method_list,0,sizeof(method_list));
                    on = 0;
                    for (int j =0;j<wlen;j++){
                        if(buf[i+j] == '\x20' | buf[i+j] == '\x0a' | buf[i+j] == '\x09'| buf[i+j] == '\x0d')
                            continue;
                        if(cnt > 15)
                            error_hander("conf error : METHOD_LIST / Maximum method count is 9");
                        if(buf[i+j] == ','){
                            if(!is_method_valid(method_list + cnt*0x10))
                                error_hander("conf error : METHOD_LIST / invalid method");
                            conf -> method_list[cnt] = method_list + cnt*0x10;
                            method_list[cnt*0x10 + idx] = '\x00';
                            cnt++;
                            idx = 0;
                        }
                        else{
                            method_list[cnt*0x10 + idx++] = buf[i+j];
                            on = 1;
                        }
                    }
                    if(!on)
                        error_hander("conf error : METHOD_LIST / empty list");
                    method_list[cnt*0x10 + idx] = '\x00';
                    conf -> method_list[cnt] = method_list + cnt*0x10;
                    if(!is_method_valid(method_list + cnt*0x10))
                        error_hander("conf error : METHOD_LIST / invalid method");
                    conf -> method_list[cnt+1] = NULL;
                    last = CONF_ST;
                    delim = 0;
                    i += wlen;
                    wlen = 0;
                    continue;
                }
                else{
                    if(buf[i] == ':'){
                        delim = 1;
                    }
                }
                i++;
                break;
            case BASE_PATH:
                if(delim){
                    wlen = word_len(buf+i);
                    if(wlen == -1)
                        error_hander("conf error : BASE_PATH / Maximum length is 31");
                    strncpy(conf -> base_path,buf+i, wlen);
                    conf -> base_path[wlen] = '\x00';
                    last = CONF_ST;
                    delim = 0;
                    i += wlen;
                    wlen = 0;
                    continue;
                }
                else{
                    if(buf[i] == ':'){
                        delim = 1;
                    }
                }
                i++;
                break;
            case FILE_LIST:
                if(delim){
                    wlen = list_len(buf+i);
                    if(wlen == -1)
                        error_hander("conf error : FILE_LIST / Maximum length is 95");
                    cnt = 0;
                    idx = 0;
                    on = 0;
                    memset(file_list,0,sizeof(file_list));
                    for (int j =0;j<wlen;j++){
                        if(buf[i+j] == '\x20' | buf[i+j] == '\x0a' | buf[i+j] == '\x09'| buf[i+j] == '\x0d')
                            continue;
                        if(cnt > 16)
                            error_hander("conf error : FILE_LIST / Maximum file count is 16");
                        if(buf[i+j] == ','){
                            conf -> file_list[cnt] = file_list + cnt*0x20;
                            file_list[cnt*0x20 + idx] = '\x00';
                            cnt++;
                            idx = 0;
                        }
                        else{
                            file_list[cnt*0x20 + idx++] = buf[i+j];
                            on = 1;
                        }
                    }
                    if(!on)
                        error_hander("conf error : FILE_LIST / empty file list");
                    file_list[cnt*0x20 + idx] = '\x00';
                    conf -> file_list[cnt] = file_list + cnt*0x20;
                    conf -> file_list[cnt+1] = NULL;
                    last = CONF_ST;
                    delim = 0;
                    i += wlen;
                    wlen = 0;
                    continue;
                }
                else{
                    if(buf[i] == ':'){
                        delim = 1;
                    }
                }
                i++;
                break;
            case DEFAULT_:
                if(delim){
                    wlen = word_len(buf+i);
                    if(wlen == -1)
                        error_hander("conf error : DEFAULT / Maximum length is 31");
                    strncpy(conf -> default_,buf+i, wlen);
                    conf -> default_[wlen] = '\x00';
                    last = CONF_ST;
                    delim = 0;
                    i += wlen;
                    wlen = 0;
                    continue;
                }
                else{
                    if(buf[i] == ':'){
                        delim = 1;
                    }
                }
                i++;
                break;
            default : 
                last = CONF_ST;
                continue;
        }
    }
    return conf;
}

char * get_file(char * filename){
    FILE * f = fopen(filename,"r"); 
    char c;
    int len = 0;
    unsigned long long int idx = 0; 
    char * buf = NULL;
    if(f == NULL) 
        return NULL;
    fseek(f,0,SEEK_END);
    len = ftell(f);
    fseek(f,0,SEEK_SET);
    if(len > 0x200000)
        error_hander("length error");
    buf = malloc(len);
    if(!buf)
        error_hander("allocation failed");
    while (!feof(f)) {
        c = fgetc(f);
        buf[idx++] = c;
    }
        
    fclose(f);
    file_sz = idx-1;
    return buf;
}
int main(){
    char * reply_buf = malloc(0x200000);
    char * file_content = get_file("mAcAt.jpg");
    ncpy(reply_buf, file_content,file_sz);
    reply(200,reply_buf,"FUCK","1.5.2","text/html");
    for (int i =0;i<0x100;i++) {
        printf("%c",reply_buf[i]);   
    }
}