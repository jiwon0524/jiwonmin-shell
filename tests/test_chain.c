#include "../src/chain.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    Chain c; char e[128];
    assert(chain_parse("pwd && ls || echo fail; echo done",&c,e,sizeof(e))==0);
    assert(c.count==4 && c.items[1].before==CHAIN_AND && c.items[2].before==CHAIN_OR && c.items[3].before==CHAIN_ALWAYS);
    chain_free(&c);
    assert(chain_parse("echo 'a && b'; echo x",&c,e,sizeof(e))==0 && c.count==2);chain_free(&c);
    assert(chain_parse("echo a\\;b && echo ok",&c,e,sizeof(e))==0 && c.count==2);chain_free(&c);
    assert(chain_parse("pwd &&",&c,e,sizeof(e))!=0);
    assert(chain_parse("pwd || || ls",&c,e,sizeof(e))!=0);
    assert(chain_parse("pwd;",&c,e,sizeof(e))==0 && c.count==1);chain_free(&c);
    puts("6 chaining parser checks passed");
}
