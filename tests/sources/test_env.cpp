#include "plots.h"
#include "cws/component/env.h"
#include "utility/arithmetic.h"
#include "utility/types.h"
#include <boost/lexical_cast.hpp>
#include <getopt.h>
#include <memory>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace cws80;

Env::Param Envp;
f64 FS = 44100;
std::vector<f64> RT;  // retrigger times
std::vector<f64> RR;  // release times
f64 D = 0.5;  // duration
uint V = 100;  // velocity
uint B = 64;  // block size
bool Plot = false;

static void parse_time_params(const char *arg);
static void parse_level_params(const char *arg);
static void process();

//
static const char usage[] =
    "Usage: test-env [options]\n"
    "   -f <sample-rate>           Set the sample rate\n"
    "   -t <t1,t2,t3,t4>           Set the time parameters (in s)\n"
    "   -l <l1,l2,l3>              Set the level parameters (in -63..63)\n"
    "   -d <duration>              Set the duration (in s)\n"
    "   -v <velocity>              Set the velocity (1..127)\n"
    "   -b <block-size>            Set the block size\n"
    "   -T <time>                  Retrigger at the given time\n"
    "   -R <time>                  Release at the given time\n"
    "   -p                         Plot the results\n";

int main(int argc, char *argv[])
{
    Envp.T1 = Env::timeidx(0.04);
    Envp.T2 = Env::timeidx(0.20);
    Envp.T3 = Env::timeidx(0.14);
    Envp.T4 = Env::timeidx(0.08);
    Envp.L1 = 63;
    Envp.L2 = -32;
    Envp.L3 = 24;

    for (int c; (c = getopt(argc, argv, "hf:t:l:d:v:b:T:R:p")) != -1;) {
        switch (c) {
        case 'h':
            fputs(usage, stderr);
            return 1;
        case 'f':
            FS = boost::lexical_cast<f64>(optarg);
            break;
        case 't':
            parse_time_params(optarg);
            break;
        case 'l':
            parse_level_params(optarg);
            break;
        case 'd':
            D = boost::lexical_cast<f64>(optarg);
            if (D <= 0)
                throw std::logic_error("invalid duration parameter");
            break;
        case 'v':
            V = boost::lexical_cast<uint>(optarg);
            if (V < 1 || V >= 128)
                throw std::logic_error("invalid velocity parameter");
            break;
        case 'b':
            B = boost::lexical_cast<uint>(optarg);
            if (B <= 0)
                throw std::logic_error("invalid block size parameter");
            break;
        case 'T':
            RT.push_back(boost::lexical_cast<f64>(optarg));
            break;
        case 'R':
            RR.push_back(boost::lexical_cast<f64>(optarg));
            break;
        case 'p':
            Plot = true;
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
    uint nsamples = (uint)ceil(D * FS);
    std::unique_ptr<i8[]> samples(new i8[nsamples]);

    Env env;
    env.setparam(&Envp);
    env.initialize(FS, B, nullptr);

    std::sort(RT.begin(), RT.end());
    std::sort(RR.begin(), RR.end());
    uint nT = 0;
    uint nR = 0;

    env.trigger(V);
    for (uint i = 0; i < nsamples;) {
        uint bs = std::min(B, nsamples - i);
        env.generate(&samples[i], bs);
        i += bs;
        // retriggering
        f64 t = (f64)i / FS;
        for (; nT < RT.size() && t >= RT[nT]; ++nT) {
            env.trigger(V);
        }
        for (; nR < RR.size() && t >= RR[nR]; ++nR) {
            env.release(V);
        }
    }

    if (Plot) {
        FILE *gp = popen("gnuplot -d", "w");
        if (!gp)
            fprintf(stderr, "Cannot run gnuplot.\n");
        else {
            SCOPE(exit) { pclose(gp); };

            const char title[] = "ENV";

            fputs("set terminal wxt size 1024,480\n", gp);

            fprintf(gp, "set xrange [0:%f]\n", (f64)(nsamples - 1) / FS);

            for (f64 t : RT)
                fprintf(gp, "set arrow from %.17e,graph(0,0)"
                        " to %.17e,graph(1,1) nohead lc rgb 'red'\n", t, t);
            for (f64 t : RR)
                fprintf(gp, "set arrow from %.17e,graph(0,0)"
                        " to %.17e,graph(1,1) nohead lc rgb 'green'\n", t, t);

            fprintf(gp, "plot '-' using 1:2 title \"%s\"\n", gp_escape(title).c_str());
            for (uint i = 0; i < nsamples; ++i) {
                f64 t = (f64)i / FS;
                fprintf(gp, "%.17e %d\n", t, samples[i]);
            }
            fputs("e\n", gp);
            fputs("pause mouse close\n", gp);
            fflush(gp);
        }
    }
    else {
        for (uint i = 0; i < nsamples; ++i) {
            f64 t = (f64)i / FS;
            printf("%f %d\n", t, samples[i]);
        }
    }
}

static void parse_time_params(const char *arg)
{
    size_t arglen = strlen(arg);
    uint n;

    uint T1, T2, T3, T4;
    if (sscanf(arg, "%u,%u,%u,%u%n", &T1, &T2, &T3, &T4, (int *)&n) != 4 || n != arglen ||
        Envp.T1 >= 64 || Envp.T2 >= 64 || Envp.T3 >= 64 || Envp.T4 >= 64)
        throw std::logic_error("invalid time parameters");

    Envp.T1 = T1;
    Envp.T2 = T2;
    Envp.T3 = T3;
    Envp.T4 = T4;
}

static void parse_level_params(const char *arg)
{
    size_t arglen = strlen(arg);
    uint n;

    int L1, L2, L3;
    if (sscanf(arg, "%d,%d,%d%n", &L1, &L2, &L3, (int *)&n) != 3 ||
        n != arglen || Envp.L1 >= 64 || Envp.L2 >= 64 || Envp.L3 >= 64 ||
        Envp.L1 <= -64 || Envp.L2 <= -64 || Envp.L3 <= -64)
        throw std::logic_error("invalid level parameters");

    Envp.L1 = L1;
    Envp.L2 = L2;
    Envp.L3 = L3;
}
