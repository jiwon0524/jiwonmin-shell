#define _POSIX_C_SOURCE 200809L
#include "command_check.h"
#include <ctype.h>
#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static const char *const builtin[] = {
    "cd", "exit", "help", "history", "jobs", "fg", "jiwonmin", "export", "unset", "env",
    "ls", "pwd", "mkdir", "rmdir", "ln", "cp", "rm", "mv", "cat", "grep", NULL
};
int command_word(const char *line, size_t *begin, size_t *end) {
    if (!line || !begin || !end) return 0;
    size_t i=0;
    while (line[i]==' ' || line[i]=='\t') ++i;
    if (!line[i] || line[i]=='|' || line[i]=='<' || line[i]=='>') return 0;
    *begin=i;
    while(line[i] && !isspace((unsigned char)line[i]) &&
          line[i]!='|' && line[i]!='<' && line[i]!='>') ++i;
    *end=i;
    return i>*begin;
}
static int executable(const char *path) {
    struct stat st;
    return stat(path,&st)==0 && S_ISREG(st.st_mode) && access(path,X_OK)==0;
}
static int path_has(const char *name) {
    const char *path=getenv("PATH");
    if (!path) path="/bin:/usr/bin";
    const char *p=path;
    for (;;) {
        const char *end=strchr(p,':');
        size_t len=end?(size_t)(end-p):strlen(p);
        const char *dir=len?p:".";
        size_t dlen=len?len:1;
        size_t nlen=strlen(name);
        if (dlen <= (size_t)-1 - nlen - 2) {
            char *full=malloc(dlen+nlen+2);
            if (full) {
                memcpy(full,dir,dlen);full[dlen]='/';
                memcpy(full+dlen+1,name,nlen+1);
                int ok=executable(full);
                free(full);
                if(ok) return 1;
            }
        }
        if (!end) break;
        p=end+1;
    }
    return 0;
}
int command_exists(const char *name) {
    if (!name || !*name) return 0;
    if (strchr(name,'/')) return executable(name);
    for (size_t i=0;builtin[i];++i)
        if (!strcmp(name,builtin[i])) return 1;
    return path_has(name);
}
/* Bounded edit distance with adjacent-character transpositions. */
static unsigned distance(const char *a,const char *b) {
    size_t n=strlen(a),m=strlen(b);
    if (n>64 || m>64) return UINT_MAX;
    unsigned d[65][65];
    for(size_t i=0;i<=n;i++)d[i][0]=(unsigned)i;
    for(size_t j=0;j<=m;j++)d[0][j]=(unsigned)j;
    for(size_t i=1;i<=n;i++)for(size_t j=1;j<=m;j++) {
        unsigned sub=d[i-1][j-1]+(a[i-1]!=b[j-1]);
        unsigned del=d[i-1][j]+1,ins=d[i][j-1]+1;
        unsigned best=sub<del?sub:del;
        if(ins<best)best=ins;
        if(i>1 && j>1 && a[i-1]==b[j-2] && a[i-2]==b[j-1]) {
            unsigned trans=d[i-2][j-2]+1;
            if(trans<best)best=trans;
        }
        d[i][j]=best;
    }
    return d[n][m];
}
static void consider(const char *input,const char *candidate,unsigned *best,char **result) {
    unsigned score=distance(input,candidate);
    if(score==0 || score>*best) return;
    if(score==*best && *result && strcmp(candidate,*result)>=0) return;
    char *copy=strdup(candidate);
    if(!copy)return;
    free(*result);*result=copy;*best=score;
}
char *command_suggest(const char *name) {
    if (!name || !*name || strchr(name,'/') || strlen(name)>64) return NULL;
    unsigned best=strlen(name)<=3?1:2;
    char *result=NULL;
    for(size_t i=0;builtin[i];i++)consider(name,builtin[i],&best,&result);
    if(result && best<=1) return result;
    const char *path=getenv("PATH");
    if(!path)path="/bin:/usr/bin";
    char *copy=strdup(path);
    if(!copy)return result;
    char *start=copy;
    for(char *p=copy;;p++) {
        if(*p!=':' && *p!='\0')continue;
        int last=*p=='\0';*p='\0';
        const char *dir=*start?start:".";
        DIR *d=opendir(dir);
        if(d){
            struct dirent *e;
            while((e=readdir(d))) {
                if(e->d_name[0]=='.' || strlen(e->d_name)>64)continue;
                unsigned score=distance(name,e->d_name);
                if(score==0 || score>best)continue;
                size_t len=strlen(dir)+strlen(e->d_name)+2;
                char *full=malloc(len);
                if(!full)continue;
                snprintf(full,len,"%s/%s",dir,e->d_name);
                if(executable(full))consider(name,e->d_name,&best,&result);
                free(full);
            }
            closedir(d);
        }
        if(last)break;
        start=p+1;
    }
    free(copy);
    return result;
}
