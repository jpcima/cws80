#include "cws80_data_plot.h"
#include "utility/dynarray.h"
#include "utility/scope_guard.h"
#include "utility/c++std/string_view.h"
#include <memory>

namespace cws80 {

static std::string escape(cxx::string_view str)
{
    size_t len = str.size();
    std::string result;
    result.reserve(len * 2);
    for (size_t i = 0; i < len; ++i) {
        char buf[8];
        uint nchars = sprintf(buf, "\\%.3o", (u8)str[i]);
        result.append(buf, nchars);
    }
    return result;
}

bool plot_waves(const Wave waves[], const char *titles[], uint count)
{
    FILE *gp = popen("gnuplot -d", "w");
    if (!gp)
        return false;
    SCOPE(exit) { pclose(gp); };

    fputs("set terminal wxt size 1024,480\n", gp);
    fprintf(gp, "set xrange [0:1]\n");

    fputs("plot", gp);
    for (uint i = 0; i < count; ++i)
        fprintf(gp, "%s '-' using 1:2 title \"%s\" with lines",
                (i == 0) ? "" : ",", escape(titles[i]).c_str());
    fputc('\n', gp);

    for (uint i = 0; i < count; ++i) {
        Sample sample = wave_sample(waves[i]);
        for (uint i = 0, n = sample.length(); i < n; ++i)
            fprintf(gp, "%f %d\n", i / (f64)(n - 1), (int)sample.data[i] - 128);
        fputs("e\n", gp);
    }
    fputs("pause mouse close\n", gp);
    fflush(gp);

    return true;
}

bool plot_waveset(Waveset waveset)
{
    Wave waves[16];
    dynarray<const char *> names(16);
    dynarray<char> namebuf(16 * 16);
    for (uint i = 0; i < 16; ++i) {
        uint id = waveset.wavenum[i];
        waves[i] = wave_by_id(id);
        names[i] = wave_name(id, &namebuf[i * 16]);
    }
    return plot_waves(waves, names.data(), 16);
}

}  // namespace cws80
