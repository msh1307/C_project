#include "serverlib.h"

extern unsigned long long file_sz;
extern size_t header_sz;

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
    add_ele(method_list);
    add_ele(file_list);
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
int perm_chk(char * p, char * file_list[]){
    int idx=0;
    int flag = 0;
    while(file_list[idx]){
        if(!strcmp(p,file_list[idx])){
            flag = 1;
        }
        printf("%s == %s %d\n",p,file_list[idx],flag);
        idx++;
    }

    return flag;
}
int ncpy(char * dest, char * src, unsigned long long int cnt){
    for(int i =0;i<cnt;i++){
        dest[i] = src[i];
    }
}
char * get_file_extension(char * buf){
    int len= strlen(buf);
    for(int i = len-1; i>=0; i--){
        if(buf[i] =='.'){
            return buf+i;
        }
    }
}