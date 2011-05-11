#	Makefile
#
#	Copyright 2011 Michel Pollet <buserror@gmail.com>
#
#	This file is part of touchstream.
#
#	touchstream is free software: you can redistribute it and/or modify
#	it under the terms of the GNU General Public License as published by
#	the Free Software Foundation, either version 3 of the License, or
#	(at your option) any later version.
#
#	touchstream is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	GNU General Public License for more details.
#
#	You should have received a copy of the GNU General Public License
#	along with touchstream.  If not, see <http://www.gnu.org/licenses/>.

MARCH 		= $(shell $(CC) -dumpmachine|awk -F - '{print $$2;}')

EXTRA_LDFLAGS += -lpthread

VPATH 		= cmd
VPATH 		+= src

IPATH 		+= src

SHARED_SRC	= ${wildcard src/*.c}

ifeq ($(MARCH),apple)
#
# Apple specific flags
#
CC = clang -m32

# This requires X11.app installed
SHARED_SRC	+= ${wildcard src/xorg/*.c}
VPATH 		+= src/xorg
EXTRA_CFLAGS += -I/ust/X11/include
EXTRA_LDFLAGS += -L/usr/X11/lib -lX11 -lXtst -lXfixes -lXext

SHARED_SRC	+= ${wildcard src/osx/*.c}
VPATH 		+= src/osx
EXTRA_CFLAGS += -Wno-deprecated-declarations
EXTRA_CFLAGS += -F/System/Library/Frameworks/Carbon.framework/Frameworks/
EXTRA_CFLAGS += -DCONFIG_OSX=1
EXTRA_LDFLAGS += -framework Carbon

else
# Generic flags (aka linux)
SHARED_SRC	+= ${wildcard src/xorg/*.c}
VPATH 		+= src/xorg
EXTRA_CFLAGS += -DCONFIG_LINUX=1
EXTRA_LDFLAGS += -lX11 -lXtst -lXfixes -lXext
EXTRA_LDFLAGS += -Wl,--relax,--gc-sections
endif

EXTRA_CFLAGS += ${patsubst %,-I%,${subst :, ,${IPATH}}}


all: touchstream

include Makefile.common
	
SHARED_OBJ	:= ${patsubst %, ${OBJ}/%, ${notdir ${SHARED_SRC:.c=.o}}}

	
touchstream: ${OBJ} ${OBJ}/touchstream.bin
	@echo $@ Done

${OBJ}/touchstream.bin : ${OBJ}/touchstream.o
${OBJ}/touchstream.bin : ${SHARED_OBJ}


install: all
	cp ${OBJ}/touchstream.bin $(DESTDIR)/bin/ 
