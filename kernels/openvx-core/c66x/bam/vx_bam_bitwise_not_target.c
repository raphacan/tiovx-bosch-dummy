/*
 *******************************************************************************
 *
 * Copyright (C) 2017 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 *******************************************************************************
 */

#include <TI/tivx.h>
#include <VX/vx.h>
#include <tivx_openvx_core_kernels.h>
#include <tivx_kernel_bitwise.h>
#include <TI/tivx_target_kernel.h>
#include <ti/vxlib/vxlib.h>
#include <tivx_kernel_utils.h>
#include <vx_bam_kernel_wrapper.h>

typedef struct
{
    tivx_bam_graph_handle graph_handle;
} tivxBitwiseNotParams;

static tivx_target_kernel vx_bitwise_not_target_kernel = NULL;

static vx_status VX_CALLBACK tivxKernelBitwiseNotProcess(
    tivx_target_kernel_instance kernel, tivx_obj_desc_t *obj_desc[],
    uint16_t num_params, void *priv_arg);

static vx_status VX_CALLBACK tivxKernelBitwiseNotCreate(
    tivx_target_kernel_instance kernel, tivx_obj_desc_t *obj_desc[],
    uint16_t num_params, void *priv_arg);

static vx_status VX_CALLBACK tivxKernelBitwiseNotDelete(
    tivx_target_kernel_instance kernel, tivx_obj_desc_t *obj_desc[],
    uint16_t num_params, void *priv_arg);

static vx_status VX_CALLBACK tivxKernelBitwiseNotControl(
    tivx_target_kernel_instance kernel, tivx_obj_desc_t *obj_desc[],
    uint16_t num_params, void *priv_arg);

static vx_status VX_CALLBACK tivxKernelBitwiseNotProcess(
    tivx_target_kernel_instance kernel, tivx_obj_desc_t *obj_desc[],
    uint16_t num_params, void *priv_arg)
{
    vx_status status = VX_SUCCESS;
    tivxBitwiseNotParams *prms = NULL;
    tivx_obj_desc_image_t *src, *dst;
    uint8_t *src_addr, *dst_addr;
    vx_rectangle_t rect;
    uint32_t size;

    if (num_params != TIVX_KERNEL_BITWISE_NOT_MAX_PARAMS)
    {
        status = VX_FAILURE;
    }
    else
    {
        /* Check for NULL */
        if ((NULL == obj_desc[TIVX_KERNEL_BITWISE_NOT_IN_IMG_IDX]) ||
            (NULL == obj_desc[TIVX_KERNEL_BITWISE_NOT_OUT_IMG_IDX]))
        {
            status = VX_ERROR_NO_MEMORY;
        }
    }

    if (VX_SUCCESS == status)
    {
        src = (tivx_obj_desc_image_t *)obj_desc[TIVX_KERNEL_BITWISE_NOT_IN_IMG_IDX];
        dst = (tivx_obj_desc_image_t *)obj_desc[TIVX_KERNEL_BITWISE_NOT_OUT_IMG_IDX];

        status = tivxGetTargetKernelInstanceContext(kernel,
            (void **)&prms, &size);

        if ((VX_SUCCESS != status) || (NULL == prms) ||
            (sizeof(tivxBitwiseNotParams) != size))
        {
            status = VX_FAILURE;
        }
    }

    if (VX_SUCCESS == status)
    {
        void *img_ptrs[2];

        src->mem_ptr[0].target_ptr = tivxMemShared2TargetPtr(
            src->mem_ptr[0].shared_ptr, src->mem_ptr[0].mem_type);

        tivxMemBufferMap(src->mem_ptr[0].target_ptr, src->mem_size[0],
            src->mem_ptr[0].mem_type, VX_READ_ONLY);

        /* Get the correct offset of the images from the valid roi parameter */
        rect = src->valid_roi;

        src_addr = (uint8_t *)((uintptr_t)src->mem_ptr[0U].target_ptr +
            ownComputePatchOffset(rect.start_x, rect.start_y,
            &src->imagepatch_addr[0U]));

        dst->mem_ptr[0].target_ptr = tivxMemShared2TargetPtr(
            dst->mem_ptr[0].shared_ptr, dst->mem_ptr[0].mem_type);

        tivxMemBufferMap(dst->mem_ptr[0].target_ptr, dst->mem_size[0],
            dst->mem_ptr[0].mem_type, VX_READ_ONLY);

        dst_addr = (uint8_t *)((uintptr_t)dst->mem_ptr[0U].target_ptr +
            ownComputePatchOffset(rect.start_x, rect.start_y,
            &dst->imagepatch_addr[0U]));

        img_ptrs[0] = src_addr;
        img_ptrs[1] = dst_addr;
        tivxBamUpdatePointers(prms->graph_handle, 1U, 1U, img_ptrs);

        status  = tivxBamProcessGraph(prms->graph_handle);

        tivxMemBufferUnmap(dst->mem_ptr[0].target_ptr, dst->mem_size[0],
            dst->mem_ptr[0].mem_type, VX_WRITE_ONLY);

        tivxMemBufferUnmap(src->mem_ptr[0].target_ptr, src->mem_size[0],
            src->mem_ptr[0].mem_type, VX_READ_ONLY);

    }
    else
    {
        status = VX_ERROR_NO_MEMORY;
    }

    return (status);
}

