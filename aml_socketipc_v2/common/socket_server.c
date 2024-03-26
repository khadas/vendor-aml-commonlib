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
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "util_list.h"
#include <pthread.h>
#include <pthread.h>
#include <sys/epoll.h>
#include "socket_server.h"

#include "aml_log.h"

ServerSocketData serverSocketData;

void EnqueueServerMsg(void* msg, MessageType msgType, int msglength, int serverMsgType)
{
    if (NULL == serverSocketData.clientFdList->head) {
        AML_LOGW("server %s: no connected client......\n", serverSocketData.serverName);
        return;
    }
    if (UNDEFINED == msgType) {
        AML_LOGW("server %s: invalid message type......\n", serverSocketData.serverName);
        return;
    }
    MsgHeader header;
    int totalSize = 0;

    if (BUSINESS_DATA == msgType) {
        header.type = BUSINESS_DATA;
        header.dataLength = msglength;
        totalSize = sizeof(header) + header.dataLength;
    } else if (HEARTBEAT == msgType) {
        header.type = HEARTBEAT;
        header.dataLength = 0;
        totalSize = sizeof(header);
    }
    char* buffer = (char*)malloc(totalSize);

    memcpy(buffer, &header, sizeof(header));
    if (BUSINESS_DATA == msgType) {
        memcpy(buffer + sizeof(header), msg, header.dataLength);
    }
    pthread_mutex_lock(&serverSocketData.serverMsgsMutex);
    push_back(serverSocketData.serverMsgsList, buffer, serverMsgType);
    pthread_cond_signal(&serverSocketData.serverMsgsCond);
    pthread_mutex_unlock(&serverSocketData.serverMsgsMutex);
}

/*
    send a message or processing result to the client
*/
void* messageSenderWorker(void *arg)
{
    while (1) {
        pthread_mutex_lock(&serverSocketData.serverMsgsMutex);
        while (empty(serverSocketData.serverMsgsList)) {
            pthread_cond_wait(&serverSocketData.serverMsgsCond, &serverSocketData.serverMsgsMutex);
        }
        if (!empty(serverSocketData.serverMsgsList)) {
            // get business message type
            int serverMsgType = -1;
            void* pMsg = front(serverSocketData.serverMsgsList, &serverMsgType);

            pthread_mutex_unlock(&serverSocketData.serverMsgsMutex);
            // get header message information
            MsgHeader extractedHeader;
            memcpy(&extractedHeader, pMsg, sizeof(MsgHeader));
            MessageType msgType = extractedHeader.type;

            if (BUSINESS_DATA == msgType) {
                int dataLenth = sizeof(MsgHeader) + extractedHeader.dataLength;
                Node* it = serverSocketData.clientFdList->head;
                while (it != NULL)
                {
                    int fd = *(int*)it->data;
                    // If the fd device registers for this event send msg
                    if ((it->nodeTypeData & (1 << serverMsgType)) != 0) {
                        AML_LOGD("server %s: send business message to fd %d......\n", serverSocketData.serverName, fd);
                        if (writen(fd, pMsg, dataLenth) != dataLenth)
                            AML_LOGW("server %s: write to client failed......\n", serverSocketData.serverName);
                    }
                    it = it->next;
                }
            } else if (HEARTBEAT == msgType) {
                int dataLenth = sizeof(MsgHeader);
                Node* it = serverSocketData.clientFdList->head;

                while (it != NULL)
                {
                    int fd = *(int*)it->data;
                    if (writen(fd, pMsg, dataLenth) != dataLenth)
                        AML_LOGW("server %s: write to client failed......\n", serverSocketData.serverName);
                    it = it->next;
                }
            }
            free(pMsg);
            pMsg = NULL;
            pop_front(serverSocketData.serverMsgsList);
        }
    }
}

/*
    handle heartbeat messages, send heartbeat messages to connected clients at regular intervals
*/
void* heartBeatSenderWorker(void *arg)
{
    while (1) {
        if (NULL != serverSocketData.clientFdList->head)
            EnqueueServerMsg(NULL, HEARTBEAT, 0, -1);
        sleep(10);
    }
}

/*
    handle the registration message from the client
*/
static void handleRegisterMessage(char* bitmapStr, int fd) {

    int bitmapLen = sizeof(int) * 8;

    int bitmap = atoi(bitmapStr);

    AML_LOGD("server %s: receive client register message from fd %d, bitmap: %d\n", serverSocketData.serverName, fd, bitmap);

    // int ptr need malloc, remember to free
    int* connfd_ptr = malloc(sizeof(int));
    *connfd_ptr = fd;
    push_back(serverSocketData.clientFdList, connfd_ptr, bitmap);
    for (int j = 0; j < bitmapLen; j++) {
        if ((bitmap & (1 << j )) != 0) {
            AML_LOGD("server %s: receive register %d , fd: %d \n", serverSocketData.serverName, j, fd);
        }
    }
}

