#include "ui/cws80_ui_nk.h"
#include "ui/detail/nk_essential.h"
#include "ui/detail/device/dev_graphics.h"
#include "utility/scope_guard.h"
#include "utility/dynarray.h"
#include "utility/debug.h"
#include <nuklear.h>
#include <fmt/format.h>
#include <memory>
#include <vector>
#include <thread>
#include <string.h>
#include <assert.h>

namespace cws80 {

struct NkScreen::Impl {
    nk_context ctx_{};
    dynarray<u8> ctxdata_;
    static constexpr uint ctxdatalen_ = 64 * 1024;

    GraphicsDevice *gdev_ = nullptr;
    uint w_ = 0;
    uint h_ = 0;

    bool init_ = false;
    std::thread::id ui_thread_id_;
    std::vector<u8> lastdata_;
    bool forced_redisplay_ = false;

#if !defined(NDEBUG)
    bool first_time_{};
#endif
};

//------------------------------------------------------------------------------
NkScreen::NkScreen()
    : P(new Impl)
{
    P->ctxdata_.reset(P->ctxdatalen_);
}

NkScreen::~NkScreen()
{
    if (!P->init_)
        return;

    if (std::this_thread::get_id() != P->ui_thread_id_) {
        debug("Error: caught cleanup attempt from outside of UI thread");
        return;
    }

    GraphicsDevice &gdev = *P->gdev_;
    gdev.cleanup();
}

void NkScreen::init(GraphicsDevice &gdev, uint w, uint h, gsl::span<const FontRequest> fontreqs)
{
    bool success = false;

    assert(!P->init_);
    if (P->init_)
        return;

    P->gdev_ = &gdev;
    P->w_ = w;
    P->h_ = h;
#if !defined(NDEBUG)
    P->first_time_ = true;
#endif

    gdev.initialize(fontreqs);
    SCOPE(exit)
    {
        if (!success)
            gdev.cleanup();
    };

    P->lastdata_.clear();
    P->forced_redisplay_ = true;

    nk_user_font *font = gdev.get_font(0);

    if (!nk_init_fixed(&P->ctx_, P->ctxdata_.data(), P->ctxdatalen_, font))
        throw std::runtime_error("could not initialize screen context");

    P->init_ = true;
    P->ui_thread_id_ = std::this_thread::get_id();
    success = true;
}

void NkScreen::clear()
{
    nk_context &ctx = P->ctx_;
    nk_clear(&ctx);
}

bool NkScreen::should_render() const
{
    if (P->forced_redisplay_)
        return true;

    nk_context &ctx = P->ctx_;
    const size_t size = ctx.memory.allocated;
    auto *cmdbuf = (const u8 *)nk_buffer_memory_const(&ctx.memory);

    //
    std::vector<u8> &lastdata = P->lastdata_;
    bool has_changes = size != lastdata.size() ||
                       std::memcmp(lastdata.data(), cmdbuf, size) != 0;
    return has_changes;
}

void NkScreen::render(void *draw_context)
{
    nk_context &ctx = P->ctx_;
    const size_t size = ctx.memory.allocated;
    auto *cmdbuf = (const u8 *)nk_buffer_memory_const(&ctx.memory);

    // //
    std::vector<u8> &lastdata = P->lastdata_;
    lastdata.assign(cmdbuf, cmdbuf + size);

    //
#if !defined(NDEBUG)
    if (P->first_time_ && size > 0) {
        debug("Context memory {} before first rendering", size);
        P->first_time_ = false;
    }
#endif

    //
    GraphicsDevice &gdev = *P->gdev_;
    gdev.render(draw_context);
    P->forced_redisplay_ = false;
}

void NkScreen::update()
{
    P->forced_redisplay_ = true;
}

nk_context *NkScreen::context() const
{
    return &P->ctx_;
}

nk_user_font *NkScreen::font(uint id) const
{
    return P->gdev_->get_font(id);
}

uint NkScreen::width() const
{
    return P->w_;
}

uint NkScreen::height() const
{
    return P->h_;
}

}  // namespace cws80
