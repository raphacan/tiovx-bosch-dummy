
ifeq ($(BUILD_HWA_DMPAC_SDE),yes)

include $(PRELUDE)
TARGET      := vx_target_kernels_dmpac_sde
TARGETTYPE  := library
CSOURCES    := $(call all-c-files)
IDIRS       += $(CUSTOM_KERNEL_PATH)/hwa/include
IDIRS       += $(HOST_ROOT)/kernels/include
IDIRS       += $(TDA4X_C_MODELS_PATH)/include
IDIRS       += $(VXLIB_PATH)/packages
DEFS        += TIOVX_SDE

ifeq ($(TARGET_CPU), $(filter $(TARGET_CPU), X86 x86_64))
endif

ifeq ($(TARGET_CPU),C66)
SKIPBUILD=1
endif

ifeq ($(TARGET_CPU),EVE)
SKIPBUILD=1
endif

ifeq ($(TARGET_CPU),A15)
SKIPBUILD=1
endif

ifeq ($(TARGET_CPU),M4)
SKIPBUILD=1
endif

include $(FINALE)

endif
# ifeq ($(BUILD_HWA_DMPAC_SDE),yes)
