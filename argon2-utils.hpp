#pragma once
#include <atomic>
#include <thread>
#include <string>
#include <memory>
#include <cstddef>

#if defined(__i386__) || defined(__x86_64__)
    #include <immintrin.h>
    #define SPIN_HINT() _mm_pause()
#elif defined(__aarch64__)
    #define SPIN_HINT() __asm__ __volatile__("yield")
#else
    #define SPIN_HINT()
#endif

#if defined(_WIN32)
    #include <windows.h>
#else
    #include <string.h>
#endif

static constexpr size_t CACHE_LINE = 64;

template <class T>
struct SecureAllocator {
    typedef T value_type;
    SecureAllocator() = default;
    
    template <class U> constexpr SecureAllocator(const SecureAllocator<U>&) noexcept {}

    T* allocate(std::size_t n) {
        return static_cast<T*>(::operator new(n * sizeof(T)));
    }

    void deallocate(T* p, std::size_t n) noexcept {
#if defined(_WIN32)
        SecureZeroMemory(p, n * sizeof(T));
#else
        explicit_bzero(p, n * sizeof(T));
#endif
        ::operator delete(p);
    }

    template <class U>
    bool operator==(const SecureAllocator<U>&) const noexcept { return true; }
    template <class U>
    bool operator!=(const SecureAllocator<U>&) const noexcept { return false; }
};

using SecureString = std::vector<char, SecureAllocator<char>>;

class SpinLock {
    std::atomic<bool> locked{false};
public:
    inline void lock() {
        int backoff = 1;
        while (locked.exchange(true, std::memory_order_acquire)) {
            while (locked.load(std::memory_order_relaxed)) {
                if (backoff < 64) {
                    for (int i = 0; i < backoff; ++i) { SPIN_HINT(); }
                    backoff *= 2;
                } else {
                    std::this_thread::yield();
                }
            }
            backoff = 1; 
        }
    }
    inline void unlock() {
        locked.store(false, std::memory_order_release);
    }
};