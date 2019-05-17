#pragma once
#include "cws/cws80_messages.h"

namespace cws80 {

class FxMaster {
public:
    virtual ~FxMaster() {}
    virtual bool emit_notification(const Notification::T &ntf) = 0;
};

}  // namespace cws80
