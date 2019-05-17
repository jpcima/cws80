#include "plots.h"
#include "cws/component/lfo.h"
#include "utility/arithmetic.h"
#include "utility/types.h"
#include <boost/lexical_cast.hpp>
#include <getopt.h>
#include <memory>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace cws80;

Lfo::Param Lfop;
f64 FS = 44100;
std::vector<f64> RT;  // reset times
f64 D = 1;  // duration
uint B = 64;  // block size
bool Plot = false;

static void parse_level_params(const char *arg);
static void process();

//
static const char usage[] =
    "Usage: test-lfo [options]\n"
    "   -f <sample-rate>           Set the sample rate\n"
    "   -l <l1,l2>                 Set the level parameters (in 0..63)\n"
    "   -w <wave>                  Set the waveform (0-3=TRI,SAW,SQR,NOI)\n"
    "   -d <duration>              Set the duration (in s)\n"
    "   -b <block-size>            Set the block size\n"
    "   -R <time>                  Reset at the given time\n"
    "   -p                         Plot the results\n";

int main(int argc, char *argv[])
{
    Lfop.FREQ = Lfo::freqidx(3.0);
    Lfop.WAV = (uint)LfoWave::TRI;
    Lfop.L1 = 0;
    Lfop.L2 = 63;
    Lfop.DELAY = 63;
    Lfop.HUMAN = false;

    for (int c; (c = getopt(argc, argv, "hf:l:w:d:b:R:p")) != -1;) {
        switch (c) {
        case 'h':
            fputs(usage, stderr);
            return 1;
        case 'f':
            FS = boost::lexical_cast<f64>(optarg);
            break;
        case 'l':
            parse_level_params(optarg);
            break;
        case 'w': {
            uint wav = boost::lexical_cast<uint>(optarg);
            if (wav >= 4)
                throw std::logic_error("invalid waveform parameter");
            Lfop.WAV = wav;
            break;
        }
        case 'd':
            D = boost::lexical_cast<f64>(optarg);
            if (D <= 0)
                throw std::logic_error("invalid duration parameter");
            break;
        case 'b':
            B = boost::lexical_cast<uint>(optarg);
            if (B <= 0)
                throw std::logic_error("invalid block size parameter");
            break;
        case 'p':
            Plot = true;
            break;
        case 'R':
            RT.push_back(boost::lexical_cast<f64>(optarg));
            break;
        default:
            return 1;
        }
    }

    if (argc != optind)
        exit(1);

    process();
    return 0;
}

static void process()
{
    f64 fs = FS;
    uint nsamples = (uint)ceil(D * fs);
    std::unique_ptr<i8[]> samples(new i8[nsamples]);

    Lfo lfo;
    lfo.setparam(&Lfop);
    lfo.initialize(fs, B, nullptr);

    std::sort(RT.begin(), RT.end());
    uint nT = 0;

    std::unique_ptr<i8[]> modulation(new i8[B]);
    for (uint i = 0; i < B; ++i) {
        modulation[i] = 0;  // TODO modulation?
    }

    for (uint i = 0; i < nsamples;) {
        uint bs = std::min(B, nsamples - i);
        lfo.generate(&samples[i], modulation.get(), bs);
        i += bs;
        // resetting
        f64 t = (f64)i / fs;
        for (; nT < RT.size() && t >= RT[nT]; ++nT) {
            lfo.reset();
        }
    }

    if (Plot) {
        FILE *gp = popen("gnuplot -d", "w");
        if (!gp)
            fprintf(stderr, "Cannot run gnuplot.\n");
        else {
            SCOPE(exit) { pclose(gp); };

            const char title[] = "LFO";

            fputs("set terminal wxt size 1024,480\n", gp);

            fprintf(gp, "set xrange [0:%f]\n", (f64)(nsamples - 1) / fs);

            for (f64 t : RT)
                fprintf(gp, "set arrow from %.17e,graph(0,0)"
                        " to %.17e,graph(1,1) nohead lc rgb 'red'\n", t, t);

            fprintf(gp, "plot '-' using 1:2 title \"%s\"\n", gp_escape(title).c_str());
            for (uint i = 0; i < nsamples; ++i) {
                f64 t = (f64)i / fs;
                fprintf(gp, "%.17e %d\n", t, samples[i]);
            }
            fputs("e\n", gp);
            fputs("pause mouse close\n", gp);
            fflush(gp);
        }
    }
    else {
        for (uint i = 0; i < nsamples; ++i) {
            f64 t = (f64)i / fs;
            printf("%f %d\n", t, samples[i]);
        }
    }
}

static void parse_level_params(const char *arg)
{
    size_t arglen = strlen(arg);
    uint n;

    uint L1, L2;
    if (sscanf(arg, "%u,%u%n", &L1, &L2, (int *)&n) != 2 || n != arglen ||
        Lfop.L1 >= 64 || Lfop.L2 >= 64)
        throw std::logic_error("invalid level parameters");

    Lfop.L1 = L1;
    Lfop.L2 = L2;
}
