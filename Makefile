DEL = rm -f


SRCDIR = src
OBJDIR = obj
BINDIR = bin
PACKDIR = package
MKDIRS = $(OBJDIR) $(BINDIR) $(PACKDIR)

# Add all c source files from $(SRCDIR)
# Create the object files in $(OBJDIR)
CFILES = $(wildcard $(SRCDIR)/*.cpp)
COBJ = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(CFILES))

BIN_NAME = tilepalquant
BIN = $(BINDIR)/$(BIN_NAME)$(EXE_EXT)
PACKFILES = $(BIN) Changelog.md README.md LICENSE
PACKBASENAME = $(BIN_NAME)


INCS = -I"$(SRCDIR)"
CFLAGS = $(INCS) -Wall -Wextra -pedantic -std=c++11
CFLAGS += -MMD -MP


# ignore package directory that conflicts with rule target
.PHONY: package

all: linux

# Compile .c to .o in a separate directory
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CC) $(CFLAGS) -c $< -o $@

# Linux MinGW build for Windows
# (static linking to avoid DLL dependencies)
wincross: EXE_EXT = .exe
wincross: TARGET=i686-w64-mingw32
wincross: CC = $(TARGET)-g++
wincross: LDFLAGS = -s -static
wincross: $(COBJ)
	$(CC) -o $(BIN)  $^ $(LDFLAGS)

# Macos uses linux target
macos: linux

# Linux build
linux: CC = $(TOOLSPREFIX)g++
linux: CFLAGS += -Os -Wall -g # -Wextra -pedantic
linux: LFLAGS += -g -s -static -lm
linux: $(COBJ)
	$(CC) $(CFLAGS) -o $(BIN) $^ $(LDFLAGS)



cleanobj:
	$(DEL) $(COBJ) $(DEPS)

clean:
	$(DEL) $(COBJ) $(BIN) $(DEPS)


macos-x64-zip: macos
	mkdir -p $(PACKDIR)
	strip $(BIN)
	# -j discards (junks) path to file
	zip -j $(PACKBASENAME)-macos-x64.zip $(PACKFILES)
	mv $(PACKBASENAME)-macos-x64.zip $(PACKDIR)

macos-arm64-zip: macos
	mkdir -p $(PACKDIR)
	strip $(BIN)
	# -j discards (junks) path to file
	zip -j $(PACKBASENAME)-macos-arm64.zip $(PACKFILES)
	mv $(PACKBASENAME)-macos-arm64.zip $(PACKDIR)

linuxzip: linux
	mkdir -p $(PACKDIR)
	strip $(BIN)
	# -j discards (junks) path to file
	zip -j $(PACKBASENAME)-linux.zip $(PACKFILES)
	mv $(PACKBASENAME)-linux.zip $(PACKDIR)

linux-arm-zip: linux
	mkdir -p $(PACKDIR)
	strip $(BIN)
	# -j discards (junks) path to file
	zip -j $(PACKBASENAME)-linux_arm.zip $(PACKFILES)
	mv $(PACKBASENAME)-linux_arm.zip $(PACKDIR)

wincrosszip: EXE_EXT = .exe
wincrosszip: wincross
	mkdir -p $(PACKDIR)
	strip $(BIN)
	# -j discards (junks) path to file
	zip -j $(PACKBASENAME)-windows.zip $(PACKFILES)
	mv $(PACKBASENAME)-windows.zip $(PACKDIR)

package:
	${MAKE} clean
	${MAKE} wincrosszip
	${MAKE} clean
	${MAKE} linuxzip



perf_capture:
	valgrind --callgrind-out-file=bin/callgrind.out --tool=callgrind bin/tilepalquant carina-nebula.png -o bin/output.png

perf_analyze:
	kcachegrind bin/callgrind.out

lesswarn: CFLAGS += -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-parameter -Wno-unused-function
lesswarn: clean
lesswarn: all


# For -MMD and -MP
# Dependencies
DEPS = $(COBJ:%.o=%.d)
-include $(DEPS)


# create necessary directories after Makefile is parsed but before build
# info prevents the command from being pasted into the makefile
$(info $(shell mkdir -p $(MKDIRS)))

