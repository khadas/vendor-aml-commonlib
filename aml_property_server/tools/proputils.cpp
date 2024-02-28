/*
 * Copyright (C) 2014-present Amlogic, Inc. All rights reserved.
 *
 * All information contained herein is Amlogic confidential.
 *
 * This software is provided to you pursuant to Software License Agreement
 * (SLA) with Amlogic Inc ("Amlogic"). This software may be used
 * only in accordance with the terms of this agreement.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification is strictly prohibited without prior written permission from
 * Amlogic.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include <memory>
#include <unistd.h>

#include "../include/property.h"

using namespace std;
using namespace Amlogic::Platform;

void onEvent(const Property::PropertyChangedEvent &e) {
  if (e.action == "modified") {
    cout << "[" << e.key << "]: [" << e.oldvalue << "]: " << e.action;
    cout << " -> [" << e.value << "]" << endl;
  } else {
    cout << "[" << e.key << "]: [" << e.value << "]: " << e.action << endl;
  }
}

void list_property() {
  auto props = Property::list();
  for (auto &p : props) {
    cout << "[" << p.first << "]: [" << p.second << "]" << endl;
  }
}

void get_property(const char *key) {
  string v;
  if (Property::get(key, v)) {
    cout << "[" << key << "]: [" << v << "]" << endl;
  } else {
    cerr << key << " not found" << endl;
  }
}

void set_property(const char *key, const char *value) {
  if (Property::set(key, value)) {
    cout << "[" << key << "]: [" << value << "]" << endl;
  } else {
    cerr << "Failed to set " << key << " to " << value << endl;
  }
}

void monitor(const char *key) {
  if (key) {
    Property::subscribe_one(onEvent, key);
  } else {
    Property::subscribe(onEvent);
  }
}

void help() {
  cerr << "Platform property utility" << endl;
  cerr << endl << "Usage:" << endl;
  cerr << "  get <key>         : Get the value of property" << endl;
  cerr << "  set <key> <value> : Set the value of property, property will be "
          "deleted if the value is set to empty"
       << endl;
  cerr << "  del <key>         : Delete specified property" << endl;
  cerr << "  list              : List all of the properties" << endl;
  cerr << "  monitor [key ...] : Show property changes" << endl;
  exit(0);
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    help();
  }
  string action(argv[1]);
  if (action == "get" && argc > 2) {
    get_property(argv[2]);
    return 0;
  }
  if (action == "set" && argc > 3) {
    set_property(argv[2], argv[3]);
    return 0;
  }
  if (action == "del" && argc > 2) {
    set_property(argv[2], "");
    return 0;
  }
  if (action == "list") {
    list_property();
    return 0;
  }
  if (action == "monitor") {
    if (argc > 2) {
      for (int i = 2; i < argc; i++) {
        monitor(argv[i]);
      }
    } else {
      monitor(nullptr);
    }
    while (true) {
      sleep(1);
    }
    return 0;
  }
  help();
  return 0;
}

