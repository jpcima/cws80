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
    gsl::make_span(bank1),
    gsl::make_span(bank2),
    gsl::make_span(bank3),
    gsl::make_span(bank4),
    gsl::make_span(bank5),
    gsl::make_span(bank6),
    gsl::make_span(bank7),
    gsl::make_span(bank8),
    gsl::make_span(bank9),
    gsl::make_span(bank10),
    gsl::make_span(bank11),
}};

}  // namespace cws80
