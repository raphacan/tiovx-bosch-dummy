/*

 * Copyright (c) 2015-2017 The Khronos Group Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*
 * Copyright (c) 2022 Texas Instruments Incorporated
 */

#include <VX/vx.h>
#include <VX/vxu.h>
#include <TI/tivx_config.h>

#include "test_engine/test.h"

TESTCASE(tivxLog, CT_VXContext, ct_setup_vx_context, 0)

TEST(tivxLog, negativeTestRegisterLogCallback)
{
    vx_context context = context_->vx_context_;

    vx_log_callback_f callback = NULL;
    vx_bool reentrant = 0;

    vxRegisterLogCallback(NULL, callback, reentrant);
}

TEST(tivxLog, negativeTestAddLogEntry)
{
    vx_context context = context_->vx_context_;

    vx_reference ref = NULL;
    vx_status status = VX_SUCCESS;

    vxAddLogEntry(ref, status, NULL);
    vxAddLogEntry((vx_reference)(context), status, NULL);
    vxAddLogEntry((vx_reference)(context), VX_ERROR_INVALID_VALUE, NULL);
}

TESTCASE_TESTS(
    tivxLog,
    negativeTestRegisterLogCallback,
    negativeTestAddLogEntry
)

