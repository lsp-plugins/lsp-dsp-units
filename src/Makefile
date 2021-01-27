#
# Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
#           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
#
# This file is part of lsp-tk-lib
#
# lsp-tk-lib is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# any later version.
#
# lsp-tk-lib is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with lsp-tk-lib.  If not, see <https://www.gnu.org/licenses/>.
#

BASEDIR                 = $(CURDIR)
CONFIG                 := $(CURDIR)/.config.mk

include $(CONFIG)
include $(BASEDIR)/../project.mk
include $(BASEDIR)/../make/functions.mk

ifeq ($(TEST),1)
  UNIQ_DEPENDENCIES      := $(call uniq, $(DEPENDENCIES) $(TEST_DEPENDENCIES))
  DEPENDENCIES            = $(UNIQ_DEPENDENCIES)
endif

# Artifact settings
ARTIFACT_BIN            = $($(ARTIFACT_ID)_BIN)
ARTIFACT_NAME           = $($(ARTIFACT_ID)_NAME)
ARTIFACT_VERSION        = $($(ARTIFACT_ID)_VERSION)

ARTIFACT_TEST_BIN       = $(ARTIFACT_BIN)/$(ARTIFACT_NAME)-test$(EXECUTABLE_EXT)
ARTIFACT_LIB            = $(ARTIFACT_BIN)/$(LIBRARY_PREFIX)$(ARTIFACT_NAME)-$(ARTIFACT_VERSION)$(LIBRARY_EXT)
ARTIFACT_SLIB           = $(ARTIFACT_BIN)/$(LIBRARY_PREFIX)$(ARTIFACT_NAME)-$(ARTIFACT_VERSION)$(STATICLIB_EXT)
ARTIFACT_LIBLINK        = $(LIBRARY_PREFIX)$(ARTIFACT_NAME)$(LIBRARY_EXT)
ARTIFACT_SLIBLINK       = $(LIBRARY_PREFIX)$(ARTIFACT_NAME)$(STATICLIB_EXT)
ARTIFACT_PC             = $(ARTIFACT_BIN)/$(ARTIFACT_NAME)$(PKGCONFIG_EXT)
ARTIFACT_OBJ            = $($(ARTIFACT_ID)_OBJ)
ARTIFACT_MFLAGS         = $($(ARTIFACT_ID)_MFLAGS) $(foreach dep,$(DEPENDENCIES),-DUSE_$(dep))
ARTIFACT_DEPS           = $(call dquery, OBJ, $(DEPENDENCIES))
ARTIFACT_CFLAGS         = $(call query, CFLAGS, $(DEPENDENCIES) $(ARTIFACT_ID))
ARTIFACT_LDFLAGS        = $(call query, LDFLAGS, $(DEPENDENCIES) $(ARTIFACT_ID))
ARTIFACT_OBJFILES       = $(call query, OBJ, $(DEPENDENCIES) $(ARTIFACT_ID))

ARTIFACT_TARGETS        = $(ARTIFACT_LIB) $(ARTIFACT_SLIB) $(ARTIFACT_PC)

# Source code
CXX_SRC_MAIN            = $(call rwildcard, main, *.cpp)
CXX_SRC_TEST            = $(call rwildcard, test, *.cpp)
CXX_SRC_EXT             =
CXX_SRC                 = $(CXX_SRC_MAIN) $(CXX_SRC_EXT)

CXX_OBJ_MAIN            = $(patsubst %.cpp, $($(ARTIFACT_ID)_BIN)/%.o, $(CXX_SRC_MAIN))
CXX_OBJ_TEST            = $(patsubst %.cpp, $($(ARTIFACT_ID)_BIN)/%.o, $(CXX_SRC_TEST))
CXX_OBJ_EXT             = $(patsubst %.cpp, $($(ARTIFACT_ID)_BIN)/%.o, $(CXX_SRC_EXT))
CXX_OBJ                 = $(CXX_OBJ_MAIN) $(CXX_OBJ_EXT)

