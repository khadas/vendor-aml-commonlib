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

/* main.c */
#include <stdio.h>
#include <stdlib.h>
#include "util_list.h"

List* list;

int main() {
    // Take the int type as an example
    list = createList();
    NodeType nodetype = UNDEFINED_NODE;

    int nodetypeData = 0;
    int* num1 = malloc(sizeof(int));
    int* num2 = malloc(sizeof(int));
    int* num3 = malloc(sizeof(int));

    *num1 = 1;
    *num2 = 2;
    *num3 = 3;

    // push 1-2-3
    push_back(list, num1, UNDEFINED_NODE, 0);
    push_back(list, num2, UNDEFINED_NODE, 0);
    push_back(list, num3, UNDEFINED_NODE, 0);

    printf("%d\n", *(int*)front(list, &nodetype, &nodetypeData)); // Prints "1"
    pop_front(list); // pop 1
    pop_front(list); // pop 2
    printf("%d\n", *(int*)front(list, &nodetype, &nodetypeData)); // Prints "3"

    Node* it = list->head;
    printf("%d\n", *(int*)it->data); // Prints "3"

    // delete special node by int value
    int value = 3;
    deleteIntNodeWithValue(list, value);
    printf("%d\n", empty(list)); // Prints "1"

    // Don't forget to free the integers after using them
    while (!empty(list)) {
        free(front(list, &nodetype, &nodetypeData));
        pop_front(list);
    }

    freeList(list);

    return 0;
}