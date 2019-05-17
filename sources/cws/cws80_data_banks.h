#pragma once
#include "utility/types.h"
#include <gsl/gsl>
#include <array>

namespace cws80 {

extern std::array<gsl::span<const u8>, 11> extra_bank_data;

}  // namespace cws80
