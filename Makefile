# Name der Datei, die als erstes gelinkt werden soll.
# Da das System gleich die erste Funktion in dieser Datei
# ausfuehrt, sollte diese den Eintrittspunkt in das System
# enthalten.
MAIN_SRC = start.c

#optional:
# Datei, die als letztes verlinkt wird.
LAST_SRC =       

# Programme
# =========
LD          = ld
GDB         = gdb
OBJCOPY     = objcopy
DISASM      = objdump --disassemble
MKIMAGE     = mkimage
MAKEDEPEND  = makedepend

# Programflags
# ============

# enable software division and modulo operations (not supported by hardware)
# DIVLIB      = ./libs/arm_divmod.a

#extra libraries to link with
EXTRA_LIBS = $(DIVLIB)

MKIMAGE_FLAGS = -A arm -O linux -T kernel -C none -a 0x22000000 -e 0x22000000

# Wenn Umgebungsvariable DEBUG=1, dann generiere debugsymbole beim kompilieren
CFLAGS      = -Wall -Wextra -Wpointer-arith -Wbad-function-cast -Wno-unused -fno-builtin -mcpu=arm920t
ifeq ($(DEBUG), 1)
CFLAGS += -g -DDEBUG
else
CFLAGS +=
endif

ASFLAGS    	= -W -mfpu=softvfp
ifeq ($(DEBUG), 1)
ASFLAGS += -g
endif

# Wenn Umgebungsvariable DEBUG=1, dann starte skyeye im debugmodus
SKYEYE_FLAGS =
ifeq ($(DEBUG), 1)
SKYEYE_FLAGS += -d
endif

ASFLAGS    	= 
CPPFLAGS    = 
LDFLAGS     = 
LSCRIPT     = link_sdram.ld

IMAGE	    = kernel$(AUFGABE)
BOOTIMAGE   = system.img

# DIST_DIR und NAMES_FILE bestimmen Namen der generierten/benoetigten Dateien
# zum generieren der Tarballs fuer die Abgabe.
DIST_DIR = abgabe$(AUFGABE)
NAMES_FILE = namen.txt

#############################################################################
# DO NOT CHANGE:
#
#############################################################################

.PHONY: dep dep2 clean distclean emu gdb dump help sloc ddd tags dist doc

ifeq ($(CROSS_COMPILE),)
CROSS_COMPILE = arm-elf-
endif

# Verzeichnisse, in denen sich die Quelltexte befinden
SRC_DIRS := $(shell find src -type d | grep -v '.svn')

# erstelle beim Start der Makefile automatisch das build-Verzeichnis
INIT := $(shell mkdir -p $(SRC_DIRS:src%=build%))

MAIN_SRC_FILE := $(shell find src -type f -name $(MAIN_SRC) | grep -v '.svn')
MAIN_OBJ_FILE := $(shell ls $(MAIN_SRC_FILE) | sed -e 's/src\///' | sed -e 's/\..$$/\.o/')

ifeq ($(LAST_SRC),)
LAST_SRC_FILE :=
LAST_OBJ_FILE :=
else
LAST_SRC_FILE := $(shell find src -type f -name $(LAST_SRC) | grep -v '.svn')
LAST_OBJ_FILE := $(shell ls $(LAST_SRC_FILE) | sed -e 's/src\///' | sed -e 's/\..$$/\.o/')
endif

FIND_SRC_FILES := find src -type f | grep -v '.svn' | grep '\.[csS]$$'
ifeq ($(LAST_SRC),)
FIND_OBJ_FILES := $(FIND_SRC_FILES)  | sed -e 's/\..$$/\.o/' | grep -v $(MAIN_OBJ_FILE) | sed -e 's/src\//build\//'
else
FIND_OBJ_FILES := $(FIND_SRC_FILES)  | sed -e 's/\..$$/\.o/' | grep -v $(MAIN_OBJ_FILE) | grep -v $(LAST_OBJ_FILE) | sed -e 's/src\//build\//'
endif

SRC := $(shell $(FIND_SRC_FILES))

# generiere Liste aller Quelltextdateien und zu generierenden Objectdateien
ifeq ($(LAST_SRC),)
OBJS := build/$(MAIN_OBJ_FILE) $(shell $(FIND_OBJ_FILES))
OBJS_AND_LIBS := build/$(MAIN_OBJ_FILE) $(shell $(FIND_OBJ_FILES)) $(EXTRA_LIBS)
else
OBJS := build/$(MAIN_OBJ_FILE) $(shell $(FIND_OBJ_FILES)) build/$(LAST_OBJ_FILE)
OBJS_AND_LIBS := build/$(MAIN_OBJ_FILE) $(shell $(FIND_OBJ_FILES)) $(EXTRA_LIBS) build/$(LAST_OBJ_FILE)
endif

# recreate dependency graph
DEPEND_FILES       = $(OBJS:%.o=%.d)
ifeq (,$(filter-out install all dump emu, $(MAKECMDGOALS)))
DEPEND := $(shell make dep > /dev/null 2>&1)
DEPEND := $(DEPEND_FILES)
endif

#CPPFLAGS += -I. $(SRC_DIRS:%=-I%)

all:	$(BOOTIMAGE)

$(BOOTIMAGE): $(IMAGE)
	$(MKIMAGE) $(MKIMAGE_FLAGS) -d $(IMAGE).bin $@

