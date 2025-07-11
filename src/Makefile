#############################################################################
# Interesting make targets:
# - exe: Just the executable. This is the default.
# - zip: Zip for standalone release.
# - clean: Delete generated files.

# make parameter CCOPT, e.g. for make CCOPT=-Wno-unused
# make parameter LDOPT, e.g. for make LDOPT=-lnew-library

#############################################################################
# Variables intended for setting on the make command line.
# - RELEASE: release number for packaging
# - VERSION_SUFFIX: suffix for version information
# - WSLTTY_VERSION: wsltty version to be checked for updates
# - TARGET: target triple for cross compiling
#	values: i686-pc-cygwin, x86_64-pc-cygwin, i686-pc-msys, x86_64-pc-msys
# - DEBUG: define to enable debug build
# - DMALLOC: define to enable the dmalloc heap debugging library
#
# The values of DEBUG and DMALLOC variables do not matter, it's just about
# whether they're defined; a debug build can be built with
#	make DEBUG=1

#############################################################################
DEBUG ?=0
#TARGET=x86_64-w64-mingw32
#TARGET ?= x86_64-pc-cygwin
NAME := mintty

all:	bin 
mingw:
	TARGET=x86_64-w64-mingw32 $(MAKE) $(MFLAGS)
.PHONY: exe src pkg zip pdf clean

ifdef TARGET
  CC := $(TARGET)-gcc
  RC := $(TARGET)-windres
else
  CC := gcc
  RC := windres
  TARGET := $(shell $(CC) -dumpmachine)
endif
ifeq ($(TARGET), i686-pc-cygwin)
  platform := cygwin32
  cygport_opts := --32
  zip_files := ../docs/readme.html ../scripts/create_shortcut.js
else ifeq ($(TARGET), x86_64-pc-cygwin)
  platform := cygwin64
  cygport_opts := --64
  zip_files := ../docs/readme.html ../scripts/create_shortcut.js
else ifeq ($(TARGET), i686-pc-msys)
  MSYS=$(shell uname -r | sed -e 's,\..*,,')
  ifeq ("$(MSYS)", "1")
    platform := msys
  else
    platform := msys32
  endif
  zip_files := ../docs/readme-msys.html
else ifeq ($(TARGET), x86_64-pc-msys)
  platform := msys64
  zip_files := ../docs/readme-msys.html
else ifeq ($(TARGET), x86_64-w64-mingw32)
  platform := mingw
  zip_files := ../docs/readme-msys.html
  $(msg Target '$(TARGET)' '$(platform)' )
else
  # just warning to support cross-generation of data files
