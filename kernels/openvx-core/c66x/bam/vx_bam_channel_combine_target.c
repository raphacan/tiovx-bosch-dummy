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
#include <tivx_kernel_channel_combine.h>
#include <TI/tivx_target_kernel.h>
#include <ti/vxlib/vxlib.h>
#include <tivx_kernel_utils.h>
#include <vx_bam_kernel_wrapper.h>
#include <edma_utils_memcpy.h>

#define SOURCE_NODE1      0
#define CHCOPY_NODE0      1
#define CHCOPY_NODE1      2
#define CHCOPY_NODE2      3
#define SINK_NODE1        4

typedef struct
{
    tivx_bam_graph_handle graph_handle;
} tivxChannelCombineParams;

static tivx_target_kernel vx_channel_combine_target_kernel = NULL;

static vx_status VX_CALLBACK tivxKernelChannelCombineProcess(
    tivx_target_kernel_instance kernel, tivx_obj_desc_t *obj_desc[],
    uint16_t num_params, void *priv_arg);

static vx_status VX_CALLBACK tivxKernelChannelCombineCreate(
    tivx_target_kernel_instance kernel, tivx_obj_desc_t *obj_desc[],
    uint16_t num_params, void *priv_arg);

static vx_status VX_CALLBACK tivxKernelChannelCombineDelete(
    tivx_target_kernel_instance kernel, tivx_obj_desc_t *obj_desc[],
    uint16_t num_params, void *priv_arg);

static vx_status VX_CALLBACK tivxKernelChannelCombineControl(
    tivx_target_kernel_instance kernel, tivx_obj_desc_t *obj_desc[],
    uint16_t num_params, void *priv_arg);

