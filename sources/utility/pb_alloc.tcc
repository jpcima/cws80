#include "utility/pb_alloc.h"

template <uint Log2Al>
pb_alloc<Log2Al>::pb_alloc(pb_alloc &&o) noexcept
    : data_(std::move(o.data_)), top_(o.top_), cap_(o.cap_) {
  o.top_ = o.cap_ = 0;
}

template <uint Log2Al>
auto pb_alloc<Log2Al>::operator=(pb_alloc &&o) noexcept -> pb_alloc & {
  data_ = std::move(o.data_);
  top_ = o.top_;
  cap_  = o.cap_;
  o.top_ = o.cap_ = 0;
  return *this;
}

template <uint Log2Al>
void *pb_alloc<Log2Al>::alloc(size_t size) noexcept {
  size_t totalsize = align_size(size + sizeof(descriptor));
  if (totalsize > cap_ - top_)
    return nullptr;
  u8 *block = &data_[top_];
  top_ += totalsize;
  descriptor *desc = (descriptor *)(block + totalsize - sizeof(descriptor));
  desc->size = size;
#ifndef NDEBUG
  desc->magic = descriptor::block_magic;
#endif
  return block;
}

template <uint Log2Al>
void *pb_alloc<Log2Al>::unchecked_alloc(size_t size) noexcept {
  size_t totalsize = align_size(size + sizeof(descriptor));
  assert(totalsize <= cap_ - top_);
  u8 *block = &data_[top_];
  top_ += totalsize;
  descriptor *desc = (descriptor *)(block + totalsize - sizeof(descriptor));
  desc->size = size;
#ifndef NDEBUG
  desc->magic = descriptor::block_magic;
#endif
  return block;
}

template <uint Log2Al>
void pb_alloc<Log2Al>::free(const void *ptr) noexcept {
  assert(ptr);
  (void)ptr;
  const u8 *ptop = &data_[top_];
  const descriptor *desc = (const descriptor *)(ptop - sizeof(descriptor));
#ifndef NDEBUG
  assert(desc->magic == descriptor::block_magic);
  desc->magic = 0;
#endif
  size_t totalsize = align_size(desc->size + sizeof(descriptor));
#ifndef NDEBUG
  const void *plast = ptop - totalsize;
  assert(ptr == plast);
#endif
  top_ -= totalsize;
}
