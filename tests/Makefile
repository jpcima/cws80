CXX = g++
WCXX = i686-w64-mingw32-g++
CXXFLAGS = -O2 -g -std=c++14
LDFLAGS =

all: bin/test_env bin/test_lfo bin/test_sq8l.exe

clean:
	rm -rf bin
	rm -rf build

.PHONY: all clean

build/%.cpp.o: sources/%.cpp
	@install -d $(dir $@)
	$(CXX) -MD -MP $(CXXFLAGS) -c -o $@ $<

build/%.wpp.o: sources/%.wpp
	@install -d $(dir $@)
	$(WCXX) -x c++ -MD -MP $(CXXFLAGS) -c -o $@ $<

#-------------------------------------------------------------------------------
test_env_OBJS = \
	build/test_env.cpp.o \
	build/plots.cpp.o \
	build/cws/cws80_data.cpp.o \
	build/cws/cws80_program.cpp.o \
	build/cws/component/env.cpp.o \
	build/cws/component/tables.cpp.o

bin/test_env: $(test_env_OBJS)
	@install -d $(dir $@)
	$(CXX) -o $@ $^ $(LDFLAGS)

bin/test_env: CXXFLAGS += -DFMT_HEADER_ONLY -Isources -Iresources -Ithirdparty/simpleini -Ithirdparty/fmt/include -Ithirdparty/gsl-lite/include -Ithirdparty/optional-lite/include -Ithirdparty/string-view-lite/include

-include $(test_env_OBJS:%.o=%.d)

#-------------------------------------------------------------------------------
test_lfo_OBJS = \
	build/test_lfo.cpp.o \
	build/plots.cpp.o \
	build/cws/cws80_data.cpp.o \
	build/cws/cws80_program.cpp.o \
	build/cws/component/lfo.cpp.o \
	build/cws/component/tables.cpp.o

bin/test_lfo: $(test_lfo_OBJS)
	@install -d $(dir $@)
	$(CXX) -o $@ $^ $(LDFLAGS)

bin/test_lfo: CXXFLAGS += -DFMT_HEADER_ONLY -Isources -Iresources -Ithirdparty/simpleini -Ithirdparty/fmt/include -Ithirdparty/gsl-lite/include -Ithirdparty/optional-lite/include -Ithirdparty/string-view-lite/include

-include $(test_lfo_OBJS:%.o=%.d)

#-------------------------------------------------------------------------------
test_sq8l_OBJS = \
	build/test_sq8l.wpp.o \
	build/host_sq8l.wpp.o

bin/test_sq8l.exe: $(test_sq8l_OBJS)
	@install -d $(dir $@)
	$(WCXX) -o $@ $^ $(LDFLAGS)

bin/test_sq8l.exe: LDFLAGS += -static

bin/test_sq8l.exe: CXXFLAGS += -DFMT_HEADER_ONLY -Isources -Iresources -Ithirdparty/vestige -Ithirdparty/simpleini -Ithirdparty/fmt/include -Ithirdparty/gsl-lite/include -Ithirdparty/optional-lite/include -Ithirdparty/string-view-lite/include
bin/test_sq8l.exe: LDFLAGS += -lsndfile

-include $(test_sq8l_OBJS:%.o=%.d)