static vx_status VX_CALLBACK tivxKernelChannelCombineProcess(
    tivx_target_kernel_instance kernel, tivx_obj_desc_t *obj_desc[],
    uint16_t num_params, void *priv_arg)
{
    vx_status status = VX_SUCCESS;
    tivxChannelCombineParams *prms = NULL;
    tivx_obj_desc_image_t *src0, *src1, *src2, *src3, *dst;
    uint8_t *src0_addr, *src1_addr, *src2_addr, *src3_addr, *dst_addr[4U] = {NULL};
    vx_rectangle_t rect;
    uint32_t size;
    uint16_t plane_idx;

    if ((num_params != TIVX_KERNEL_CHANNEL_COMBINE_MAX_PARAMS)
        || (NULL == obj_desc[TIVX_KERNEL_CHANNEL_COMBINE_SRC0_IDX])
        || (NULL == obj_desc[TIVX_KERNEL_CHANNEL_COMBINE_SRC1_IDX])
        || (NULL == obj_desc[TIVX_KERNEL_CHANNEL_COMBINE_DST_IDX]))
    {
        status = VX_FAILURE;
    }

    if (VX_SUCCESS == status)
    {
        src0 = (tivx_obj_desc_image_t *)obj_desc[TIVX_KERNEL_CHANNEL_COMBINE_SRC0_IDX];
        src1 = (tivx_obj_desc_image_t *)obj_desc[TIVX_KERNEL_CHANNEL_COMBINE_SRC1_IDX];
        src2 = (tivx_obj_desc_image_t *)obj_desc[TIVX_KERNEL_CHANNEL_COMBINE_SRC2_IDX];
        src3 = (tivx_obj_desc_image_t *)obj_desc[TIVX_KERNEL_CHANNEL_COMBINE_SRC3_IDX];
        dst = (tivx_obj_desc_image_t *)obj_desc[TIVX_KERNEL_CHANNEL_COMBINE_DST_IDX];

        status = tivxGetTargetKernelInstanceContext(kernel,
            (void **)&prms, &size);

        if ((VX_SUCCESS != status) || (NULL == prms) ||
            (sizeof(tivxChannelCombineParams) != size))
        {
            status = VX_FAILURE;
        }
    }
    if (VX_SUCCESS == status)
    {
        void *img_ptrs[6];

        /* Get the correct offset of the images from the valid roi parameter */
        src0->mem_ptr[0].target_ptr = tivxMemShared2TargetPtr(
            src0->mem_ptr[0].shared_ptr, src0->mem_ptr[0].mem_type);
        tivxMemBufferMap(src0->mem_ptr[0].target_ptr, src0->mem_size[0],
            src0->mem_ptr[0].mem_type, VX_READ_ONLY);
        rect = src0->valid_roi;
        src0_addr = (uint8_t *)((uintptr_t)src0->mem_ptr[0U].target_ptr +
            ownComputePatchOffset(rect.start_x, rect.start_y,
            &src0->imagepatch_addr[0U]));

        src1->mem_ptr[0].target_ptr = tivxMemShared2TargetPtr(
            src1->mem_ptr[0].shared_ptr, src1->mem_ptr[0].mem_type);
        tivxMemBufferMap(src1->mem_ptr[0].target_ptr, src1->mem_size[0],
            src1->mem_ptr[0].mem_type, VX_READ_ONLY);
        rect = src1->valid_roi;
        src1_addr = (uint8_t *)((uintptr_t)src1->mem_ptr[0U].target_ptr +
            ownComputePatchOffset(rect.start_x, rect.start_y,
            &src1->imagepatch_addr[0U]));
        if( src2 != NULL)
        {
            src2->mem_ptr[0].target_ptr = tivxMemShared2TargetPtr(
              src2->mem_ptr[0].shared_ptr, src2->mem_ptr[0].mem_type);
            tivxMemBufferMap(src2->mem_ptr[0].target_ptr,
               src2->mem_size[0], src2->mem_ptr[0].mem_type,
                VX_READ_ONLY);
            rect = src2->valid_roi;
            src2_addr = (uint8_t *)((uintptr_t)src2->mem_ptr[0U].target_ptr +
                ownComputePatchOffset(rect.start_x, rect.start_y,
                &src2->imagepatch_addr[0U]));
        }
        if( src3 != NULL)
        {
            src3->mem_ptr[0].target_ptr = tivxMemShared2TargetPtr(
              src3->mem_ptr[0].shared_ptr, src3->mem_ptr[0].mem_type);
            tivxMemBufferMap(src3->mem_ptr[0].target_ptr,
               src3->mem_size[0], src3->mem_ptr[0].mem_type,
                VX_READ_ONLY);
            rect = src3->valid_roi;
            src3_addr = (uint8_t *)((uintptr_t)src3->mem_ptr[0U].target_ptr +
                ownComputePatchOffset(rect.start_x, rect.start_y,
                &src3->imagepatch_addr[0U]));
        }
        for(plane_idx=0; plane_idx<dst->planes; plane_idx++)
        {
            dst->mem_ptr[plane_idx].target_ptr = tivxMemShared2TargetPtr(
              dst->mem_ptr[plane_idx].shared_ptr, dst->mem_ptr[plane_idx].mem_type);
            tivxMemBufferMap(dst->mem_ptr[plane_idx].target_ptr,
               dst->mem_size[plane_idx], dst->mem_ptr[plane_idx].mem_type,
                VX_WRITE_ONLY);
            rect = dst->valid_roi;
            dst_addr[plane_idx] = (uint8_t *)((uintptr_t)dst->mem_ptr[plane_idx].target_ptr +
                ownComputePatchOffset(rect.start_x, rect.start_y,
                &dst->imagepatch_addr[plane_idx]));
        }

        if ( (src2 != NULL) && (src3 != NULL) )
        {
            img_ptrs[0] = src0_addr;
            img_ptrs[1] = src1_addr;
            img_ptrs[2] = src2_addr;
            img_ptrs[3] = src3_addr;
            img_ptrs[4] = dst_addr[0U];
            tivxBamUpdatePointers(prms->graph_handle, 4U, 1U, img_ptrs);

            status  = tivxBamProcessGraph(prms->graph_handle);
        }
        else if (src2 != NULL)
        {
            if (dst->format == VX_DF_IMAGE_RGB ||
                dst->format == VX_DF_IMAGE_YUYV ||
                dst->format == VX_DF_IMAGE_UYVY)
            {
                img_ptrs[0] = src0_addr;
                img_ptrs[1] = src1_addr;
                img_ptrs[2] = src2_addr;
                img_ptrs[3] = dst_addr[0U];
                tivxBamUpdatePointers(prms->graph_handle, 3U, 1U, img_ptrs);
                status  = tivxBamProcessGraph(prms->graph_handle);
            }
            else if (dst->format == VX_DF_IMAGE_YUV4 )
            {
                img_ptrs[0] = src0_addr;
                img_ptrs[1] = dst_addr[0U];
                tivxBamUpdatePointers(prms->graph_handle, 1U, 1U, img_ptrs);
                status  = tivxBamProcessGraph(prms->graph_handle);
                img_ptrs[0] = src1_addr;
                img_ptrs[1] = dst_addr[1U];
                tivxBamUpdatePointers(prms->graph_handle, 1U, 1U, img_ptrs);
                status  = tivxBamProcessGraph(prms->graph_handle);
                img_ptrs[0] = src2_addr;
                img_ptrs[1] = dst_addr[2U];
                tivxBamUpdatePointers(prms->graph_handle, 1U, 1U, img_ptrs);
                status  = tivxBamProcessGraph(prms->graph_handle);
            }
            else if (dst->format == VX_DF_IMAGE_IYUV )
            {
                img_ptrs[0] = src0_addr;
                img_ptrs[1] = src1_addr;
                img_ptrs[2] = src2_addr;
                img_ptrs[3] = dst_addr[0U];
                img_ptrs[4] = dst_addr[1U];
                img_ptrs[5] = dst_addr[2U];
                tivxBamUpdatePointers(prms->graph_handle, 3U, 3U, img_ptrs);
                status  = tivxBamProcessGraph(prms->graph_handle);
            }
            else if (dst->format == VX_DF_IMAGE_NV12 )
            {
                img_ptrs[0] = src1_addr;
                img_ptrs[1] = src2_addr;
                img_ptrs[2] = dst_addr[1U];
                EDMA_UTILS_memcpy2D(dst_addr[0U], src0_addr, src0->imagepatch_addr[0U].dim_x,
                                    src0->imagepatch_addr[0U].dim_y, dst->imagepatch_addr[0U].stride_y,
                                    src0->imagepatch_addr[0U].stride_y);
                tivxBamUpdatePointers(prms->graph_handle, 2U, 1U, img_ptrs);
                status  = tivxBamProcessGraph(prms->graph_handle);
            }
            else if(dst->format == VX_DF_IMAGE_NV21)
            {
                img_ptrs[0] = src2_addr;
                img_ptrs[1] = src1_addr;
                img_ptrs[2] = dst_addr[1U];
                EDMA_UTILS_memcpy2D(dst_addr[0U], src0_addr, src0->imagepatch_addr[0U].dim_x,
                                    src0->imagepatch_addr[0U].dim_y, dst->imagepatch_addr[0U].stride_y,
                                    src0->imagepatch_addr[0U].stride_y);
                tivxBamUpdatePointers(prms->graph_handle, 2U, 1U, img_ptrs);
                status  = tivxBamProcessGraph(prms->graph_handle);
            }
        }
        else 
        {
            img_ptrs[0] = src0_addr;
            img_ptrs[1] = src1_addr;
            img_ptrs[2] = dst_addr[0U];
            tivxBamUpdatePointers(prms->graph_handle, 2U, 1U, img_ptrs);

            status  = tivxBamProcessGraph(prms->graph_handle);
        }

        tivxMemBufferUnmap(src0->mem_ptr[0].target_ptr, src0->mem_size[0],
            src0->mem_ptr[0].mem_type, VX_READ_ONLY);
        tivxMemBufferUnmap(src1->mem_ptr[0].target_ptr, src1->mem_size[0],
            src1->mem_ptr[0].mem_type, VX_READ_ONLY);
        if( src2 != NULL)
        {
            tivxMemBufferUnmap(src2->mem_ptr[0].target_ptr,
               src2->mem_size[0], src2->mem_ptr[0].mem_type,
                VX_READ_ONLY);
        }
        if( NULL != src3)
        {
            tivxMemBufferUnmap(src3->mem_ptr[0].target_ptr,
               src3->mem_size[0], src3->mem_ptr[0].mem_type,
                VX_READ_ONLY);
        }
        for(plane_idx=0; plane_idx<dst->planes; plane_idx++)
        {
            tivxMemBufferUnmap(dst->mem_ptr[plane_idx].target_ptr,
               dst->mem_size[plane_idx], dst->mem_ptr[plane_idx].mem_type,
                VX_WRITE_ONLY);
        }
    }
    else
    {
        status = VX_ERROR_NO_MEMORY;
    }

    return (status);
}

