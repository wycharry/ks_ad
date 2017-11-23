#pragma once
#include <cstdlib>
#include "common/common.h"
namespace ks {
namespace ad_base {

static uint64_t djb2_hash64(uint64_t key){
  const unsigned char *p = reinterpret_cast<unsigned char*>(&key);
  uint64_t hash = 5381;
  for (size_t i = 0; i < sizeof(uint64_t); ++i){
    hash = ((hash << 5ULL) + hash) + p[i];
  }
  return hash;
}

static uint64_t dek_hashstr(const char* str) {
  uint64_t hash = 1315423911;
  if (str == nullptr) {
    return hash;
  }
  for (; *str != '\0'; ++str) {
    hash = ((hash << 5ULL) ^ (hash >> 27ULL)) ^ *(reinterpret_cast<const uint8_t*>(str));
  }
  return hash;
}

static uint64_t dek_hash64(uint64_t key){
  const unsigned char *p = reinterpret_cast<unsigned char*>(&key);
  uint64_t hash = 1315423911;
  for(size_t i = 0; i < sizeof(uint64_t); ++i) {
    hash = ((hash << 5ULL) ^ (hash >> 27ULL)) ^ p[i];
  }
  return hash;
}

static uint64_t fnv_hash64(uint64_t key)
{
  // http://isthe.com/chongo/tech/comp/fnv/
  const unsigned char *p = reinterpret_cast<unsigned char*>(&key);
  // 2166136261 * 16777619
  uint64_t hash = 14695981039346656037ULL;
  for(size_t i = 0; i < sizeof(uint64_t); ++i) {
    hash = (hash * 1099511628211ULL) ^ p[i];
  }
  return hash;
}

static uint64_t bkdr_hash64(uint64_t key)
{
  const unsigned char *p = reinterpret_cast<unsigned char*>(&key);
  uint64_t hash = 0;
  for(size_t i = 0; i < sizeof(uint64_t); ++i) {
    hash = (hash << 7) + (hash << 1) + hash + p[i];
  }
  return hash;
}

static uint64_t sdmb_hash64(uint64_t key)
{
  const unsigned char *p = reinterpret_cast<unsigned char*>(&key);
  uint64_t hash = 0;
  for(size_t i = 0; i < sizeof(uint64_t); ++i) {
    hash = (hash << 6) + (hash << 16) - hash + p[i];
  }
  return hash;
}

static uint64_t rs_hash64(uint64_t key)
{
  const unsigned char *p = reinterpret_cast<unsigned char*>(&key);
  uint64_t hash = 0;
  uint32_t magic = 63689;
  for(size_t i = 0; i < sizeof(uint64_t); ++i) {
    hash = hash * magic + p[i];
    magic *= 378551;
  }
  return hash;
}

static uint64_t ap_hash64(uint64_t key)
{
  const unsigned char *p = reinterpret_cast<unsigned char*>(&key);
  uint64_t hash = 0;
  for(size_t i = 0; i < sizeof(uint64_t); ++i) {
    if ((i & 1) == 0)
    {
      hash ^= ((hash << 7) ^ p[i] ^ (hash >> 3));
    }
    else
    {
      hash ^= (~((hash << 11) ^ p[i] ^ (hash >> 5)));
    }
  }
  return hash;
}

static uint64_t murmur3_hash64(uint64_t key)
{
  key ^= key >> 33;
  key *= 0xff51afd7ed558ccd;
  key ^= key >> 33;
  key *= 0xc4ceb9fe1a85ec53;
  key ^= key >> 33;
  return key;
}

static uint64_t github_hash64(uint64_t key)
{
  // https://gist.github.com/badboy/6267743
  key = (~key) + (key << 21); // key = (key << 21) - key - 1;
  key = key ^ (key >> 24);
  key = (key + (key << 3)) + (key << 8); // key * 265
  key = key ^ (key >> 14);
  key = (key + (key << 2)) + (key << 4); // key * 21
  key = key ^ (key >> 28);
  key = key + (key << 31);
  return key;
}

static uint64_t knuth_hash64(uint64_t key)
{
  // Knuth multiplicative hash
  // 2654435761, 0x9E3779B97F4A7C13
  return (key * 2654435761 >> 8);
}


//////////////////////////////////////////////////////////////////////////
class DJB2Hash {
public:
  KS_INLINE uint64_t operator()(const uint64_t key) const {
    return djb2_hash64(key);
  }
};
class DEKHash {
public:
  KS_INLINE uint64_t operator()(const uint64_t key) const {
    return dek_hash64(key);
  }
};
class DEKStrHash {
public:
  KS_INLINE uint64_t operator()(const char* key) const {
    return dek_hashstr(key);
  }
};
class FNVHash {
public:
  KS_INLINE uint64_t operator()(const uint64_t key) const {
    return fnv_hash64(key);
  }
};
class BKDRHash {
public:
  KS_INLINE uint64_t operator()(const uint64_t key) const {
    return bkdr_hash64(key);
  }
};
class SDMBHash {
public:
  KS_INLINE uint64_t operator()(const uint64_t key) const {
    return sdmb_hash64(key);
  }
};
class RSHash {
public:
  KS_INLINE uint64_t operator()(const uint64_t key) const {
    return rs_hash64(key);
  }
};
class APHash {
public:
  KS_INLINE uint64_t operator()(const uint64_t key) const {
    return ap_hash64(key);
  }
};
class MurMurHash {
public:
  KS_INLINE uint64_t operator()(const uint64_t key) const {
    return murmur3_hash64(key);
  }
};
class GithubHash {
public:
  KS_INLINE uint64_t operator()(const uint64_t key) const {
    return github_hash64(key);
  }
};
class KnuthHash {
public:
  KS_INLINE uint64_t operator()(const uint64_t key) const {
    return knuth_hash64(key);
  }
};




}
}

