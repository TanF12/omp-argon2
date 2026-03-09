#pragma once
#include <atomic>

#if defined(__i386__) || defined(__x86_64__)
    #include <immintrin.h>
    #define SPIN_HINT() _mm_pause()
#elif defined(__aarch64__)
    #define SPIN_HINT() __asm__ __volatile__("yield")
#else
    #define SPIN_HINT()
#endif

class SpinLock {
    std::atomic_flag locked = ATOMIC_FLAG_INIT;
public:
    inline void lock() {
        while (locked.test_and_set(std::memory_order_acquire)) {
            SPIN_HINT();
        }
    }
    inline void unlock() {
        locked.clear(std::memory_order_release);
    }
};