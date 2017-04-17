/*
   +----------------------------------------------------------------------+
   | HipHop for PHP                                                       |
   +----------------------------------------------------------------------+
   | Copyright (c) 2010-2013 Facebook, Inc. (http://www.facebook.com)     |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
*/

#include "hphp/runtime/ext/extension.h"
#include "hphp/util/lock.h"
#include "hphp/util/mutex.h"
#include <unordered_map>
#include <string>

namespace HPHP {

class SharedHashMap {
  ReadWriteMutex hashmap_mutex;
  const String& name;
  std::unordered_map<std::string, std::string> hashmap;
public:
  SharedHashMap(const String& _name) : name(_name) { }

  int64_t size() {
    ReadLock read_lock(hashmap_mutex);
    return hashmap.size();
  }

  bool add(const String& key, const String& value) {
    WriteLock write_lock(hashmap_mutex);

    try {
      hashmap.at(key.toCppString());
      return false;
    } catch (const std::out_of_range& oor) {
      hashmap[key.toCppString()] = value.toCppString();
    }

    return true;
  }

  Variant get(const String& key) {
    ReadLock read_lock(hashmap_mutex);

    try {
      const std::string value = hashmap.at(key.toCppString());
      return Variant(String(value));
    } catch (const std::out_of_range& oor) {
      return Variant(Variant::NullInit{});
    }
  }
};

static ReadWriteMutex shared_hashmap_mutex;
static std::unordered_map<std::string, SharedHashMap*> shared_hashmaps;

static bool HHVM_FUNCTION(shhashmap_init, const String& name) {
  const std::string cpp_name = name.toCppString();

  {
    ReadLock read_lock(shared_hashmap_mutex);
    std::unordered_map<std::string, SharedHashMap *>::const_iterator shared_hashmap = shared_hashmaps.find(cpp_name);
    if (shared_hashmap != shared_hashmaps.end()) return false;
  }

  {
    WriteLock write_lock(shared_hashmap_mutex);

    // Have to gate against a race condition by checking if it was just created here!
    std::unordered_map<std::string, SharedHashMap *>::const_iterator shared_hashmap = shared_hashmaps.find(cpp_name);
    if (shared_hashmap != shared_hashmaps.end()) return false;

    shared_hashmaps[cpp_name] = new SharedHashMap(name);
  }

  return true;
}

static int64_t HHVM_FUNCTION(shhashmap_size, const String& map_name) {
  ReadLock read_lock(shared_hashmap_mutex);
  std::unordered_map<std::string, SharedHashMap *>::const_iterator shared_hashmap = shared_hashmaps.find(map_name.toCppString());
  if (shared_hashmap == shared_hashmaps.end()) return false;

  return shared_hashmap->second->size();
}

static bool HHVM_FUNCTION(shhashmap_add, const String& map_name, const String& key, const String& value) {
  ReadLock read_lock(shared_hashmap_mutex);
  std::unordered_map<std::string, SharedHashMap *>::const_iterator shared_hashmap = shared_hashmaps.find(map_name.toCppString());
  if (shared_hashmap == shared_hashmaps.end()) return false;

  shared_hashmap->second->add(key, value);

  return true;
}

static Variant HHVM_FUNCTION(shhashmap_get, const String& map_name, const String& key) {
  ReadLock read_lock(shared_hashmap_mutex);
  std::unordered_map<std::string, SharedHashMap *>::const_iterator shared_hashmap = shared_hashmaps.find(map_name.toCppString());
  if (shared_hashmap == shared_hashmaps.end()) return Variant(false);

  return shared_hashmap->second->get(key);
}

class SharedHashMapExtension : public Extension {
 public:
  SharedHashMapExtension() : Extension("shared_hashmap") {}
  virtual void moduleInit() {
    HHVM_FE(shhashmap_init);
    HHVM_FE(shhashmap_size);
    HHVM_FE(shhashmap_add);
    HHVM_FE(shhashmap_get);
    loadSystemlib();
  }
} s_shared_hashmap_extension;

HHVM_GET_MODULE(shared_hashmap);

} // namespace HPHP