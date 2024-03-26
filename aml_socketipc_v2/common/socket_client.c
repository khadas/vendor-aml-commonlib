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

static void clientInitConnection()
{
    struct sockaddr_in serv_addr;

    clientSocketData.sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocketData.sockfd < 0) {
        AML_LOGE("client %s: create socket failed\n", clientSocketData.clientName);
        exit(-1);
    }
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(clientSocketData.connectPort);
    if (inet_pton(AF_INET, clientSocketData.connectIp, &serv_addr.sin_addr) != 1) {
        AML_LOGE("client %s: invalid address\n", clientSocketData.clientName);
        exit(-1);
    }

    int ret;
    int reconnectionCount = 0;

    while (true) {
        ret = connect(clientSocketData.sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        if (ret == 0) {
            break;
        }
        switch (errno) {
        case EISCONN:
            AML_LOGW("client %s: Socket is already connected. Closing and retrying.\n", clientSocketData.clientName);
            close(clientSocketData.sockfd);
            clientSocketData.sockfd = socket(AF_INET, SOCK_STREAM, 0);  // creat new socket
            if (clientSocketData.sockfd == -1) {
                AML_LOGE("Error creating socket");
                exit(EXIT_FAILURE);
            }
            break;
        default:
            if (reconnectionCount > 1 && ((reconnectionCount + 1) % 10 == 0)) {
                reconnectionCount = 0;
                AML_LOGW("client %s: try to connect to server, ret=%d, errno=%d \n", clientSocketData.clientName, ret, errno);
            }
        }
        sleep(1);
        reconnectionCount++;
    }

    AML_LOGI("client %s: connect to server\n", clientSocketData.clientName);
}

static void clientRegister2Server(void)
{
    AML_LOGI("client %s: in register2Server......\n", clientSocketData.clientName);
    int msgLength = sizeof(clientSocketData.registerBitmap);
    char msg[10];
    sprintf(msg, "%d", clientSocketData.registerBitmap);
    int len = ClientWriteMsg2Server(REGISTER, msg, msgLength);
    if (len == msgLength + sizeof(MsgHeader)) {
        AML_LOGI("client %s: register server message succeed......\n", clientSocketData.clientName);
    } else {
        AML_LOGE("client %s: register server message failed......\n", clientSocketData.clientName);
        exit(-1);
    }
}

static void handleClientConnection() {

    clientInitConnection(&clientSocketData.sockfd, clientSocketData.connectIp, clientSocketData.connectPort);

    pthread_mutex_lock(&clientSocketData.connectionStatusMutex);
    clientSocketData.isConnected = true;
    pthread_mutex_unlock(&clientSocketData.connectionStatusMutex);
    if (clientSocketData.clientConnectedHandler) {
        clientSocketData.clientConnectedHandler();
    }

    clientRegister2Server();
}

static void handleClientDisconnection() {
    pthread_mutex_lock(&clientSocketData.connectionStatusMutex);
    clientSocketData.isConnected = false;
    pthread_mutex_unlock(&clientSocketData.connectionStatusMutex);
    if (clientSocketData.clientDisconnectedHandler) {
        clientSocketData.clientDisconnectedHandler();
    }
    if (clientSocketData.sockfd >= 0)
        close(clientSocketData.sockfd);
}

static void handleClientReconnection() {
    pthread_mutex_lock(&clientSocketData.reconnectionMutex);

    handleClientDisconnection();
    handleClientConnection();

    pthread_mutex_unlock(&clientSocketData.reconnectionMutex);
}

static void* serverMessageDispatchWorker(void* arg)
{
    AML_LOGI("client %s: serverMessageDispatchWorker begin work......\n", clientSocketData.clientName);

    while (1) {
        // get header message part
        MsgHeader header;
        int length = sizeof(header);
        int n = readn(clientSocketData.sockfd, &header, length);
        if (n != length) {
            if (0 == n) {
                AML_LOGW("client %s: Disconnected with server, about to reconnect......\n", clientSocketData.clientName);
                handleClientReconnection();
            }
            continue;
        }
        // get business message part
        MessageType type = header.type;
        int dataLength = header.dataLength;
        if (type == BUSINESS_DATA) {
            char businessData[dataLength];
            int n = readn(clientSocketData.sockfd, businessData, sizeof(businessData));
            if (n != dataLength) {
                if (n == 0) {
                    // Connection closed by the server, need to reconnect
                    handleClientReconnection();
                }
                continue;
            }
            AML_LOGD("client %s: receive server business message......\n", clientSocketData.clientName);
            clientSocketData.clientHandler(businessData);
        } else if (type == HEARTBEAT) {
            pthread_mutex_lock(&clientSocketData.beatMutex);
            clientSocketData.beats = 0;
            pthread_mutex_unlock(&clientSocketData.beatMutex);
        }
    }
    return NULL;
}

