/*
*
* Copyright (c) 2017 Texas Instruments Incorporated
*
* All rights reserved not granted herein.
*
* Limited License.
*
* Texas Instruments Incorporated grants a world-wide, royalty-free, non-exclusive
* license under copyrights and patents it now or hereafter owns or controls to make,
* have made, use, import, offer to sell and sell ("Utilize") this software subject to the
* terms herein.  With respect to the foregoing patent license, such license is granted
* solely to the extent that any such patent is necessary to Utilize the software alone.
* The patent license shall not apply to any combinations which include this software,
* other than combinations with devices manufactured by or for TI ("TI Devices").
* No hardware patent is licensed hereunder.
*
* Redistributions must preserve existing copyright notices and reproduce this license
* (including the above copyright notice and the disclaimer and (if applicable) source
* code license limitations below) in the documentation and/or other materials provided
* with the distribution
*
* Redistribution and use in binary form, without modification, are permitted provided
* that the following conditions are met:
*
* *       No reverse engineering, decompilation, or disassembly of this software is
* permitted with respect to any software provided in binary form.
*
* *       any redistribution and use are licensed by TI for use only with TI Devices.
*
* *       Nothing shall obligate TI to provide you with source code for the software
* licensed and provided to you in object code.
*
* If software source code is provided to you, modification and redistribution of the
* source code are permitted provided that the following conditions are met:
*
* *       any redistribution and use of the source code, including any resulting derivative
* works, are licensed by TI for use only with TI Devices.
*
* *       any redistribution and use of any object code compiled from the source code
* and any resulting derivative works, are licensed by TI for use only with TI Devices.
*
* Neither the name of Texas Instruments Incorporated nor the names of its suppliers
*
* may be used to endorse or promote products derived from this software without
* specific prior written permission.
*
* DISCLAIMER.
*
* THIS SOFTWARE IS PROVIDED BY TI AND TI'S LICENSORS "AS IS" AND ANY EXPRESS
* OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL TI AND TI'S LICENSORS BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
* OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
* OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/


#include <TI/tivx.h>
#include <VX/vx.h>
#include <tivx_openvx_core_kernels.h>
#include <tivx_target_kernels_priv.h>
#include <tivx_kernel_meanstddev.h>
#include <TI/tivx_target_kernel.h>
#include <ti/vxlib/vxlib.h>
#include <tivx_kernels_target_utils.h>
#include <tivx_bam_kernel_wrapper.h>

typedef struct
{
    tivx_bam_graph_handle graph_handle;

} tivxMeanStdDevParams;

static tivx_target_kernel vx_meanstddev_target_kernel = NULL;

static vx_status VX_CALLBACK tivxKernelMeanStdDevProcess(
    tivx_target_kernel_instance kernel, tivx_obj_desc_t *obj_desc[],
    uint16_t num_params, void *priv_arg);

static vx_status VX_CALLBACK tivxKernelMeanStdDevCreate(
    tivx_target_kernel_instance kernel, tivx_obj_desc_t *obj_desc[],
    uint16_t num_params, void *priv_arg);

static vx_status VX_CALLBACK tivxKernelMeanStdDevDelete(
    tivx_target_kernel_instance kernel, tivx_obj_desc_t *obj_desc[],
    uint16_t num_params, void *priv_arg);

static vx_status VX_CALLBACK tivxKernelMeanStdDevProcess(
    tivx_target_kernel_instance kernel, tivx_obj_desc_t *obj_desc[],
    uint16_t num_params, void *priv_arg)
{
    vx_status status = VX_SUCCESS;
    tivxMeanStdDevParams *prms = NULL;
    uint32_t size;

    if (num_params != TIVX_KERNEL_MSD_MAX_PARAMS)
    {
        status = VX_FAILURE;
    }
    else
    {
        if ((NULL == obj_desc[TIVX_KERNEL_MSD_IN_IMG_IDX]) ||
            (NULL == obj_desc[TIVX_KERNEL_MSD_OUT_MEAN_IDX]) ||
            (NULL == obj_desc[TIVX_KERNEL_MSD_OUT_STDDEV_IDX]))
        {
            status = VX_ERROR_NO_MEMORY;
        }
    }

    if (VX_SUCCESS == status)
    {
        status = tivxGetTargetKernelInstanceContext(kernel,
            (void **)&prms, &size);

        if ((VX_SUCCESS != status) || (NULL == prms) ||
            (sizeof(tivxMeanStdDevParams) != size))
        {
            status = VX_FAILURE;
        }
    }

    if (VX_SUCCESS == status)
    {
        tivx_obj_desc_image_t *src;
        uint8_t *src_addr;
        tivx_obj_desc_scalar_t *sc[2U];
        VXLIB_F32 mean_val, stddev_val;
        void *img_ptrs[1];
        void *src_target_ptr;

        src = (tivx_obj_desc_image_t *)obj_desc[TIVX_KERNEL_MSD_IN_IMG_IDX];
        sc[0U] = (tivx_obj_desc_scalar_t*)obj_desc[TIVX_KERNEL_MSD_OUT_MEAN_IDX];
        sc[1U] = (tivx_obj_desc_scalar_t*)obj_desc[TIVX_KERNEL_MSD_OUT_STDDEV_IDX];

        src_target_ptr = tivxMemShared2TargetPtr(
            src->mem_ptr[0].shared_ptr, src->mem_ptr[0].mem_heap_region);

        tivxSetPointerLocation(src, &src_target_ptr, &src_addr);

        img_ptrs[0] = src_addr;
        tivxBamUpdatePointers(prms->graph_handle, 1U, 0U, img_ptrs);

        status  = tivxBamProcessGraph(prms->graph_handle);

        tivxBamControlNode(prms->graph_handle, 0,
                           VXLIB_MEANSTDDEV_I8U_O32F_CMD_GET_MEAN_VAL,
                           &mean_val);

        tivxBamControlNode(prms->graph_handle, 0,
                           VXLIB_MEANSTDDEV_I8U_O32F_CMD_GET_STDDEV_VAL,
                           &stddev_val);

        sc[0U]->data.f32 = mean_val;
        sc[1U]->data.f32 = stddev_val;
    }

    return (status);
}

static vx_status VX_CALLBACK tivxKernelMeanStdDevCreate(
    tivx_target_kernel_instance kernel, tivx_obj_desc_t *obj_desc[],
    uint16_t num_params, void *priv_arg)
{
    vx_status status = VX_SUCCESS;
    tivxMeanStdDevParams *prms = NULL;

    if (num_params != TIVX_KERNEL_MSD_MAX_PARAMS)
    {
        status = VX_FAILURE;
    }
    else
    {
        if ((NULL == obj_desc[TIVX_KERNEL_MSD_IN_IMG_IDX]) ||
            (NULL == obj_desc[TIVX_KERNEL_MSD_OUT_MEAN_IDX]) ||
            (NULL == obj_desc[TIVX_KERNEL_MSD_OUT_STDDEV_IDX]))
        {
            status = VX_ERROR_NO_MEMORY;
        }
    }

    if (VX_SUCCESS == status)
    {
        tivx_obj_desc_image_t *src;

        src = (tivx_obj_desc_image_t *)obj_desc[TIVX_KERNEL_MSD_IN_IMG_IDX];

        prms = tivxMemAlloc(sizeof(tivxMeanStdDevParams), TIVX_MEM_EXTERNAL);

        if (NULL != prms)
        {
            tivx_bam_kernel_details_t kernel_details;
            VXLIB_bufParams2D_t vxlib_src;
            VXLIB_bufParams2D_t *buf_params[1];

            memset(prms, 0, sizeof(tivxMeanStdDevParams));

            tivxInitBufParams(src, &vxlib_src);

            /* Fill in the frame level sizes of buffers here. If the port
             * is optionally disabled, put NULL */
            buf_params[0] = &vxlib_src;

            kernel_details.compute_kernel_params = NULL;

            BAM_VXLIB_meanStdDev_i8u_o32f_getKernelInfo( NULL,
                                                   &kernel_details.kernel_info);

            status = tivxBamCreateHandleSingleNode(BAM_KERNELID_VXLIB_MEAN_STDDEV_I8U_O32F,
                                                   buf_params, &kernel_details,
                                                   &prms->graph_handle);

        }
        else
        {
            status = VX_ERROR_NO_MEMORY;
        }

        if (VX_SUCCESS == status)
        {
            tivxSetTargetKernelInstanceContext(kernel, prms,
                sizeof(tivxMeanStdDevParams));
        }
        else
        {
            if (NULL != prms)
            {
                tivxMemFree(prms, sizeof(tivxMeanStdDevParams), TIVX_MEM_EXTERNAL);
            }
        }
    }

    return status;
}

