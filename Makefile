VERSION  = 0.1

CXXFLAGS = -O3 -std=c++0x -stdlib=libc++ -arch x86_64 -mmacosx-version-min=10.7 -fPIC -I../include -Wno-parentheses-equality -Wno-parentheses
BUILDDIR = build

SDK_SRC  = $(wildcard ../common/*.cpp)
SDK_OBJ  = $(addprefix $(BUILDDIR)/,$(notdir $(SDK_SRC:.cpp=.o)))

PLUGIN_SRC = $(wildcard ./*.cpp)
PLUGIN_OBJ = $(addprefix $(BUILDDIR)/,$(notdir $(PLUGIN_SRC:.cpp=.o)))

KIT_PATH = /Library/Application\ Support/Luxology/Content/Kits/threeio

all: $(SDK_OBJ) $(BUILDDIR)/libcommon.a $(PLUGIN_OBJ) $(BUILDDIR)/threeio.lx

$(BUILDDIR)/%.o: ../common/%.cpp
	g++ $(CXXFLAGS) -pthread -c -o $@ $<

$(BUILDDIR)/libcommon.a: $(SDK_OBJ)
	ar rcs $(BUILDDIR)/libcommon.a $(SDK_OBJ)

$(BUILDDIR)/%.o: ./%.cpp
	g++ $(CXXFLAGS) -c -o $@ $<

$(BUILDDIR)/threeio.lx: $(PLUGIN_OBJ)
	g++ $(CXXFLAGS) -shared -lcommon -L$(BUILDDIR) -o $(BUILDDIR)/threeio.lx $(PLUGIN_OBJ)

$(SDK_OBJ): |$(BUILDDIR)

$(PLUGIN_OBJ): |$(BUILDDIR)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	rm -r $(BUILDDIR)/*

uninstall:
	-rm -r $(KIT_PATH)

install: uninstall
	mkdir -p $(KIT_PATH)/config
	mkdir -p $(KIT_PATH)/osx
	ln -s `pwd`/kit/index.cfg $(KIT_PATH)/index.cfg
	ln -s `pwd`/kit/config/threeio.cfg $(KIT_PATH)/config/threeio.cfg
	ln -s `pwd`/build/threeio.lx $(KIT_PATH)/osx/threeio.lx

osx:
	cp $(BUILDDIR)/threeio.lx kit/threeio/osx/
	cd ./kit && zip -r ../threeio-$(VERSION)-osx-x64.zip ./threeio

.PHONY: all clean install uninstall osx