static vx_status VX_CALLBACK tivxKernelChannelCombineCreate(
    tivx_target_kernel_instance kernel, tivx_obj_desc_t *obj_desc[],
    uint16_t num_params, void *priv_arg)
{
    vx_status status = VX_SUCCESS;
    tivx_obj_desc_image_t *src0, *src1, *src2, *src3, *dst;
    tivxChannelCombineParams *prms = NULL;
    uint16_t plane_idx;

    if ((num_params != TIVX_KERNEL_CHANNEL_COMBINE_MAX_PARAMS)
        || (NULL == obj_desc[TIVX_KERNEL_CHANNEL_COMBINE_SRC0_IDX])
        || (NULL == obj_desc[TIVX_KERNEL_CHANNEL_COMBINE_SRC1_IDX])
        || (NULL == obj_desc[TIVX_KERNEL_CHANNEL_COMBINE_DST_IDX]))
    {
        status = VX_FAILURE;
    }

    if (VX_SUCCESS == status)
    {
        src0 = (tivx_obj_desc_image_t *)obj_desc[TIVX_KERNEL_CHANNEL_COMBINE_SRC0_IDX];
        src1 = (tivx_obj_desc_image_t *)obj_desc[TIVX_KERNEL_CHANNEL_COMBINE_SRC1_IDX];
        src2 = (tivx_obj_desc_image_t *)obj_desc[TIVX_KERNEL_CHANNEL_COMBINE_SRC2_IDX];
        src3 = (tivx_obj_desc_image_t *)obj_desc[TIVX_KERNEL_CHANNEL_COMBINE_SRC3_IDX];
        dst = (tivx_obj_desc_image_t *)obj_desc[TIVX_KERNEL_CHANNEL_COMBINE_DST_IDX];

        prms = tivxMemAlloc(sizeof(tivxChannelCombineParams), TIVX_MEM_EXTERNAL);

        if (NULL != prms)
        {
            tivx_bam_kernel_details_t kernel_details;
            VXLIB_bufParams2D_t vxlib_src0, vxlib_src1, vxlib_src2, vxlib_src3, vxlib_dst, vxlib_dst1, vxlib_dst2;
            VXLIB_bufParams2D_t *buf_params[6];

            kernel_details.compute_kernel_params = NULL;

            memset(prms, 0, sizeof(tivxChannelCombineParams));

            vxlib_src0.dim_x = src0->imagepatch_addr[0U].dim_x;
            vxlib_src0.dim_y = src0->imagepatch_addr[0U].dim_y;
            vxlib_src0.stride_y = src0->imagepatch_addr[0U].stride_y;
            vxlib_src0.data_type = VXLIB_UINT8;

            vxlib_src1.dim_x = src1->imagepatch_addr[0U].dim_x;
            vxlib_src1.dim_y = src1->imagepatch_addr[0U].dim_y;
            vxlib_src1.stride_y = src1->imagepatch_addr[0U].stride_y;
            vxlib_src1.data_type = VXLIB_UINT8;

            /* Fill in the frame level sizes of buffers here. If the port
             * is optionally disabled, put NULL */
            buf_params[0] = &vxlib_src0;
            buf_params[1] = &vxlib_src1;

            if(src2 != NULL)
            {
                vxlib_src2.dim_x = src2->imagepatch_addr[0U].dim_x;
                vxlib_src2.dim_y = src2->imagepatch_addr[0U].dim_y;
                vxlib_src2.stride_y = src2->imagepatch_addr[0U].stride_y;
                vxlib_src2.data_type = VXLIB_UINT8;
                buf_params[2] = &vxlib_src2;
            }

            if(src3 != NULL)
            {
                vxlib_src3.dim_x = src3->imagepatch_addr[0U].dim_x;
                vxlib_src3.dim_y = src3->imagepatch_addr[0U].dim_y;
                vxlib_src3.stride_y = src3->imagepatch_addr[0U].stride_y;
                vxlib_src3.data_type = VXLIB_UINT8;
                buf_params[3] = &vxlib_src3;
            }

            if (   (dst->format == VX_DF_IMAGE_RGB)
                || (dst->format == VX_DF_IMAGE_RGBX)
                || (dst->format == VX_DF_IMAGE_YUYV)
                || (dst->format == VX_DF_IMAGE_UYVY)
                )
            {
                vxlib_dst.dim_x = dst->imagepatch_addr[0U].dim_x;
                vxlib_dst.dim_y = dst->imagepatch_addr[0U].dim_y;
                vxlib_dst.stride_y = dst->imagepatch_addr[0U].stride_y;
                vxlib_dst.data_type = VXLIB_UINT8;

                if( dst->format == VX_DF_IMAGE_RGB)
                {
                    vxlib_dst.data_type = VXLIB_UINT24;
                    buf_params[3] = &vxlib_dst;

                    BAM_VXLIB_channelCombine_3to1_i8u_o8u_getKernelInfo(NULL,
                        &kernel_details.kernel_info);

                    status = tivxBamCreateHandleSingleNode(
                        BAM_KERNELID_VXLIB_CHANNEL_COMBINE_3TO1_I8U_O8U, buf_params,
                        &kernel_details, &prms->graph_handle);
                }
                else
                if( dst->format == VX_DF_IMAGE_RGBX)
                {
                    vxlib_dst.data_type = VXLIB_UINT32;
                    buf_params[4] = &vxlib_dst;

                    BAM_VXLIB_channelCombine_4to1_i8u_o8u_getKernelInfo(NULL,
                        &kernel_details.kernel_info);

                    status = tivxBamCreateHandleSingleNode(
                        BAM_KERNELID_VXLIB_CHANNEL_COMBINE_4TO1_I8U_O8U, buf_params,
                        &kernel_details, &prms->graph_handle);
                }
                else
                if( dst->format == VX_DF_IMAGE_YUYV)
                {
                    vxlib_dst.data_type = VXLIB_UINT16;

                    BAM_VXLIB_channelCombine_yuyv_i8u_o8u_params kernel_params;

                    kernel_params.yidx = 0;

                    kernel_details.compute_kernel_params = (void*)&kernel_params;

                    buf_params[3] = &vxlib_dst;

                    BAM_VXLIB_channelCombine_yuyv_i8u_o8u_getKernelInfo(NULL,
                        &kernel_details.kernel_info);

                    status = tivxBamCreateHandleSingleNode(
                        BAM_KERNELID_VXLIB_CHANNEL_COMBINE_YUYV_I8U_O8U, buf_params,
                        &kernel_details, &prms->graph_handle);
                }
                else /* format is VX_DF_IMAGE_UYVY */
                {
                    vxlib_dst.data_type = VXLIB_UINT16;

                    BAM_VXLIB_channelCombine_yuyv_i8u_o8u_params kernel_params;

                    kernel_params.yidx = 1;

                    kernel_details.compute_kernel_params = (void*)&kernel_params;

                    buf_params[3] = &vxlib_dst;

                    BAM_VXLIB_channelCombine_yuyv_i8u_o8u_getKernelInfo(NULL,
                        &kernel_details.kernel_info);

                    status = tivxBamCreateHandleSingleNode(
                        BAM_KERNELID_VXLIB_CHANNEL_COMBINE_YUYV_I8U_O8U, buf_params,
                        &kernel_details, &prms->graph_handle);
                }
            }
            else
            if (dst->format == VX_DF_IMAGE_YUV4)
            {
                for(plane_idx=0; plane_idx<dst->planes; plane_idx++)
                {
                    if (plane_idx==0)
                    {
                        buf_params[0] = &vxlib_src0;
                    }
                    else if (plane_idx==1)
                    {
                        buf_params[0] = &vxlib_src1;
                    }
                    else if (plane_idx==2)
                    {
                        buf_params[0] = &vxlib_src2;
                    }

                    vxlib_dst.dim_x =
                        dst->imagepatch_addr[plane_idx].dim_x
                        /dst->imagepatch_addr[plane_idx].step_x;
                    vxlib_dst.dim_y =
                        dst->imagepatch_addr[plane_idx].dim_y
                        /dst->imagepatch_addr[plane_idx].step_y;
                    vxlib_dst.stride_y =
                        dst->imagepatch_addr[plane_idx].stride_y;
                    vxlib_dst.data_type = VXLIB_UINT8;

                    buf_params[1] = &vxlib_dst;

                    BAM_VXLIB_channelCopy_1to1_i8u_o8u_getKernelInfo(NULL,
                        &kernel_details.kernel_info);

                    status = tivxBamCreateHandleSingleNode(
                        BAM_KERNELID_VXLIB_CHANNELCOPY_1TO1_I8U_O8U, buf_params,
                        &kernel_details, &prms->graph_handle);
                }
            }
            else
            if (dst->format == VX_DF_IMAGE_IYUV)
            {
                tivx_bam_kernel_details_t multi_kernel_details[6];
                BAM_NodeParams node_list[] = { \
                    {SOURCE_NODE1, BAM_KERNELID_DMAREAD_AUTOINCREMENT, NULL}, \
                    {CHCOPY_NODE0, BAM_KERNELID_VXLIB_CHANNELCOPY_1TO1_I8U_O8U, NULL}, \
                    {CHCOPY_NODE1, BAM_KERNELID_VXLIB_CHANNELCOPY_1TO1_I8U_O8U, NULL}, \
                    {CHCOPY_NODE2, BAM_KERNELID_VXLIB_CHANNELCOPY_1TO1_I8U_O8U, NULL}, \
                    {SINK_NODE1, BAM_KERNELID_DMAWRITE_AUTOINCREMENT, NULL}, \
                    {BAM_END_NODE_MARKER,   0,                          NULL},\
                };

                BAM_EdgeParams edge_list[]= {\
                    {{SOURCE_NODE1, 0},
                        {CHCOPY_NODE0, BAM_VXLIB_CHANNELCOPY_1TO1_I8U_IO8U_INPUT_IMAGE_PORT}},\

                    {{SOURCE_NODE1, 1},
                        {CHCOPY_NODE1, BAM_VXLIB_CHANNELCOPY_1TO1_I8U_IO8U_INPUT_IMAGE_PORT}},\

                    {{SOURCE_NODE1, 2},
                        {CHCOPY_NODE2, BAM_VXLIB_CHANNELCOPY_1TO1_I8U_IO8U_INPUT_IMAGE_PORT}},\

                    {{CHCOPY_NODE0, BAM_VXLIB_CHANNELCOPY_1TO1_I8U_IO8U_OUTPUT_PORT},
                        {SINK_NODE1, 0}},\

                    {{CHCOPY_NODE1, BAM_VXLIB_CHANNELCOPY_1TO1_I8U_IO8U_OUTPUT_PORT},
                        {SINK_NODE1, 1}},\

                    {{CHCOPY_NODE2, BAM_VXLIB_CHANNELCOPY_1TO1_I8U_IO8U_OUTPUT_PORT},
                        {SINK_NODE1, 2}},\

                    {{BAM_END_NODE_MARKER, 0},
                        {BAM_END_NODE_MARKER, 0}},\
                };

                BAM_VXLIB_channelCopy_1to1_i8u_o8u_getKernelInfo(NULL,
                    &multi_kernel_details[CHCOPY_NODE0].kernel_info);

                BAM_VXLIB_channelCopy_1to1_i8u_o8u_getKernelInfo(NULL,
                    &multi_kernel_details[CHCOPY_NODE1].kernel_info);

                BAM_VXLIB_channelCopy_1to1_i8u_o8u_getKernelInfo(NULL,
                    &multi_kernel_details[CHCOPY_NODE2].kernel_info);

                multi_kernel_details[SOURCE_NODE1].compute_kernel_params = NULL;
                multi_kernel_details[CHCOPY_NODE0].compute_kernel_params = NULL;
                multi_kernel_details[CHCOPY_NODE1].compute_kernel_params = NULL;
                multi_kernel_details[CHCOPY_NODE2].compute_kernel_params = NULL;
                multi_kernel_details[SINK_NODE1].compute_kernel_params = NULL;

                buf_params[0] = &vxlib_src0;
                buf_params[1] = &vxlib_src1;
                buf_params[2] = &vxlib_src2;

                vxlib_dst.dim_x =
                    dst->imagepatch_addr[0].dim_x
                    /dst->imagepatch_addr[0].step_x;
                vxlib_dst.dim_y =
                    dst->imagepatch_addr[0].dim_y
                    /dst->imagepatch_addr[0].step_y;
                vxlib_dst.stride_y =
                    dst->imagepatch_addr[0].stride_y;
                vxlib_dst.data_type = VXLIB_UINT8;

                buf_params[3] = &vxlib_dst;

                vxlib_dst1.dim_x =
                    dst->imagepatch_addr[1].dim_x
                    /dst->imagepatch_addr[1].step_x;
                vxlib_dst1.dim_y =
                    dst->imagepatch_addr[1].dim_y
                    /dst->imagepatch_addr[1].step_y;
                vxlib_dst1.stride_y =
                    dst->imagepatch_addr[1].stride_y;
                vxlib_dst1.data_type = VXLIB_UINT8;

                buf_params[4] = &vxlib_dst1;

                vxlib_dst2.dim_x =
                    dst->imagepatch_addr[2].dim_x
                    /dst->imagepatch_addr[2].step_x;
                vxlib_dst2.dim_y =
                    dst->imagepatch_addr[2].dim_y
                    /dst->imagepatch_addr[2].step_y;
                vxlib_dst2.stride_y =
                    dst->imagepatch_addr[2].stride_y;
                vxlib_dst2.data_type = VXLIB_UINT8;

                buf_params[5] = &vxlib_dst2;

                status = tivxBamCreateHandleMultiNode(node_list, edge_list,
                                                      buf_params, multi_kernel_details,
                                                      &prms->graph_handle);
            }
            else
            if ((dst->format == VX_DF_IMAGE_NV12)
                || (dst->format == VX_DF_IMAGE_NV21)
                )
            {
                buf_params[2] = &vxlib_dst1;

                vxlib_dst1.dim_x =
                    dst->imagepatch_addr[1].dim_x
                    /dst->imagepatch_addr[1].step_x;
                vxlib_dst1.dim_y =
                    dst->imagepatch_addr[1].dim_y
                    /dst->imagepatch_addr[1].step_y;
                vxlib_dst1.stride_y =
                    dst->imagepatch_addr[1].stride_y;
                vxlib_dst1.data_type = VXLIB_UINT16;

                if(dst->format == VX_DF_IMAGE_NV21)
                {
                    buf_params[0] = &vxlib_src2;
                    buf_params[1] = &vxlib_src1;
                }
                else
                {
                    buf_params[0] = &vxlib_src1;
                    buf_params[1] = &vxlib_src2;
                }

                BAM_VXLIB_channelCombine_2to1_i8u_o8u_getKernelInfo(NULL,
                        &kernel_details.kernel_info);

                status = tivxBamCreateHandleSingleNode(
                        BAM_KERNELID_VXLIB_CHANNEL_COMBINE_2TO1_I8U_O8U, buf_params,
                        &kernel_details, &prms->graph_handle);
            }
            else
            {
                status = VX_FAILURE;
            }
        }
        else
        {
            status = VX_ERROR_NO_MEMORY;
        }

        if (VX_SUCCESS == status)
        {
            tivxSetTargetKernelInstanceContext(kernel, prms,
                sizeof(tivxChannelCombineParams));
        }
        else
        {
            if (NULL != prms)
            {
                tivxMemFree(prms, sizeof(tivxChannelCombineParams), TIVX_MEM_EXTERNAL);
            }
        }
    }
    return status;
}

