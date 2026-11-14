#define _POSIX_C_SOURCE 200809L
#include "chain.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void chain_free(Chain *c) {
    for (size_t i=0;i<c->count;i++) { free(c->items[i].text); c->items[i].text=NULL; }
    c->count=0;
}
static int error(char *err,size_t cap,const char *msg) {
    if (cap) snprintf(err,cap,"%s",msg);
    return -1;
}
int chain_parse(const char *input,Chain *out,char *err,size_t cap) {
    memset(out,0,sizeof(*out));
    ChainOp before=CHAIN_FIRST;
    size_t start=0, i=0;
    char quote=0;
    for (;;i++) {
        char ch=input[i];
        if (ch=='\\' && input[i+1]) { i++; continue; }
        if (quote) {
            if (!ch) { chain_free(out); return error(err,cap,"닫히지 않은 따옴표"); }
            if (ch==quote) quote=0;
            continue;
        }
        if (ch=='\'' || ch=='"') { quote=ch; continue; }
        ChainOp next=CHAIN_FIRST;
        size_t advance=0;
        if (ch==';') { next=CHAIN_ALWAYS; advance=1; }
        else if (ch=='&' && input[i+1]=='&') { next=CHAIN_AND; advance=2; }
        else if (ch=='|' && input[i+1]=='|') { next=CHAIN_OR; advance=2; }
        if (!ch || advance) {
            size_t a=start,b=i;
            while (a<b && isspace((unsigned char)input[a])) a++;
            while (b>a && isspace((unsigned char)input[b-1])) b--;
            if (a==b) {
                /* A trailing semicolon is legal, but empty commands elsewhere are not. */
                if (!ch && before==CHAIN_ALWAYS && out->count) return 0;
                if (!ch && !out->count) return 0;
                chain_free(out);return error(err,cap,"명령어 연결 연산자 주변에 빈 명령어가 있습니다");
            }
            if (out->count==CHAIN_MAX) {chain_free(out);return error(err,cap,"명령어 연결 개수 초과");}
            char *part=strndup(input+a,b-a);
            if (!part) {chain_free(out);return error(err,cap,"메모리 부족");}
            out->items[out->count].text=part;
            out->items[out->count].before=before;
            out->count++;
            if (!ch) return 0;
            before=next;
            i+=advance-1;
            start=i+1;
        }
    }
}
