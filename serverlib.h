#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#define MAX_URI_LEN 0x50-1
#define MAX_PAY_LEN 0x500
#define MAX_REP_LEN 0x600
#define add_ele(ele) mem_list[j++] = ele

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
extern int j; 
extern void * mem_list[0x20];

int port_valid_chk(char ** argv);
void error_hander(const char * buf);
char * get_uri(const char * buf);
char * get_method(const char * buf);
int is_method_valid(const char * method);
int is_method_allowed(const char* method, char ** allow_list);
void reply(unsigned int code, char * reply_buf, char * server_name, char * server_version);
char * get_file(char * filename);
int dif(char * str1, const char * str2);
int word_len(char * str);
int list_len(char * str);
struct config * parse_conf(char * buf);
int perm_chk(char * p, char * file_list[]);
int ncpy(char * dest, char * src, size_t cnt);