static vx_status VX_CALLBACK tivxKernelBitwiseNotCreate(
    tivx_target_kernel_instance kernel, tivx_obj_desc_t *obj_desc[],
    uint16_t num_params, void *priv_arg)
{

    vx_status status = VX_SUCCESS;
    tivx_obj_desc_image_t *src, *dst;
    tivxBitwiseNotParams *prms = NULL;

    /* Check number of buffers and NULL pointers */
    if (num_params != TIVX_KERNEL_BITWISE_NOT_MAX_PARAMS)
    {
        status = VX_FAILURE;
    }
    else
    {
        if ((NULL == obj_desc[TIVX_KERNEL_BITWISE_NOT_IN_IMG_IDX]) ||
            (NULL == obj_desc[TIVX_KERNEL_BITWISE_NOT_OUT_IMG_IDX]))
        {
            status = VX_ERROR_NO_MEMORY;
        }
    }

    if (VX_SUCCESS == status)
    {
        src = (tivx_obj_desc_image_t *)obj_desc[
            TIVX_KERNEL_BITWISE_NOT_IN_IMG_IDX];
        dst = (tivx_obj_desc_image_t *)obj_desc[
            TIVX_KERNEL_BITWISE_NOT_OUT_IMG_IDX];

        prms = tivxMemAlloc(sizeof(tivxBitwiseNotParams), TIVX_MEM_EXTERNAL);

        if (NULL != prms)
        {
            tivx_bam_kernel_details_t kernel_details;
            VXLIB_bufParams2D_t vxlib_src, vxlib_dst;
            VXLIB_bufParams2D_t *buf_params[2];

            memset(prms, 0, sizeof(tivxBitwiseNotParams));

            vxlib_src.dim_x = src->imagepatch_addr[0U].dim_x;
            vxlib_src.dim_y = src->imagepatch_addr[0U].dim_y;
            vxlib_src.stride_y = src->imagepatch_addr[0U].stride_y;
            vxlib_src.data_type = VXLIB_UINT8;

            vxlib_dst.dim_x = dst->imagepatch_addr[0U].dim_x;
            vxlib_dst.dim_y = dst->imagepatch_addr[0U].dim_y;
            vxlib_dst.stride_y = dst->imagepatch_addr[0U].stride_y;
            vxlib_dst.data_type = VXLIB_UINT8;

            /* Fill in the frame level sizes of buffers here. If the port
             * is optionally disabled, put NULL */
            buf_params[0] = &vxlib_src;
            buf_params[1] = &vxlib_dst;

            kernel_details.compute_kernel_params = NULL;

            BAM_VXLIB_not_i8u_o8u_getKernelInfo(NULL,
                &kernel_details.kernel_info);

            status = tivxBamCreateHandleSingleNode(
                BAM_KERNELID_VXLIB_NOT_I8U_O8U, buf_params,
                &kernel_details, &prms->graph_handle);
        }
        else
        {
            status = VX_ERROR_NO_MEMORY;
        }

        if (VX_SUCCESS == status)
        {
            tivxSetTargetKernelInstanceContext(kernel, prms,
                sizeof(tivxBitwiseNotParams));
        }
        else
        {
            if (NULL != prms)
            {
                tivxMemFree(prms, sizeof(tivxBitwiseNotParams), TIVX_MEM_EXTERNAL);
            }
        }
    }

    return status;
}

static vx_status VX_CALLBACK tivxKernelBitwiseNotDelete(
    tivx_target_kernel_instance kernel, tivx_obj_desc_t *obj_desc[],
    uint16_t num_params, void *priv_arg)
{
    vx_status status = VX_SUCCESS;
    uint32_t size;
    tivxBitwiseNotParams *prms = NULL;

    /* Check number of buffers and NULL pointers */
    if (num_params != TIVX_KERNEL_BITWISE_NOT_MAX_PARAMS)
    {
        status = VX_FAILURE;
    }
    else
    {
        if ((NULL == obj_desc[TIVX_KERNEL_BITWISE_NOT_IN_IMG_IDX]) ||
            (NULL == obj_desc[TIVX_KERNEL_BITWISE_NOT_OUT_IMG_IDX]))
        {
            status = VX_ERROR_NO_MEMORY;
        }
    }

    if (VX_SUCCESS == status)
    {
        status = tivxGetTargetKernelInstanceContext(kernel,
            (void **)&prms, &size);

        if ((VX_SUCCESS == status) && (NULL != prms) &&
            (sizeof(tivxBitwiseNotParams) == size))
        {
            tivxBamDestroyHandle(prms->graph_handle);
            tivxMemFree(prms, sizeof(tivxBitwiseNotParams), TIVX_MEM_EXTERNAL);
        }
    }

    return (status);
}

static vx_status VX_CALLBACK tivxKernelBitwiseNotControl(
    tivx_target_kernel_instance kernel, tivx_obj_desc_t *obj_desc[],
    uint16_t num_params, void *priv_arg)
{
    return (VX_SUCCESS);
}

void tivxAddTargetKernelBamBitwiseNot(void)
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

        vx_bitwise_not_target_kernel = tivxAddTargetKernel(
            VX_KERNEL_NOT,
            target_name,
            tivxKernelBitwiseNotProcess,
            tivxKernelBitwiseNotCreate,
            tivxKernelBitwiseNotDelete,
            tivxKernelBitwiseNotControl,
            NULL);
    }
}


void tivxRemoveTargetKernelBamBitwiseNot(void)
{
    tivxRemoveTargetKernel(vx_bitwise_not_target_kernel);
}
