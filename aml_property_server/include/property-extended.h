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

#pragma once

#include "property.h"

#include <algorithm>
#include <sstream>

namespace Amlogic {
namespace Platform {
namespace Property {

// extended functions

bool get(const std::string &key, bool &value) {
  std::string v;
  bool ret = get(key, v);
  if (ret) {
    std::transform(v.begin(), v.end(), v.begin(), ::tolower);
    value = v == "1" || v == "true" || v == "on" || v == "y" || v == "yes";
  }
  return ret;
}

template <typename T> bool get(const std::string &key, T &value) {
  std::string v;
  bool ret = get(key, v);
  if (ret) {
    std::stringstream ss(v);
    ss >> value;
  }
  return ret;
}

bool set(const std::string &key, const bool &value) {
  std::string v;
  v = value ? "true" : "false";
  return set(key, v);
}

template <typename T> bool set(const std::string &key, const T &value) {
  std::stringstream ss;
  ss << value;
  return set(key, ss.str());
}

bool del(const std::string &key) { return set(key, ""); }

} // namespace Property
} // namespace Platform
} // namespace Amlogic

