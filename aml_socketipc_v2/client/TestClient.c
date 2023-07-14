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
#include <unistd.h>
#include <pthread.h>
#include "socket_client.h"

//This is a demo about how to use client api
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

typedef void (*ServerMsgHandler)(ServerMsgT*);
extern ClientSocketData clientSocketData;
ServerMsgHandler msgCbks[MAXServerMsg];

// 1.callback function
void handleServerMsgAction(char* buf);

// 2.business action functions for sending to server
void sendClientMsgType1(int msgTypeValue);
void sendClientMsgType2(enum ClientMsgType2 msgTypeValue);

// 3.business action functions for receiving server message
void serverMsgType1ActHandle(ServerMsgT* msg);
void serverMsgType2ActHandle(ServerMsgT* msg);
void serverMsgType3ActHandle(ServerMsgT* msg);

// 4.Registration and initialization related parameters
int clientRegisterServerMsgType(enum ServerMsgType* types, ServerMsgHandler* cbks, int num);
void initializeClientInputData(ClientInputData* clientInputData, char* connectIp, int connectPort, int bitmap);

/*
    *******************************************************
    Specific function definitions
    *******************************************************
*/

void handleServerMsgAction(char* buf) {
     // Change the current state based on the type of server message received
    ServerMsgT *msg = (ServerMsgT *)buf;
    if (msg->type == SERVERMSGTYPE1) {
        msgCbks[msg->type](msg);
    } else if (msg->type == SERVERMSGTYPE2) {
        msgCbks[msg->type](msg);
    } else if (msg->type == SERVERMSGTYPE3) {
        msgCbks[msg->type](msg);
    }
}

void serverMsgType1ActHandle(ServerMsgT* msg)
{
    printf("receive server MsgType1, value is %d \n", msg->u.serverMsgType1Value);
}

void serverMsgType2ActHandle(ServerMsgT* msg)
{
    printf("receive server MsgType2, value is %d \n", msg->u.serverMsgType2Value);
}

void serverMsgType3ActHandle(ServerMsgT* msg)
{
    printf("receive server MsgType3, value is %f \n", msg->u.serverMsgType3Value);
}

void sendClientMsgType1(int msgTypeValue)
{
    ClientMsgT msg;
    msg.type = CLIENTMSGTYPE1;
    msg.u.clientMsgType1Value = msgTypeValue;

    printf("send client MsgType1 to server, value is %d \n", msgTypeValue);
    ClientWriteMsg2Server(BUSINESS_DATA, &msg, sizeof(msg));
}

void sendClientMsgType2(enum ClientMsgType2 msgTypeValue)
{
    ClientMsgT msg;
    msg.type = CLIENTMSGTYPE2;
    msg.u.clientMsgType1Value = msgTypeValue;
    printf("send client MsgType2 to server, value is %d \n", msgTypeValue);

    ClientWriteMsg2Server(BUSINESS_DATA, &msg, sizeof(msg));
}

// Register the message type with the server, which may be different for different clients
int clientRegisterServerMsgType(enum ServerMsgType* types, ServerMsgHandler* cbks, int num)
{
    printf("in ClientRegisterServerMsgType\n");
    int bitmap = 0;
    for (int i = 0; i < num; i++) {
        if (types[i] > MAXServerMsg) {
            printf("ClientRegisterServerMsgType error, invalid ServerMsgType\n");
            exit(-1);
        }
        printf("client register types[i] = %d\n", types[i]);
        msgCbks[types[i]] = cbks[i];
        bitmap |= 1 << types[i];
    }
    printf("bitmap value is:%d\n", bitmap);

    return bitmap;
}

// Required, otherwise there are too many parameters
void initializeClientInputData(ClientInputData* clientInputData, char* connectIp, int connectPort, int bitmap)
{

    clientInputData->connectIp = connectIp;
    clientInputData->connectPort = connectPort;

    clientInputData->registerBitmap = bitmap;

    clientInputData->clientHandler = handleServerMsgAction;
}

int main(int argc, char *argv[])
{
/*     // register 3 event
    enum ServerMsgType types[] = {SERVERMSGTYPE1, SERVERMSGTYPE2, SERVERMSGTYPE3};
    ServerMsgHandler cbks[]= {serverMsgType1ActHandle, serverMsgType2ActHandle, serverMsgType3ActHandle};
    int bitmap = clientRegisterServerMsgType(types, cbks, sizeof(cbks)/sizeof(ServerMsgHandler)); */


    // register 2 event
    enum ServerMsgType types[] = {SERVERMSGTYPE1, SERVERMSGTYPE2};
    ServerMsgHandler cbks[]= {serverMsgType1ActHandle, serverMsgType2ActHandle};
    int bitmap = clientRegisterServerMsgType(types, cbks, sizeof(cbks)/sizeof(ServerMsgHandler));


/*     // register 1 event
    enum ServerMsgType types[] = {SERVERMSGTYPE1};
    ServerMsgHandler cbks[]= {serverMsgType1ActHandle};
    int bitmap = clientRegisterServerMsgType(types, cbks, sizeof(cbks)/sizeof(ServerMsgHandler));
 */
    ClientInputData clientInputData;
    initializeClientInputData(&clientInputData, DEFAULT_IP, DEFAULT_PORT, bitmap);
    ClientInit(&clientInputData);
    printf("----------------finsh initialize------------------\n");

    int msgType = -1;
    int value = -1;
    while (1) {
        /*
        Client sends command simulation
        clientMsgType1 cmd "1":
        enter int value:

        clientMsgType2 cmd "2":
        enter int value:
        */
        printf("please enter the msgtype 1-2: ");
        printf("\n");
        scanf("%d", &msgType);
        printf("\n");
        if (1 == msgType) {

            printf("please input msgtype1 value: ");
            scanf("%d", &value);
            printf("\n");
            sendClientMsgType1(value);

        } else if (2 == msgType) {
            printf("please input msgtype2 value 0-1: ");
            scanf("%d", &value);
            printf("\n");
            sendClientMsgType2(value);
        }

        usleep(500 * 1000);
        msgType = -1;
        value = -1;
        printf("\n");
    }

    ClientExit();

    return 0;
}