/*
    listening to different events from client and handling them
*/
static void* epollEventHandlerWorker(void *arg)
{
    int epollfd = epoll_create1(0);
    if (epollfd == -1) {
        AML_LOGE("epoll_create1 failed......\n");
        exit(-1);
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = serverSocketData.listen_fd;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, serverSocketData.listen_fd, &ev) == -1) {
        AML_LOGE("epoll_ctl failed......\n");
        close(epollfd);
        exit(-1);
    }

    #define MAX_EVENT 8
    struct epoll_event events[MAX_EVENT];
    while (1) {
        int nfds = epoll_wait(epollfd, events, MAX_EVENT, -1);
        if (nfds < 0) {
            continue;
        }

        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == serverSocketData.listen_fd) {
                int connfd = accept(serverSocketData.listen_fd, NULL, NULL);
                if (connfd == -1 && errno != EINTR) {
                    AML_LOGE("accept failed %d......\n", errno);
                    close(epollfd);
                    exit(-1);
                }
                if (connfd >= 0) {
                    AML_LOGI("client connection arrvied, fd: %d......\n", connfd);
                    ev.events = EPOLLIN;
                    ev.data.fd = connfd;

                    setNonblocking(connfd);
                    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev) == -1) {
                        AML_LOGE("epoll_ctl failed......\n");
                        close(epollfd);
                        exit(-1);
                    }
                }
            } else {
                // get message header
                MsgHeader header;
                int n = readn(events[i].data.fd, &header, sizeof(header));

                if (0 == n) {
                    AML_LOGI("client fd %d close connection......\n", events[i].data.fd);

                    // delete special value node
                    deleteIntNodeWithValue(serverSocketData.clientFdList, events[i].data.fd);

                    if (epoll_ctl(epollfd, EPOLL_CTL_DEL, events[i].data.fd, NULL) == -1)
                        AML_LOGE("epoll_ctl failed......\n");
                    close(events[i].data.fd);
                    continue;
                }
                MessageType type = header.type;
                int dataLength = header.dataLength;

                // get message body
                char businessData[dataLength];

                n = readn(events[i].data.fd, &businessData, sizeof(businessData));
                if (0 == n) {
                    AML_LOGI("client fd %d close connection......\n", events[i].data.fd);
                    // delete special value node
                    deleteIntNodeWithValue(serverSocketData.clientFdList, events[i].data.fd);

                    if (epoll_ctl(epollfd, EPOLL_CTL_DEL, events[i].data.fd, NULL) == -1)
                        AML_LOGE("epoll_ctl failed......\n");
                    close(events[i].data.fd);
                    continue;
                }

                if (type == REGISTER) {
                    // Processing register message from the client
                    handleRegisterMessage(businessData, events[i].data.fd);
                } else if (type == BUSINESS_DATA) {
                    // Processing business messages from the client, calling callback functions
                    serverSocketData.serverHandler(businessData, events[i].data.fd);
                }
            }
        }

    }
    close(epollfd);
}

static void initServerData(ServerSocketData* serverSocketData, ServerInputData* serverInputData)
{
    if (!serverSocketData || !serverInputData) {
        AML_LOGE("Null pointer passed to initServerData......\n");
        return;
    }

    serverSocketData->serverHandler = serverInputData->serverHandler;
    serverSocketData->serverName = serverInputData->serverName;

    serverSocketData->clientFdList = createList();
    serverSocketData->serverMsgsList= createList();
    pthread_mutex_init(&serverSocketData->serverMsgsMutex, NULL);
    pthread_cond_init(&serverSocketData->serverMsgsCond, NULL);
    createListener(&serverSocketData->listen_fd, serverInputData->connectPort);
}


void ServerInit(ServerInputData* serverInputData)
{
    initServerData(&serverSocketData, serverInputData);

    int ret = pthread_create(&serverSocketData.epollEventHandler, NULL, epollEventHandlerWorker, NULL);
    if (ret != 0) {
        AML_LOGE("server %s: epollEventHandlerWorker pthread create failed......\n", serverSocketData.serverName);
        exit(-1);
    }
    ret = pthread_create(&serverSocketData.messageSender, NULL, messageSenderWorker, NULL);
    if (ret != 0) {
        AML_LOGE("server %s: messageSenderWorker pthread create failed......\n", serverSocketData.serverName);
        exit(-1);
    }
    ret = pthread_create(&serverSocketData.heartBeatSender, NULL, heartBeatSenderWorker, NULL);
    if (ret != 0) {
        AML_LOGE("server %s: heartBeatSenderWorker pthread create failed......\n", serverSocketData.serverName);
        exit(-1);
    }
}

void ServerExit(void)
{
    pthread_cancel(serverSocketData.epollEventHandler);
    pthread_join(serverSocketData.epollEventHandler, NULL);
    pthread_cancel(serverSocketData.heartBeatSender);
    pthread_join(serverSocketData.heartBeatSender, NULL);
    pthread_cancel(serverSocketData.messageSender);
    pthread_join(serverSocketData.messageSender, NULL);
    // free and close
    pthread_mutex_destroy(&serverSocketData.serverMsgsMutex);
    pthread_cond_destroy(&serverSocketData.serverMsgsCond);

    freeList(serverSocketData.clientFdList);
    freeList(serverSocketData.serverMsgsList);
    close(serverSocketData.listen_fd);
}
