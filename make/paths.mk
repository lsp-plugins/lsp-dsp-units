#
# Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
#           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
#
# This file is part of lsp-dsp-units
#
# lsp-dsp-units is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# any later version.
#
# lsp-dsp-units is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with lsp-dsp-units.  If not, see <https://www.gnu.org/licenses/>.
#

# Installation prefix
ifndef PREFIX
  ifeq ($(PLATFORM),Windows)
    PREFIX                     := $(BASEDIR)/INSTALL
  else ifeq ($(CROSS_COMPILE),1)
    PREFIX                     := $(BASEDIR)/INSTALL
  else
    PREFIX                     := /usr/local
  endif
endif

# Path to configuration
ifndef ETCDIR
  ifeq ($(PLATFORM),Windows)
    ETCDIR                     := $(PREFIX)/etc
  else
    ETCDIR                     := /etc
  endif
endif

LIBDIR                     := $(PREFIX)/lib
BINDIR                     := $(PREFIX)/bin
INCDIR                     := $(PREFIX)/include
BUILDDIR                   := $(BASEDIR)/.build
TARGET_BUILDDIR            := $(BUILDDIR)/target
ifeq ($(CROSS_COMPILE),1)
  HOST_BUILDDIR              := $(BUILDDIR)/host
else
  HOST_BUILDDIR              := $(TARGET_BUILDDIR)
endif
MODULES                    := $(BASEDIR)/modules
CONFIG                     := $(BASEDIR)/.config.mk

# Library prefix
ifndef LIBDIR
  LIBDIR                   := $(PREFIX)/lib
endif

# Binaries prefix
ifndef BINDIR
  BINDIR                   := $(PREFIX)/bin
endif

# Binaries prefix
ifndef INCDIR
  INCDIR                   := $(PREFIX)/include
endif

# Shared resources
ifndef SHAREDDIR
  ifeq ($(PLATFORM),Haiku)
    SHAREDDIR                     := $(PREFIX)/data
  else
    SHAREDDIR                     := $(PREFIX)/share
  endif
endif

# Temporary directory
ifndef TEMPDIR
  ifeq ($(PLATFORM),Windows)
    TEMPDIR                  := $(TEMP)
  else
    TEMPDIR                  := /tmp
  endif
endif

# Set-up list of common variables
PATH_VARS = \
	BINDIR \
	BUILDDIR \
	ETCDIR \
	INCDIR \
	LIBDIR \
	PREFIX \
	ROOTDIR \
	SHAREDDIR \
	TEMPDIR

.PHONY: pathvars

pathvars:
	echo "List of available path variables:"
	echo "  BINDIR                    location of the binaries"
	echo "  BUILDDIR                  location of the build directory"
	echo "  ETCDIR                    location of system configuration files"
	echo "  INCDIR                    location of the header files"
	echo "  LIBDIR                    location of the library"
	echo "  PREFIX                    installation prefix for binary files"
	echo "  SHAREDDIR                 location of the shared files"
	echo "  TEMPDIR                   location of temporary directory"


