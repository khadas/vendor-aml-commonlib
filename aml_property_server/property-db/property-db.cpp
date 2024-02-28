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

#include <dirent.h>
#include <fstream>
#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include <list>
#include <map>
#include <mutex>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

#include "property-db.h"

#include "../common/log.h"
#define LOG_TAG "property-db"

using namespace std;

namespace Amlogic {
namespace Platform {
namespace Property {

using namespace std;

#ifndef DEVICE_PROPERTY_FILE
#define DEVICE_PROPERTY_FILE "/etc/device.properties"
#endif
#define ENV_PROP_FILE "PLF_DEVICE_PROPERTY_FILE"

#ifndef PROPERTY_DB_PATH
#define PROPERTY_DB_PATH "/data/persistent/device.prop"
#endif
#define ENV_DB_PATH "PLF_PROPERTY_DB_PATH"

static leveldb::DB *gs_persist_db = nullptr;
static map<string, string> gs_ro_device_properties;
static map<string, string> gs_temp_properties;
static mutex gs_temp_prop_lock;

enum DataAttr { ATTR_TEMP = 0, ATTR_PERSIST = 1, ATTR_RO = 2, ATTR_MAX };

static DataAttr get_attribute(const string &key) {
  DataAttr attr = ATTR_TEMP;
  if (key.rfind("ro.") == 0) {
    attr = ATTR_RO;
  } else if (key.rfind("persist.") == 0) {
    attr = ATTR_PERSIST;
  }
  return attr;
}

static mutex m_lock;
static std::list<PropertyChangedCallback> m_callbacks;

static void notify(PropertyChangedEvent &e) {
  const lock_guard<mutex> lock(m_lock);
  for (auto &callback : m_callbacks) {
    callback(e);
  }
}

static bool mkdir_p(string s, mode_t mode) {
  size_t pos = 0;

  if (s[s.size() - 1] != '/') {
    s += '/';
  }

  while ((pos = s.find_first_of('/', pos)) != string::npos) {
    string dir = s.substr(0, pos++);
    if (dir.size() == 0)
      continue;
    if (mkdir(dir.c_str(), mode) == -1 && errno != EEXIST) {
      return false;
    }
  }
  return true;
}

static bool rm_p(const char *path) {
  struct stat statbuf;
  DIR *dir;
  struct dirent *entry;
  char full_path[PATH_MAX];

  // Check if path exists
  if (stat(path, &statbuf) == -1) {
    LOGE("stat %s failed\n", path);
    return false;
  }

  // Handle directory
  if (S_ISDIR(statbuf.st_mode)) {
    dir = opendir(path);
    if (dir == NULL) {
      LOGE("opendir %s failed\n", path);
      return false;
    }

    while ((entry = readdir(dir)) != NULL) {
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
        continue;
      }

      snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
      if (!rm_p(full_path)) {
        closedir(dir);
        return false;
      }
    }

    closedir(dir);
    // Remove empty directory after removing contents
    if (rmdir(path) == -1) {
      LOGE("rmdir %s failed\n", path);
      return false;
    }
    return true;
  }

  // Handle file
  if (unlink(path) == -1) {
    LOGE("unlink %s failed\n", path);
    return false;
  }

