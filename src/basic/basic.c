#include "basic.h"
#include <stdlib.h>

char * int2str(int n){
    if (n == 0){
        return "0";
    }
    char * str;
    int m=n,len=0;
    while(m > 0){
        len++;
        m /= 10;
    }
    str = (char *)malloc(len+1);
    if(str == NULL){
        return "\0";
    }
    *(str+len) = '\0';
    while(--len >= 0){
        *(str+len) = '0'+(n % 10);
        n /= 10;
    }
    return str;
}

long str2int(char *s,size_t len){
    long n = 0,negative=0;
    char *end = s + len;
    if(*s == '-'){
        negative = 1;
        s++;
    }

    while(s < end){
        n = n*10 + (*s - '0');
        s++;
    }
    if(negative){
        n = -n;
    }
    return n;
}

long hexstr2int(char *s,size_t len){
    long n = 0,negative=0;
    char *end = s + len;
    if(*s == '-'){
        negative = 1;
        s++;
    }

    while(s < end){
        n = n*16 + (*s - '0');
        s++;
    }
    if(negative){
        n = -n;
    }
    return n;
}

void strtolower(char *s,size_t len){
    int i;
    for(i=0;i<len;++i){
        if(*(s+i) >= 'A' && *(s+i) <= 'Z'){
            *(s+i) = 'a' + (*(s+i)-'A');
        }
    }
}