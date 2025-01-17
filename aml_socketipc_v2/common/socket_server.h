// Copyright (C) 2023 Amlogic, Inc. All rights reserved.
//
// All information contained herein is Amlogic confidential.
//
// This software is provided to you pursuant to Software License
// Agreement (SLA) with Amlogic Inc ("Amlogic"). This software may be
// used only in accordance with the terms of this agreement.
//
// Redistribution and use in source and binary forms, with or without
// modification is strictly prohibited without prior written permission
// from Amlogic.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef SOCKET_SERVER_H_
#define SOCKET_SERVER_H_
#include <pthread.h>
#include "util_list.h"
#include "socket_util.h"

#ifdef __cplusplus
extern "C" {
#endif

// Business general execution functions for messages from the client
typedef void (*PerformActionForClientMessage)(char* buf, int fd);

typedef struct {
    int listen_fd;
    char* serverName;

    List* clientFdList; // connected client fd
    List* serverMsgsList; // message need to send

    pthread_mutex_t serverMsgsMutex;
    pthread_cond_t serverMsgsCond;
    pthread_t epollEventHandler;
    pthread_t messageSender;
    pthread_t heartBeatSender;

    PerformActionForClientMessage serverHandler;
} ServerSocketData;

typedef struct {
    int connectPort;
    char* serverName;

    PerformActionForClientMessage serverHandler;
} ServerInputData;

void ServerInit(ServerInputData* clientInputData);
void ServerExit(void);
void EnqueueServerMsg(void* msg, MessageType msgType, int msgLength, int serverMsgType);

#ifdef __cplusplus
}
#endif

#endif