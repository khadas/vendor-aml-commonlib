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
#include <pthread.h>
#include "socket_server.h"

extern ServerSocketData serverSocketData;

//This is a demo about how to use server api
/*
    **************************************************************
    Defining business data
    **************************************************************
*/

enum ServerMsgType {
    SERVERMSGTYPE1,
    SERVERMSGTYPE2,
    SERVERMSGTYPE3,
    //...... other type message

    MAXServerMsg
};

enum ServerMsgType2 {
    SERVERMSGTYPE2VALUE1,
    SERVERMSGTYPE2VALUE2
};

typedef struct ServerMsg {
    enum ServerMsgType type;
    union {
        int serverMsgType1Value;
        enum ServerMsgType2 serverMsgType2Value;
        float serverMsgType3Value;
        //...... other message's arguments
    } u;
} ServerMsgT;

enum ClientMsgType {
    CLIENTMSGTYPE1,
    CLIENTMSGTYPE2,
    //...... other type message

    MAXClientMsg
};

enum ClientMsgType2 {
    CLIENTMSGTYPE2VALUE1,
    CLIENTMSGTYPE2VALUE2
};

typedef struct ClientMsg {
    enum ClientMsgType type;
    union {
        int clientMsgType1Value;
        enum ClientMsgType2 clientMsgType2Value;
        //...... other message's arguments
    } u;
} ClientMsgT;
/*
    **************************************************************
    Defining business data finish
    **************************************************************
*/

typedef int (*ServerMsgHandler)(ServerMsgT*);

// 1.callback function
void handleClientMsgAction(char* buf, int fd);

// 2.business action functions for sending to client
void sendServerMsgType1(int msgTypeValue);
void sendServerMsgType2(enum ServerMsgType2 msgTypeValue);
void sendServerMsgType3(float msgTypeValue);

// 3.business action functions for receiving client message
void clientMsgType1ActHandle(ClientMsgT* msg);
void clientMsgType2ActHandle(ClientMsgT* msg);

// 4.initiclientMsgType1ActHandlealization related parameters
void initializeServerInputData(ServerInputData* serverInputData, int port);

/*
    *******************************************************
    Specific function definitions
    *******************************************************
*/

void handleClientMsgAction(char* buf, int fd)
{
    ClientMsgT* msg = (ClientMsgT*) buf;

    if (msg->type == CLIENTMSGTYPE1) { //handle message from client
        printf("receive client clientMsgType1 message from fd %d, value is : %d \n", fd, msg->u.clientMsgType1Value);
        clientMsgType1ActHandle(msg);
    } else if (msg->type == CLIENTMSGTYPE2) { //handle message from client
        printf("receive client clientMsgType2 message from fd %d, value is : %d \n", fd, msg->u.clientMsgType2Value);
        clientMsgType2ActHandle(msg);
    }
}

void clientMsgType1ActHandle(ClientMsgT* msg)
{
    printf("receive client MsgType1, value is %d \n", msg->u.clientMsgType1Value);
}
void clientMsgType2ActHandle(ClientMsgT* msg)
{
    printf("receive client MsgType2, value is %d \n", msg->u.clientMsgType2Value);
}

void sendServerMsgType1(int msgTypeValue)
{
    ServerMsgT msg;
    msg.type = SERVERMSGTYPE1;
    msg.u.serverMsgType1Value = msgTypeValue;

    printf("send server MsgType1 to client, value is %d \n", msgTypeValue);

    EnqueueServerMsg(&msg, BUSINESS_DATA, sizeof(msg), msg.type);
}

void sendServerMsgType2(enum ServerMsgType2 msgTypeValue)
{
    ServerMsgT msg;
    msg.type = SERVERMSGTYPE2;
    msg.u.serverMsgType2Value = msgTypeValue;

    printf("send server MsgType2 to client, value is %d \n", msgTypeValue);

    EnqueueServerMsg(&msg, BUSINESS_DATA, sizeof(msg), msg.type);
}

void sendServerMsgType3(float msgTypeValue)
{
    ServerMsgT msg;
    msg.type = SERVERMSGTYPE3;
    msg.u.serverMsgType3Value = msgTypeValue;

    printf("send server MsgType3 to client, value is %f \n", msgTypeValue);

    EnqueueServerMsg(&msg, BUSINESS_DATA, sizeof(msg), msg.type);
}

void initializeServerInputData(ServerInputData* serverInputData, int port)
{

    serverInputData->connectPort = port;
    serverInputData->serverName = "testServer";

    serverInputData->serverHandler = handleClientMsgAction;
}

int main(int argc, char *argv[])
{
    setvbuf(stdout, NULL, _IONBF, 0);

    ServerInputData serverInputData;
    initializeServerInputData(&serverInputData, DEFAULT_PORT);
    ServerInit(&serverInputData);

    printf("----------------finsh initialize------------------\n");

    int msgType = -1;
    int value = -1;
    while (1) {
        /*
        Server sends command simulation
        serverMsgType1 cmd "1":
        enter int value:

        serverMsgType2 cmd "2":
        enter int value:

        serverMsgType3 cmd "3":
        enter int value:
        */
        printf("please enter the msgtype 1-3: ");
        printf("\n");
        scanf("%d", &msgType);
        printf("\n");
        if (1 == msgType) {

            printf("please input msgtype1 value: ");
            scanf("%d", &value);
            printf("\n");
            sendServerMsgType1(value);

        } else if (2 == msgType) {
            printf("please input msgtype2 value 0-1: ");
            scanf("%d", &value);
            printf("\n");
            sendServerMsgType2(value);
        } else if (3 == msgType) {
            float value;
            printf("please input msgtype3 value: ");
            scanf("%f", &value);
            printf("\n");
            sendServerMsgType3(value);
        }
        msgType = -1;
        value = -1;
        printf("\n");
        printf("\n");
    }
    ServerExit();
}