#ifndef JIWONMIN_CHAIN_H
#define JIWONMIN_CHAIN_H
#include <stddef.h>
#define CHAIN_MAX 32
typedef enum { CHAIN_FIRST, CHAIN_AND, CHAIN_OR, CHAIN_ALWAYS } ChainOp;
typedef struct { char *text; ChainOp before; } ChainItem;
typedef struct { ChainItem items[CHAIN_MAX]; size_t count; } Chain;
int chain_parse(const char *input, Chain *out, char *err, size_t err_size);
void chain_free(Chain *chain);
#endif
