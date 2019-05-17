#include "cws80_data_plot.h"
#include "cws/cws80_data.h"
#include "cws/cws80_program.h"
#include "utility/scope_guard.h"
#include "utility/types.h"
#include <fmt/format.h>
#include <algorithm>
#include <vector>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#if defined(_WIN32)
#include <io.h>
#define isatty(fd) _isatty(fd)
#else
#include <unistd.h>
#endif

struct Subcommand {
    const char *name;
    const char *description;
    int (*entry)(int argc, char *argv[]);
};

extern Subcommand subcommands[];

//------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: cws80_tool <subcommand> <args>\n"
                        "\n"
                        "Available subcommands:\n");
        for (Subcommand *sp = subcommands; sp->name; ++sp)
            fprintf(stderr, "  %-30s -- %s\n", sp->name, sp->description);
        return 1;
    }

    ++argv;
    --argc;

    Subcommand *cmd = nullptr;
    for (Subcommand *sp = subcommands; !cmd && sp->name; ++sp)
        if (!strcmp(sp->name, argv[0]))
            cmd = sp;

    if (!cmd)
        return 1;

    return cmd->entry(argc, argv);
}

//------------------------------------------------------------------------------
static int cmd_bank(int argc, char *argv[])
{
    using namespace cws80;

    bool hex_dump = false;

    for (int c; (c = getopt(argc, argv, "h")) != -1;) {
        switch (c) {
        case 'h':
            hex_dump = true;
            break;
        default:
            return 1;
        }
    }

    uint count = argc - optind;

    std::vector<Bank> banks;
    std::vector<const char *> filenames;

    if (count == 0) {
        banks = {load_factory_bank()};
        filenames = {"<factory>"};
        count = 1;
    }
    else {
        banks.resize(count);
        filenames.assign(argv + optind, argv + optind + count);
        for (uint i = 0; i < count; ++i) {
            const char *filename = argv[optind];
            FILE *fh = fopen(filename, "rb");
            if (!fh) {
                fprintf(stderr, "Cannot read bank file: %s\n", filename);
                return 1;
            }
            SCOPE(exit) { fclose(fh); };
            banks[i] = Bank::read_sysex(fh);
        }
    }

    for (uint banknum = 0; banknum < count; ++banknum) {
        const Bank &bank = banks[banknum];
        std::string filename = filenames[banknum];

        // keep filenames with funny characters parsable
        std::transform(filename.begin(), filename.end(), filename.begin(), [](char c) -> char {
            return (c == '\n' || c == '\r') ? ' ' : c;
        });

        for (uint i = 0, n = bank.pgm_count; i < n; ++i) {
            const Program &pgm = bank.pgm[i];
            bool lastpgm = i + 1 == n && banknum + 1 == count;

            fmt::print("; program {} bank '{}'\n", i, filename);
            if (hex_dump) {
                const u8 *data = reinterpret_cast<const u8 *>(&pgm);
                size_t size = sizeof(pgm);
                for (size_t i = 0; i < size; ++i) {
                    if (i == 0)
                        fmt::print(";");
                    else if ((i & 15) == 0)
                        fmt::print("\n;");
                    fmt::print(" {:02X}", data[i]);
                }
                fmt::print("\n");
            }

            fmt::print("{}", pgm.to_string());

            if (!lastpgm)
                fmt::print("\n");
        }
    }

    return 0;
}

//------------------------------------------------------------------------------
static int cmd_list_samples(int argc, char *argv[])
{
    using namespace cws80;

    if (argc != 1)
        return 1;

    fmt::print("# {:14s} {:>8s} {:>8s} {:>8s} {:>8s}\n", "NAME", "INDEX",
               "LENGTH", "SEMI", "FINE");
    for (uint i = 0; i < wave_count; ++i) {
        Wave wave = wave_by_id(i);
        Sample sample = wave_sample(wave);
        char namebuf[16];

        fmt::print("{:16s} {:8d} {:8d} {:8d} {:8d}\n", wave_name(i, namebuf), i,
                   sample.length(), wave.semi, wave.fine);
    }
    fmt::print("# {:14s} {:>8s} {:>8s} {:>8s} {:>8s}\n", "NAME", "INDEX",
               "LENGTH", "SEMI", "FINE");

    return 0;
}

