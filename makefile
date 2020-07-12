# Makefile created by Rhyscitlema
# Explanation of file structure available at:
# http://rhyscitlema.com/applications/makefile.html
#
# UIDT => User Interface Definition Text
# RODT => Rhyscitlema Objects Definition Text

UIDT_OUT_FILE = libuidt.a
RODT_OUT_FILE = librodt.a

UIDT_OBJ_FILES = mouse.o \
                 timer.o \
                 tools.o \
                 userinterface_.o \
                 outsider.o

RODT_OBJ_FILES = object.o \
                 camera.o \
                 surface.o \
                 getimage.o

LIBALGO = ../algorithms
LIB_STD = ../lib_std
LIBRFET = ../librfet
LIBRODT = .
LIBRWIF = ../read_write_image_file

#-------------------------------------------------

# C compiler
CC = gcc

# archiver
AR = ar

# compiler flags
CC_FLAGS = -I$(LIBALGO) \
           -I$(LIB_STD) \
           -I$(LIBRFET) \
           -I$(LIBRODT) \
           -I$(LIBRWIF) \
           -Wall \
           $(CFLAGS)

# archiver flags
AR_FLAGS = -crs #$(ARFLAGS)

#-------------------------------------------------

all:
	$(MAKE) uidt CFLAGS+="-DLIBRODT"
	$(MAKE) rodt CFLAGS+="-DLIBRODT"

uidt: $(UIDT_OUT_FILE)
rodt: $(RODT_OUT_FILE)

$(UIDT_OUT_FILE): $(UIDT_OBJ_FILES)
	$(AR) $(AR_FLAGS) $(UIDT_OUT_FILE) $(UIDT_OBJ_FILES)

$(RODT_OUT_FILE): $(RODT_OBJ_FILES)
	$(AR) $(AR_FLAGS) $(RODT_OUT_FILE) $(RODT_OBJ_FILES)

# remove all created files
clean:
	$(RM) *.o *.a

#-------------------------------------------------

INCLUDE_FILES = $(LIBALGO)/*.h \
                $(LIB_STD)/*.h \
                $(LIBRFET)/*.h \
                $(LIBRODT)/*.h \
                $(LIBRWIF)/*.h

# compile .c files to .o files
%.o: %.c $(INCLUDE_FILES)
	$(CC) $(CC_FLAGS) -c -o $@ $<

