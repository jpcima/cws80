#pragma once
#include "utility/arithmetic.h"
#include "utility/attributes.h"
#include "utility/types.h"
#include <array>
#include <cmath>

namespace dsp {
namespace lpcfmoog {

    enum non_linearity { nl_tanh, nl_fast_tanh };
    enum tuning { tun_direct, tun_table };

    template <class Pcy> class filter {
    public:
        void lp(f64 f, f64 q);
        f64 tick(f64 in);
        void run(const f64 *in, f64 *out, uint n);
        void reset();

    private:
        f64 g_ = 0;
        f64 q_ = 0;
        f64 fbdelay_ = 0;
        struct moogstage {
            f64 m1_ = 0;
            f64 m2_ = 0;
        };
        std::array<moogstage, 4> stage_{};
    };

    //------------------------------------------------------------------------------
    struct nice_policy {
        static constexpr non_linearity nl = nl_tanh;
        static constexpr tuning tun = tun_direct;
        static constexpr uint over = 3;
    };
    struct fast_policy {
        static constexpr non_linearity nl = nl_fast_tanh;
        static constexpr tuning tun = tun_table;
        static constexpr uint over = 2;
    };

    template <class Policy> class filter;
    typedef filter<fast_policy> fast_filter;
    typedef filter<nice_policy> nice_filter;

    //------------------------------------------------------------------------------
    namespace detail {
        template <tuning> struct tuning_traits {
            static f64 compute_g(f64 f);
        };
        static f64 correction(f64 f, f64 q);
        template <non_linearity> struct non_linearity_traits {
            static f64 saturate(f64 x);
        };
        template <size_t n>
        static std::array<f32, n + 1> compute_frequency_table();
        template <size_t n>
        static const std::array<f32, n + 1> ftable_ = compute_frequency_table<n>();
    }  // namespace detail

    //------------------------------------------------------------------------------
    namespace detail {
        template <>
        ForceInline f64 detail::tuning_traits<tun_direct>::compute_g(f64 f)
        {
            return 1 - std::exp(-2 * M_PI * f);
        }

        template <>
        ForceInline f64 detail::tuning_traits<tun_table>::compute_g(f64 f)
        {
            constexpr size_t ftabsize = 256;
            const std::array<f32, ftabsize + 1> &ftable = ftable_<ftabsize>;
            f64 index = f * (2 * (ftabsize - 1));
            index = clamp<f64>(index, 0, ftabsize - 1);
            uint i0 = (uint)index;
            f32 ft[2] = {ftable[i0], ftable[i0 + 1]};
            return ft[0] + (index - i0) * (ft[1] - ft[0]);
        }

        static ForceInline f64 correction(f64 f, f64 q)
        {
            const f64 p00 = 47.7075588396789e-003, p10 = 1.58879917317302e+000,
                      p01 = -203.211294022804e-003, p20 = 1.58534782723064e+000,
                      p11 = -938.412964110806e-003, p02 = 214.871420454546e-003;
            return p00 + p10 * f + p01 * q + p20 * f * f + p11 * f * q + p02 * q * q;
        }
    }  // namespace detail

    template <class Pcy> inline void filter<Pcy>::lp(f64 f, f64 q)
    {
        typedef detail::tuning_traits<Pcy::tun> tun_traits;
        g_ = tun_traits::compute_g(detail::correction(f, q) / Pcy::over);
        q_ = q;
    }

    namespace detail {
        template <>
        ForceInline f64 non_linearity_traits<nl_tanh>::saturate(f64 x)
        {
            return std::tanh(x);
        }

        template <>
        ForceInline f64 non_linearity_traits<nl_fast_tanh>::saturate(f64 xx)
        {
            f32 x = (f32)xx;
            f32 x2 = x * x;
            f32 a = x * (135135.0f + x2 * (17325.0f + x2 * (378.0f + x2)));
            f32 b = 135135.0f + x2 * (62370.0f + x2 * (3150.0f + x2 * 28.0f));
            return a / b;
        }
    }  // namespace detail

    template <class Pcy> inline f64 filter<Pcy>::tick(f64 in)
    {
        typedef detail::non_linearity_traits<Pcy::nl> nl_traits;

        const f64 g = g_;
        const f64 q = q_;
        std::array<moogstage, 4> stage = stage_;
        f64 fbdelay = fbdelay_;

        constexpr f64 comp = 0.5;
        constexpr f64 coefs[5] = {0, 0, 0, 0, 1};

        // in -= - q * (4 * nl_traits::saturate(fbdelay) - in * comp);
        in -= (nl_traits::saturate(fbdelay) - in * comp) * q * 4;

        f64 y[Pcy::over];
        for (unsigned o = 0; o < Pcy::over; ++o) {
            f64 stagein = in;
            std::array<f64, 4> stageout;
            for (unsigned i = 0; i < 4; ++i) {
                moogstage &st = stage[i];
                f64 m1 = st.m1_;
                f64 m2 = st.m2_;
                f64 out = m2 + g * (stagein * (1.0 / 1.3) + m1 * (0.3 / 1.3) - m2);
                stageout[i] = st.m2_ = out;
                st.m1_ = stagein;
                stagein = out;
            }
            f64 out = in * coefs[0];
            for (unsigned i = 0; i < 4; ++i) {
                fbdelay = stageout[i];
                out += fbdelay * coefs[i + 1];
            }
            y[o] = out;
        }

        stage_ = stage;
        fbdelay_ = fbdelay;
        return y[0];
    }

    template <class Pcy> void filter<Pcy>::run(const f64 *in, f64 *out, uint n)
    {
        for (uint i = 0; i < n; ++i)
            out[i] = tick(in[i]);
    }

    template <class Pcy> void filter<Pcy>::reset()
    {
        fbdelay_ = 0;
        stage_ = {};
    }

    namespace detail {
        template <size_t n> std::array<f32, n + 1> compute_frequency_table()
        {
            std::array<f32, n + 1> ft{};
            for (uint i = 0; i < n; ++i) {
                f64 f = 0.5 * i / (n - 1);
                ft[i] = (f32)(1 - std::exp(-2 * M_PI * f));
            }
            for (uint i = n; i < ft.size(); ++i)
                ft[i] = ft[n - 1];
            return ft;
        }
    }  // namespace detail

}  // namespace lpcfmoog
}  // namespace dsp
