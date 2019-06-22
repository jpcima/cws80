#pragma once
#include "attributes.h"
#include <algorithm>
#include <memory>
#include <cstddef>

template <class T> class dynarray : private std::unique_ptr<T[]> {
public:
    typedef T value_type;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef T &reference;
    typedef const T &const_reference;
    typedef T *pointer;
    typedef const T *const_pointer;
    typedef T *iterator;
    typedef const T *const_iterator;
    typedef std::reverse_iterator<T *> reverse_iterator;
    typedef std::reverse_iterator<const T *> const_reverse_iterator;

    ForceInline dynarray() noexcept {}
    ForceInline explicit dynarray(size_t size) { reset(size); }
    template <class I> ForceInline dynarray(I first, I last)
    {
        assign(first, last);
    }
    ForceInline explicit dynarray(std::initializer_list<T> ilist)
    {
        assign(ilist);
    }
    ForceInline dynarray(size_t count, const T &value)
    {
        assign(count, value);
    }

    void reset(size_t size)
    {
        std::unique_ptr<T[]>::reset(new T[size]{});
        size_ = size;
    }
    void swap(dynarray &other) noexcept;

    template <class I> void assign(I first, I last);
    ForceInline void assign(std::initializer_list<T> ilist)
    {
        assign(ilist.begin(), ilist.end());
    }
    void assign(size_t count, const T &value);

    ForceInline T *data() const noexcept { return this->get(); }
    ForceInline size_t size() const noexcept { return size_; }
    ForceInline bool empty() const noexcept { return size_ == 0; }

    using std::unique_ptr<T[]>::operator[];
    using std::unique_ptr<T[]>::operator bool;

    T &at(size_t pos);
    const T &at(size_t pos) const;

    ForceInline T &front() noexcept { return (*this)[0]; }
    ForceInline const T &front() const noexcept { return (*this)[0]; }
    ForceInline T &back() noexcept { return (*this)[size_ - 1]; }
    ForceInline const T &back() const noexcept
    {
        return (*this)[size_ - 1];
    }

    ForceInline T *begin() noexcept { return data(); }
    ForceInline const T *begin() const noexcept { return data(); }
    ForceInline const T *cbegin() const noexcept { return data(); }
    ForceInline T *end() noexcept { return data() + size_; }
    ForceInline const T *end() const noexcept { return data() + size_; }
    ForceInline const T *cend() const noexcept { return data() + size_; }

    dynarray(dynarray &&other) noexcept = default;
    dynarray &operator=(dynarray &&other) noexcept = default;

    dynarray(const dynarray &other) { assign(other.begin(), other.end()); }
    dynarray &operator=(const dynarray &other)
    {
        assign(other.begin(), other.end());
        return *this;
    }

    bool operator==(const dynarray &other)
    {
        return std::equal(begin(), end(), other.begin(), other.end());
    }
    ForceInline bool operator!=(const dynarray &other)
    {
        return !operator==(other);
    }

private:
    size_t size_ = 0;
};

template <class T> void dynarray<T>::swap(dynarray &other) noexcept
{
    std::unique_ptr<T>::swap(other);
    std::swap(size_, other.size_);
}

template <class T> template <class I> void dynarray<T>::assign(I first, I last)
{
    size_t n = last - first;
    if (n != size_) {
        std::unique_ptr<T[]>::reset(new T[n]);
        size_ = n;
    }
    std::copy_n(first, n, data());
}

template <class T> void dynarray<T>::assign(size_t count, const T &value)
{
    if (count != size_) {
        std::unique_ptr<T[]>::reset(new T[count]);
        size_ = count;
    }
    std::fill_n(data(), count, value);
}

template <class T> T &dynarray<T>::at(size_t pos)
{
    if (pos >= size_)
        throw std::out_of_range("dynarray<T>::at");
    return (*this)[pos];
}

template <class T> const T &dynarray<T>::at(size_t pos) const
{
    if (pos >= size_)
        throw std::out_of_range("dynarray<T>::at");
    return (*this)[pos];
}

namespace std {
template <class T> void swap(dynarray<T> &lhs, dynarray<T> &rhs) noexcept
{
    lhs.swap(rhs);
}
}  // namespace std
