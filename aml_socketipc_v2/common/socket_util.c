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

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <stdbool.h>
#include "socket_util.h"

#include "aml_log.h"

// Read "n" bytes from a descriptor
ssize_t readn(int fd, void* vptr, ssize_t n)
{
    ssize_t  nleft;
    ssize_t nread;
    char* ptr;

    ptr =(char*) vptr;
    nleft = n;
    while (nleft > 0) {
        if ( (nread = read(fd, ptr, nleft)) < 0) {
            if (errno == EINTR)
                nread = 0;      /* and call read() again */
            else
                return -1;
        } else if (nread == 0) {
            break;              /* EOF */
        }

        nleft -= nread;
        ptr   += nread;
    }
    return (n - nleft);      /* return >= 0 */
}


// Write "n" bytes to a descriptor
ssize_t writen(int fd, const void* vptr, ssize_t n)
{
    ssize_t      nleft;
    ssize_t     nwritten;
    const char*  ptr;

    ptr =(char*) vptr;
    nleft = n;
    while (nleft > 0) {
        if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
            if (nwritten < 0 && errno == EINTR)
                nwritten = 0;       // and call write() again
            else
                return(-1);         // error
        }

        nleft -= nwritten;
        ptr   += nwritten;
    }
    return n;
}


// for server
void setNonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int createListener(int* global_listen_fd, int serverPort){
    int flag = 1;
    struct sockaddr_in serv_addr;
    int listen_fd;
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    *global_listen_fd = listen_fd;
    if (listen_fd < 0) {
        AML_LOGE("create socket failed\n");
        goto fail;
    }
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) < 0) {
        AML_LOGE("setsockopt SO_REUSEADDR failed\n");
        goto fail;
    }
    setNonblocking(listen_fd);
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(serverPort);

    if (bind(listen_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0) {
        AML_LOGE("bind failed, %s\n", strerror(errno));
        goto fail;
    }
    if (listen(listen_fd, 20) != 0) {
        AML_LOGE("server listen failed\n");
        goto fail;
    }
    AML_LOGI("server listen_fd: %d\n", listen_fd);
    return 0;
fail:
    if (listen_fd > 0) {
        close(listen_fd);
        listen_fd = -1;
    }
    return -1;
}

// for client
void initConnection(int* sockfd, const char *serverIP, int serverPort)
{
    struct sockaddr_in serv_addr;

    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (*sockfd < 0) {
        AML_LOGE("create socket failed\n");
        exit(-1);
    }
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(serverPort);
    if (inet_pton(AF_INET, serverIP, &serv_addr.sin_addr) != 1) {
        AML_LOGE("invalid address\n");
        exit(-1);
    }

    int ret;
    int seconds = 0;

    while (true) {
        ret = connect(*sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        if (ret == 0) {
            break;
        }
        switch (errno) {
        case EISCONN:
            AML_LOGW("Socket is already connected. Closing and retrying.\n");
            close(*sockfd);
            *sockfd = socket(AF_INET, SOCK_STREAM, 0);  // creat new socket
            if (*sockfd == -1) {
                AML_LOGE("Error creating socket");
                exit(EXIT_FAILURE);
            }
            break;
        default:
            AML_LOGW("try to connect to server, elapsed time: %d seconds, ret=%d, errno=%d \n", seconds + 1, ret, errno);
        }

        if (seconds > 1 && ((seconds + 1) % 120 == 0)) {
            AML_LOGE("Error: Socket reconnection timed out.\n");
            exit(EXIT_FAILURE);
        }
        sleep(1);
        seconds++;
    }

    AML_LOGI("connect to server\n");
}