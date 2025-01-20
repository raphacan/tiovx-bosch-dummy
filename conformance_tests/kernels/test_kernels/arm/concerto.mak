
ifeq ($(TARGET_CPU), $(filter $(TARGET_CPU), X86 x86_64 C66 A15 M4 A72 A53 R5F C71 C7120 C7504 C7524))

include $(PRELUDE)
TARGET      := vx_target_kernels_source_sink
TARGETTYPE  := library
CSOURCES    := $(call all-c-files)
IDIRS       += $(TIOVX_PATH)/conformance_tests/kernels/include
IDIRS       += $(TIOVX_PATH)/conformance_tests/kernels/test_kernels/include
IDIRS       += $(VXLIB_PATH)/packages
IDIRS       += $(TIOVX_PATH)/source/include
IDIRS       += $(TIOVX_PATH)/source/platform/psdk_j7/common
IDIRS       += $(PSDK_PATH)/app_utils
IDIRS       += $(RB_VISION_APPS)
IDIRS       += ${TIOVX_PATH}/utils/include
IDIRS       += $(IPPC_PATH)
IDIRS       += $(APP_UTILS_PATH)

include $(FINALE)

endif
