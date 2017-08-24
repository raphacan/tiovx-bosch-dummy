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
#include <TI/tivx_debug.h>
#include <VX/vx.h>
#include <TI/tivx_obj_desc.h>
#include <tivx_kernels_target_utils.h>
#include <tivx_bam_kernel_wrapper.h>

void tivxRegisterTargetKernels(Tivx_Target_Kernel_List *kernel_list, uint32_t num_kernels)
{
    vx_uint32 i;

    for (i = 0; i < num_kernels; i ++)
    {
        if (kernel_list[i].add_kernel)
        {
            kernel_list[i].add_kernel();
        }
    }
}

void tivxUnRegisterTargetKernels(Tivx_Target_Kernel_List *kernel_list, uint32_t num_kernels)
{
    vx_uint32 i;

    for (i = 0; i < num_kernels; i ++)
    {
        if (kernel_list[i].remove_kernel)
        {
            kernel_list[i].remove_kernel();
        }
    }
}


void tivxInitBufParams(
    tivx_obj_desc_image_t *obj_desc,
    VXLIB_bufParams2D_t buf_params[])
{
    uint32_t i;

    for (i = 0; i < obj_desc->planes; i ++)
    {
        buf_params[i].dim_x = obj_desc->valid_roi.end_x - obj_desc->valid_roi.start_x;
        buf_params[i].dim_y = obj_desc->valid_roi.end_y - obj_desc->valid_roi.start_y;
        buf_params[i].stride_y = obj_desc->imagepatch_addr[i].stride_y;

        if (512 == obj_desc->imagepatch_addr[i].scale_x)
        {
            buf_params[i].dim_y = buf_params[i].dim_y / 2;

            if (VX_DF_IMAGE_IYUV == obj_desc->format)
            {
                buf_params[i].dim_x = buf_params[i].dim_x / 2;
            }
        }

        switch(obj_desc->format)
        {
            case VX_DF_IMAGE_NV12:
            case VX_DF_IMAGE_NV21:
            case VX_DF_IMAGE_IYUV:
            case VX_DF_IMAGE_YUV4:
            case VX_DF_IMAGE_U8:
                buf_params[i].data_type = VXLIB_UINT8;
                break;
            case VX_DF_IMAGE_U16:
                buf_params[i].data_type = VXLIB_UINT16;
                break;
            case VX_DF_IMAGE_S16:
                buf_params[i].data_type = VXLIB_INT16;
                break;
            case VX_DF_IMAGE_RGBX:
            case VX_DF_IMAGE_U32:
                buf_params[i].data_type = VXLIB_UINT32;
                break;
            case VX_DF_IMAGE_S32:
                buf_params[i].data_type = VXLIB_INT32;
                break;
            case VX_DF_IMAGE_RGB:
                buf_params[i].data_type = VXLIB_UINT24;
                break;
            case VX_DF_IMAGE_YUYV:
            case VX_DF_IMAGE_UYVY:
                buf_params[i].data_type = VXLIB_UINT16;
                break;
        }
    }
}

void tivxInitTwoBufParams(
    tivx_obj_desc_image_t *obj_desc0,
    tivx_obj_desc_image_t *obj_desc1,
    VXLIB_bufParams2D_t buf_params0[],
    VXLIB_bufParams2D_t buf_params1[])
{
    uint32_t i;
    vx_rectangle_t rect;

    rect = obj_desc0->valid_roi;

    if (obj_desc1->valid_roi.start_x > rect.start_x)
    {
        rect.start_x = obj_desc1->valid_roi.start_x;
    }
    if (obj_desc1->valid_roi.start_y > rect.start_y)
    {
        rect.start_y = obj_desc1->valid_roi.start_y;
    }

    if (obj_desc1->valid_roi.end_x < rect.end_x)
    {
        rect.end_x = obj_desc1->valid_roi.end_x;
    }
    if (obj_desc1->valid_roi.end_y < rect.end_y)
    {
        rect.end_y = obj_desc1->valid_roi.end_y;
    }

    for (i = 0; i < obj_desc0->planes; i ++)
    {
        buf_params0[i].dim_x = rect.end_x - rect.start_x;
        buf_params0[i].dim_y = rect.end_y - rect.start_y;
        buf_params1[i].dim_x = rect.end_x - rect.start_x;
        buf_params1[i].dim_y = rect.end_y - rect.start_y;

        buf_params0[i].stride_y = obj_desc0->imagepatch_addr[i].stride_y;
        buf_params1[i].stride_y = obj_desc1->imagepatch_addr[i].stride_y;

        if (512 == obj_desc0->imagepatch_addr[i].scale_x)
        {
            buf_params0[i].dim_y = buf_params0[i].dim_y / 2;

            if (VX_DF_IMAGE_IYUV == obj_desc0->format)
            {
                buf_params0[i].dim_x = buf_params0[i].dim_x / 2;
            }
        }

        if (512 == obj_desc1->imagepatch_addr[i].scale_x)
        {
            buf_params1[i].dim_y = buf_params1[i].dim_y / 2;

            if (VX_DF_IMAGE_IYUV == obj_desc1->format)
            {
                buf_params1[i].dim_x = buf_params1[i].dim_x / 2;
            }
        }

        switch(obj_desc0->format)
        {
            case VX_DF_IMAGE_NV12:
            case VX_DF_IMAGE_NV21:
            case VX_DF_IMAGE_IYUV:
            case VX_DF_IMAGE_YUV4:
            case VX_DF_IMAGE_U8:
                buf_params0[i].data_type = VXLIB_UINT8;
                break;
            case VX_DF_IMAGE_U16:
                buf_params0[i].data_type = VXLIB_UINT16;
                break;
            case VX_DF_IMAGE_S16:
                buf_params0[i].data_type = VXLIB_INT16;
                break;
            case VX_DF_IMAGE_RGBX:
            case VX_DF_IMAGE_U32:
                buf_params0[i].data_type = VXLIB_UINT32;
                break;
            case VX_DF_IMAGE_S32:
                buf_params0[i].data_type = VXLIB_INT32;
                break;
            case VX_DF_IMAGE_RGB:
                buf_params0[i].data_type = VXLIB_UINT24;
                break;
            case VX_DF_IMAGE_YUYV:
            case VX_DF_IMAGE_UYVY:
                buf_params0[i].data_type = VXLIB_UINT16;
                break;
        }

        switch(obj_desc1->format)
        {
            case VX_DF_IMAGE_NV12:
            case VX_DF_IMAGE_NV21:
            case VX_DF_IMAGE_IYUV:
            case VX_DF_IMAGE_YUV4:
            case VX_DF_IMAGE_U8:
                buf_params1[i].data_type = VXLIB_UINT8;
                break;
            case VX_DF_IMAGE_U16:
                buf_params1[i].data_type = VXLIB_UINT16;
                break;
            case VX_DF_IMAGE_S16:
                buf_params1[i].data_type = VXLIB_INT16;
                break;
            case VX_DF_IMAGE_RGBX:
            case VX_DF_IMAGE_U32:
                buf_params1[i].data_type = VXLIB_UINT32;
                break;
            case VX_DF_IMAGE_S32:
                buf_params1[i].data_type = VXLIB_INT32;
                break;
            case VX_DF_IMAGE_RGB:
                buf_params1[i].data_type = VXLIB_UINT24;
                break;
            case VX_DF_IMAGE_YUYV:
            case VX_DF_IMAGE_UYVY:
                buf_params1[i].data_type = VXLIB_UINT16;
                break;
        }
    }
}

