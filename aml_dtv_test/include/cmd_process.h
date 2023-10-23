#ifndef _CMD_PROCESS_H_
#define _CMD_PROCESS_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef __cplusplus
extern "C" {
#endif

void ParseCmd(int conn, const char *cmd);

#ifdef __cplusplus
}
#endif

#endif