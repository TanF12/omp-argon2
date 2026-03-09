#pragma once
#include <atomic>
#include <thread>

#if defined(__i386__) || defined(__x86_64__)
    #include <immintrin.h>
    #define SPIN_HINT() _mm_pause()
#elif defined(__aarch64__)
    #define SPIN_HINT() __asm__ __volatile__("yield")
#else
    #define SPIN_HINT()
#endif

class SpinLock {
    std::atomic<bool> locked{false};
public:
    inline void lock() {
        int spins = 0;
        while (locked.exchange(true, std::memory_order_acquire)) {
            while (locked.load(std::memory_order_relaxed)) {
                if (spins < 10) {
                    SPIN_HINT();
                    ++spins;
                } else {
                    std::this_thread::yield();
                }
            }
        }
    }
    inline void unlock() {
        locked.store(false, std::memory_order_release);
    }
};