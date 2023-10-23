#ifndef _UTIL_H_
#define _UTIL_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include<string.h>

#ifdef __cplusplus
extern "C" {
#endif

int startsWith(const char *s, const char *prefix);
int endWith(const char *s, const char *t);
bool contains(const char *str, const char *sub);

#ifdef __cplusplus
}
#endif

#endif