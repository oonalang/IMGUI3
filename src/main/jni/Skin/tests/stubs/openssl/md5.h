#pragma once
#include <stddef.h>

#define MD5_DIGEST_LENGTH 16

typedef struct MD5_CTX {
    unsigned int state[16];
    unsigned int count[2];
    unsigned char buffer[64];
} MD5_CTX;

extern "C" {
int MD5_Init(MD5_CTX *c);
int MD5_Update(MD5_CTX *c, const void *data, size_t len);
int MD5_Final(unsigned char *md, MD5_CTX *c);
}

