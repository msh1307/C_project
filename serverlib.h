#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#define MAX_URI_LEN 0x50-1
#define MAX_PAY_LEN 0x500
#define MAX_REP_LEN 0x500


struct http{
    char ver[0x10];
    char code_str[0x20];// crlf
    char content_type[0x20]; // crlf
    char date[0x20]; // crlf
    char ser_name[0x20]; // crlf
    char content_length[0x20]; // crlf
    char connect[0x20]; // crlf*2
    char content[0x500];
};

int port_valid_chk(char ** argv);
void error_hander(const char * buf);
char * get_uri(const char * buf);
char * get_method(const char * buf);
int is_method_valid(const char * method);
int is_method_allowed(const char* method, char ** allow_list);
char * reply(unsigned int code, char * reply_buf);
char * get_conf();