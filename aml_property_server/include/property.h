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

#include <map>
#include <string>

namespace Amlogic {
namespace Platform {
namespace Property {

/**
 * @brief get entry value by key name
 *
 * @param key key name
 * @param[out] value entry value
 *
 * @return true: succeed, false: failed
 */
bool get(const std::string &key, std::string &value);

/**
 * @brief  set entry value by key name
 *
 * @param key key name
 * @param value entry value
 *
 * @return true: succeed, false: failed
 */
bool set(const std::string &key, const std::string &value);

/**
 * @brief list key/value in the current property database
 *
 * @return key/value map
 */
std::map<std::string, std::string> list();

struct PropertyChangedEvent {
  int event;
  std::string ns; // "property"
  std::string key;
  std::string action; // includes: added/deleted/modified
  std::string value;
  std::string oldvalue; // only assigned value when modify action
};

typedef void (*PropertyChangedCallback)(const PropertyChangedEvent &event);

/**
 * @brief subscribe property changed event
 *
 * @param callback
 *
 * @return true: succeed, false: failed
 */
bool subscribe(PropertyChangedCallback callback);

/**
 * @brief unsubscribe property changed event
 *
 * @param callback
 *
 */
void unsubscribe(PropertyChangedCallback callback);

// for client only
/**
 * @brief subscribe one specific property changed event
 *
 * @param callback
 *
 * @return true: succeed, false: failed
 */
bool subscribe_one(PropertyChangedCallback callback, const std::string &key);

} // namespace Property
} // namespace Platform
} // namespace Amlogic