static void* heartBeatDetector(void* arg)
{
    while (1) {
        pthread_mutex_lock(&clientSocketData.beatMutex);
        if (clientSocketData.beats > 30) {
            AML_LOGW("client %s: disconnected with server because of the heartbeat timeout, about to reconnect......\n", clientSocketData.clientName);
            handleClientReconnection();
        } else {
            clientSocketData.beats += 10;
        }
        pthread_mutex_unlock(&clientSocketData.beatMutex);
        sleep(10);
    }
    return NULL;
}

void initClientData(ClientSocketData* clientSocketData, ClientInputData* clientInputData) {
    if (!clientSocketData || !clientInputData) {
        AML_LOGE("Null pointer passed to initClientData......\n");
        return;
    }

    clientSocketData->sockfd = -1;
    clientSocketData->beats = 0;

    clientSocketData->connectIp = clientInputData->connectIp;
    clientSocketData->clientName = clientInputData->clientName;
    clientSocketData->connectPort = clientInputData->connectPort;
    clientSocketData->registerBitmap = clientInputData->registerBitmap;

    pthread_mutex_init(&clientSocketData->beatMutex, NULL);
    pthread_mutex_init(&clientSocketData->writeMsgMutex, NULL);
    pthread_mutex_init(&clientSocketData->connectionStatusMutex, NULL);
    pthread_mutex_init(&clientSocketData->reconnectionMutex, NULL);

    clientSocketData->dispatch = 0;
    clientSocketData->clientHandler = clientInputData->clientHandler;
    clientSocketData->clientConnectedHandler = clientInputData->clientConnectedHandler;
}

void ClientInit(ClientInputData* clientInputData)
{
    initClientData(&clientSocketData, clientInputData);
    clientInitConnection(&clientSocketData.sockfd, clientSocketData.connectIp, clientSocketData.connectPort);
    if (clientSocketData.clientConnectedHandler)
        clientSocketData.clientConnectedHandler();
    int ret = pthread_create(&clientSocketData.dispatch, NULL, serverMessageDispatchWorker, NULL);
    if (ret != 0) {
        AML_LOGE("client %s: serverMessageDispatchWorker pthread create failed......\n", clientSocketData.clientName);
        exit(-1);
    }
    ret = pthread_create(&clientSocketData.heartBeatDetect, NULL, heartBeatDetector, NULL);
    if (ret != 0) {
        AML_LOGE("client %s: heartBeatDetector pthread create failed......\n", clientSocketData.clientName);
        exit(-1);
    }
    clientRegister2Server();
}

void ClientExit(void)
{
    pthread_cancel(clientSocketData.dispatch);
    pthread_join(clientSocketData.dispatch, NULL);
    pthread_cancel(clientSocketData.heartBeatDetect);
    pthread_join(clientSocketData.heartBeatDetect, NULL);

    pthread_mutex_destroy(&clientSocketData.beatMutex);
    pthread_mutex_destroy(&clientSocketData.writeMsgMutex);
    pthread_mutex_destroy(&clientSocketData.connectionStatusMutex);
    pthread_mutex_destroy(&clientSocketData.reconnectionMutex);

    handleClientDisconnection();
}

int ClientWriteMsg2Server(MessageType msgType, void* msg, const int msgLength) {
    if (BUSINESS_DATA != msgType && REGISTER != msgType) {
        AML_LOGW("client %s: Invalid message type, please send business data......\n", clientSocketData.clientName);
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
        AML_LOGW("client %s: Failed to send message, please check connection status......\n", clientSocketData.clientName);
    return sent_len;
}

void ClientGetConnectionStatus(int* connectStatus) {
    pthread_mutex_lock(&clientSocketData.connectionStatusMutex);
    *connectStatus = clientSocketData.isConnected;
    pthread_mutex_unlock(&clientSocketData.connectionStatusMutex);
}