#pragma once
#include <cstring>
#include "common/common.h"
#include "hash/hash.h"

namespace ks {
namespace ad_base {

//Note : only support uint62_t key!!!!!!
template<typename Value = uint64_t, typename HashFunction1 = KnuthHash,
    typename HashFunction2 = MurMurHash>
class AdCuckooHashMap {
public:
  struct MapNode {
    //key的最高位用来代表槽是否可用，为0表示可用，为1表示已被占用；
    //次高位代表hash function idx
    //也就是说，这个hashmap，有效的key type其实是uint62_t
    uint64_t key_;
    Value value_;
  };
  enum class OpStatus {
    SUCCESS = 0,
    INVALID_ARGUMENT = 1,
    INSERT_FAILED = 2,
    NOT_INITED = 3,
    KEY_EXISTS = 4,
    KEY_DOES_NOT_EXIST = 5,
    REPLACE_FAILED = 6,
    REHASH_FAILED = 7,
    COPY_FAILED = 8,
    INVALID_KEY = 9,
  };
private:
  static const uint64_t valid_mask = (1ULL << 63);
  static const uint64_t hash_mask = (1ULL << 62);
  static const uint64_t flag_mask = (3ULL << 62);
  static const uint64_t key_mask = ((1ULL << 62) - 1);
  // 最大10E，不能再多了
  static const uint64_t MAX_CAPACITY = (1ULL << 29);
  static const uint32_t SLOT_WIDE = 64 / sizeof(MapNode);
  //data_是一个 capacity_ * 4的二维数组
  MapNode (*data_)[SLOT_WIDE];
  //任何时候，capacity_都是一个2的k次幂
  //这样，idx可以根据idx = this->hf1_(key) & (capacity_ - 1)来计算
  uint64_t capacity_;
  //hashmap中目前存在的元素的个数
  uint64_t size_;
  //对应的哈希函数，目前只支持两个
  HashFunction1 hf1_;
  HashFunction2 hf2_;
  bool is_inited_;
  uint32_t max_replace_size_;
public:
  AdCuckooHashMap();
  ~AdCuckooHashMap();
  AdCuckooHashMap(uint64_t init_capacity, uint32_t max_replace_size);
  //如果key已经存在，看overwrite的值是否为true，如果true，覆盖；否则返回KEY_ALREADY_EXIST
  //不存在，则尝试插入，返回插入的结果
  OpStatus insert(const uint64_t &key, const Value &value, bool overwrite = false);
  //如果key存在，函数返回true
  //如果key不存在，返回false
  KS_INLINE OpStatus exist(const uint64_t &key) const;
  //如果key存在，函数返回KEY_EXISTS，得到对应的value
  //如果key不存在，返回KEY_DOES_NOT_EXIST
  KS_INLINE OpStatus get(const uint64_t &key, Value &value) const;
  KS_INLINE Value get(const uint64_t &key) const;
  KS_INLINE uint64_t size() const {return size_;}
  KS_INLINE void clear();
  KS_INLINE OpStatus erase(const uint64_t &key);
  KS_INLINE double load_factor() const;
private:
  KS_INLINE bool allocte(MapNode (*&p)[SLOT_WIDE], uint64_t capacity);
  // level用来判断是第几次rehash，传入data，data可能会变，成功返回SUCCESS，否则返回INSERT_FAILED
  OpStatus insert(MapNode (*&data)[SLOT_WIDE], uint64_t &capacity, const uint64_t &key, const Value &value, const bool &overwrite);
  OpStatus insert_new(MapNode (*&data)[SLOT_WIDE], uint64_t &capacity, const uint64_t &key, const Value &value);
  // 给定kv尝试replace，成功返回SUCCESS，否则REHASH_FAILED
  OpStatus replace(MapNode (*&data)[SLOT_WIDE], const uint64_t &capacity, uint64_t &key, Value &value, uint64_t &seed);
  // 根据level决定rehash多大，copy原来的data和kv到新的data，成功返回SUCCESS，否则REHASH_FAILED
  OpStatus rehash(MapNode (*&data)[SLOT_WIDE], uint64_t &capacity, uint64_t &new_capacity, const uint64_t &key, const Value &value);
};

template<typename Value, typename HashFunction1, typename HashFunction2>
AdCuckooHashMap<Value, HashFunction1, HashFunction2>::AdCuckooHashMap() {
  static_assert(sizeof(MapNode) <= 16, "sizeof(Value) should not exceed 8");
  capacity_ = (1 << 10);
  size_ = 0;
  max_replace_size_ = 100;
  is_inited_ = false;
  if (allocte(data_, capacity_)) {
    is_inited_ = true;
  } else {
    capacity_ = 0;
    size_ = 0;
    is_inited_ = false;
  }
}

template<typename Value, typename HashFunction1, typename HashFunction2>
AdCuckooHashMap<Value, HashFunction1, HashFunction2>::~AdCuckooHashMap() {
  delete []data_;
}

template<typename Value, typename HashFunction1, typename HashFunction2>
AdCuckooHashMap<Value, HashFunction1, HashFunction2>::AdCuckooHashMap(uint64_t init_capacity, uint32_t max_replace_size) {
  static_assert(sizeof(MapNode) <= 16, "sizeof(Value) should not exceed 8");
  capacity_ = 1;
  while (init_capacity) {
    init_capacity >>= 1;
    capacity_ <<= 1;
  }
  size_ = 0;
  max_replace_size_ = max_replace_size;
  is_inited_ = false;
  if (allocte(data_, capacity_)) {
    is_inited_ = true;
  } else {
    capacity_ = 0;
    size_ = 0;
    is_inited_ = false;
  }
}

template<typename Value, typename HashFunction1, typename HashFunction2>
bool AdCuckooHashMap<Value, HashFunction1, HashFunction2>::allocte(MapNode (*&p)[SLOT_WIDE], uint64_t capacity) {
  p = new MapNode[capacity][SLOT_WIDE];
  if (p != nullptr) {
    return true;
  }
  return false;
}

template<typename Value, typename HashFunction1, typename HashFunction2>
Value AdCuckooHashMap<Value, HashFunction1, HashFunction2>::get(const uint64_t &key) const {
  Value value{};
  get(key, value);
  return value;
}

template<typename Value, typename HashFunction1, typename HashFunction2>
void AdCuckooHashMap<Value, HashFunction1, HashFunction2>::clear() {
  size_ = 0;
  memset(data_, 0, capacity_ * SLOT_WIDE * sizeof(MapNode));
}

template<typename Value, typename HashFunction1, typename HashFunction2>
double AdCuckooHashMap<Value, HashFunction1, HashFunction2>::load_factor() const {
  uint64_t occupied = 0;
  for (uint64_t i = 0; i < capacity_; ++i) {
    occupied += ((data_[i][0].key_ & valid_mask) > 0);
  }
  return (double)occupied / capacity_;
}

template<typename Value, typename HashFunction1, typename HashFunction2>
typename AdCuckooHashMap<Value, HashFunction1, HashFunction2>::OpStatus AdCuckooHashMap<Value, HashFunction1, HashFunction2>::erase(const uint64_t &key) {
  if (KS_UNLIKELY(!is_inited_)) {
    return OpStatus::NOT_INITED;
  } else {
    uint64_t idx = this->hf1_(key) & (capacity_ - 1);
    for (uint32_t j = 0; j < SLOT_WIDE; ++j) {
      if ((data_[idx][j].key_ & valid_mask) && (data_[idx][j].key_ & key_mask) == key) {
        --size_;
        data_[idx][j].key_ = 0;
        return OpStatus::SUCCESS;
      }
    }
    idx = this->hf2_(key) & (capacity_ - 1);
    for (uint32_t j = 0; j < SLOT_WIDE; ++j) {
      if ((data_[idx][j].key_ & valid_mask) && (data_[idx][j].key_ & key_mask) == key) {
        --size_;
        data_[idx][j].key_ = 0;
        return OpStatus::SUCCESS;
      }
    }
    return OpStatus::KEY_DOES_NOT_EXIST;
  }
}

template<typename Value, typename HashFunction1, typename HashFunction2>
typename AdCuckooHashMap<Value, HashFunction1, HashFunction2>::OpStatus AdCuckooHashMap<Value, HashFunction1, HashFunction2>::get(const uint64_t &key, Value &value) const {
  if (KS_UNLIKELY(!is_inited_)) {
    return OpStatus::NOT_INITED;
  } else {
    if (KS_UNLIKELY(key & flag_mask)) {
      return OpStatus::INVALID_KEY;
    }
    uint64_t idx = this->hf1_(key) & (capacity_ - 1);
    for (uint32_t j = 0; j < SLOT_WIDE; ++j) {
      if ((data_[idx][j].key_ & key_mask) == key && (data_[idx][j].key_ & valid_mask)) {
        value = data_[idx][j].value_;
        return OpStatus::SUCCESS;
      }
    }
    idx = this->hf2_(key) & (capacity_ - 1);
    for (uint32_t j = 0; j < SLOT_WIDE; ++j) {
      if ((data_[idx][j].key_ & key_mask) == key && (data_[idx][j].key_ & valid_mask)) {
        value = data_[idx][j].value_;
        return OpStatus::SUCCESS;
      }
    }
    return OpStatus::KEY_DOES_NOT_EXIST;
  }
}

template<typename Value, typename HashFunction1, typename HashFunction2>
typename AdCuckooHashMap<Value, HashFunction1, HashFunction2>::OpStatus AdCuckooHashMap<Value, HashFunction1, HashFunction2>::exist(const uint64_t &key) const {
  Value value;
  return get(key, value);
}

template<typename Value, typename HashFunction1, typename HashFunction2>
typename AdCuckooHashMap<Value, HashFunction1, HashFunction2>::OpStatus AdCuckooHashMap<Value, HashFunction1, HashFunction2>::insert(const uint64_t &key, const Value &value, bool overwrite) {
  if (KS_UNLIKELY(key & flag_mask)) {
    return OpStatus::INVALID_KEY;
  }
  return insert(data_, capacity_, key, value, overwrite);
}

template<typename Value, typename HashFunction1, typename HashFunction2>
typename AdCuckooHashMap<Value, HashFunction1, HashFunction2>::OpStatus AdCuckooHashMap<Value, HashFunction1, HashFunction2>::insert(MapNode (*&data)[SLOT_WIDE], uint64_t &capacity, const uint64_t &key, const Value &value, const bool &overwrite) {
  // 判断存不存在，根据overwrite分开讨论
  uint64_t idx1, idx2;
  if (KS_UNLIKELY(overwrite)) {
    idx1 = this->hf1_(key) & (capacity - 1);
    for (uint32_t j = 0; j < SLOT_WIDE; ++j) {
      if ((data[idx1][j].key_ & key_mask) == key && (data[idx1][j].key_ & valid_mask)) {
        data[idx1][j].value_ = value;
        return OpStatus::SUCCESS;
      }
    }
    idx2 = this->hf2_(key) & (capacity - 1);
    for (uint32_t j = 0; j < SLOT_WIDE; ++j) {
      if ((data[idx2][j].key_ & key_mask) == key && (data[idx2][j].key_ & valid_mask)) {
        data[idx2][j].value_ = value;
        return OpStatus::SUCCESS;
      }
    }
  } else {
    idx1 = this->hf1_(key) & (capacity - 1);
    for (uint32_t j = 0; j < SLOT_WIDE; ++j) {
      if ((data[idx1][j].key_ & key_mask) == key && (data[idx1][j].key_ & valid_mask)) {
        return OpStatus::KEY_EXISTS;
      }
    }
    idx2 = this->hf2_(key) & (capacity - 1);
    for (uint32_t j = 0; j < SLOT_WIDE; ++j) {
      if ((data[idx2][j].key_ & key_mask) == key && (data[idx2][j].key_ & valid_mask)) {
        return OpStatus::KEY_EXISTS;
      }
    }
  }
  // 
  for (uint32_t j = 0; j < SLOT_WIDE; ++j) {
    if ((data[idx1][j].key_ & valid_mask) == 0) {
      data[idx1][j].key_ = (key | valid_mask);
      data[idx1][j].value_ = value;
      ++size_;
      return OpStatus::SUCCESS;
    }
  }
  for (uint32_t j = 0; j < SLOT_WIDE; ++j) {
    if ((data[idx2][j].key_ & valid_mask) == 0) {
      data[idx2][j].key_ = (key | valid_mask | hash_mask);
      data[idx2][j].value_ = value;
      ++size_;
      return OpStatus::SUCCESS;
    }
  }
  // aha, let's replace some times
  uint64_t seed = this->hf1_(idx1 + idx2 + key);
  uint32_t random_pos = seed % (SLOT_WIDE * 2);
  uint64_t replace_key;
  Value replace_value;
  if (random_pos < SLOT_WIDE) {
    replace_key = data[idx1][random_pos].key_;
    replace_value = data[idx1][random_pos].value_;
    data[idx1][random_pos].key_ = (key | valid_mask);
    data[idx1][random_pos].value_ = value;
  } else {
    random_pos = random_pos - SLOT_WIDE;
    replace_key = data[idx2][random_pos].key_;
    replace_value = data[idx2][random_pos].value_;
    data[idx2][random_pos].key_ = (key | valid_mask | hash_mask);
    data[idx2][random_pos].value_ = value;
  }
  uint32_t replace_try_cnt = 0;
  while (KS_UNLIKELY(++replace_try_cnt <= max_replace_size_)) {
    OpStatus replace_status = replace(data, capacity, replace_key, replace_value, seed);
    if (replace_status == OpStatus::SUCCESS) {
      //if (replace_try_cnt > 100) std::cout << "replace : " << replace_try_cnt << std::endl;
      ++size_;
      return OpStatus::SUCCESS;
    }
  }
  // std::cout << "insert : rehash ? " << replace_try_cnt << std::endl;
  // let's rehash, if failed, just damn it
  uint64_t new_capacity = capacity;
  OpStatus rehash_status;
  do {
    new_capacity <<= 1;
    rehash_status = rehash(data, capacity, new_capacity, replace_key, replace_value);
  } while (KS_UNLIKELY(rehash_status == OpStatus::INSERT_FAILED && new_capacity < MAX_CAPACITY));
  size_ += (rehash_status == OpStatus::SUCCESS);
  return rehash_status;
}

template<typename Value, typename HashFunction1, typename HashFunction2>
typename AdCuckooHashMap<Value, HashFunction1, HashFunction2>::OpStatus AdCuckooHashMap<Value, HashFunction1, HashFunction2>::insert_new(MapNode (*&data)[SLOT_WIDE], uint64_t &capacity, const uint64_t &key, const Value &value) {
  // try insert new element
  uint64_t idx1 = this->hf1_(key) & (capacity - 1);
  for (uint32_t j = 0; j < SLOT_WIDE; ++j) {
    if ((data[idx1][j].key_ & valid_mask) == 0) {
      data[idx1][j].key_ = (key | valid_mask);
      data[idx1][j].value_ = value;
      return OpStatus::SUCCESS;
    }
  }
  uint64_t idx2 = this->hf2_(key) & (capacity - 1);
  for (uint32_t j = 0; j < SLOT_WIDE; ++j) {
    if ((data[idx2][j].key_ & valid_mask) == 0) {
      data[idx2][j].key_ = (key | valid_mask | hash_mask);
      data[idx2][j].value_ = value;
      return OpStatus::SUCCESS;
    }
  }
  // aha, let's replace some times
  uint64_t seed = this->hf1_(idx1 + idx2 + key);
  uint32_t random_pos = seed % (SLOT_WIDE * 2);
  uint64_t replace_key;
  Value replace_value;
  if (random_pos < SLOT_WIDE) {
    replace_key = data[idx1][random_pos].key_;
    replace_value = data[idx1][random_pos].value_;
    data[idx1][random_pos].key_ = (key | valid_mask);
    data[idx1][random_pos].value_ = value;
  } else {
    random_pos = random_pos - SLOT_WIDE;
    replace_key = data[idx2][random_pos].key_;
    replace_value = data[idx2][random_pos].value_;
    data[idx2][random_pos].key_ = (key | valid_mask | hash_mask);
    data[idx2][random_pos].value_ = value;
  }
  uint32_t replace_try_cnt = 0;
  while (KS_UNLIKELY(++replace_try_cnt <= max_replace_size_)) {
    OpStatus replace_status = replace(data, capacity, replace_key, replace_value, seed);
    if (replace_status == OpStatus::SUCCESS) {
      return OpStatus::SUCCESS;
    }
  }
  // insert failed during rehashing, if failed, let's return and rehash larger
  return OpStatus::INSERT_FAILED;
}

template<typename Value, typename HashFunction1, typename HashFunction2>
typename AdCuckooHashMap<Value, HashFunction1, HashFunction2>::OpStatus AdCuckooHashMap<Value, HashFunction1, HashFunction2>::replace( MapNode (*&data)[SLOT_WIDE], const uint64_t &capacity, uint64_t &key, Value &value, uint64_t &seed) {
  uint64_t idx;
  if (key & hash_mask) {
    idx = this->hf1_(key & key_mask) & (capacity - 1);
  } else {
    idx = this->hf2_(key & key_mask) & (capacity - 1);
  }
  key = (key ^ hash_mask);
  int32_t hash2_idx = -1;
  for (uint32_t j = 0; j < SLOT_WIDE; ++j) {
    if ((data[idx][j].key_ & valid_mask) == 0) {
      data[idx][j].key_ = key;
      data[idx][j].value_ = value;
      return OpStatus::SUCCESS;
    }
  }
  seed = this->hf1_(seed + key + idx);
  uint32_t random_pos = seed % SLOT_WIDE;
  std::swap(data[idx][random_pos].key_, key);
  std::swap(data[idx][random_pos].value_, value);
  return OpStatus::REPLACE_FAILED;
}

template<typename Value, typename HashFunction1, typename HashFunction2>
typename AdCuckooHashMap<Value, HashFunction1, HashFunction2>::OpStatus AdCuckooHashMap<Value, HashFunction1, HashFunction2>::rehash(MapNode (*&data)[SLOT_WIDE], uint64_t &capacity, uint64_t &new_capacity, const uint64_t &key, const Value &value) {
  MapNode (*new_data)[SLOT_WIDE];
  if (allocte(new_data, new_capacity)) {
    insert_new(new_data, new_capacity, (key & key_mask), value);
    for (uint64_t i = 0; i < capacity; ++i) {
      for (uint32_t j = 0; j < SLOT_WIDE; ++j) {
        if (data[i][j].key_ & valid_mask) {
          OpStatus insert_status = insert_new(new_data, new_capacity, (data_[i][j].key_ & key_mask), data_[i][j].value_);
          if (KS_UNLIKELY(insert_status != OpStatus::SUCCESS)) {
            delete []new_data;
            return OpStatus::INSERT_FAILED;
          }
        }
      }
    }
    std::swap(data, new_data);
    std::swap(capacity, new_capacity);
    delete []new_data;
    return OpStatus::SUCCESS;
  } else {
    return OpStatus::REHASH_FAILED;
  }
}

}  //end space ad_base
}  //end space ks
