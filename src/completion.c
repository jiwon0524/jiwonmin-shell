#define _POSIX_C_SOURCE 200809L
#include "completion.h"
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char *append(const char *a, size_t an, const char *b, size_t bn, const char *tail, size_t tn) {
    if (an > (size_t)-1 - bn - tn - 1) return NULL;
    char *p=malloc(an+bn+tn+1);
    if (!p) return NULL;
    memcpy(p,a,an); memcpy(p+an,b,bn); memcpy(p+an+bn,tail,tn);
    p[an+bn+tn]='\0';
    return p;
}
char *completion_expand(const char *line, size_t cursor, size_t *new_cursor, size_t *matches) {
    if (!line || !new_cursor || !matches) return NULL;
    *matches=0;
    size_t len=strlen(line);
    if (cursor>len) return NULL;
    /* Basic unquoted tokens only. Do not silently alter quoted or escaped paths. */
    size_t begin=cursor;
    while (begin && line[begin-1]!=' ' && line[begin-1]!='\t' && line[begin-1]!='|' && line[begin-1]!='<' && line[begin-1]!='>') --begin;
    for (size_t i=begin;i<cursor;i++) if (line[i]=='"'||line[i]=='\''||line[i]=='\\') return NULL;
    if (cursor==begin) return NULL;
    size_t slash=cursor;
    while (slash>begin && line[slash-1]!='/') --slash;
    size_t prefix_len=cursor-slash;
    char *prefix=strndup(line+slash,prefix_len);
    char *directory=slash==begin ? strdup(".") : strndup(line+begin,slash-begin);
    if (!prefix||!directory) {free(prefix);free(directory);return NULL;}
    DIR *dir=opendir(directory);
    if (!dir) {free(prefix);free(directory);return NULL;}
    char *common=NULL, *single=NULL;
    size_t count=0;
    struct dirent *entry;
    while ((entry=readdir(dir))!=NULL) {
        const char *name=entry->d_name;
        if (name[0]=='.' && prefix[0]!='.') continue;
        if (strncmp(name,prefix,prefix_len)) continue;
        /* Unquoted whitespace requires parser escaping; do not insert it. */
        if (strpbrk(name," \t\n\\\"'")) continue;
        if (!common) {
            common=strdup(name); single=strdup(name);
            if (!common||!single) {free(common);free(single);closedir(dir);free(prefix);free(directory);return NULL;}
        } else {
            size_t k=0;
            while (common[k] && name[k] && common[k]==name[k]) ++k;
            common[k]='\0';
        }
        ++count;
    }
    closedir(dir);
    *matches=count;
    if (!count) {free(prefix);free(directory);return NULL;}
    const char *chosen=count==1?single:common;
    size_t chosen_len=strlen(chosen);
    if (chosen_len<=prefix_len && count!=1) {free(common);free(single);free(prefix);free(directory);return NULL;}
    int is_directory=0;
    if (count==1) {
        size_t dn=strlen(directory),sn=strlen(single);
        char *full=malloc(dn+sn+2);
        if (full) {
            memcpy(full,directory,dn);full[dn]='/';memcpy(full+dn+1,single,sn+1);
            struct stat st;
            if (stat(full,&st)==0 && S_ISDIR(st.st_mode)) is_directory=1;
            free(full);
        }
    }
    char *addition=append(chosen,chosen_len,is_directory?"/":"",is_directory?1:0,"",0);
    char *result=NULL;
    if (addition) {
        size_t n=strlen(addition);
        result=append(line,slash,addition,n,line+cursor,len-cursor);
        if (result) *new_cursor=slash+n;
    }
    free(addition);free(common);free(single);free(prefix);free(directory);
    return result;
}
