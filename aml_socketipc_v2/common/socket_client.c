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
#include "socket_client.h"

#include "aml_log.h"

ClientSocketData clientSocketData;

int ClientWriteMsg2Server(MessageType msgType, void* msg, const int msgLength) {
    if (BUSINESS_DATA != msgType && REGISTER != msgType) {
        AML_LOGW("Invalid message type, please send business data......\n");
        return -1;
    }
    MsgHeader header;
    int totalSize = 0;

    header.type = msgType;
    header.dataLength = msgLength;
    totalSize = sizeof(header) + header.dataLength;
    char buffer[totalSize];
    memcpy(buffer, &header, sizeof(header));
    memcpy(buffer + sizeof(header), msg, header.dataLength);

    pthread_mutex_lock(&clientSocketData.writeMsgMutex);
    int sent_len = writen(clientSocketData.sockfd, buffer, totalSize);
    pthread_mutex_unlock(&clientSocketData.writeMsgMutex);
    if (sent_len != totalSize)
        AML_LOGW("Invalid message type, please send business data......\n");
    return sent_len;
}

void register2Server(void)
{
    AML_LOGI("in register2Server......\n");
    int msgLength = sizeof(clientSocketData.registerBitmap);
    char msg[10];
    sprintf(msg, "%d", clientSocketData.registerBitmap);
    int len = ClientWriteMsg2Server(REGISTER, msg, msgLength);
    if (len == msgLength + sizeof(MsgHeader)) {
        AML_LOGI("register server message succeed......\n");
    } else {
        AML_LOGE("register server message failed......\n");
        exit(-1);
    }
}

void* msgDispatchWorker(void* arg)
{
    AML_LOGI("msgDispatchWorker begin work......\n");

    while (1) {
        // get header message part
        MsgHeader header;
        int length = sizeof(header);
        int n = readn(clientSocketData.sockfd, &header, length);
        if (n != length) {
            if (0 == n) {
                AML_LOGW("Disconnected with server, about to reconnect......\n");
                pthread_mutex_lock(&clientSocketData.beatMutex);
                close(clientSocketData.sockfd);
                initConnection(&clientSocketData.sockfd, clientSocketData.connectIp, clientSocketData.connectPort);
                register2Server();
                pthread_mutex_unlock(&clientSocketData.beatMutex);
            }
            continue;
        }

        // get business message part
        MessageType type = header.type;
        int dataLength = header.dataLength;
        if (type == BUSINESS_DATA) {
            char businessData[8];
            int n = readn(clientSocketData.sockfd, businessData, 8);
            if (n != dataLength) {
                if (n == 0) {
                    // Connection closed by the server, need to reconnect

                    pthread_mutex_lock(&clientSocketData.beatMutex);
                    close(clientSocketData.sockfd);
                    initConnection(&clientSocketData.sockfd, clientSocketData.connectIp, clientSocketData.connectPort);
                    register2Server();
                    pthread_mutex_unlock(&clientSocketData.beatMutex);
                }
                continue;
            }
            AML_LOGD("receive server business message......\n");
            clientSocketData.clientHandler(businessData);
        } else if (type == HEARTBEAT) {
            pthread_mutex_lock(&clientSocketData.beatMutex);
            clientSocketData.beats = 0;
            AML_LOGD("receive server heart message......\n");
            pthread_mutex_unlock(&clientSocketData.beatMutex);
        }
    }
}

void* heartBeatDetectWorker(void* arg)
{
    while (1) {
        pthread_mutex_lock(&clientSocketData.beatMutex);
        if (clientSocketData.beats > 30) {
            AML_LOGW("Disconnected with server because of the heartbeat timeout., about to reconnect......\n");
            close(clientSocketData.sockfd);
            initConnection(&clientSocketData.sockfd, clientSocketData.connectIp, clientSocketData.connectPort);
            register2Server();
        } else {
            clientSocketData.beats += 10;
        }
        pthread_mutex_unlock(&clientSocketData.beatMutex);
        sleep(10);
    }
}

void initClientData(ClientSocketData* clientSocketData, ClientInputData* clientInputData) {
    if (!clientSocketData || !clientInputData) {
        AML_LOGE("Null pointer passed to initClientData......\n");
        return;
    }

    clientSocketData->sockfd = -1;
    clientSocketData->beats = 0;

    clientSocketData->connectIp = clientInputData->connectIp;
    clientSocketData->connectPort = clientInputData->connectPort;
    clientSocketData->registerBitmap = clientInputData->registerBitmap;

    pthread_mutex_init(&clientSocketData->beatMutex, NULL);
    pthread_mutex_init(&clientSocketData->writeMsgMutex, NULL);

    clientSocketData->dispatch = 0;
    clientSocketData->clientHandler = clientInputData->clientHandler;
}

void ClientInit(ClientInputData* clientInputData)
{
    initClientData(&clientSocketData, clientInputData);
    initConnection(&clientSocketData.sockfd, clientSocketData.connectIp, clientSocketData.connectPort);
    register2Server();
    int ret = pthread_create(&clientSocketData.dispatch, NULL, msgDispatchWorker, NULL);
    if (ret != 0) {
        AML_LOGE("msgDispatchWorker pthread create failed......\n");
        exit(-1);
    }
    ret = pthread_create(&clientSocketData.heartBeatDetect, NULL, heartBeatDetectWorker, NULL);
    if (ret != 0) {
        AML_LOGE("heartBeatDetectWorker pthread create failed......\n");
        exit(-1);
    }

}

void ClientExit(void)
{
    pthread_join(clientSocketData.dispatch, NULL);
    pthread_join(clientSocketData.heartBeatDetect, NULL);
}
