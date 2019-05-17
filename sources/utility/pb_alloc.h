#pragma once
#include "utility/types.h"
#include <memory>
#include <utility>
#include <assert.h>

#if defined(__GNUC__)
#define PB_ALLOC_ATTRIBUTE_MALLOC [[gnu::malloc]]
#else
#define PB_ALLOC_ATTRIBUTE_MALLOC
#endif

///
template <uint Log2Al = 4> class pb_alloc {
public:
    pb_alloc() {}
    explicit pb_alloc(size_t cap)
        : data_(new u8[cap])
        , cap_(cap)
    {
    }
    pb_alloc(pb_alloc &&o) noexcept;
    pb_alloc &operator=(pb_alloc &&o) noexcept;

private:
    struct descriptor {
        size_t size;
#ifndef NDEBUG
        mutable u32 magic;
        static constexpr u32 block_magic = 0x12345678u;
#endif
    };

    static constexpr size_t alignment = 1u << Log2Al;
    static constexpr size_t mask = alignment - 1;
    static size_t align_size(size_t size) { return (size + mask) & ~mask; }

public:
    size_t allocated() const { return top_; }
    size_t capacity() const { return cap_; }
    void clear() noexcept { top_ = 0; }
    bool empty() const noexcept { return top_ == 0; }
    PB_ALLOC_ATTRIBUTE_MALLOC void *alloc(size_t size) noexcept;
    PB_ALLOC_ATTRIBUTE_MALLOC void *unchecked_alloc(size_t size) noexcept;
    void free(const void *ptr) noexcept;

private:
    std::unique_ptr<u8[]> data_;
    size_t top_ = 0, cap_ = 0;
};

#undef PB_ALLOC_ATTRIBUTE_MALLOC

#include "pb_alloc.tcc"
