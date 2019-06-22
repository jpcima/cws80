#include "cws/cws80_data_banks.h"

namespace cws80 {

static const u8 bank1[] = {
#include "sqdata/extrabank1.dat.h"
};
static const u8 bank2[] = {
#include "sqdata/extrabank2.dat.h"
};
static const u8 bank3[] = {
#include "sqdata/extrabank3.dat.h"
};
static const u8 bank4[] = {
#include "sqdata/extrabank4.dat.h"
};
static const u8 bank5[] = {
#include "sqdata/extrabank5.dat.h"
};
static const u8 bank6[] = {
#include "sqdata/extrabank6.dat.h"
};
static const u8 bank7[] = {
#include "sqdata/extrabank7.dat.h"
};
static const u8 bank8[] = {
#include "sqdata/extrabank8.dat.h"
};
static const u8 bank9[] = {
#include "sqdata/extrabank9.dat.h"
};
static const u8 bank10[] = {
#include "sqdata/extrabank10.dat.h"
};
static const u8 bank11[] = {
#include "sqdata/extrabank11.dat.h"
};

std::array<gsl::span<const u8>, 11> extra_bank_data{{
    {bank1, sizeof(bank1)},
    {bank2, sizeof(bank2)},
    {bank3, sizeof(bank3)},
    {bank4, sizeof(bank4)},
    {bank5, sizeof(bank5)},
    {bank6, sizeof(bank6)},
    {bank7, sizeof(bank7)},
    {bank8, sizeof(bank8)},
    {bank9, sizeof(bank9)},
    {bank10, sizeof(bank10)},
    {bank11, sizeof(bank11)},
}};

}  // namespace cws80