ALL_HEADERS             = $(call rwildcard, $($(ARTIFACT_ID)_INC), *.h)
CXX_FILE                = $(patsubst $($(ARTIFACT_ID)_BIN)/%.o,%.cpp, $(@))
CXX_HDR_PATHS           = $(foreach hdr,$(ARTIFACT_HEADERS),$($(ARTIFACT_ID)_INC)/$(hdr))
CXX_HEADERS             = $(foreach path,$(CXX_HDR_PATHS),$(call rwildcard, $(path), *.h))
CXX_INSTHEADERS         = $(patsubst $($(ARTIFACT_ID)_INC)/%,$(DESTDIR)$(INCDIR)/%,$(CXX_HEADERS))

BUILD_ALL               = $(ARTIFACT_LIB) $(ARTIFACT_SLIB) $(ARTIFACT_PC)

ifeq ($($(ARTIFACT_ID)_TESTING),1)
  CXX_SRC                += $(CXX_SRC_TEST)
  CXX_OBJ                += $(CXX_OBJ_TEST)
  ARTIFACT_TARGETS       += $(ARTIFACT_TEST_BIN)
endif

DEP_CXX                 = $(foreach src,$(CXX_SRC_MAIN) $(CXX_SRC_EXT) $(CXX_SRC_TEST),$(patsubst %.cpp,$(ARTIFACT_BIN)/%.d,$(src)))
DEP_CXX_FILE            = $(patsubst $(ARTIFACT_BIN)/%.d,%.cpp,$(@))
DEP_DEP_FILE            = $(patsubst $(ARTIFACT_BIN)/%.d,%.o,$(@))

.DEFAULT_GOAL = all
.PHONY: compile depend dep_clean all install uninstall
.PHONY: $(ARTIFACT_DEPS)

# Dependencies
dep_clean:

$(DEP_CXX): dep_clean
	@echo "  dep  [$(ARTIFACT_NAME)] $(DEP_CXX_FILE)"
	@mkdir -p $(dir $(@))
	$(CXX) -MM -MT "\$$($(ARTIFACT_ID)_BIN)/$(DEP_DEP_FILE)" -MF $(@) $(DEP_CXX_FILE) $(CXXFLAGS) $(INCLUDE) $(ARTIFACT_CFLAGS)

depend: $(DEP_CXX)
	$(foreach dep,$(DEPENDENCIES) $(ARTIFACT_ID),\
	  $(if $($(dep)_INC), \
	    $(eval SED_RULES += s/$(shell echo "$($(dep)_INC)" | sed "s/\\//\\\\\//g")/\$$$$\\($(dep)_INC\\)/g;)\
	  )\
	  $(if $($(dep)_BIN), \
	    $(eval SED_RULES += s/$(shell echo "$($(dep)_BIN)" | sed "s/\\//\\\\\//g")/\\$$$$\\($(dep)_BIN\\)/g;)\
	  )\
	)
	@cat $(DEP_CXX) | sed -E "$(SED_RULES)" >Makefile.d

$(ARTIFACT_DEPS):
	@echo "make $(notdir $($(@)_OBJ))"
	@$(MAKE) -s -C "$($(@)_PATH)" compile CONFIG="$(CONFIG)" DEMO_TEST="0"

# Compilation
compile: $(ARTIFACT_OBJ)

$(CXX_OBJ_MAIN) $(CXX_OBJ_EXT) $(CXX_OBJ_TEST):
	@echo "  $(CXX)  [$(ARTIFACT_NAME)] $(CXX_FILE)"
	@mkdir -p $(dir $@)
	@$(CXX) -o $(@) -c $(CXX_FILE) -fPIC $(CXXFLAGS) $(ARTIFACT_MFLAGS) $(EXT_FLAGS) $(INCLUDE) $(ARTIFACT_CFLAGS)
	
# Producing large object files
$(ARTIFACT_OBJ): $(CXX_OBJ)
	@echo "  $(LD)   [$(ARTIFACT_NAME)] $(notdir $(ARTIFACT_OBJ))"
	@$(LD) -o $(ARTIFACT_OBJ) -r $(CXX_OBJ)
	
# Linking
all: $(ARTIFACT_TARGETS)

$(ARTIFACT_LIB): $(ARTIFACT_DEPS) $(ARTIFACT_OBJ)
	@echo "  $(CXX)  [$(ARTIFACT_NAME)] $(notdir $(ARTIFACT_LIB))"
	@$(CXX) -o $(ARTIFACT_LIB) $(ARTIFACT_OBJFILES) $(SO_FLAGS) $(ARTIFACT_LDFLAGS)
	
