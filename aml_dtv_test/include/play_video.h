#ifndef _PLAY_VIDEO_H_
#define _PLAY_VIDEO_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include<string.h>

#ifdef __cplusplus
extern "C" {
#endif

void AmTsPlayer(int vid , int vfmt, int pid, int fmt);

#ifdef __cplusplus
}
#endif

#endif