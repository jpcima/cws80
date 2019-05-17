#pragma once
#include "utility/types.h"
#include <array>

namespace cws80 {

// Stereo pan law Q16,16 (-3dB at the center)
//  (cos(r)+sin(r))/sqrt(2), where r=(pi/4)*((2*n/(N-1))-1)
extern const std::array<u32, 511> Pan_table;

// time values for the T1-T4 parameters
extern const std::array<f32, 64> Env_times;

// LFO frequency table (approx with data from Transoniq Hacker #26)
//    a1 * x if x < 8, a2 * (x - 6) otherwise
//    a1=0.04347471496598639 a2=0.31470527365073292
extern const std::array<f32, 64> Lfo_freqs;

// LFO delay table (low quality approx by ear)
//    a*exp(b*x) + c*exp(d*x)
//    a=10.87 b=-0.4496 c=1.724 d=-0.03158
extern const std::array<f32, 64> Lfo_delays;

// antialias 4X filter Q32,32
//  b = firceqrip(63, 0.25, [0.001 10.^(-80./20.)])
//  floor((b / sum(b)) * 2^32)
extern const std::array<i32, 64> Sat_aa4x;

// antialias 4X filter F32
//  b = firceqrip(63, 0.25, [0.001 10.^(-80./20.)])
// normalized for sum < 1
extern const std::array<f32, 64> Sat_aa4x_real;

// VCF frequency table (approx from spectral analysis)
//    a*exp(b*x) with a=48.87, b=0.05792
extern const std::array<f32, 128> Vcf_freqs;

};  // namespace cws80
