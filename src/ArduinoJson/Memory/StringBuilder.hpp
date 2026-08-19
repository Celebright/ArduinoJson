// ArduinoJson - https://arduinojson.org
// Copyright © 2014-2024, Benoit BLANCHON
// MIT License

#pragma once

#include <ArduinoJson/Memory/ResourceManager.hpp>

ARDUINOJSON_BEGIN_PRIVATE_NAMESPACE

class StringBuilder {
 public:
  static const size_t initialCapacity = 31;

  StringBuilder(ResourceManager* resources) : resources_(resources) {}

  ~StringBuilder() {
    if (node_)
      resources_->destroyString(node_);
  }

  void startString() {
    size_ = 0;
    if (!node_)
      node_ = resources_->createString(initialCapacity);
  }

  StringNode* save() {
    ARDUINOJSON_ASSERT(node_ != nullptr);
    node_->data[size_] = 0;
#if defined(ARDUINOJSON_SKIP_DEDUP) && ARDUINOJSON_SKIP_DEDUP
    // Celebright patch: skip string interning. getString() is a linear scan of
    // the pool, run for every parsed string, so many UNIQUE strings (e.g. 3000
    // distinct numeric bulb keys) make deserialization O(n^2) -- the ~27s lockup.
    // Forcing node == nullptr always allocates a fresh string, keeping parsing
    // O(n); the only cost is that identical strings are no longer shared (a little
    // more PSRAM). This is the hot path for deserialized keys/values (see also the
    // StringPool::add and JsonDeserializer.hpp patches). Enabled via
    // ARDUINOJSON_SKIP_DEDUP in the top-level CMakeLists.txt. Re-check after any
    // ArduinoJson bump.
    StringNode* node = nullptr;
#else
    StringNode* node = resources_->getString(adaptString(node_->data, size_));
#endif
    if (!node) {
      node = resources_->resizeString(node_, size_);
      ARDUINOJSON_ASSERT(node != nullptr);  // realloc to smaller can't fail
      resources_->saveString(node);
      node_ = nullptr;  // next time we need a new string
    } else {
      node->references++;
    }
    return node;
  }

  void append(const char* s) {
    while (*s)
      append(*s++);
  }

  void append(const char* s, size_t n) {
    while (n-- > 0)  // TODO: memcpy
      append(*s++);
  }

  void append(char c) {
    if (node_ && size_ == node_->length)
      node_ = resources_->resizeString(node_, size_ * 2U + 1);
    if (node_)
      node_->data[size_++] = c;
  }

  bool isValid() const {
    return node_ != nullptr;
  }

  size_t size() const {
    return size_;
  }

  JsonString str() const {
    ARDUINOJSON_ASSERT(node_ != nullptr);
    node_->data[size_] = 0;
    return JsonString(node_->data, size_, JsonString::Copied);
  }

 private:
  ResourceManager* resources_;
  StringNode* node_ = nullptr;
  size_t size_ = 0;
};

ARDUINOJSON_END_PRIVATE_NAMESPACE
