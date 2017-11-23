#pragma once

#include <cstdint>
#include <cstdlib>

#define KS_LIKELY(x) __builtin_expect(!!(x),1)
#define KS_UNLIKELY(x) __builtin_expect(!!(x),0)
#define KS_INLINE inline __attribute__((always_inline))
#define KS_ISNULL(x) KS_UNLIKELY((x) == nullptr)
#define CAS(address,oldValue,newValue) __sync_bool_compare_and_swap(address,oldValue,newValue)
#define CACHELINE_SIZE 64
#define CACHE_ALIGNED __attribute__((aligned(CACHELINE_SIZE)))
#define CPU_RELAX() __asm__ __volatile__("pause\n": : :"memory")
#define ACCESS_ONCE(x) (*((volatile __typeof__(x) *) &x))
#define CPU_BARRIER() __asm__ __volatile__("mfence": : :"memory")
#define COMPILER_BARRIER() __asm__ __volatile__("" : : : "memory")
#define ATOMIC_STORE(var, value) CAS(&var, var, value)
#define ADD_AND_FETCH(address,offset) __sync_add_and_fetch(address,offset)
#define FETCH_AND_ADD(address,offset) __sync_fetch_and_add(address,offset)
#define MAX_THREAD_NUM 503
