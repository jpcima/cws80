#include "cws80_program.h"
#include <SimpleIni.h>
#include <fmt/format.h>
#include <stdexcept>
#include <cstring>

namespace cws80 {

std::string Program::to_string() const
{
    CSimpleIniA ini;
    const Program &x = *this;
    char namebuf[8];

    ini.SetValue("SQ80\\File", "type", "SQ80PROG");
    ini.SetValue("SQ80\\File", "name", this->name(namebuf));

    #define PARAMETER_CASE(index, cat, name, gett, sett, min, max) \
        ini.SetLongValue("SQ80\\" #cat, #name, (gett));
    EACH_PARAMETER(PARAMETER_CASE)
    #undef PARAMETER_CASE

    std::string text;
    if (ini.Save(text) != SI_OK)
        throw std::logic_error("Cannot save SQ80 text program");

    return text;
}

Program Program::from_string(const std::string &text)
{
    Program x;

    CSimpleIniA ini;
    bool load_ok = ini.LoadData(text) == SI_OK &&
        strcmp(ini.GetValue("SQ80\\File", "type", ""), "SQ80PROG") == 0;

    if (!load_ok)
        throw std::logic_error("Cannot load SQ80 text program");

    const char *name = ini.GetValue("SQ80\\File", "name", "");
    memset(x.NAME, 0, 6);
    memcpy(x.NAME, name, strnlen(name, 6));

    #define PARAMETER_CASE(index, cat, name, gett, sett, min, max) \
        { i32 v = ini.GetLongValue("SQ80\\" #cat, #name); (sett); }
    EACH_PARAMETER(PARAMETER_CASE)
    #undef PARAMETER_CASE

    return x;
}

}  // namespace cws80
