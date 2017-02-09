SUBNAME = backend
SPEC = smartmet-plugin-$(SUBNAME)
INCDIR = smartmet/plugins/$(SUBNAME)

# Installation directories

processor := $(shell uname -p)

ifeq ($(origin PREFIX), undefined)
  PREFIX = /usr
else
  PREFIX = $(PREFIX)
endif

ifeq ($(processor), x86_64)
  libdir = $(PREFIX)/lib64
else
  libdir = $(PREFIX)/lib
endif

bindir = $(PREFIX)/bin
includedir = $(PREFIX)/include
datadir = $(PREFIX)/share
plugindir = $(datadir)/smartmet/plugins
objdir = obj

# Compiler options

DEFINES = -DUNIX -D_REENTRANT

ifeq ($(CXX), clang++)

 FLAGS = \
	-std=c++11 -fPIC -MD \
	-Weverything \
	-Wno-c++98-compat \
	-Wno-float-equal \
	-Wno-padded \
	-Wno-missing-prototypes

 INCLUDES = \
	-isystem $(includedir) \
	-isystem $(includedir)/smartmet

else

 FLAGS = -std=c++11 -fPIC -MD -Wall -W -Wno-unused-parameter -fno-omit-frame-pointer -fdiagnostics-color=always

 FLAGS_DEBUG = \
	-Wcast-align \
	-Wcast-qual \
	-Winline \
	-Wno-multichar \
	-Wno-pmf-conversions \
	-Woverloaded-virtual  \
	-Wpointer-arith \
	-Wredundant-decls \
	-Wwrite-strings

 FLAGS_RELEASE = -Wuninitialized

 INCLUDES = \
	-I$(includedir) \
	-I$(includedir)/smartmet

endif

# Compile options in detault, debug and profile modes

CFLAGS_RELEASE = $(DEFINES) $(FLAGS) $(FLAGS_RELEASE) -DNDEBUG -O2 -g
CFLAGS_DEBUG   = $(DEFINES) $(FLAGS) $(FLAGS_DEBUG)   -Werror  -O0 -g

ifneq (,$(findstring debug,$(MAKECMDGOALS)))
  override CFLAGS += $(CFLAGS_DEBUG)
else
  override CFLAGS += $(CFLAGS_RELEASE)
endif

LIBS = -L$(libdir) \
	-lsmartmet-spine \
	-lboost_system

# What to install

LIBFILE = $(SUBNAME).so

# How to install

INSTALL_PROG = install -p -m 775
INSTALL_DATA = install -p -m 664

# Compilation directories

vpath %.cpp $(SUBNAME)
vpath %.h $(SUBNAME)

# The files to be compiled

SRCS = $(wildcard $(SUBNAME)/*.cpp)
HDRS = $(wildcard $(SUBNAME)/*.h)
OBJS = $(patsubst %.cpp, obj/%.o, $(notdir $(SRCS)))

INCLUDES := -I$(SUBNAME) $(INCLUDES)

.PHONY: test rpm

# The rules

all: configtest objdir $(LIBFILE)
debug: all
release: all
profile: all

configtest:

$(LIBFILE): $(OBJS)
	$(CXX) $(CFLAGS) -shared -rdynamic -o $(LIBFILE) $(OBJS) $(LIBS)

clean:
	rm -f $(LIBFILE) *~ $(SUBNAME)/*~
	rm -rf obj

format:
	clang-format -i -style=file $(SUBNAME)/*.h $(SUBNAME)/*.cpp

install:
	@mkdir -p $(plugindir)
	$(INSTALL_PROG) $(LIBFILE) $(plugindir)/$(LIBFILE)

test:
	cd test && make test

objdir:
	@mkdir -p $(objdir)

rpm: clean
	@if [ -e $(SPEC).spec ]; \
	then \
	  tar -czvf $(SPEC).tar.gz --transform "s,^,$(SPEC)/," * ; \
	  rpmbuild -ta $(SPEC).tar.gz ; \
	  rm -f $(SPEC).tar.gz ; \
	else \
	  echo $(SPEC).spec missing; \
	fi;

.SUFFIXES: $(SUFFIXES) .cpp

obj/%.o: %.cpp
	$(CXX) $(CFLAGS) $(INCLUDES) -c -o $@ $<

-include $(wildcard obj/*.d)
