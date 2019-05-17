#include "cws80_program.h"
#include "cws80_data.h"
#include "utility/dynarray.h"
#include <stdexcept>
#include <memory>
#include <cstring>

namespace cws80 {

uint Program::name_length() const
{
    const char *name = this->NAME;
    uint length = 6;
    for (char c; length > 0 && ((c = name[length - 1]) == ' ' || c == '\0');)
        --length;
    return length;
}

char *Program::name(char namebuf[8]) const
{
    uint n = name_length();
    std::memcpy(namebuf, this->NAME, n);
    namebuf[n] = 0;
    return namebuf;
}

bool Program::rename(cxx::string_view name)
{
    uint n = std::min<size_t>(name.size(), 6);
    name = cxx::string_view(name.data(), n);
    if (name == cxx::string_view(this->NAME, name_length()))
        return false;
    for (uint i = 0; i < n; ++i)
        this->NAME[i] = name[i];
    for (uint i = n; i < 6; ++i)
        this->NAME[i] = ' ';
    return true;
}

Program Program::load(const u8 *data, size_t length)
{
    Program pgm;

    if (length != Program::data_length)
        throw std::logic_error("cannot load program: invalid length");

    std::memcpy(&pgm, data, length);
    return pgm;
}

Bank Bank::load(const u8 *data, size_t length)
{
    Bank bank;

    uint pgm_count = length / Program::data_length;
    if (pgm_count >= Bank::max_programs)
        throw std::logic_error("cannot load bank: invalid program count");
    bank.pgm_count = pgm_count;

    uint i = 0;
    for (; i < pgm_count; ++i)
        bank.pgm[i] = Program::load(data + i * Program::data_length, Program::data_length);

    if (i < Bank::max_programs) {
        Program init = Program::load(init_prog_data, Program::data_length);
        do
            bank.pgm[i] = init;
        while (++i < Bank::max_programs);
    }

    return bank;
}

Bank Bank::load_sysex(const u8 *data, size_t length)
{
    Bank bank;

    if (auto *endp = (u8 *)std::memchr(data, 0xf7, length))
        length = endp - data;
    else
        throw std::logic_error("cannot load bank: invalid sysex format");

    if (length < 5)
        throw std::logic_error("cannot load bank: invalid length");

    const u8 sysexhdr[5] = {0xf0, 0x0f, 0x02, 0x00, 0x02};
    if (std::memcmp(data, sysexhdr, 5))
        throw std::logic_error("cannot load bank: invalid sysex format");
    data += 5;
    length -= 5;

    uint pgm_count = length / (2 * Program::data_length);
    if (pgm_count >= Bank::max_programs)
        throw std::logic_error("cannot load bank: invalid program count");
    bank.pgm_count = pgm_count;

    uint i = 0;
    for (; i < pgm_count; ++i) {
        u8 pgmdata[Program::data_length];
        for (uint i = 0; i < Program::data_length; ++i)
            pgmdata[i] = ((data[2 * i + 1] & 0xf) << 4) | (data[2 * i] & 0xf);
        bank.pgm[i] = Program::load(pgmdata, Program::data_length);
        data += 2 * Program::data_length;
    }

    if (i < Bank::max_programs) {
        Program init = Program::load(init_prog_data, Program::data_length);
        do
            bank.pgm[i] = init;
        while (++i < Bank::max_programs);
    }

    return bank;
}

size_t Bank::save_sysex(u8 *data, const Bank &bank)
{
    u8 pgm_count = bank.pgm_count;
    u8 *outp = data;

    if (pgm_count >= Bank::max_programs)
        throw std::logic_error("cannot save bank: invalid program count");

    const u8 sysexhdr[5] = {0xf0, 0x0f, 0x02, 0x00, 0x02};
    for (u8 byte : sysexhdr)
        *outp++ = byte;
    for (uint num = 0; num < pgm_count; ++num) {
        const Program &pgm = bank.pgm[num];
        const u8 *pgdata = (const u8 *)&pgm;
        for (uint i = 0; i < sizeof(Program); ++i) {
            u8 byte = pgdata[i];
            *outp++ = byte & 0x0f;
            *outp++ = (byte & 0xf0) >> 4;
        }
    }
    *outp++ = 0xf7;

    return outp - data;
}

Bank Bank::read_sysex(FILE *file)
{
    u8 data[Bank::sysex_max_length];
    size_t length = 0;

    do {
        if (length == Bank::sysex_max_length)
            throw std::logic_error("cannot load bank: file too large");
        int c = fgetc(file);
        if (c == EOF) {
            if (ferror(file))
                throw std::runtime_error("cannot load bank: file input error");
            else
                throw std::logic_error(
                    "cannot load bank: invalid sysex format");
        }
        data[length++] = c;
    } while (data[length - 1] != 0xf7);

    return load_sysex(data, length);
}

void Bank::write_sysex(FILE *file, const Bank &bank)
{
    u8 data[sysex_max_length];
    size_t size = save_sysex(data, bank);

    fwrite(data, 1, size, file);
    fflush(file);

    if (ferror(file))
        throw std::runtime_error("cannot load bank: file input error");
}

}  // namespace cws80
