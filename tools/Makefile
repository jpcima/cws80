CXX = g++
CXXFLAGS = -O2 -g

all: bin/cws80_tool

clean:
	rm -rf bin
	rm -rf build

.PHONY: all clean

build/%.cpp.o: sources/%.cpp
	@install -d $(dir $@)
	$(CXX) -MD -MP $(CXXFLAGS) -c -o $@ $<

cws80_tool_OBJS = \
	build/cws80_tool.cpp.o \
	build/cws80_data_plot.cpp.o \
	build/cws/cws80_data.cpp.o \
	build/cws/cws80_program.cpp.o \
	build/cws/cws80_program_realfmt.cpp.o \
	build/cws/cws80_program_textfmt.cpp.o

bin/cws80_tool: $(cws80_tool_OBJS)
	@install -d $(dir $@)
	$(CXX) -o $@ $^

bin/cws80_tool: CXXFLAGS += -DFMT_HEADER_ONLY -Isources -Iresources -Ithirdparty/vestige -Ithirdparty/fmt/include -Ithirdparty/gsl-lite/include -Ithirdparty/optional-lite/include -Ithirdparty/string-view-lite/include

-include $(cws80_tool_OBJS:%.o=%.d)
