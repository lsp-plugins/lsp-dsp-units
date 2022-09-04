#
# Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
#           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#------------------------------------------------------------------------------
# List of common dependencies
DEPENDENCIES = \
  LSP_COMMON_LIB \
  LSP_DSP_LIB \
  LSP_LLTL_LIB \
  LSP_RUNTIME_LIB

TEST_DEPENDENCIES = \
  LSP_TEST_FW

#------------------------------------------------------------------------------
# Linux dependencies
LINUX_DEPENDENCIES = \
  LIBPTHREAD \
  LIBDL \
  LIBSNDFILE

LINUX_TEST_DEPENDENCIES =

ifeq ($(PLATFORM),Linux)
  DEPENDENCIES             += $(LINUX_DEPENDENCIES)
  TEST_DEPENDENCIES        += $(LINUX_TEST_DEPENDENCIES)
endif

#------------------------------------------------------------------------------
# BSD dependencies
BSD_DEPENDENCIES = \
  LIBPTHREAD \
  LIBDL \
  LIBICONV \
  LIBSNDFILE

BSD_TEST_DEPENDENCIES =

ifeq ($(PLATFORM),BSD)
  DEPENDENCIES             += $(BSD_DEPENDENCIES)
  TEST_DEPENDENCIES        += $(BSD_TEST_DEPENDENCIES)
endif

#------------------------------------------------------------------------------
# Windows dependencies
WINDOWS_DEPENDENCIES = \
  LIBSHLWAPI \
  LIBWINMM \
  LIBMSACM

WINDOWS_TEST_DEPENDENCIES =

ifeq ($(PLATFORM),Windows)
  DEPENDENCIES             += $(WINDOWS_DEPENDENCIES)
  TEST_DEPENDENCIES        += $(WINDOWS_TEST_DEPENDENCIES)
endif

#------------------------------------------------------------------------------
# Overall system dependencies
ALL_DEPENDENCIES = \
  $(DEPENDENCIES) \
  $(LINUX_DEPENDENCIES) \
  $(BSD_DEPENDENCIES) \
  $(WINDOWS_DEPENDENCIES) \
  $(TEST_DEPENDENCIES) \
  $(LINUX_TEST_DEPENDENCIES) \
  $(BSD_TEST_DEPENDENCIES) \
  $(WINDOWS_TEST_DEPENDENCIES)




