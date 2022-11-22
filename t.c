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

struct config {
    char SERVER_NAME[0x20];
    char VERSION[0x20];
    char * METHODS_LIST[0x10];
    char METHODS_MODE[0x10];
    char * PATH_LIST[0x20];
    char PATH_MODE[0x20];
    char * FILE_LIST[0x20];
    char FILE_MODE[0x20];
};

struct config * parse_conf(char * conf){
    
}

int main(){
    char * conf = get_conf();
    puts(conf);
}