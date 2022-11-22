#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

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
        puts("allocation failed");
    
    while ((c = fgetc(f)) && !feof(f)) 
        buf[idx++] = c;
    fclose(f);
    return buf;
}

void error_hander(const char * buf){
    fputs(buf,stderr);
    fputc('\x0a',stderr);
    exit(-1);
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

struct config {
    char server_name[0x20];
    char version[0x20];
    char * method_list[0x10];
    char * path_list[0x10];
    char * file_list[0x10];
};
enum last {
    NONE = 1, CONF_ST, SER_NAME, VER, METH_LIST,FILE_LIST,PATH_LIST
};
struct config * parse_conf(char * buf){
    struct config * conf = malloc(sizeof(struct config));
    enum last last = NONE;
    int len = strlen(buf);
    int i =0, delim=0;
    int wlen = 0;
    char * method_list = malloc(0xA0);
    char * file_list = malloc(0xA0);
    char * path_list = malloc(0xA0);

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
                else if(buf[i] == 'P'){
                    if(!dif(buf + i,"PATH_LIST")){
                        last = PATH_LIST;
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
                i++;
                break;
            case SER_NAME:
                if(delim){
                    wlen = word_len(buf+i);
                    if(wlen == -1)
                        error_hander("conf length error : SERVER_NAME");
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
                        error_hander("conf length error : VERSION");
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
                        error_hander("conf length error : METHOD_LIST");
                    strncpy(method_list,buf+i, wlen);
                    method_list[wlen] = '\x00';
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
                        error_hander("conf length error : FILE_LIST");
                    strncpy(file_list,buf+i, wlen);
                    file_list[wlen] = '\x00';
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
            case PATH_LIST:
                if(delim){
                    wlen = list_len(buf+i);
                    if(wlen == -1)
                        error_hander("conf length error : PATH_LIST");
                    strncpy(path_list,buf+i, wlen);
                    path_list[wlen] = '\x00';
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

int main(){
    char * buf = get_conf();
    struct config * conf = parse_conf(buf);

}