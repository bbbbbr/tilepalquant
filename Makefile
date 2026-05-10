
DEL = rm -f
SRCDIR = src
OBJDIR = obj
BINDIR = bin
PACKDIR = package

CC = $(TOOLSPREFIX)g++
CFLAGS = -Os -Wall -g # -Wextra -pedantic
LFLAGS = -g -s -static -lm


# Add all c source files from $(SRCDIR)
# Create the object files in $(OBJDIR)
CFILES = $(wildcard $(SRCDIR)/*.cpp) \
         $(wildcard $(LODEPNGDIR)/*.cpp) \
         $(wildcard $(QUANTDIR)/*.cpp)

MKDIRS = $(OBJDIR) $(BINDIR)

INCS = -I"$(SRCDIR)" -I"$(LODEPNGDIR)" -I"$(QUANTDIR)"
CFLAGS = $(INCS) -Wall -Wextra -pedantic
CFLAGS += -MMD -MP



COBJ = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(CFILES))

BIN_NAME = tilepalquant
BIN = $(BINDIR)/$(BIN_NAME)


# ignore package directory that conflicts with rule target
.PHONY: package

all: $(COBJ)
	$(CC) $(CFLAGS) -o $(BIN) $^ $(LDFLAGS)


# Compile .c to .o in a separate directory
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CC) $(CFLAGS) -c $< -o $@



cleanobj:
	$(DEL) $(COBJ) $(DEPS)

clean:
	$(DEL) $(COBJ) $(BIN) $(DEPS)



# For -MMD and -MP
# Dependencies
DEPS = $(COBJ:%.o=%.d)
-include $(DEPS)


# create necessary directories after Makefile is parsed but before build
# info prevents the command from being pasted into the makefile
$(info $(shell mkdir -p $(MKDIRS)))

