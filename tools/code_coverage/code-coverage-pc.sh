#!/bin/sh

#
# Utility script to generate TIOVX code coverage reports at the location /tmp/tiovx_ccov
#
# The script will do the following:
# 1. Scrub build the SDK for PC emulation with the binaries instrumented
# 2. Run the full conformance test suite
# 3. Generate a code coverage report from the test run
#
# Steps to run
# 1. Ensure you have installed lcov
# 2. Set the SDK_INSTALL_PATH to your workarea
# 3. Execute the script by running the below:
#    ./code-coverage-pc.sh
#

# You will also need to install lcov and genhtml prior to running this script
print_usage()
{
    echo "make sure you set SDK_INSTALL_PATH before running this script"
}

TIOVX_CCOV_DIR=/tmp/tiovx_ccov

build_for_test_coverage()
{
	cd ${SDK_INSTALL_PATH}/sdk_builder
	make sdk_scrub
	NUM_PROCS=$(cat /proc/cpuinfo | grep processor | wc -l)
	BUILD_EMULATION_MODE=yes BUILD_TARGET_MODE=no ENABLE_GCOV_PC_EMULATION=1 make sdk -j${NUM_PROCS}
}

run_for_test_coverage()
{
	export VX_TEST_DATA_PATH=${SDK_INSTALL_PATH}/tiovx/conformance_tests/test_data
	${SDK_INSTALL_PATH}/tiovx/out/PC/x86_64/LINUX/release/vx_conformance_tests_exe
}

gen_test_coverage()
{
	mkdir -p ${TIOVX_CCOV_DIR}
	lcov --rc lcov_branch_coverage=1 --capture --directory ${SDK_INSTALL_PATH}/tiovx/out/PC/x86_64/LINUX/release/ --output-file ${TIOVX_CCOV_DIR}/test_coverage.info
	lcov --rc lcov_branch_coverage=1 --remove ${TIOVX_CCOV_DIR}/test_coverage.info  "/usr/include/*" -o ${TIOVX_CCOV_DIR}/test_coverage_filtered.info
	genhtml --branch-coverage ${TIOVX_CCOV_DIR}/test_coverage_filtered.info --output-directory ${TIOVX_CCOV_DIR}
}

if [ -z "${SDK_INSTALL_PATH}" ]; then
	print_usage
else
	build_for_test_coverage
	run_for_test_coverage
	gen_test_coverage
fi
