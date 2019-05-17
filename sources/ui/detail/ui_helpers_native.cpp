#include "ui/detail/ui_helpers_native.h"
#include "ui/detail/ui_helpers_tk.h"

namespace cws80 {


NativeUI *NativeUI::create()
{
    return new TkUI;
}

}  // namespace cws80
