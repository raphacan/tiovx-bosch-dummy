# Copyright (C) 2013 Texas Instruments
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

ifndef CGT7X_ROOT
$(error You must define CGT7X_ROOT!)
endif

# check for the supported CPU types for this compiler
ifeq ($(filter $(TARGET_FAMILY),DSP),)
$(error TARGET_FAMILY $(TARGET_FAMILY) is not supported by this compiler)
endif

# check for the support OS types for this compiler
ifeq ($(filter $(TARGET_OS),SYSBIOS NO_OS LINUX),)
$(error TARGET_OS $(TARGET_OS) is not supported by this compiler)
endif

ifeq ($(TARGET_OS),LINUX)
$(if $(C7X_UCLINUX_ROOT),,$(error C7X_UCLINUX_ROOT must be defined to support LINUX on C7x targets)
endif

CC=$(CGT7X_ROOT)/bin/cl7x
CP=$(CGT7X_ROOT)/bin/cl7x
AS=$(CGT7X_ROOT)/bin/cl7x
AR=$(CGT7X_ROOT)/bin/ar7x
LD=$(CGT7X_ROOT)/bin/cl7x

ifdef LOGFILE
LOGGING:=&>$(LOGFILE)
else
LOGGING:=
endif

OBJ_EXT=obj
LIB_PRE=
LIB_EXT=lib
# if an object is encountered as dsmo, make it a library.
ifeq ($(strip $($(_MODULE)_TYPE)),dsmo)
$(_MODULE)_TYPE := library
endif
ifeq ($(strip $($(_MODULE)_TYPE)),library)
	BIN_PRE=
	BIN_EXT=.$(LIB_EXT)
else ifeq ($(strip $($(_MODULE)_TYPE)),exe)
	BIN_PRE=
	BIN_EXT=.out
endif

$(_MODULE)_BIN  := $(TDIR)/$(BIN_PRE)$($(_MODULE)_TARGET)$(BIN_EXT)

#If linker file does not exist inside the module directory
#then expect it to be generated by some other module in the output target directory
$(_MODULE)_DEPS := $(foreach linkf,$(LINKER_FILES),$(if $(wildcard $(SDIR)/$(linkf)),$(SDIR)/$(linkf),$(TDIR)/$(linkf)))
$(_MODULE)_OBJS := $(ASSEMBLY:%.asm=$(ODIR)/%.obj) $(CPPSOURCES:%.cpp=$(ODIR)/%.obj) $(CSOURCES:%.c=$(ODIR)/%.obj)
# Redefine the local static libs and shared libs with REAL paths and pre/post-fixes
$(_MODULE)_STATIC_LIBS := $(foreach lib,$(STATIC_LIBS),$(TDIR)/$(LIB_PRE)$(lib).$(LIB_EXT))
$(_MODULE)_SHARED_LIBS := $(foreach lib,$(SHARED_LIBS),$(TDIR)/$(LIB_PRE)$(lib).$(LIB_EXT))
$(_MODULE)_COPT := --abi=eabi --gcc
$(_MODULE)_XDC_TARGET := ti.targets.elf.nda.C71

# Ensure the compilation rules syntactically match the rules generated by the compiler
$(_MODULE)_OBJS := $(call PATH_CONV,$($(_MODULE)_OBJS))

ifeq ($(TARGET_BUILD),debug)
$(_MODULE)_COPT += -g -D_DEBUG_=1
else ifneq ($(filter $(TARGET_BUILD),release production),)
$(_MODULE)_COPT += --opt_level=3 --gen_opt_info=2 -DNDEBUG
endif

ifeq ($(TARGET_CPU),C71)
$(_MODULE)_COPT += --silicon_version=7100 -D=xdc_target_name=C71
endif

ifneq ($(MISRA_RULES),) # If module specifies rules,
$(_MODULE)_MISRA := --check_misra=$(MISRA_RULES)
else ifeq ($(CHECK_MISRA),1) # else, check the environment variable
$(_MODULE)_MISRA := --check_misra=required
endif

ifeq ($(KEEP_ASM),1)
$(_MODULE)_COPT += --keep_asm
endif

ifeq ($(DEBUG_PIPELINE),1)
$(_MODULE)_COPT += --debug_software_pipeline
endif

ifeq ($(TARGET_OS),LINUX)
$(_MODULE)_COPT += --linux
$(_MODULE)_IDIRS += $(C7X_UCLINUX_ROOT)/libc/usr/include
$(_MODULE)_DEFS += __GNUC_MINOR__=3 __extensions__ __nothrow__ \
	__STDC__="unsigned int" __WINT_TYPE__="unsigned int" \
	__WCHAR_TYPE__="unsigned int" __gnuc_va_list=va_list
endif

$(_MODULE)_DEFS += xdc_target_name__=C71 xdc_target_types__=ti/targets/elf/nda/std.h
$(_MODULE)_CGT_ROOT = $(CGT7X_ROOT)

$(_MODULE)_MAP      := $($(_MODULE)_BIN).map
$(_MODULE)_INCLUDES := $(foreach inc,$($(_MODULE)_IDIRS),-I="$(basename $(inc))") $(foreach inc,$($(_MODULE)_SYSIDIRS),-I="$(basename $(inc))")
$(_MODULE)_DEFINES  := $(foreach def,$($(_MODULE)_DEFS),-D=$(def))
$(_MODULE)_LIBRARIES:= $(foreach ldir,$($(_MODULE)_LDIRS),--search_path="$(ldir)") $(foreach ldir,$($(_MODULE)_SYSLDIRS),--search_path="$(ldir)") $(foreach lib,$(STATIC_LIBS),--library=$(LIB_PRE)$(lib).$(LIB_EXT)) $(foreach lib,$(ADDITIONAL_STATIC_LIBS),--library=$(lib)) $(foreach lib,$(SYS_STATIC_LIBS),--library=$(LIB_PRE)$(lib).$(LIB_EXT)) $(foreach linkerf,$(LINKER_FILES),--library=$(linkerf))
$(_MODULE)_AFLAGS   := $($(_MODULE)_INCLUDES)
$(_MODULE)_CFLAGS   := $($(_MODULE)_INCLUDES) $($(_MODULE)_DEFINES) $($(_MODULE)_COPT) $(CFLAGS)
$(_MODULE)_LDFLAGS  := $($(_MODULE)_CFLAGS) -z --warn_sections --reread_libs --rom_model $($(_MODULE)_LINKER_CMD_FILES) $(CGT7X_ROOT)/lib/libc.a

###################################################
# COMMANDS
###################################################

$(_MODULE)_LINK_LIB   = $(call PATH_CONV,$(AR) ru2 $($(_MODULE)_BIN) $($(_MODULE)_OBJS) $($(_MODULE)_STATIC_LIBS))
$(_MODULE)_LINK_EXE   = $(call PATH_CONV,$(LD) $($(_MODULE)_LDFLAGS) $($(_MODULE)_OBJS) $($(_MODULE)_LIBRARIES) --output_file=$($(_MODULE)_BIN) --map_file=$($(_MODULE)_MAP))

###################################################
# MACROS FOR COMPILING
###################################################

define $(_MODULE)_BUILD
build:: $($(_MODULE)_BIN)
endef

define $(_MODULE)_COMPILE_TOOLS
$(call PATH_CONV,$(ODIR)$(PATH_SEP)%.$(OBJ_EXT)): $(SDIR)/%.c
	@echo [C7X] Compiling C $$(notdir $$<)
	$(Q)$$(call PATH_CONV,$(CC) $($(_MODULE)_CFLAGS) --preproc_dependency=$(ODIR)/$$*.dep --preproc_with_compile -fr=$$(dir $$@) -fs=$$(dir $$@) -ft=$$(dir $$@) -eo=.$(OBJ_EXT) -fc=$$< $(LOGGING)) $($(_MODULE)_MISRA)

$(call PATH_CONV,$(ODIR)$(PATH_SEP)%.$(OBJ_EXT)): $(SDIR)/%.cpp
	@echo [C7X] Compiling C++ $$(notdir $$<)
	$(Q)$$(call PATH_CONV,$(CP) $($(_MODULE)_CFLAGS) --preproc_dependency=$(ODIR)/$$*.dep --preproc_with_compile -fr=$$(dir $$@) -fs=$$(dir $$@) -ft=$$(dir $$@) -eo=.$(OBJ_EXT) -fp=$$< $(LOGGING))

$(call PATH_CONV,$(ODIR)$(PATH_SEP)%.$(OBJ_EXT)): $(SDIR)/%.asm
	@echo [C7X] Assembling $$(notdir $$<)
	$(Q)$$(call PATH_CONV,$(AS) $($(_MODULE)_AFLAGS) --preproc_dependency=$(ODIR)/$$*.dep --preproc_with_compile -fr=$$(dir $$@) -ft=$$(dir $$@) -eo=.$(OBJ_EXT) -fa=$$< $(LOGGING))

endef

ifeq ($(strip $($(_MODULE)_TYPE)),library)

define $(_MODULE)_BUILD
build:: $($(_MODULE)_BIN)
	@echo Building $$(notdir $$<) as static library
endef

else ifeq ($(strip $($(_MODULE)_TYPE)),dsmo)

define $(_MODULE)_BUILD

$($(_MODULE)_BIN): $($(_MODULE)_OBJS) $($(_MODULE)_STATIC_LIBS) $($(_MODULE)_SHARED_LIBS)
	@echo Linking $$@
	-$(Q)$(call $(_MODULE)_LINK_LIB) $(LOGGING)

build:: $($(_MODULE)_BIN)
	@echo Building $$(notdir $$<) as static library
endef

else ifeq ($(strip $($(_MODULE)_TYPE)),exe)

define $(_MODULE)_BUILD
build:: $($(_MODULE)_BIN)
	@echo Building $$(notdir $$<) as static library
endef

endif

