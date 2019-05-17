#pragma once
#include "cws/cws80_messages.h"
#include "utility/types.h"

namespace cws80 {

class UIMaster {
public:
    virtual ~UIMaster() {}
    virtual void emit_request(const Request::T &req) = 0;
    virtual void set_parameter_automated(uint idx, i32 val) = 0;
    virtual void begin_edit(uint idx) = 0;
    virtual void end_edit(uint idx) = 0;
};

}  // namespace cws80
