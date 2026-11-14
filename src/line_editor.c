#define _POSIX_C_SOURCE 200809L
#include "line_editor.h"
#include "completion.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

static int write_all(const char *s, size_t n) {
    while (n) {
        ssize_t r = write(STDOUT_FILENO, s, n);
        if (r < 0 && errno == EINTR) continue;
        if (r <= 0) return -1;
        s += r; n -= (size_t)r;
    }
    return 0;
}
static int redraw(const char *prompt, const char *s, size_t len, size_t pos) {
    char seq[64];
    if (write_all("\r\033[2K", 5) < 0 ||
        write_all(prompt, strlen(prompt)) < 0 ||
        write_all(s, len) < 0) return -1;
    if (len > pos) {
        int n = snprintf(seq, sizeof(seq), "\033[%zuD", len-pos);
        if (n < 0 || (size_t)n >= sizeof(seq)) return -1;
        if (write_all(seq, (size_t)n) < 0) return -1;
    }
    return 0;
}
static int replace(char **buf, size_t *cap, size_t *len, size_t *pos, const char *s) {
    size_t n = strlen(s);
    if (n + 1 > *cap) {
        char *p = realloc(*buf, n+1);
        if (!p) return -1;
        *buf=p; *cap=n+1;
    }
    memcpy(*buf,s,n+1); *len=*pos=n;
    return 0;
}
char *line_editor_read(const char *prompt, const char *const *history, size_t count) {
    struct termios old, raw;
    if (tcgetattr(STDIN_FILENO, &old) < 0) return NULL;
    raw=old;
    raw.c_lflag &= (tcflag_t)~(ICANON | ECHO);
    raw.c_cc[VMIN]=1; raw.c_cc[VTIME]=0;
    if (tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw)<0) return NULL;
    size_t cap=128,len=0,pos=0,nav=count;
    int tab_pending=0;
    char *buf=malloc(cap), *draft=NULL;
    if (!buf) goto fail;
    buf[0]='\0';
    if (write_all(prompt,strlen(prompt))<0) goto fail;
    for (;;) {
        unsigned char c;
        ssize_t r=read(STDIN_FILENO,&c,1);
        if (r<0 && errno==EINTR) continue;
        if (r<=0) { free(buf); buf=NULL; break; }
        if (c=='\r'||c=='\n') { write_all("\r\n",2); break; }
        if (c==4 && !len) { free(buf); buf=NULL; write_all("\r\n",2); break; }
        if (c=='\t') {
            int was_pending=tab_pending;
            tab_pending=1;
            size_t next_pos=pos, matches=0;
            char *completed=completion_expand(buf,pos,&next_pos,&matches);
            if (completed) {
                size_t n=strlen(completed);
                if (n+1>cap) {
                    char *grown=realloc(buf,n+1);
                    if (!grown) { free(completed); goto fail; }
                    buf=grown; cap=n+1;
                }
                memcpy(buf,completed,n+1);
                len=n; pos=next_pos;
                free(completed);
            } else if (matches>1 && was_pending) {
                size_t n=0;
                char **names=completion_candidates(buf,pos,&n);
                if (names && n) {
                    write_all("\r\n",2);
                    for (size_t i=0;i<n;i++) {
                        write_all(names[i],strlen(names[i]));
                        write_all((i+1)%4==0?"\r\n":"  ",(i+1)%4==0?2:2);
                    }
                    if (n%4) write_all("\r\n",2);
                    completion_free_candidates(names,n);
                }
            } else if (matches>1) write_all("\a",1);
        } else if (c==127||c==8) {
            tab_pending=0;
            if (pos) { memmove(buf+pos-1,buf+pos,len-pos+1); --pos; --len; }
        } else if (c==1) {tab_pending=0;pos=0;} /* Ctrl+A */
        else if (c==5) {tab_pending=0;pos=len;} /* Ctrl+E */
        else if (c==27) {
            tab_pending=0;
            unsigned char a,b;
            if (read(STDIN_FILENO,&a,1)!=1 || a!='[') continue;
            if (read(STDIN_FILENO,&b,1)!=1) continue;
            if (b=='A' && count && nav>0) {
                if (nav==count) { free(draft); draft=strdup(buf); if(!draft) goto fail; }
                --nav;
                if(replace(&buf,&cap,&len,&pos,history[nav])<0) goto fail;
            } else if (b=='B' && nav<count) {
                ++nav;
                const char *next=(nav==count)?(draft?draft:""):history[nav];
                if(replace(&buf,&cap,&len,&pos,next)<0) goto fail;
            } else if (b=='C' && pos<len) ++pos;
            else if (b=='D' && pos>0) --pos;
            else if (b=='3') {
                unsigned char tilde;
                if (read(STDIN_FILENO,&tilde,1)==1 && tilde=='~' && pos<len) {
                    memmove(buf+pos,buf+pos+1,len-pos);
                    --len;
                }
            }
        } else if (c>=32) {
            tab_pending=0;
            if (len+2>cap) {
                size_t next=cap*2;
                char *p=realloc(buf,next);
                if(!p) goto fail;
                buf=p;cap=next;
            }
            memmove(buf+pos+1,buf+pos,len-pos+1);
            buf[pos++]=(char)c; ++len;
        } else continue;
        if(redraw(prompt,buf,len,pos)<0) goto fail;
    }
    free(draft);
    tcsetattr(STDIN_FILENO,TCSANOW,&old);
    return buf;
fail:
    free(draft); free(buf);
    tcsetattr(STDIN_FILENO,TCSANOW,&old);
    return NULL;
}
