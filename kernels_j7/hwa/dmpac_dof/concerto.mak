
ifeq ($(TARGET_CPU), $(filter $(TARGET_CPU), X86 x86_64 R5F))
ifeq ($(BUILD_HWA_KERNELS),yes)
ifeq ($(BUILD_DMPAC_DOF),yes)

include $(PRELUDE)
TARGET      := vx_target_kernels_dmpac_dof
TARGETTYPE  := library
CSOURCES    := vx_kernels_hwa_target.c
ifeq ($(TARGET_CPU),R5F)
  ifeq ($(BUILD_VLAB),yes)
    CSOURCES    += vx_dmpac_dof_target_sim.c
    IDIRS       += $(J7_C_MODELS_PATH)/include
  else
    CSOURCES    += vx_dmpac_dof_target.c
    ifeq ($(RTOS_SDK), mcu_plus_sdk)
    IDIRS       += $(MCU_PLUS_SDK_PATH)/source
    IDIRS       += $(MCU_PLUS_SDK_PATH)/source/drivers
    else
    IDIRS       += $(PDK_PATH)/packages
    IDIRS       += $(PDK_PATH)/packages/ti/drv
    endif
    IDIRS       += $(VISION_APPS_PATH)/
    ifeq ($(TARGET_OS),SYSBIOS)
      IDIRS       += $(XDCTOOLS_PATH)/packages
      IDIRS       += $(BIOS_PATH)/packages
    endif
  endif
else
CSOURCES    += vx_dmpac_dof_target_sim.c
IDIRS       += $(J7_C_MODELS_PATH)/include
endif
IDIRS       += $(CUSTOM_KERNEL_PATH)/hwa/include
IDIRS       += $(VXLIB_PATH)/packages

ifeq ($(TARGET_CPU)$(BUILD_VLAB),R5Fyes)
DEFS += VLAB_HWA
endif

include $(FINALE)

endif
endif
endif