warn=[30;48;5;11m
norm=[m
  $(warning $(warn) Target '$(TARGET)' not supported $(norm))
endif
BINFOLDER = ../bin
#BINDIR = $(BINFOLDER)/$(TARGET)
ifeq ($(DEBUG), 1)
	BINDIR = $(BINFOLDER)/$(platform)g
	NAME := minttyg
  CFLAGS += -Wno-error
	DFLAGS =-DDEBUG -g
else
	BINDIR = $(BINFOLDER)/$(platform)
	NAME := mintty
  CFLAGS += -fomit-frame-pointer 
	DFLAGS =-DNDEBUG -O2
endif
CPPFLAGS := -DTARGET=$(TARGET)
ifdef VERSION_SUFFIX
	CPPFLAGS += -DVERSION_SUFFIX="$(VERSION_SUFFIX)"
endif
ifdef WSLTTY_VERSION
	CPPFLAGS += -DWSLTTY_VERSION="$(WSLTTY_VERSION)"
endif

ifndef RELEASE
  svn_rev := $(shell svn info 2>/dev/null | grep ^Revision: | sed 's/Revision: //')
  ifneq ($(svn_rev),)
    CPPFLAGS += -DSVN_DIR=$(shell basename "`svn info | grep ^URL:`") \
                -DSVN_REV=$(svn_rev)
  endif
endif

srcversion := $(shell if test ../VERSION -nt appinfo.h;then  cat ../VERSION;else echo VERSION | cpp -P $(CPPFLAGS) --include src/appinfo.h |sed "s/\"//g"| tee ../VERSION ;fi )
name_ver := $(NAME)-$(srcversion)
#############################################################################
# compilation parameters

t_srcs := $(wildcard *.t)
h_srcs := $(wildcard *.h)
c_srcs := $(wildcard *.c)
rc_srcs := $(wildcard *.rc)
objs := $(c_srcs:.c=.o) $(rc_srcs:.rc=.o)
bins := $(patsubst %.o,$(BINDIR)/%.o,$(objs))

ifeq ($(platform), mingw)
  # ensure avoidance of trampolines (a gcc mechanism to implement 
  # some nested functions) as they cause some security software to choke
  #SECUR=-Wtrampolines
	CFLAGS += -DMINGW
	MWLIB =  -lwsock32
endif
$(info make for $(TARGET) $(platform) $(CFLAGS))
ifneq ($(platform), msys)
  # ensure avoidance of trampolines (a gcc mechanism to implement 
  # some nested functions) as they cause some security software to choke
  #SECUR=-Wtrampolines
endif
# support cygwin debug package (use += to accept injected options)
#-Wall 
CFLAGS +=  -std=gnu99  -Wall -Wextra -Wundef  \
					-Werror  -Wno-stringop-truncation -Wno-stringop-overflow
# verify additional flags that are also injected into pkg build by cygport
CFLAGS += -Wformat-security

#ifeq ($(shell VER=`$(CC) -dumpversion`; expr $${VER%%.*} '>=' 5), 1)
#  CFLAGS += -mtune=atom $(SECUR)
#endif
CFLAGS += -mtune=atom $(SECUR)

scriptlib=-lusp10
#downldlib=-lurlmon
#downldlib="${SYSTEMROOT}/System32/urlmon.dll"
downldlib=
ifneq ($(platform), msys)
  emojilib=-lgdiplus
endif
extralibs=$(scriptlib) $(downldlib) $(emojilib) -lshlwapi
LDFLAGS := -L$(shell cd `$(CC) -print-file-name=w32api`;pwd ) $(LDFLAGS) -static-libgcc
LDLIBS := -mwindows $(extralibs) $(MWLIB) -lcomctl32 -limm32 -lwinmm -lwinspool -lole32 -luuid -lmpr
#override LDLIBS += -mwindows $(extralibs) -lcomctl32 -limm32 -lwinmm -lwinspool -lole32 -luuid


ifdef DMALLOC
  CPPFLAGS += -DDMALLOC
  LDLIBS += -ldmallocth
endif

#############################################################################
# build

GFLAGS =  -include std.hh -I$(BINDIR)/ 

DEPOPT=-MMD 

CCOPT=
CCFLAGS:=$(CFLAGS) $(DFLAGS) $(CCOPT)

GCHF=$(BINDIR)/std.hh.gch

$(GCHF): std.h stamp $(BINDIR)/cfck.t
	$(CC) -c $(DEPOPT) $(CCFLAGS) $(CPPFLAGS) -fstack-check $< -o $@

$(objs): $(GCHF)

$(BINDIR)/term%.o: term%.c $(GCHF)
	$(CC) -c $(DEPOPT) $(CCFLAGS) $(GFLAGS) $(CPPFLAGS) $< -o $(BINDIR)/term$*.o

$(BINDIR)/term.o: term.c emojibase.t emojiseqs.t $(GCHF)
	$(CC) -c $(DEPOPT) $(CCFLAGS) $(GFLAGS) $(CPPFLAGS) -fstack-check $< -o $(BINDIR)/term.o

# do not optimize wintext.c; this causes a mysterious delay if 
# win_char_width is called with a non-BMP character
#$(BINDIR)/wintext.o: wintext.c
#	$(CC) -c $(DEPOPT) $(CCFLAGS) $(GFLAGS) -O0 $(CPPFLAGS) -fstack-check $< -o $(BINDIR)/wintext.o

$(BINDIR)/appinfo.o: appinfo.c appinfo.h appinfo.t
	$(CC) -c $(DEPOPT) -O0 $(CPPFLAGS) -fstack-check $< -o $(BINDIR)/appinfo.o

$(BINDIR)/%.o: %.c  $(GCHF)
	$(CC) -c $(DEPOPT) $(CCFLAGS) $(GFLAGS) $(CPPFLAGS) -fstack-check $< -o $(BINDIR)/$*.o

$(BINDIR)/%.o: %.rc %.h %.mft
	#$(RC) -c 65001 --preprocessor '$(CC) -E -xc -DRC_INVOKED $(DEPOPT) $(CPPFLAGS)' $< $(BINDIR)/$*.o
	# broken in binutils 2.36; workaround:
	$(CC) -E -xc -DRC_INVOKED $(DEPOPT) $(CPPFLAGS) $< | $(RC) -c 65001 -o $(BINDIR)/$*.o
	-mv $*.d $(BINDIR)/



stamp:$(BINDIR)/cfck.t cdef.t appinfo.t 
cdef.t:cdef
cdef: configdef.h
	gcc -DCFGDEFT=DEFVAR -P -E $<|sed '/^ *$$/d'>$@ ;\
	[ -e $@.t ] || /bin/cp $@  $@.t ;\
	cmp -s $@  $@.t || /bin/cp $@  $@.t

$(BINDIR)/cfck.t:$(BINDIR)/cfck
$(BINDIR)/cfck:Makefile
	@echo $(DEPOPT) $(CCFLAGS) $(CPPFLAGS) > $@ ;\
	[ -e $@.t ] || /bin/cp $@  $@.t ;\
	cmp -s $@  $@.t || /bin/cp $@  $@.t

../VERSION:appinfo.h
	echo VERSION | cpp -P $(CPPFLAGS) --include $< |sed "s/\"//g" > $@
appinfo.t:appinfo
#gitversion := $(shell ./mkvertag "$(srcversion)")
#		"$(cat VERSION) $$(date +%Y-%m-%d_%H:00)" 
appinfo:$(c_srcs) $(h_srcs) $(rc_srcs) ../VERSION
	@gv=`./mkvertag $(srcversion)` ;\
	echo "#define VERSION_STAMP \"$$gv\"" > $@ ;\
	[ -e $@.t ] || /bin/cp $@  $@.t ;\
	cmp -s $@  $@.t || /bin/cp $@  $@.t


$(BINDIR)/cfck.t:$(BINDIR)/cfck

exe := $(NAME).exe
exe: $(exe)
$(exe): $(objs)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) $(LDOPT) -o $@

bin := $(BINDIR)/$(NAME).exe
bin: $(BINDIR) stamp $(bin) #$(BINFOLDER)/minttyhook.dll
$(BINDIR):
	mkdir -p $(BINDIR)
$(bin): $(bins)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) $(LDOPT) -o $@
	cp -f $@ $(BINFOLDER)/		