# Binary Image aus Objektfiles machen
$(IMAGE): $(OBJS_AND_LIBS)
	$(CROSS_COMPILE)$(LD) $(LDFLAGS) -T$(LSCRIPT) -o $@ $^
	$(CROSS_COMPILE)$(OBJCOPY) $@ -Obinary $@.bin

install: $(BOOTIMAGE)
	@cp $(BOOTIMAGE) /srv/tftp/test

# dep und dep2 werden intern verwendet um Abhaengigkeitsgraphen neu
# zu erstellen
dep:
	@[ ! -d build ] || find build -name '*.d' -exec rm -f {} \;
	$(MAKE) dep2

dep2: $(DEPEND_FILES)

# loesche alle Dateien, ausser der Images und der Dokumentation
clean:
	if [ -e skyeye.conf ]; then rm -f skyeye.conf; fi
	find src -name '*~' -exec rm -f {} \;
	rm -fR build

# loesche alle generierten Dateien
distclean: clean
	if [ -e tags ]; then rm -f tags; fi
	if [ -e $(DIST_DIR) ]; then rm -fR $(DIST_DIR); fi
	if [ -e $(DIST_DIR).tar.bz2 ]; then rm -f $(DIST_DIR).tar.bz2; fi
	if [ -d doxygen ]; then rm -fR doxygen; fi
	rm -f $(IMAGE) $(IMAGE).bin $(BOOTIMAGE)

emu:    $(BOOTIMAGE) skyeye.conf
	skyeye $(SKYEYE_FLAGS)

skyeye.conf:
	cpp -DIMAGE=$(IMAGE).bin -P skyeye.conf.tmpl skyeye.conf

gdb: $(IMAGE)
	$(CROSS_COMPILE)$(GDB) $(IMAGE)

# 1. starte skyeye im Debugmodus im Hintergrund
# 2. starte ddd und lasse ddd sich automatisch mit skyeye verbinden
# 
# wenn ddd beendet wird, wird skyeye automatisch abgeschossen
ddd: $(IMAGE)
	@{ skyeye -d & } && echo "target remote :12345" > /tmp/gdb.conf && \
		ddd --debugger $(CROSS_COMPILE)$(GDB) --command /tmp/gdb.conf --no-exec-window $(IMAGE) > /dev/null 2>&1; \
		kill %1

# erstelle ctags fuer vim/emacs zum 'browsen' innerhalb des Quelltextes
tags:
	ctags -R src

# Wenn Datei NAMES_FILE existiert und nicht leer ist, dann erstelle Tarball
dist:
	@tst=$$(cat $(NAMES_FILE) 2>/dev/null) && [ -f $(NAMES_FILE) -a -n "$$tst" ] || \
		{ echo; echo; \
		  echo "Fehler:"; \
		  echo "  Bitte erstellt die Datei \"$(NAMES_FILE)\" mit euren Namen oder Matrikelnummern"; \
		  echo "  um das tarball zu erstellen"; \
		  echo; echo; \
		  exit 1; }
	@echo "erstelle tarball: $(DIST_DIR).tar.bz2"
	@rm -fR $(DIST_DIR)
	@mkdir $(DIST_DIR)
	@cp -R src libs Makefile link_sdram.ld skyeye.conf u-boot.bin $(NAMES_FILE) $(DIST_DIR)
	@find $(DIST_DIR) -name ".*" | xargs rm -fR
	@tar -cjf $(DIST_DIR).tar.bz2 $(DIST_DIR)
	@rm -fR $(DIST_DIR)

dump: $(BOOTIMAGE)
	$(CROSS_COMPILE)$(DISASM) $(IMAGE) 

doc:
	doxygen doxygen.conf

sloc:
	@find src -type f | grep -v '.svn' | grep '\.[chsS]$$' | xargs cat | wc -l

rebuild:
	$(MAKE) clean dep
	$(MAKE) all

reinstall:
	$(MAKE) clean dep
	$(MAKE) install

#call makedepend -> replace srcdir with build-dir -> add .d - files to rule -> write to file
build/%.d: src/%.c
	@echo "build dependencies for $@"
	@$(MAKEDEPEND) $(CPPFLAGS) -f - $^ | sed -e 's/^src/build/' | sed -e 's/^\(.*\).o:/\1.o \1.d:/' > $@

build/%.d: src/%.S
	@echo "build dependencies for $@"
	@$(MAKEDEPEND) $(CPPFLAGS) -f - $^ | sed -e 's/^src/build/' | sed -e 's/^\(.*\).o:/\1.o \1.d:/' > $@

build/%.d: src/%.s
	@echo "build dependencies for $@"
	@$(MAKEDEPEND) $(CPPFLAGS) -f - $^ | sed -e 's/^src/build/' | sed -e 's/^\(.*\).o:/\1.o \1.d:/' > $@

build/%.o: src/%.c
	@echo "compiling $<"
	@$(CROSS_COMPILE)gcc $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

#add debug info -> remove comments -> assemble
build/%.o: src/%.s
	@echo "compiling $<"
	@$(CROSS_COMPILE)asm $< $@ ---cpp-flags $(CPPFLAGS) -D__ASSEMBLER__ ---c-flags $(CFLAGS) -c ---

#exec preprocessor -> remove comments -> assemble
build/%.o: src/%.S
	@echo "compiling $<"
	@$(CROSS_COMPILE)-as $< $@ ---cpp-flags $(CPPFLAGS) -D__ASSEMBLER__ ---c-flags $(CFLAGS) -c ---

include $(DEPEND)