  return true;
}

static vector<string> split(string const &s, char delim) {
  vector<string> elems;

  istringstream iss(s);
  string item;
  while (getline(iss, item, delim)) {
    *(back_inserter(elems))++ = item;
  }

  return elems;
}

static string &ltrim(string &s) {
  auto it = find_if(s.begin(), s.end(), [](char c) {
    return !isspace<char>(c, locale::classic());
  });
  s.erase(s.begin(), it);
  return s;
}

static string &rtrim(string &s) {
  auto it = find_if(s.rbegin(), s.rend(), [](char c) {
    return !isspace<char>(c, locale::classic());
  });
  s.erase(it.base(), s.end());
  return s;
}

static string &trim(string &s) { return ltrim(rtrim(s)); }

static bool temp_get(const string &key, string &value) {
  unique_lock<mutex> lock(gs_temp_prop_lock);
  auto it = gs_temp_properties.find(key);
  if (it != gs_temp_properties.end()) {
    value = it->second;
    return true;
  }
  return false;
}

static bool temp_set(const string &key, const string &value) {
  unique_lock<mutex> lock(gs_temp_prop_lock);
  gs_temp_properties[key] = value;
  return true;
}

static bool temp_del(const string &key) {
  unique_lock<mutex> lock(gs_temp_prop_lock);
  gs_temp_properties.erase(key);
  return true;
}

static void temp_list(map<string, string> &ret) {
  unique_lock<mutex> lock(gs_temp_prop_lock);
  for (auto it = gs_temp_properties.begin(); it != gs_temp_properties.end();
       ++it) {
    ret.insert({it->first, it->second});
  }
}

static bool persist_get(const string &key, string &value) {
  if (!gs_persist_db)
    return false;
  if (key.empty())
    return false;
  leveldb::Status s = gs_persist_db->Get(leveldb::ReadOptions(), key, &value);
  return s.ok();
}

static bool persist_set(const string &key, const string &value) {
  if (!gs_persist_db)
    return false;
  if (key.empty())
    return false;
  leveldb::Status s = gs_persist_db->Put(leveldb::WriteOptions(), key, value);
  return s.ok();
}

static bool persist_del(const string &key) {
  if (!gs_persist_db)
    return false;
  if (key.empty())
    return false;
  leveldb::Status s = gs_persist_db->Delete(leveldb::WriteOptions(), key);
  return s.ok();
}

static void persist_list(map<string, string> &ret) {
  if (!gs_persist_db)
    return;
  leveldb::ReadOptions options;
  options.snapshot = gs_persist_db->GetSnapshot();
  leveldb::Iterator *it = gs_persist_db->NewIterator(options);
  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    ret.insert({it->key().ToString(), it->value().ToString()});
  }
  if (!it->status().ok()) {
    LOGE("%s\n", it->status().ToString().c_str());
  }
  delete it;
  gs_persist_db->ReleaseSnapshot(options.snapshot);
}

static bool ro_get(const string &key, string &value) {
  auto it = gs_ro_device_properties.find(key);
  if (it != gs_ro_device_properties.end()) {
    value = it->second;
    return true;
  }
  return false;
}

static void ro_list(map<string, string> &ret) {
  for (auto it = gs_ro_device_properties.begin();
       it != gs_ro_device_properties.end(); ++it) {
    ret.insert({it->first, it->second});
  }
}

// Database single action
static struct Database {
  bool (*get)(const string &key, string &value);
  bool (*set)(const string &key, const string &value);
  bool (*del)(const string &key);
  void (*list)(map<string, string> &ret);
} gs_database[] = {
    {temp_get, temp_set, temp_del, temp_list},
    {persist_get, persist_set, persist_del, persist_list},
    {ro_get, nullptr, nullptr, ro_list},
};

static bool load_device_properties(const char *propfile) {
  ifstream fin(propfile);
  if (!fin) {
    LOGE("failed to load %s\n", propfile);
    return false;
  }
  leveldb::WriteBatch batch;
  string linebuf;
  while (getline(fin, linebuf)) {
    auto kvs = split(linebuf, '=');
    if (kvs.size() > 1) {
      string key(kvs.at(0));
      string value(kvs.at(1));
      trim(key), trim(value);
      DataAttr attr = get_attribute(key);
      if (attr == ATTR_RO) {
        // load ro properties into memory map
        gs_ro_device_properties[key] = kvs.at(1);
        continue;
      }

      // if key in device.properties not exists, add into database
      // the existing persistent properties will not be updated
      if (attr == ATTR_PERSIST && gs_persist_db) {
        string old_value("");
        leveldb::Status s =
            gs_persist_db->Get(leveldb::ReadOptions(), key, &old_value);
        if (!s.ok()) {
          batch.Put(kvs.at(0), kvs.at(1));
        }
      }
    }
  }

  if (gs_persist_db) {
    leveldb::Status s = gs_persist_db->Write(leveldb::WriteOptions(), &batch);
    if (!s.ok()) {
      LOGE("failed with %s\n", s.ToString().c_str());
      return false;
    }
  }
  return true;
}

static bool init_persist_db() {
  if (!gs_persist_db) {
    const char *env = getenv(ENV_DB_PATH);
    const char *db_file;
    if (env) {
      db_file = env;
    } else {
      db_file = PROPERTY_DB_PATH;
    }

    string db_dir =
        string(db_file).substr(0, string(db_file).find_last_of('/'));

    if (!mkdir_p(db_dir, 0755)) {
      LOGE("failed to create directory: %s\n", db_dir.c_str());
      goto load_device_properties;
    }

    leveldb::Options options;
    options.create_if_missing = true;
    leveldb::Status s;
    bool db_repaired = false;
    bool db_cleaned = false;
    // open -> repair -> reopen -> delete -> reopen -> failed
  retry_open:
    s = leveldb::DB::Open(options, db_file, &gs_persist_db);
    if (!s.ok()) {
      LOGE("failed to open db: %s, %s\n", db_file, s.ToString().c_str());
      if (s.IsIOError()) {
        LOGE("IO Error, db should not be touched\n");
        goto load_device_properties;
      }
      if (db_cleaned) {
        LOGE("persist db could not be recovered or created, aborting ...\n");
        goto load_device_properties;
      }
      if (!db_repaired && s.IsCorruption()) {
        LOGE("try to repair db\n");
        s = leveldb::RepairDB(db_file, options);
        if (!s.ok()) {
          LOGE("failed to repair persist db, error: %s, try to remove "
               "corrupted db...\n",
               s.ToString().c_str());
          goto fail_unlink;
        }
        db_repaired = true;
        LOGE("persist db is repaired, try to reopen...\n");
        goto retry_open;
      }
      LOGE("persist db is not corrupt or is repaired, try to unlink...\n");
      goto fail_unlink;
    }
    goto load_device_properties;
  fail_unlink:
    LOGE("CAUTION: database permanently lost, recreating...\n");
    if (!rm_p(db_file)) {
      LOGE("failed to unlink db: %s, %s\n", db_file, strerror(errno));
      goto load_device_properties;
    }
    db_cleaned = true;
    goto retry_open;
  }
load_device_properties : {
  // load readonly device properties
  const char *env = getenv(ENV_PROP_FILE);
  const char *prop_file;
  if (env) {
    prop_file = env;
  } else {
    prop_file = DEVICE_PROPERTY_FILE;
  }
  load_device_properties(prop_file);
}
  return true;
}

bool init() { return init_persist_db(); }

void deinit() {
  if (gs_persist_db) {
    delete gs_persist_db;
    gs_persist_db = nullptr;
  }
}

static bool internal_set(const string &key, const string &value) {
  if (key.empty())
    return false;

  DataAttr attr = get_attribute(key);
  if (attr == ATTR_RO) {
    LOGW("entry `%s` readonly, should not be modified\n", key.c_str());
    return false;
  }

  string action;
  string old_value("");
  bool ret = gs_database[attr].get(key, old_value);
  if (value.empty() && ret) {
    action = "deleted";
    ret = gs_database[attr].del(key);
  } else {
    if (ret) {
      if (old_value == value) {
        return true;
      }
      action = "modified";
    } else {
      if (value.empty())
        return true;
      action = "added";
    }
    ret = gs_database[attr].set(key, value);
  }
  if (ret) {
    // send changed event
    PropertyChangedEvent e;
    e.event = 0;
    e.ns = "property";
    e.key = key;
    e.action = action;
    if (action == "deleted") {
      e.value = old_value;
    } else {
      e.value = value;
    }
    e.oldvalue = old_value;
    notify(e);
  }
  return ret;
}

bool get(const string &key, string &value) {
  string _key(key);
  trim(_key);
  DataAttr attr = get_attribute(_key);
  return gs_database[attr].get(_key, value);
}

bool set(const string &key, const string &value) {
  string _key(key);
  string _value(value);
  trim(_key), trim(_value);
  return internal_set(_key, _value);
}

map<string, string> list() {
  map<string, string> ret;
  for (int i = 0; i < ATTR_MAX; i++) {
    gs_database[i].list(ret);
  }
  return ret;
}

bool subscribe(PropertyChangedCallback callback) {
  const lock_guard<mutex> lock(m_lock);
  m_callbacks.push_back(callback);
  return true;
}

void unsubscribe(PropertyChangedCallback callback) {
  const lock_guard<mutex> lock(m_lock);
  auto i = find(m_callbacks.begin(), m_callbacks.end(), callback);
  if (i != m_callbacks.end()) {
    m_callbacks.erase(i);
    return;
  }
  return;
}

} // namespace Property
} // namespace Platform
} // namespace Amlogic