void tivxSetPointerLocation(
    tivx_obj_desc_image_t *obj_desc,
    uint8_t *addr[])
{
    uint32_t i;

    for (i = 0; i < obj_desc->planes; i ++)
    {
        addr[i] = (uint8_t *)((uintptr_t)obj_desc->mem_ptr[i].target_ptr +
            tivxComputePatchOffset(obj_desc->valid_roi.start_x,
            obj_desc->valid_roi.start_y,
            &obj_desc->imagepatch_addr[i]));
    }
}

void tivxSetTwoPointerLocation(
    tivx_obj_desc_image_t *obj_desc0,
    tivx_obj_desc_image_t *obj_desc1,
    uint8_t *addr0[],
    uint8_t *addr1[])
{
    uint32_t i;
    vx_rectangle_t rect;

    rect = obj_desc0->valid_roi;

    if (obj_desc1->valid_roi.start_x > rect.start_x)
    {
        rect.start_x = obj_desc1->valid_roi.start_x;
    }
    if (obj_desc1->valid_roi.start_y > rect.start_y)
    {
        rect.start_y = obj_desc1->valid_roi.start_y;
    }

    if (obj_desc1->valid_roi.end_x < rect.end_x)
    {
        rect.end_x = obj_desc1->valid_roi.end_x;
    }
    if (obj_desc1->valid_roi.end_y < rect.end_y)
    {
        rect.end_y = obj_desc1->valid_roi.end_y;
    }

    for (i = 0; i < obj_desc0->planes; i ++)
    {
        addr0[i] = (uint8_t *)((uintptr_t)obj_desc0->mem_ptr[i].target_ptr +
            tivxComputePatchOffset(rect.start_x,
            rect.start_y,
            &obj_desc0->imagepatch_addr[i]));
        addr1[i] = (uint8_t *)((uintptr_t)obj_desc1->mem_ptr[i].target_ptr +
            tivxComputePatchOffset(rect.start_x,
            rect.start_y,
            &obj_desc1->imagepatch_addr[i]));
    }
}

void tivxReserveC66xL2MEM(void)
{
#if defined(BUILD_BAM)
    tivx_mem_stats mem_stats;
    void *ibuf_ptr, *wbuf_ptr;
    vx_uint32 ibuf_size, wbuf_size;
    vx_uint32 buf_align = 4*1024;

    /* find L2MEM size */
    tivxMemStats(&mem_stats, TIVX_MEM_INTERNAL_L2);

    /* reserve L2MEM to BAM */
    #if 1
    wbuf_size = mem_stats.free_size / 5;
    #else
    wbuf_size = 32*1024;
    #endif

    /* floor to 'buf_align' bytes */
    wbuf_size = (wbuf_size/buf_align)*buf_align;
    ibuf_size = wbuf_size * 4;

    ibuf_ptr = tivxMemAlloc(ibuf_size, TIVX_MEM_INTERNAL_L2);
    wbuf_ptr = tivxMemAlloc(wbuf_size, TIVX_MEM_INTERNAL_L2);

    VX_PRINT(VX_ZONE_INIT,
        "BAM memory config: IBUF %d bytes @ 0x%08x, WBUF %d bytes @ 0x%08x !!! \n",
        ibuf_size, ibuf_ptr, wbuf_size, wbuf_ptr);

    tivxBamMemInit(ibuf_ptr, ibuf_size, wbuf_ptr, wbuf_size);

    /* memory is allocated only to get a base address, otherwise
     * this L2 memory is used as scratch, hence we free immediately afterwards
     * so that some other algorithm can reuse the memory as scratch
     */
    tivxMemFree(ibuf_ptr, ibuf_size, TIVX_MEM_INTERNAL_L2);
    tivxMemFree(wbuf_ptr, wbuf_size, TIVX_MEM_INTERNAL_L2);
#endif

}
