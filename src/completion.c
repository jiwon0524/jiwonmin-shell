#define _POSIX_C_SOURCE 200809L
#include "completion.h"
#include <dirent.h>
#include <stdio.h>
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
static const char *const builtins[] = {
    "cd", "exit", "help", "history", "jobs", "fg", "jiwonmin",
    "ls", "pwd", "mkdir", "rmdir", "ln", "cp", "rm", "mv", "cat", "grep", NULL
};

static int is_command_position(const char *line, size_t begin) {
    size_t i=begin;
    while (i && (line[i-1]==' ' || line[i-1]=='\t')) --i;
    return i==0 || line[i-1]=='|';
}
static int candidate_add(char ***items, size_t *count, size_t *cap, const char *name) {
    for (size_t i=0;i<*count;i++) if (!strcmp((*items)[i],name)) return 0;
    if (*count==*cap) {
        size_t n=*cap ? *cap*2 : 16;
        char **p=realloc(*items,n*sizeof(*p));
        if (!p) return -1;
        *items=p;*cap=n;
    }
    (*items)[*count]=strdup(name);
    if (!(*items)[*count]) return -1;
    ++*count;
    return 0;
}
static int candidate_cmp(const void *a, const void *b) {
    const char *const *x=a,*const *y=b;
    return strcmp(*x,*y);
}
void completion_free_candidates(char **items, size_t count) {
    for (size_t i=0;i<count;i++) free(items[i]);
    free(items);
}
char **completion_candidates(const char *line, size_t cursor, size_t *count) {
    if (!line || !count || cursor>strlen(line)) return NULL;
    *count=0;
    size_t begin=cursor;
    while (begin && line[begin-1]!=' ' && line[begin-1]!='\t' && line[begin-1]!='|' && line[begin-1]!='<' && line[begin-1]!='>') --begin;
    if (cursor==begin) return NULL;
    for(size_t i=begin;i<cursor;i++) if (line[i]=='"'||line[i]=='\''||line[i]=='\\') return NULL;
    size_t slash=cursor;
    while(slash>begin && line[slash-1]!='/') --slash;
    char *prefix=strndup(line+slash,cursor-slash);
    char *directory=slash==begin?strdup("."):strndup(line+begin,slash-begin);
    if (!prefix||!directory) {free(prefix);free(directory);return NULL;}
    char **items=NULL;size_t cap=0;
    int command=(slash==begin && is_command_position(line,begin));
    if (command) {
        for(size_t i=0;builtins[i];i++)
            if (!strncmp(builtins[i],prefix,strlen(prefix)) && candidate_add(&items,count,&cap,builtins[i])) goto fail;
        const char *path=getenv("PATH");
        if (!path) path="/usr/bin:/bin";
        char *copy=strdup(path);
        if (!copy) goto fail;
        char *start=copy;
        for(char *q=copy;;q++) {
            if (*q!=':' && *q!='\0') continue;
            int last=(*q=='\0');*q='\0';
            const char *d=*start?start:".";
            DIR *dir=opendir(d);
            if (dir) {
                struct dirent *e;
                while((e=readdir(dir))) {
                    if (strncmp(e->d_name,prefix,strlen(prefix))) continue;
                    size_t n=strlen(d)+strlen(e->d_name)+2;
                    char *full=malloc(n);
                    if (!full) {closedir(dir);free(copy);goto fail;}
                    snprintf(full,n,"%s/%s",d,e->d_name);
                    struct stat st;
                    if (stat(full,&st)==0 && S_ISREG(st.st_mode) && access(full,X_OK)==0 &&
                        candidate_add(&items,count,&cap,e->d_name)) {free(full);closedir(dir);free(copy);goto fail;}
                    free(full);
                }
                closedir(dir);
            }
            if (last) break;
            start=q+1;
        }
        free(copy);
    } else {
        DIR *dir=opendir(directory);
        if (dir) {
            struct dirent *e;
            while((e=readdir(dir))) {
                if (e->d_name[0]=='.' && prefix[0]!='.') continue;
                if (strncmp(e->d_name,prefix,strlen(prefix))) continue;
                if (strpbrk(e->d_name," \t\n\\\"'")) continue;
                size_t n=strlen(directory)+strlen(e->d_name)+2;
                char *full=malloc(n);
                if (!full) {closedir(dir);goto fail;}
                snprintf(full,n,"%s/%s",directory,e->d_name);
                struct stat st;
                int folder=stat(full,&st)==0 && S_ISDIR(st.st_mode);
                free(full);
                n=strlen(e->d_name);
                char *display=malloc(n+2);
                if (!display) {closedir(dir);goto fail;}
                memcpy(display,e->d_name,n);
                if(folder) display[n++]='/';
                display[n]='\0';
                int rc=candidate_add(&items,count,&cap,display);
                free(display);
                if(rc) {closedir(dir);goto fail;}
            }
            closedir(dir);
        }
    }
    free(prefix);free(directory);
    qsort(items,*count,sizeof(*items),candidate_cmp);
    return items;
fail:
    free(prefix);free(directory);
    completion_free_candidates(items,*count);*count=0;
    return NULL;
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
    size_t count=0;
    char **items=completion_candidates(line,cursor,&count);
    *matches=count;
    if (!items || !count) {completion_free_candidates(items,count);return NULL;}
    size_t prefix_len=cursor-slash;
    size_t common_len=strlen(items[0]);
    for(size_t i=1;i<count;i++) {
        size_t k=0;
        while(k<common_len && items[i][k] && items[0][k]==items[i][k]) k++;
        common_len=k;
    }
    if(count>1 && common_len<=prefix_len) {completion_free_candidates(items,count);return NULL;}
    size_t chosen_len=count==1?strlen(items[0]):common_len;
    char *result=append(line,slash,items[0],chosen_len,line+cursor,len-cursor);
    if(result) *new_cursor=slash+chosen_len;
    completion_free_candidates(items,count);
    return result;
}
