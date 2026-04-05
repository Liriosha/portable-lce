#include <cstdlib>
#include <new>
#include <mutex>
#include <unordered_set>
#ifdef _WIN32
  #include <malloc.h>
#endif

#ifdef TRACY_ENABLE
  #include <tracy/Tracy.hpp>

namespace {
template <typename T>
struct TracyMallocAllocator {
    using value_type = T;
    TracyMallocAllocator() noexcept = default;
    template <typename U>
    TracyMallocAllocator(const TracyMallocAllocator<U>&) noexcept {}
    T* allocate(std::size_t n) {
        return static_cast<T*>(std::malloc(n * sizeof(T)));
    }
    void deallocate(T* p, std::size_t) noexcept { std::free(p); }
};

using PtrSet =
    std::unordered_set<void*, std::hash<void*>, std::equal_to<void*>,
                       TracyMallocAllocator<void*>>;

inline PtrSet* GetAllocSet() {
    static PtrSet* set = []() -> PtrSet* {
        void* mem = std::malloc(sizeof(PtrSet));
        return mem ? new (mem) PtrSet() : nullptr;
    }();
    return set;
}

inline std::mutex* GetAllocMutex() {
    static std::mutex* mtx = []() -> std::mutex* {
        void* mem = std::malloc(sizeof(std::mutex));
        return mem ? new (mem) std::mutex() : nullptr;
    }();
    return mtx;
}

inline void* TracyAlignedAlloc(std::size_t size, std::size_t alignment) {
    if (alignment < alignof(void*)) alignment = alignof(void*);
#if defined(_WIN32)
    return _aligned_malloc(size, alignment);
#else
    void* ptr = nullptr;
    if (posix_memalign(&ptr, alignment, size) != 0) {
        return nullptr;
    }
    return ptr;
#endif
}

inline void TracyAlignedFree(void* ptr) {
#if defined(_WIN32)
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

inline void TracyTrackAlloc(void* ptr, std::size_t size) {
    if (!ptr) return;
    auto* set = GetAllocSet();
    auto* mtx = GetAllocMutex();
    if (!set || !mtx) return;
    std::lock_guard<std::mutex> lock(*mtx);
    set->insert(ptr);
    TracyAlloc(ptr, size);
}

inline void TracyTrackFree(void* ptr) {
    if (!ptr) return;
    auto* set = GetAllocSet();
    auto* mtx = GetAllocMutex();
    if (!set || !mtx) return;
    std::lock_guard<std::mutex> lock(*mtx);
    if (set->erase(ptr) != 0) {
        TracyFree(ptr);
    }
}
}  // namespace

void* operator new(std::size_t count) {
    void* ptr = std::malloc(count);
    if (!ptr) throw std::bad_alloc();
    TracyTrackAlloc(ptr, count);
    return ptr;
}

void* operator new[](std::size_t count) {
    void* ptr = std::malloc(count);
    if (!ptr) throw std::bad_alloc();
    TracyTrackAlloc(ptr, count);
    return ptr;
}

void operator delete(void* ptr) noexcept {
    TracyTrackFree(ptr);
    std::free(ptr);
}

void operator delete[](void* ptr) noexcept {
    TracyTrackFree(ptr);
    std::free(ptr);
}

void operator delete(void* ptr, std::size_t) noexcept {
    TracyTrackFree(ptr);
    std::free(ptr);
}

void operator delete[](void* ptr, std::size_t) noexcept {
    TracyTrackFree(ptr);
    std::free(ptr);
}

void* operator new(std::size_t count, const std::nothrow_t&) noexcept {
    void* ptr = std::malloc(count);
    TracyTrackAlloc(ptr, count);
    return ptr;
}

void* operator new[](std::size_t count, const std::nothrow_t&) noexcept {
    void* ptr = std::malloc(count);
    TracyTrackAlloc(ptr, count);
    return ptr;
}

void operator delete(void* ptr, const std::nothrow_t&) noexcept {
    TracyTrackFree(ptr);
    std::free(ptr);
}

void operator delete[](void* ptr, const std::nothrow_t&) noexcept {
    TracyTrackFree(ptr);
    std::free(ptr);
}

void* operator new(std::size_t count, std::align_val_t align) {
    void* ptr = TracyAlignedAlloc(count, (std::size_t)align);
    if (!ptr) throw std::bad_alloc();
    TracyTrackAlloc(ptr, count);
    return ptr;
}

void* operator new[](std::size_t count, std::align_val_t align) {
    void* ptr = TracyAlignedAlloc(count, (std::size_t)align);
    if (!ptr) throw std::bad_alloc();
    TracyTrackAlloc(ptr, count);
    return ptr;
}

void* operator new(std::size_t count, std::align_val_t align,
                   const std::nothrow_t&) noexcept {
    void* ptr = TracyAlignedAlloc(count, (std::size_t)align);
    TracyTrackAlloc(ptr, count);
    return ptr;
}

void* operator new[](std::size_t count, std::align_val_t align,
                     const std::nothrow_t&) noexcept {
    void* ptr = TracyAlignedAlloc(count, (std::size_t)align);
    TracyTrackAlloc(ptr, count);
    return ptr;
}

void operator delete(void* ptr, std::align_val_t) noexcept {
    TracyTrackFree(ptr);
    TracyAlignedFree(ptr);
}

void operator delete[](void* ptr, std::align_val_t) noexcept {
    TracyTrackFree(ptr);
    TracyAlignedFree(ptr);
}

void operator delete(void* ptr, std::align_val_t, const std::nothrow_t&) noexcept {
    TracyTrackFree(ptr);
    TracyAlignedFree(ptr);
}

void operator delete[](void* ptr, std::align_val_t, const std::nothrow_t&) noexcept {
    TracyTrackFree(ptr);
    TracyAlignedFree(ptr);
}

void operator delete(void* ptr, std::size_t, std::align_val_t) noexcept {
    TracyTrackFree(ptr);
    TracyAlignedFree(ptr);
}

void operator delete[](void* ptr, std::size_t, std::align_val_t) noexcept {
    TracyTrackFree(ptr);
    TracyAlignedFree(ptr);
}
#endif