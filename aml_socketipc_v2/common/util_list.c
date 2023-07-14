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
#include "util_list.h"

#include "aml_log.h"

List* createList() {
    List* list = (List*) malloc(sizeof(List));
    list->head = NULL;
    list->tail = NULL;
    return list;
}

void push_back(List* list, void* data, int nodetypeData) {
    Node* node = (Node*) malloc(sizeof(Node));
    node->data = data;
    node->nodeTypeData = nodetypeData;
    node->next = NULL;
    if (list->head == NULL) {
        node->prev = NULL;
        list->head = node;
        list->tail = node;
    } else {
        node->prev = list->tail;
        list->tail->next = node;
        list->tail = node;
    }
}

void* front(List* list, int* nodetypeData) {
    if (list->head != NULL) {
        if (nodetypeData != NULL) {
            *nodetypeData = list->head->nodeTypeData;
        }

        return list->head->data;
    } else {
        AML_LOGW("List is empty.\n");
        return NULL;
    }
}

void pop_front(List* list) {
    if (list->head != NULL) {
        Node* node = list->head;
        list->head = list->head->next;
        if (list->head != NULL) {
            list->head->prev = NULL;
        } else {
            list->tail = NULL;
        }
        free(node);
    } else {
        AML_LOGW("List is empty.\n");
    }
}

int empty(List* list) {
    return list->head == NULL;
}

void freeList(List* list) {
    Node* current = list->head;
    Node* next;
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
    free(list);
}

void deleteIntNodeWithValue(List* list, int value){
    Node* current = list->head;
    while (current != NULL) {
        if (*(int*)(current->data) == value) {
            if (current->prev != NULL) {
                current->prev->next = current->next;
            } else {
                list->head = current->next;
            }

            if (current->next != NULL) {
                current->next->prev = current->prev;
            } else {
                list->tail = current->prev;
            }

            Node* toDelete = current;
            current = current->next;
            free((int*)(toDelete->data));
            free(toDelete);
        } else {
            current = current->next;
        }
    }
}