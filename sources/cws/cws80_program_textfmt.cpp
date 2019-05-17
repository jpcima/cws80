#include "cws80_program.h"
#include <fmt/format.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <stdexcept>
namespace pt = boost::property_tree;

namespace cws80 {

static pt::ptree ptree_from_text(const std::string &text)
{
    pt::ptree tree;
    std::istringstream ins(text);
    pt::read_info(ins, tree);
    return tree;
};

static std::string ptree_to_text(const pt::ptree &tree)
{
    std::ostringstream outs;
    pt::write_info(outs, tree);
    return outs.str();
}

std::string Program::to_string() const
{
    pt::ptree tree;
    const Program &x = *this;
    char namebuf[8];

    tree.put("SQ80\\File.type", "SQ80PROG");
    tree.put("SQ80\\File.name", this->name(namebuf));

#define PARAMETER_CASE(index, cat, name, gett, sett, min, max) \
    {                                                          \
        const char *key = "SQ80\\" #cat "." #name;             \
        tree.put<i32>(key, (gett));                            \
    }

    EACH_PARAMETER(PARAMETER_CASE)

#undef PARAMETER_CASE

    return ptree_to_text(tree);
}

Program Program::from_string(const std::string &text)
{
    Program x;
    pt::ptree tree = ptree_from_text(text);

    const std::string type = tree.get<std::string>("SQ80\\File.type");
    const std::string name = tree.get<std::string>("SQ80\\File.name");

    if (type != "SQ80PROG")
        throw std::logic_error(
            "Cannot load SQ80 text program: invalid file format");

    memset(x.NAME, 0, 6);
    memcpy(x.NAME, name.data(), std::min<size_t>(name.size(), 6));

#define PARAMETER_CASE(index, cat, name, gett, sett, min, max) \
    {                                                          \
        const char *key = "SQ80\\" #cat "." #name;             \
        i32 v = tree.get<i32>(key);                            \
        (sett);                                                \
    }

    EACH_PARAMETER(PARAMETER_CASE)

#undef PARAMETER_CASE

    return x;
}

}  // namespace cws80
