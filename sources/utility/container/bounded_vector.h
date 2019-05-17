/*
  bounded_vector: array-like container of variable size and fixed capacity
    arrays bounds are not checked
*/

#pragma once
#include <algorithm>
#include <utility>
#include <stddef.h>

template <class T, size_t N> class bounded_vector {
    typedef T value_type;
    typedef T &reference;
    typedef const T &const_reference;
    typedef T *pointer;
    typedef const T *const_pointer;
    typedef T *iterator;
    typedef const T *const_iterator;
    typedef ptrdiff_t difference_type;
    typedef size_t size_type;

private:
    T data_[N];
    size_t length_ = 0;

public:
    constexpr bounded_vector() {}

    bounded_vector(const bounded_vector &) = default;
    bounded_vector(bounded_vector &&) = default;
    bounded_vector &operator=(const bounded_vector &) = default;
    bounded_vector &operator=(bounded_vector &&) = default;

    size_t size() const { return length_; }
    static size_t capacity() { return N; }

    const T *data() const { return data_; }
    T *data() { return data_; }

    bool empty() const { return length_ == 0; }

    void resize(size_t n)
    {
        for (size_t i1 = n, i2 = length_; i1 < i2; ++i1)
            data_[i1] = T();
        length_ = n;
    }
    void clear() { resize(0); }
    void push_back(T &&x) { data_[length_++] = std::forward<T>(x); }
    void push_back(const T &x) { data_[length_++] = x; }
    void pop_back() { data_[--length_] = T(); }

    T &back() { return data_[length_ - 1]; }
    const T &back() const { return data_[length_ - 1]; }
    T &front() { return data_[0]; }
    const T &front() const { return data_[0]; }

    void erase(T *pos)
    {
        std::move(pos + 1, &data_[length_], pos);
        data_[--length_] = T();
    }
    void erase(const T *pos) { erase(const_cast<T *>(pos)); }
    void insert(T *pos, T &&val)
    {
        std::move(pos, &data_[length_], pos + 1);
        *pos = std::forward<T>(val);
        ++length_;
    }
    void insert(T *pos, const T &val)
    {
        std::move(pos, &data_[length_], pos + 1);
        *pos = val;
        ++length_;
    }
    void insert(const T *pos, T &&val)
    {
        insert(const_cast<T *>(pos), std::forward<T>(val));
    }
    void insert(const T *pos, const T &val)
    {
        insert(const_cast<T *>(pos), val);
    }

    const T *cbegin() const { return data_; }
    const T *begin() const { return data_; }
    T *begin() { return data_; }

    const T *cend() const { return &data_[length_]; }
    const T *end() const { return &data_[length_]; }
    T *end() { return &data_[length_]; }

    T &operator[](size_t i) { return data_[i]; }
    const T &operator[](size_t i) const { return data_[i]; }

    bool operator==(const bounded_vector &b) const
    {
        return length_ == b.length_ &&
               std::equal(data_, &data_[length_], b.data_, &b.data_[length_]);
    }
    bool operator!=(const bounded_vector &b) const { return !operator==(b); }
};