$(ARTIFACT_SLIB): $(ARTIFACT_DEPS) $(ARTIFACT_OBJ)
	@echo "  $(AR)   [$(ARTIFACT_NAME)] $(notdir $(ARTIFACT_SLIB))"
	@$(AR) rcs $(ARTIFACT_SLIB) $(ARTIFACT_OBJFILES)

$(ARTIFACT_TEST_BIN): $(ARTIFACT_DEPS) $(ARTIFACT_OBJ) $(ARTIFACT_TEST_OBJ)
	@echo "  $(CXX)  [$(ARTIFACT_NAME)] $(notdir $(ARTIFACT_TEST_BIN))"
	@$(CXX) -o $(ARTIFACT_TEST_BIN) $(ARTIFACT_OBJFILES) $(EXE_FLAGS) $(ARTIFACT_LDFLAGS)

$(ARTIFACT_PC):
	@mkdir -p $(dir $(ARTIFACT_PC))
	@echo "prefix=$(PREFIX)" > "$(ARTIFACT_PC)"
	@echo "exec_prefix=$(BINDIR)" >> "$(ARTIFACT_PC)"
	@echo "includedir=$(INCDIR)" >> "$(ARTIFACT_PC)"
	@echo "libdir=$(LIBDIR)" >> "$(ARTIFACT_PC)"
	@echo "" >> $(ARTIFACT_PC) >> "$(ARTIFACT_PC)"
	@echo "Name: $($(ARTIFACT_ID)_NAME)" >> "$(ARTIFACT_PC)"
	@echo "Description: $($(ARTIFACT_ID)_DESC)" >> "$(ARTIFACT_PC)"
	@echo "Version: $($(ARTIFACT_ID)_VERSION)" >> "$(ARTIFACT_PC)"
	@echo "Cflags: -I\$${includedir}" >> "$(ARTIFACT_PC)"
	@echo "Libs: -L\$${libdir} -l$(notdir $($(ARTIFACT_ID)_NAME))" >> "$(ARTIFACT_PC)"

# Installation/deinstallation
install: all
	@echo "Installing $($(ARTIFACT_ID)_NAME)"
	@mkdir -p "$(DESTDIR)$(INCDIR)"
	@mkdir -p "$(DESTDIR)$(LIBDIR)/pkgconfig"
	@cp -r "$(CXX_HDR_PATHS)" "$(DESTDIR)$(INCDIR)/"
	@cp $(ARTIFACT_PC) "$(DESTDIR)$(LIBDIR)/pkgconfig/"
	@echo $(INSTALL) $(ARTIFACT_LIB) -t "$(DESTDIR)$(LIBDIR)"
	@$(INSTALL) $(ARTIFACT_LIB) -t "$(DESTDIR)$(LIBDIR)"
	@echo cp $(ARTIFACT_SLIB) -t "$(DESTDIR)$(LIBDIR)"
	@cp $(ARTIFACT_SLIB) -t "$(DESTDIR)$(LIBDIR)"
	@ln -sf $(notdir $(ARTIFACT_LIB)) "$(DESTDIR)$(LIBDIR)/$(ARTIFACT_LIBLINK)"
	@ln -sf $(notdir $(ARTIFACT_SLIB)) "$(DESTDIR)$(LIBDIR)/$(ARTIFACT_SLIBLINK)"
	@echo "Install OK"

uninstall:
	@echo "Uninstalling $($(ARTIFACT_ID)_NAME)"
	@-rm -f "$(DESTDIR)$(LIBDIR)/$(notdir $(ARTIFACT_LIB))"
	@-rm -f "$(DESTDIR)$(LIBDIR)/$(notdir $(ARTIFACT_SLIB))"
	@-rm -f "$(DESTDIR)$(LIBDIR)/$(ARTIFACT_LIBLINK)"
	@-rm -f "$(DESTDIR)$(LIBDIR)/$(ARTIFACT_SLIBLINK)"
	@-rm -f "$(DESTDIR)$(LIBDIR)/pkgconfig/$(notdir $(ARTIFACT_PC))"
	@-rm -f $(CXX_INSTHEADERS)
	@echo "Uninstall OK"

# Dependencies
-include $(CURDIR)/Makefile.d
