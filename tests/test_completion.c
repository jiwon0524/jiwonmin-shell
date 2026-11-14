#define _POSIX_C_SOURCE 200809L
#include "../src/completion.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
int main(void) {
    char tmp[]="/tmp/jiwonmin-tab-XXXXXX";
    assert(mkdtemp(tmp));
    assert(chdir(tmp)==0);
    assert(mkdir("Documents",0700)==0);
    assert(mkdir("Downloads",0700)==0);
    FILE *f=fopen("report.txt","w");assert(f);fclose(f);
    size_t pos=0,matches=0;
    char *s=completion_expand("cd Doc",6,&pos,&matches);
    assert(s && !strcmp(s,"cd Documents/") && pos==13 && matches==1);free(s);
    s=completion_expand("cd Do",5,&pos,&matches);
    assert(!s && matches==2); /* no additional shared characters */
    free(s);
    s=completion_expand("cat rep",7,&pos,&matches);
    assert(s && !strcmp(s,"cat report.txt") && matches==1);free(s);
    s=completion_expand("cat missing",11,&pos,&matches);
    assert(!s && matches==0);
    s=completion_expand("mkd",3,&pos,&matches);
    assert(s && !strcmp(s,"mkdir") && matches==1);free(s);
    size_t n=0;
    char **names=completion_candidates("cd Do",5,&n);
    assert(names && n==2 && !strcmp(names[0],"Documents/") && !strcmp(names[1],"Downloads/"));
    completion_free_candidates(names,n);
    s=completion_expand("cat rep",7,&pos,&matches);
    assert(s && !strcmp(s,"cat report.txt"));free(s);
    assert(unlink("report.txt")==0);
    assert(rmdir("Documents")==0);
    assert(rmdir("Downloads")==0);
    assert(chdir("/")==0);
    assert(rmdir(tmp)==0);
    puts("7 tab completion checks passed");
    return 0;
}
