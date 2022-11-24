#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
unsigned long long file_sz;

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
    buf = malloc(len);
    while (!feof(f)) {
        c = fgetc(f);
        buf[idx++] = c;
    }
        
    fclose(f);
    file_sz = idx;
    return buf;
}
int main(){
    char * ptr = get_file("./mAcAt.jpg");
    printf("%llu",file_sz);
}