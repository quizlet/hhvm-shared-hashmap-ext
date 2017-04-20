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

  bool set(const String& key, const String& value) {
    WriteLock write_lock(hashmap_mutex);

    hashmap[key.toCppString()] = value.toCppString();

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

  bool erase(const String& key) {
    WriteLock write_lock(hashmap_mutex);

    return hashmap.erase(key.toCppString());
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

static bool HHVM_FUNCTION(shhashmap_set, const String& map_name, const String& key, const String& value) {
  ReadLock read_lock(shared_hashmap_mutex);
  std::unordered_map<std::string, SharedHashMap *>::const_iterator shared_hashmap = shared_hashmaps.find(map_name.toCppString());
  if (shared_hashmap == shared_hashmaps.end()) return false;

  return shared_hashmap->second->set(key, value);
}

static Variant HHVM_FUNCTION(shhashmap_get, const String& map_name, const String& key) {
  ReadLock read_lock(shared_hashmap_mutex);
  std::unordered_map<std::string, SharedHashMap *>::const_iterator shared_hashmap = shared_hashmaps.find(map_name.toCppString());
  if (shared_hashmap == shared_hashmaps.end()) return Variant(false);

  return shared_hashmap->second->get(key);
}

static bool HHVM_FUNCTION(shhashmap_delete, const String& map_name, const String& key) {
  ReadLock read_lock(shared_hashmap_mutex);
  std::unordered_map<std::string, SharedHashMap *>::const_iterator shared_hashmap = shared_hashmaps.find(map_name.toCppString());
  if (shared_hashmap == shared_hashmaps.end()) return false;

  return shared_hashmap->second->erase(key);
}

static void HHVM_FUNCTION(shhashmap_close, const String& map_name) {
  ReadLock write_lock(shared_hashmap_mutex);
  const std::string  cpp_map_name = map_name.toCppString();
  std::unordered_map<std::string, SharedHashMap *>::const_iterator shared_hashmap = shared_hashmaps.find(cpp_map_name);
  if (shared_hashmap == shared_hashmaps.end()) return;

  delete shared_hashmap->second;
  shared_hashmaps.erase(cpp_map_name);
}

class SharedHashMapExtension : public Extension {
 public:
  SharedHashMapExtension() : Extension("shared_hashmap") {}
  virtual void moduleInit() {
    HHVM_FE(shhashmap_init);
    HHVM_FE(shhashmap_size);
    HHVM_FE(shhashmap_set);
    HHVM_FE(shhashmap_get);
    HHVM_FE(shhashmap_delete);
    HHVM_FE(shhashmap_close);
    loadSystemlib();
  }
} s_shared_hashmap_extension;

HHVM_GET_MODULE(shared_hashmap);

} // namespace HPHP