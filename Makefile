SDK_SRC = $(wildcard ../common/*.cpp)
SDK_OBJ = $(addprefix build/,$(notdir $(SDK_SRC:.cpp=.o)))
CC_FLAGS = -arch x86_64 -mmacosx-version-min=10.6 -I../include -pthread -Wno-parentheses-equality -Wno-parentheses

PLUGIN_SRC = $(wildcard ./*.cpp)
PLUGIN_OBJ = $(addprefix build/,$(notdir $(PLUGIN_SRC:.cpp=.o)))

all: $(SDK_OBJ) build/libcommon.a $(PLUGIN_OBJ) build/threeio.lx

build/%.o: ../common/%.cpp
	gcc $(CC_FLAGS) -c -o $@ $<
	# -g -O2 -fPIC -m64 -pthread -fno-common

build/libcommon.a: $(SDK_OBJ)
	ar rcs build/libcommon.a $(SDK_OBJ)

build/%.o: ./%.cpp
	g++ -stdlib=libstdc++ -c -fPIC -I../include -lstdc++ -o $@ $<

build/threeio.lx: $(PLUGIN_OBJ)
	g++ -stdlib=libstdc++ -arch x86_64 -mmacosx-version-min=10.6 -shared -lcommon -L./build -I../include -o build/threeio.lx $(PLUGIN_OBJ)

clean:
	rm -f build/*

.PHONY: all clean
