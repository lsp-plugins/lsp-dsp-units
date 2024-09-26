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

# Deduplicates all strings in the list
# $(call uniq, <list>)
# $(call uniq, $(DEPENDENCIES))
uniq                    = $(if $1,$(firstword $1) $(call uniq,$(filter-out $(firstword $1),$1)))

# Recursively lookup directory for specific file pattern
# $(call rwildcard, <path>, <file-name-pattern>)
# $(call rwildcard, main, *.cpp)
rwildcard               = $(foreach d,$(wildcard $1/*),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

# Fetch different flags from symbolic dependencies
# $(call query, <field>, <list>)
# $(call query, CFLAGS, $(DEPENDENCIES))
query                   = $(foreach d,$(call uniq, $2),$($(d)_$(strip $1)))

# Fetch conditionally if dependency field is present
# $(call dquery, <field>, <list>)
# $(call dquery, OBJ, $(DEPENDENCIES))
dquery                  = $(foreach d,$(call uniq, $2),$(if $($(d)_$(strip $1)),$(d)))

# Fetch different flags from symbolic dependencies
# $(call cquery, <test-field>, <return-field>, <list>)
# $(call cquery, OBJ_META, BIN, $(DEPENDENCIES))
cquery                  = $(foreach d,$(call uniq, $3),$(if $($(d)_$(strip $1)),$($(d)_$(strip $2))))

# Find intersection between two sets
# $(call intersection, list1, list2)
intersection            = $(sort $(foreach v,$1,$(if $(findstring $(v),$2),$(v))))

# Subtract the first set from second set
# $(call subtraction, list1, list2)
subtraction             = $(sort $(foreach v,$2,$(if $(findstring $(v),$1),,$(v))))

# Check feature presence in list
# $(call fcheck, features-to-check, all-feature-list, action-if-enabled, action-if-disabled)
fcheck                  = $(if $(call intersection,$1,$2),$3,$4)

# Fetch different versions from version string
# $(call vmajor, <version-string>)
vmajor                  = $(shell echo "$(strip $1)" | sed -E 's/([0-9]+)\.([0-9]+)\.([0-9]+)(-(.*))?/\1/')
vminor                  = $(shell echo "$(strip $1)" | sed -E 's/([0-9]+)\.([0-9]+)\.([0-9]+)(-(.*))?/\2/')
vmicro                  = $(shell echo "$(strip $1)" | sed -E 's/([0-9]+)\.([0-9]+)\.([0-9]+)(-(.*))?/\3/')
vbranch                 = $(shell echo "$(strip $1)" | sed -E 's/([0-9]+)\.([0-9]+)\.([0-9]+)(-(.*))?/\5/')

ifeq ("$(MSYSTEM)","")
  pathconv                = $1
else
  pathconv                = $(shell cygpath -w "$1")
endif