//------------------------------------------------------------------------------
static int cmd_show_sample(int argc, char *argv[])
{
    using namespace cws80;

    bool sequential = false;

    for (int c; (c = getopt(argc, argv, "s")) != -1;) {
        switch (c) {
        case 's':
            sequential = true;
            break;
        default:
            return 1;
        }
    }

    uint count = argc - optind;

    if (sequential) {
        if (count != 0)
            return 1;

        for (uint i = 0; i < wave_count; ++i) {
            char namebuf[16];
            Wave wave = wave_by_id(i);
            const char *name = wave_name(i, namebuf);
            if (!plot_waves(&wave, &name, 1))
                fprintf(stderr, "Cannot run gnuplot.\n");
        }
    }
    else {
        if (count < 1)
            return 1;

        std::vector<Wave> waves(count);
        std::vector<const char *> names(count);

        for (uint i = 0; i < count; ++i) {
            const char *name = argv[optind + i];
            uint id = wave_id_by_name(name);
            if (id == ~0u) {
                fprintf(stderr, "Sample not found: %s\n", name);
                return 1;
            }
            waves[i] = wave_by_id((WaveId)id);
            names[i] = name;
        }

        if (!plot_waves(waves.data(), names.data(), count))
            fprintf(stderr, "Cannot run gnuplot.\n");
    }

    return 0;
}

//------------------------------------------------------------------------------
static int cmd_list_wavesets(int argc, char *argv[])
{
    using namespace cws80;

    if (argc != 1)
        return 1;

    for (uint i = 0; i < waveset_count; ++i) {
        char namebuf[8];
        printf("%s\n", waveset_name(i, namebuf));
    }

    return 0;
}

//------------------------------------------------------------------------------
static int cmd_show_waveset(int argc, char *argv[])
{
    using namespace cws80;

    if (argc != 2)
        return 1;

    const char *name = argv[1];
    Waveset wavesetbuf;
    Waveset *waveset = waveset_by_name(name, &wavesetbuf);

    if (!waveset) {
        fprintf(stderr, "Waveset not found: %s\n", name);
        return 1;
    }

    if (!plot_waveset(*waveset))
        fprintf(stderr, "Cannot run gnuplot.\n");

    return 0;
}

//------------------------------------------------------------------------------
static int cmd_list_parameters(int argc, char *argv[])
{
    using namespace cws80;

    if (argc != 1)
        return 1;

    uint num_params = Param::num_params;

    fmt::print("# {:22s} {:>8s} {:>8s} {:>8s} {:>8s}\n", "NAME", "INDEX", "MIN",
               "MAX", "DEFAULT");
    for (uint i = 0; i < num_params; ++i) {
        std::pair<i32, i32> range = Program::get_parameter_range(i);
        i32 min = range.first, max = range.second;
        const char *name = Program::get_parameter_name(i);

        const Program &init = initial_program();
        i32 def = init.get_parameter(i);

        fmt::print("{:24s} {:8d} {:8d} {:8d} {:8d}\n", name, i, min, max, def);
    }
    fmt::print("# {:22s} {:>8s} {:>8s} {:>8s} {:>8s}\n", "NAME", "INDEX", "MIN",
               "MAX", "DEFAULT");

    return 0;
}

//------------------------------------------------------------------------------
Subcommand subcommands[16] = {
    {"bank", "Print bank contents", &cmd_bank},
    {"list-samples", "List samples", &cmd_list_samples},
    {"show-sample", "Show sample", &cmd_show_sample},
    {"list-wavesets", "List wavesets", &cmd_list_wavesets},
    {"show-waveset", "Show waveset", &cmd_show_waveset},
    {"list-parameters", "List parameters", &cmd_list_parameters},
    {},
};