static vx_status VX_CALLBACK tivxKernelMeanStdDevDelete(
    tivx_target_kernel_instance kernel, tivx_obj_desc_t *obj_desc[],
    uint16_t num_params, void *priv_arg)
{
    vx_status status = VX_SUCCESS;
    uint32_t size;
    tivxMeanStdDevParams *prms = NULL;

    if (num_params != TIVX_KERNEL_MSD_MAX_PARAMS)
    {
        status = VX_FAILURE;
    }
    else
    {
        if ((NULL == obj_desc[TIVX_KERNEL_MSD_IN_IMG_IDX]) ||
            (NULL == obj_desc[TIVX_KERNEL_MSD_OUT_MEAN_IDX]) ||
            (NULL == obj_desc[TIVX_KERNEL_MSD_OUT_STDDEV_IDX]))
        {
            status = VX_ERROR_NO_MEMORY;
        }
    }

    if (VX_SUCCESS == status)
    {
        status = tivxGetTargetKernelInstanceContext(kernel,
            (void **)&prms, &size);

        if ((VX_SUCCESS == status) && (NULL != prms) &&
            (sizeof(tivxMeanStdDevParams) == size))
        {
            tivxBamDestroyHandle(prms->graph_handle);
            tivxMemFree(prms, sizeof(tivxMeanStdDevParams), TIVX_MEM_EXTERNAL);
        }
    }

    return (status);
}

void tivxAddTargetKernelBamMeanStdDev(void)
{
    char target_name[TIVX_TARGET_MAX_NAME];
    vx_enum self_cpu;

    self_cpu = tivxGetSelfCpuId();

    if ((self_cpu == TIVX_CPU_ID_DSP1) || (self_cpu == TIVX_CPU_ID_DSP2))
    {
        if (self_cpu == TIVX_CPU_ID_DSP1)
        {
            strncpy(target_name, TIVX_TARGET_DSP1,
                TIVX_TARGET_MAX_NAME);
        }
        else
        {
            strncpy(target_name, TIVX_TARGET_DSP2,
                TIVX_TARGET_MAX_NAME);
        }

        vx_meanstddev_target_kernel = tivxAddTargetKernel(
            VX_KERNEL_MEAN_STDDEV,
            target_name,
            tivxKernelMeanStdDevProcess,
            tivxKernelMeanStdDevCreate,
            tivxKernelMeanStdDevDelete,
            NULL,
            NULL);
    }
}


void tivxRemoveTargetKernelBamMeanStdDev(void)
{
    tivxRemoveTargetKernel(vx_meanstddev_target_kernel);
}