#must use mingw32-gcc ,otherwise ,cannot hook normal program
#src file cannot use .c ,for c_srcs := $(wildcard *.c)
# so need option "-x c"
$(BINFOLDER)/minttyhook.dll:minttyhook.ca
	x86_64-w64-mingw32-gcc --share $(DFLAGS) -s -x c $^ -o $@
	#x86_64-w64-mingw32-gcc --share -DNDEBUG -nostartfiles -s -Os -Wl,-e,DllMain -x c $^ -o $@
-include $(wildcard $(BINDIR)/*.d)

#############################################################################
# generate

#WGET=wget -N -t 1 --timeout=55
WGET=curl -R -O --connect-timeout 55
WGET+=-z $@

clean-x11:
	rm -f rgb.t composed.t

rgb.t:	# /usr/share/X11/rgb.txt # X11 color names, from package 'rgb'
#	255 250 250		snow
#	->
#		{255, 250, 250, "snow"},
	sed -e 's,	, ,g' -e 's/ *\([0-9][0-9]*\) *\([0-9][0-9]*\) *\([0-9][0-9]*\) *\([^ ].*[^ ]\) */	{\1, \2, \3, "\4"},/' /usr/share/X11/rgb.txt > rgb.t

combdata=combining.t combdouble.t combined.t formatting.t
unidata=$(combdata) ambiguous.t scripts.t blocks.t casefold.t bidiclasses.t mirroring.t brackets.t canonical.t wide.t unicodever.t
emojidata=emojibase.t emojiseqs.t

unicodedata=UnicodeData.txt EastAsianWidth.txt Blocks.txt Scripts.txt CaseFolding.txt BidiMirroring.txt BidiBrackets.txt BidiCharacterTest.txt BidiTest.txt NameAliases.txt
unicodeemoji=emoji-data.txt emoji-sequences.txt emoji-variation-sequences.txt emoji-zwj-sequences.txt emoji-test.txt

clean-unicode:
	rm -f $(unidata) $(emojidata)

clean-unicode-data:	clean-unicode
	rm -f $(unicodedata) $(unicodeemoji)

unicode-update:	clean-unicode-data bin

combining.t:	# UnicodeData.txt Blocks.txt
	uniset +cat=Me +cat=Mn +cat=Cf -00AD +1160-11FF +200B +D7B0-D7C6 +D7CB-D7FB c > combining.t
	# xterm 373:
	#uniset +cat=Me +cat=Mn +0600-0605 +061C +06DD +070F +1160-11FF +D7B0-D7C6 +D7CB-D7FB c > combining.t

formatting.t:	# UnicodeData.txt Blocks.txt
	# xterm 373:
	#uniset +cat=Cf -00AD -0600-0605 -061C -06DD -070F c > formatting.t

ambiguous.t:	# WIDTH-A # UnicodeData.txt Blocks.txt
	sh ./mkwidthA
	uniset +WIDTH-A -cat=Me -cat=Mn -cat=Cf c > ambiguous.t


combdouble.t:	# UnicodeData.txt Blocks.txt
	combdouble=$(sed -e "s,^\([^;]*\);.*Mn;23[34];.*, +\1," -e t -e d UnicodeData.txt );\
	uniset $(combdouble) c > combdouble.t

WIDTH-A:	# UnicodeData.txt Blocks.txt EastAsianWidth.txt
	sh ./mkwidthA

wide.t:	# UnicodeData.txt Blocks.txt EastAsianWidth.txt
	sh ./mkwide

scripts.t:	# Scripts.txt
	sh ./mkscripts

blocks.t:	# Blocks.txt
	sh ./mkblocks

casefold.t:	# CaseFolding.txt
	sh ./mkcasefold > casefold.t

combined.t:	# UnicodeData.txt
	sh ./mkcombinedt > combined.t

bidiclasses.t:	# UnicodeData.txt
	sh ./mkbidiclasses > bidiclasses.t

mirroring.t:	# BidiMirroring.txt
	sh ./mkmirroring > mirroring.t

brackets.t:	# BidiBrackets.txt
	sh ./mkbrackets

canonical.t:	# BidiBrackets.txt UnicodeData.txt
	sh ./mkbrackets

testbidi.exe:	testbidi.tc minibidi.c # BidiCharacterTest.txt BidiTest.txt
	$(CC) -x c testbidi.tc -DTEST_BIDI --include std.h minibidi.c -o testbidi.exe

unicode:	UnicodeData.txt Blocks.txt EastAsianWidth.txt

#full-emoji-list.html:
#	$(WGET) http://www.unicode.org/emoji/charts/$@

univer=$(shell cat univer)

# Emoji version may advance Unicode version
#emojiver = $(unibasever)
# extract Emoji version from emoji-data.txt
emojiver = `sed -e 's,^\#.*Emoji Version \([0-9][0-9.]*\).*,\1,' -e t -e d emoji-data.txt`
# override Emoji version if not in sync
##emojiver = 15.0


# until 12.0:
#unibasever = `echo $(univer )|sed -e 's,[^0-9]*\([0-9]*\)\..*,\1.0,'`
#emoji-%.txt:	Blocks.txt
#	$(WGET) http://www.unicode.org/Public/emoji/$(unibasever)/$@
# from 13.0:
emoji-sequences.txt:	Blocks.txt
	$(WGET) http://www.unicode.org/Public/emoji/$(emojiver)/$@
emoji-zwj-sequences.txt:	Blocks.txt
	$(WGET) http://www.unicode.org/Public/emoji/$(emojiver)/$@
emoji-test.txt:	Blocks.txt
	$(WGET) http://www.unicode.org/Public/emoji/$(emojiver)/$@
emoji-data.txt:	Blocks.txt
	$(WGET) http://www.unicode.org/Public/$(univer)/ucd/emoji/$@
emoji-variation-sequences.txt:	Blocks.txt
	$(WGET) http://www.unicode.org/Public/$(univer)/ucd/emoji/$@
Blocks.txt:	
	vert = `sed -e '/^\# Blocks-/ s,[^0-9]*\([0-9]*\)\.\([0-9]\)\.\([0-9]\)[^0-9]*,\1.\2.\3,' -e t -e d Blocks.txt`;\
	if[ $(univer) != $$vert ];then\
		$(WGET) http://www.unicode.org/Public/$(univer)/ucd/$@ \
	fi

releasever:
	sed -e 's,\(Mintty .*\)`.*`,\1`$(srcversion)`,' -i ../wiki/Versions.md

unicodever.t:	Blocks.txt
	echo $(univer)|sed -e 's,[^0-9]*\([0-9]*\)\.\([0-9]\)\.\([0-9]\)[^0-9]*,#define UNICODE_VERSION \1\2\3,'` >$@
	sed -e 's,\(Unicode .*\)`.*`,\1`$(univer)`,' -i ../wiki/Versions.md

univer:	Blocks.txt
	@echo univer: $(univer)
	@echo unicodever: $(univer)
	@echo emojiver: $(emojiver)
att:
%.txt:
	$(WGET) http://www.unicode.org/Public/UNIDATA/$@

emojibase.t: $(unicodeemoji)	
	bash ./mkemojis base > $@

emojiseqs.t: $(unicodeemoji)	
	bash ./mkemojis seqs > $@

#compose_list=/usr/share/X11/locale/en_US.UTF-8/Compose
#keysymdefs=/usr/include/X11/keysymdef.h

composed.t:	# $(compose_list) $(keysymdefs)
	sh ./mkcomposedt > composed.t
	x11ver = `cygcheck -f /usr/share/X11/locale/en_US.UTF-8/Compose | sed -e "s,^[^-]*-,," -e "s,-.*,,"`;\
	sed -e 's,\(X11 .*\)`.*`,\1`$$x11ver`,' -i ../wiki/Versions.md

XGETTEXT_OPTS=--package-name=mintty #--package-version=$(srcversion)
XGETTEXT_OPTS+=--msgid-bugs-address=https://github.com/mintty/mintty/issues/700
pot_srcs=$(c_srcs) appinfo.h configdef.h funtips.h defkeyfun.h 

_:	localization #checkloc
localization:	../lang/messages ../lang/zh_CN.po

../lang/messages:$(pot_srcs) 
	@mkdir -p ../lang
	xgettext --from-code=UTF-8 -k_ -k_W -k__ -c__ $(XGETTEXT_OPTS) $(pot_srcs) *.c -o- \
		| grep -v "^#, c-format" > $@ 
	@cd ../lang; if ! diff -I POT-Creation-Date $@.pot $@ > /dev/null; then\
	 	/bin/cp $@ $@.pot;  fi

../lang/zh_CN.po:../lang/messages.pot
	@cd ../lang; for po in *.po; do if [ "$$po" != en.po ] ;then \
		printf "%8s " "$$po"; msgmerge -U "$$po" messages.pot; fi;done
	@touch ../lang/zh_CN.po

checkloc:
	@echo checkloc
	cd ../lang; ! grep "^#, fuzzy" *.po

#############################################################################
# release targets

DIST := ../rel

ifndef RELEASE
RELEASE=0
endif

ifdef RELEASE
pkg := $(name_ver)-$(RELEASE)
cygport := ../cygwin/mintty.cygport
pkg: $(pkg)
$(pkg): $(cygport) $(src)
	cp $(cygport) $(pkg).cygport
	cygport $(cygport_opts) $(pkg).cygport almostall
endif

zip := $(DIST)/$(name_ver)-$(platform).zip
zip: $(zip)
$(zip): $(exe) $(zip_files)
	mkdir -p $(DIST)
	zip -9 -j $@ $^
	#-du -b $@

pdf := $(DIST)/$(name_ver).pdf
pdf: $(pdf)
$(pdf): ../docs/$(NAME).1
	(echo .fp 5 CR; echo .fp 7 CB; cat $<) | groff -t -man -Tps | ps2pdf - $@

html := ../docs/$(NAME).1.html
html: $(html)
$(html): ../docs/$(NAME).1 htmlroff.sed html.sed
	sed -f htmlroff.sed $< | groff -t -man -mwww -Thtml | sed -f htmlhtml.sed | tr -d '\003\004' > $@

clean:
	#rm -rf *.d *.o $(NAME)*
	rm -rf $(BINDIR)/*.d $(BINDIR)/*.o $(BINDIR)/$(NAME)*

check:	checksrc checkresource checkloc

# check whether supportedOS Id list is up-to-date
stripsupp=sed -e "/supportedOS/ s,>.*,>," -e t -e d
defsupp=/usr/lib/default-manifest.o
checkresource:
	echo checkresource
	$(stripsupp) res.mft > .osmin
	cat "$(defsupp)" /dev/null | strings | $(stripsupp) > .osdef
	if [ -f "$(defsupp)" ]; then diff .osdef .osmin; else true; fi
	rm -f .osdef .osmin

# check whether sources are free of remaining debugging code
SRCDEBUG:= -e "^(f*printf|\#define debug)" # \# make-escape to #
SRCDEBUG+= -e "^(if|do|for|while) *[({]"
SRCDEBUG+= -e "\((false|0) *&"
SRCDEBUG+= -e "\((true|1) *\|"
#SRCDEBUG+= -e "& 0($|[^bx])"
SRCDEBUG+= -e "\(0 &"
SRCDEBUG+= -e "^///"
checksrc:
	echo checksrc
	if egrep -an $(SRCDEBUG) *.[hc]; then false; fi
	if sed -e "s,.*IDM_.*0x\([0-9A-F]*\) *,\1," -e t -e d winids.h | sort | uniq -d | grep .; then false; fi

#############################################################################
# development and debug targets

tags:	*.c *.h
	ctags -w *.c *.h

_wm.t:	# Windows Message names, only for debugging
#	#define WM_NULL 0x0000
#	->
#		WMN(WM_NULL)
	sed 's/^#define \(WM_[^ ]*\) *\(0x[0-9A-Fa-f]*\)$$/  WMN(\1)/;t;d' /usr/include/w32api/winuser.h|grep -vE "WM_KEYLAST|WM_KEYFIRST|WM_MOUSEFIRST|WM_MOUSELAST|WM_USER|WM_APP|WM_IME_KEYLAST"  > _wm.t
#	#	#define WM_NULL 0x0000
#	#	->
#	#		{0x0000, "WM_NULL"},
#	sed -e 's/^#define \(WM_[^ ]*\) *\(0x[0-9A-Fa-f]*\)$$/  {\2, "\1"},/' -e t -e d /usr/include/w32api/winuser.h | tr 'abcdef' 'ABCDEF' > _wm.t
#	# or, with include file copied from Windows SDK:
#	#sed -e 's/^#define \(WM_[^ ]*\) *\(0x[0-9A-Fa-f]*\)$$/  {\2, "\1"},/' -e t -e d etc/WinUser.h | tr 'abcdef' 'ABCDEF' > _wm.t

#	#define VK_TAB 0x09
#	->
#		{0x09, "VK_TAB"},
_vk.t:	# Windows Virtual Key Code names, only for debugging
	sed -e 's/^#define \(VK_[^ ]*\) \(0x[0-9A-Fa-f]*\)$$/	{\2, "\1"},/' -e t -e d /usr/include/w32api/winuser.h > _vk.t

_winidm.t:	winids.h
	sed -e '$$ d' -e '/#ifdef/ b' -e '/#endif/ b' -e 's/#define *\(IDM_[^ ]*\).*/	{\1, "\1"},/' -e t -e d winids.h > _winidm.t

_wstyles.t:	# Windows GWL_STYLE and GWL_EXSTYLE values
	./mkstyles > $@

target:
	echo TARGET $(TARGET)
	echo platform $(platform)
	echo BINDIR $(BINDIR)
	echo $(bins)

getappinfo=$(shell echo "$(1)" ; echo "$(1)" | cpp -P $(CPPFLAGS) --include appinfo.h)

ver:
	echo $(getappinfo)
	@echo "$(call getappinfo,VERSION)";\
	echo "$(call getappinfo,POINT_VERSION)";\
	echo "$(call getappinfo,COMMA_VERSION)";\
	echo "$(call getappinfo,DECIMAL_VERSION)";\
	echo src version $(srcversion);\
	echo version_stamp `./mkvertag "$(srcversion)"`

#############################################################################
# end
