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

#ifndef SOCKET_UTIL_H_
#define SOCKET_UTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

// 1、Public interface for socket communication
#define LOCALHOST_IP "127.0.0.1"
#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_PORT 6800

// Basic socket operations
// Read "n" bytes from a descriptor
ssize_t readn(int fd, void* vptr, ssize_t n);
// Write "n" bytes to a descriptor
ssize_t writen(int fd, const void* vptr, ssize_t n);


// 2、for server and client
// for server
void setNonblocking(int fd);
int createListener(int* listen_fd, int serverPort);

// for client
void initConnection(int* sockfd, const char *serverIP, int serverPort); //init socket；return sockfd


typedef enum {
    UNDEFINED,
    BUSINESS_DATA,
    REGISTER,
    HEARTBEAT
} MessageType;

typedef struct {
    MessageType type;
    int dataLength;
} MsgHeader;


#ifdef __cplusplus
}
#endif

#endif