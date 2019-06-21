#pragma once
#include "cws/cws80_program.h"
#include "utility/types.h"
#include <assert.h>

namespace cws80 {

static constexpr char CWS80__Notification[] = "urn:jpcima:cws80#Notification";
static constexpr char CWS80__Request[] = "urn:jpcima:cws80#Request";

//------------------------------------------------------------------------------
#define EACH_NOTIFICATION_TYPE(F) F(Bank) F(Program) F(Write)

enum class NotificationType {
#define EACH(x) x,
    EACH_NOTIFICATION_TYPE(EACH)
#undef EACH
};

namespace Notification {

    struct T {
        NotificationType type{};
    };

    struct Bank : T {
        Bank() { type = NotificationType::Bank; }
        u32 num;
        cws80::Bank data;
    };

    struct Program : T {
        Program() { type = NotificationType::Program; }
        u32 bank;
        u32 prog;
        cws80::Program data;
    };

    struct Write : T {
        Write() { type = NotificationType::Write; }
    };

}  // namespace Notification

struct NotificationTraits {
    explicit constexpr NotificationTraits(NotificationType type)
        : type(type)
    {
    }
    const NotificationType type;
    constexpr size_t size() const;
    static constexpr size_t max_size();
    static Notification::T *clone(const Notification::T &ntf);
    static void free(const Notification::T *req);
};

//------------------------------------------------------------------------------
#define EACH_REQUEST_TYPE(F)                                               \
    F(SetProgram)                                                          \
    F(SetBank) F(LoadBank) F(RenameProgram) F(InitProgram) F(WriteProgram) \
        F(SetParameter) F(GetBankData) F(NoteOn) F(NoteOff)

enum class RequestType {
#define EACH(x) x,
    EACH_REQUEST_TYPE(EACH)
#undef EACH
};

namespace Request {

    struct T {
        RequestType type{};
    };

    struct SetProgram : T {
        SetProgram() { type = RequestType::SetProgram; }
        u32 prog;
    };

    struct SetBank : T {
        SetBank() { type = RequestType::SetBank; }
        u32 bank;
    };

    struct LoadBank : T {
        LoadBank() { type = RequestType::LoadBank; }
        Bank data;
    };

    struct RenameProgram : T {
        RenameProgram() { type = RequestType::RenameProgram; }
        char name[6];
    };

    struct InitProgram : T {
        InitProgram() { type = RequestType::InitProgram; }
    };

    struct WriteProgram : T {
        WriteProgram() { type = RequestType::WriteProgram; }
    };

    struct SetParameter : T {
        SetParameter() { type = RequestType::SetParameter; }
        uint index;
        i32 value;
    };

    struct GetBankData : T {
        GetBankData() { type = RequestType::GetBankData; }
        u32 bank;
    };

    struct NoteOn : T {
        NoteOn() { type = RequestType::NoteOn; }
        u8 key;
        u8 velocity;
    };

    struct NoteOff : T {
        NoteOff() { type = RequestType::NoteOff; }
        u8 key;
        u8 velocity;
    };

}  // namespace Request

struct RequestTraits {
    explicit constexpr RequestTraits(RequestType type)
        : type(type)
    {
    }
    const RequestType type;
    constexpr size_t size() const;
    static constexpr size_t max_size();
    static Request::T *clone(const Request::T &req);
    static void free(const Request::T *req);
};

//------------------------------------------------------------------------------
constexpr size_t NotificationTraits::size() const
{
    switch (type) {
#define EACH(x)               \
    case NotificationType::x: \
        return sizeof(Notification::x);
        EACH_NOTIFICATION_TYPE(EACH)
#undef EACH
    default:
        return sizeof(Notification::T);
    }
}

constexpr size_t NotificationTraits::max_size()
{
    size_t size = 0;
#define EACH(x) \
    size = (size > sizeof(Notification::x)) ? size : sizeof(Notification::x);
    EACH_NOTIFICATION_TYPE(EACH)
#undef EACH
    return size;
}

inline Notification::T *NotificationTraits::clone(const Notification::T &ntf)
{
    switch (ntf.type) {
#define EACH(x)               \
    case NotificationType::x: \
        return new Notification::x(static_cast<const Notification::x &>(ntf));
        EACH_NOTIFICATION_TYPE(EACH)
    default:
        assert(false);
        return {};
#undef EACH
    }
}

inline void NotificationTraits::free(const Notification::T *ntf)
{
    if (!ntf)
        return;

    switch (ntf->type) {
#define EACH(x)               \
    case NotificationType::x: \
        delete static_cast<const Notification::x *>(ntf); break;
        EACH_NOTIFICATION_TYPE(EACH)
    default:
        assert(false); break;
#undef EACH
    }
}

//------------------------------------------------------------------------------
constexpr size_t RequestTraits::size() const
{
    switch (type) {
#define EACH(x)          \
    case RequestType::x: \
        return sizeof(Request::x);
        EACH_REQUEST_TYPE(EACH)
#undef EACH
    default:
        return sizeof(Request::T);
    }
}

constexpr size_t RequestTraits::max_size()
{
    size_t size = 0;
#define EACH(x) size = (size > sizeof(Request::x)) ? size : sizeof(Request::x);
    EACH_REQUEST_TYPE(EACH)
#undef EACH
    return size;
}

inline Request::T *RequestTraits::clone(const Request::T &req)
{
    switch (req.type) {
#define EACH(x)          \
    case RequestType::x: \
        return new Request::x(static_cast<const Request::x &>(req));
        EACH_REQUEST_TYPE(EACH)
    default:
        assert(false);
        return {};
#undef EACH
    }
}

inline void RequestTraits::free(const Request::T *req)
{
    if (!req)
        return;

    switch (req->type) {
#define EACH(x)               \
    case RequestType::x: \
        delete static_cast<const Request::x *>(req); break;
        EACH_REQUEST_TYPE(EACH)
    default:
        assert(false); break;
#undef EACH
    }
}

}  // namespace cws80
