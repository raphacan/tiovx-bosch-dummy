/*
 * Copyright (c) 2012-2016 The Khronos Group Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and/or associated documentation files (the
 * "Materials"), to deal in the Materials without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Materials, and to
 * permit persons to whom the Materials are furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Materials.
 *
 * MODIFICATIONS TO THIS FILE MAY MEAN IT NO LONGER ACCURATELY REFLECTS
 * KHRONOS STANDARDS. THE UNMODIFIED, NORMATIVE VERSIONS OF KHRONOS
 * SPECIFICATIONS AND HEADER INFORMATION ARE LOCATED AT
 *    https://www.khronos.org/registry/
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 */
/*
 *******************************************************************************
 *
 * Copyright (C) 2016 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 *******************************************************************************
 */

#include <vx_internal.h>
#include <tivx_platform_vision_sdk.h>

#include <xdc/std.h>
#include <include/link_api/system_if.h>
#include <include/link_api/system_procId.h>
#include <src/links_common/system/system_priv_openvx.h>

/*! \brief An array of vision sdk CPU Ids. This is used for mapping
 *   tivx_cpu_id_e to vision sdk cpu id. This mapping is required for
 *   sending notify event using vision sdk API.
 *   vx_enum tivx_cpu_id_e is used as index into this array to get
 *   vision sdk cpu id.
 * \ingroup group_tivx_ipc
 */
static uint32_t g_ipc_cpu_id_map[TIVX_CPU_ID_MAX] = {
    SYSTEM_PROC_DSP1,
    SYSTEM_PROC_DSP2,
    SYSTEM_PROC_EVE1,
    SYSTEM_PROC_EVE2,
    SYSTEM_PROC_EVE3,
    SYSTEM_PROC_EVE4,
    SYSTEM_PROC_IPU1_0,
    SYSTEM_PROC_IPU1_1,
    SYSTEM_PROC_IPU2,
    SYSTEM_PROC_A15_0
};

/*! \brief Pointer to the IPC notify event handler.
 *         It can be registered using #tivxIpcRegisterHandler API
 * \ingroup group_tivx_ipc
 */
static tivx_ipc_handler_f g_ipc_handler = NULL;


/*! \brief Global IPC handler
 * \ingroup group_tivx_ipc
 */
static void tivxIpcHandler(uint32_t payload)
{
    if (NULL != g_ipc_handler)
    {
        g_ipc_handler(payload);
    }
}

void tivxIpcRegisterHandler(tivx_ipc_handler_f notifyCb)
{
    g_ipc_handler = notifyCb;
}

vx_status tivxIpcSendMsg(
    vx_enum cpu_id, uint32_t payload)
{
    /* convert OpenVX CPU ID to VSDK CPU ID */
    uint32_t vsdk_cpu_id;
    vx_status status;

    if( cpu_id < TIVX_CPU_ID_MAX)
    {
        vsdk_cpu_id  = g_ipc_cpu_id_map[cpu_id];

        status = System_openVxSendNotify(
            vsdk_cpu_id,
            payload);

        if( status != VX_SUCCESS)
        {
            status = VX_ERROR_INVALID_PARAMETERS;
        }
    }
    else
    {
        status = VX_ERROR_INVALID_PARAMETERS;
    }

    return status;
}

vx_enum tivxGetSelfCpuId(void)
{
    vx_enum cpu_id = TIVX_INVALID_CPU_ID;
    uint32_t i, vsdk_cpu_id;

    vsdk_cpu_id =  System_getSelfProcId();

    for (i = 0; i < dimof(g_ipc_cpu_id_map); i ++)
    {
        if (vsdk_cpu_id == g_ipc_cpu_id_map[i])
        {
            cpu_id = (vx_enum)i;
            break;
        }
    }

    return (cpu_id);
}

void tivxIpcInit(void)
{
    /* Register IPC Handler */
    System_registerOpenVxNotifyCb(tivxIpcHandler);
}

void tivxIpcDeInit(void)
{
    /* Un-Register IPC Handler */
    System_registerOpenVxNotifyCb(NULL);
}
