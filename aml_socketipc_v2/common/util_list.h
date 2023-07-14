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

#ifndef UTIL_LIST_H
#define UTIL_LIST_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Node {
    // serverMsgsList : nodeTypeData represents the server message type
    // clientFdList : nodeTypeData represents the event information registered for this fd (bitmap)
    int nodeTypeData;
    void* data;
    struct Node* next;
    struct Node* prev;
} Node;

typedef struct List {
    Node* head;
    Node* tail;
} List;

List* createList();
void push_back(List* list, void* data, int nodetypeData);
void* front(List* list, int* nodetypeData);
void pop_front(List* list);
int empty(List* list);
void freeList(List* list);
void deleteIntNodeWithValue(List* list, int value);

#ifdef __cplusplus
}
#endif

#endif