static vx_status VX_CALLBACK tivxKernelChannelCombineDelete(
    tivx_target_kernel_instance kernel, tivx_obj_desc_t *obj_desc[],
    uint16_t num_params, void *priv_arg)
{
    vx_status status = VX_SUCCESS;
    uint32_t size;
    tivxChannelCombineParams *prms = NULL;

    if ((num_params != TIVX_KERNEL_CHANNEL_COMBINE_MAX_PARAMS)
        || (NULL == obj_desc[TIVX_KERNEL_CHANNEL_COMBINE_SRC0_IDX])
        || (NULL == obj_desc[TIVX_KERNEL_CHANNEL_COMBINE_SRC1_IDX])
        || (NULL == obj_desc[TIVX_KERNEL_CHANNEL_COMBINE_DST_IDX]))
    {
        status = VX_FAILURE;
    }

    if (VX_SUCCESS == status)
    {
        status = tivxGetTargetKernelInstanceContext(kernel,
            (void **)&prms, &size);

        if ((VX_SUCCESS == status) && (NULL != prms) &&
            (sizeof(tivxChannelCombineParams) == size))
        {
            tivxBamDestroyHandle(prms->graph_handle);
            tivxMemFree(prms, sizeof(tivxChannelCombineParams), TIVX_MEM_EXTERNAL);
        }
    }

    return (status);
}

static vx_status VX_CALLBACK tivxKernelChannelCombineControl(
    tivx_target_kernel_instance kernel, tivx_obj_desc_t *obj_desc[],
    uint16_t num_params, void *priv_arg)
{
    return (VX_SUCCESS);
}

void tivxAddTargetKernelBamChannelCombine(void)
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

        vx_channel_combine_target_kernel = tivxAddTargetKernel(
            VX_KERNEL_CHANNEL_COMBINE,
            target_name,
            tivxKernelChannelCombineProcess,
            tivxKernelChannelCombineCreate,
            tivxKernelChannelCombineDelete,
            tivxKernelChannelCombineControl,
            NULL);
    }
}


void tivxRemoveTargetKernelBamChannelCombine(void)
{
    tivxRemoveTargetKernel(vx_channel_combine_target_kernel);
}
