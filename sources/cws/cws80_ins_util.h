#pragma once
#include "utility/types.h"
#include <memory>
#include <algorithm>

namespace cws80 {

template <class T> class basic_mod_buffer {
    std::unique_ptr<T[]> buf_;
    uint fli_ = 0;

public:
    basic_mod_buffer() {}

    // set the capacity of the buffer
    explicit basic_mod_buffer(uint size) { buf_.reset(new T[size]()); }

    // reinitialize the buffer
    void clear(const T &val = {})
    {
        buf_[0] = val;
        fli_ = 0;
    }

    // make the buffer's last element the first and reset the fill index
    void cycle()
    {
        T *buf = buf_.get();
        uint fli = fli_;
        buf[0] = buf[fli];
        fli_ = 0;
    }

    // fill the buffer with its last value up to pos included,
    //  and update the fill index
    void repeat_upto(uint pos)
    {
        T *buf = buf_.get();
        uint fli = fli_;
        std::fill(buf + fli, buf + pos + 1, buf[fli]);
        fli_ = pos;
    }

    // fill the entire buffer with the given value, and update the fill index
    void fill_entire(const T &val, uint size)
    {
        T *buf = buf_.get();
        std::fill(buf, buf + size, val);
        fli_ = size - 1;
    }

    // fill the buffer with its last value up to pos, assign value to pos,
    //  and update the fill index
    void append(uint pos, const T &val)
    {
        T *buf = buf_.get();
        uint fli = fli_;
        std::fill(buf + fli, buf + pos, buf[fli]);
        buf[pos] = val;
        fli_ = pos;
    }

    //
    const T *for_input(uint size)
    {
        repeat_upto(size - 1);
        return buf_.get();
    }

    //
    T *for_output(uint size)
    {
        fli_ = size - 1;
        return buf_.get();
    }
};

}  // namespace cws80
