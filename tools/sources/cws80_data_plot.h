#pragma once
#include "cws/cws80_data.h"
#include "utility/types.h"

namespace cws80 {

bool plot_waves(const Wave waves[], const char *titles[], uint count);
bool plot_waveset(Waveset waveset);

}  // namespace cws80
