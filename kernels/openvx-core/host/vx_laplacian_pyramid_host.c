/*
 *
 * Copyright (c) 2019 Texas Instruments Incorporated
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
#include <tivx_openvx_core_kernels.h>
#include <tivx_kernel_laplacian_pyramid.h>
#include <TI/tivx_target_kernel.h>
#include <math.h>

static vx_kernel vx_laplacian_pyramid_kernel = NULL;

static vx_status VX_CALLBACK tivxAddKernelLaplacianPyramidValidate(vx_node node,
            const vx_reference parameters[ ],
            vx_uint32 num,
            vx_meta_format metas[]);
static vx_status VX_CALLBACK tivxAddKernelLaplacianPyramidInitialize(vx_node node,
            const vx_reference parameters[ ],
            vx_uint32 num_params);
vx_status tivxAddKernelLaplacianPyramid(vx_context context);
vx_status tivxRemoveKernelLaplacianPyramid(vx_context context);

static vx_status VX_CALLBACK tivxAddKernelLaplacianPyramidValidate(vx_node node,
            const vx_reference parameters[ ],
            vx_uint32 num,
            vx_meta_format metas[])
{
    vx_status status = VX_SUCCESS;

    vx_image input = NULL;
    vx_uint32 input_w;
    vx_uint32 input_h;
    vx_df_image input_fmt;

    vx_pyramid laplacian = NULL;
    vx_uint32 laplacian_w;
    vx_uint32 laplacian_h;
    vx_df_image laplacian_fmt;
    vx_float32 laplacian_scale;
    vx_size laplacian_levels;

    vx_image output = NULL;
    vx_uint32 output_w;
    vx_uint32 output_h;
    vx_df_image output_fmt;

    vx_border_t border;

    vx_bool laplacian_is_virtual = vx_false_e;
    vx_bool output_is_virtual = vx_false_e;

    vx_uint32 w;
    vx_uint32 h;
    vx_uint32 i;

    if ( (num != TIVX_KERNEL_LAPLACIAN_PYRAMID_MAX_PARAMS)
        || (NULL == parameters[TIVX_KERNEL_LAPLACIAN_PYRAMID_INPUT_IDX])
        || (NULL == parameters[TIVX_KERNEL_LAPLACIAN_PYRAMID_LAPLACIAN_IDX])
        || (NULL == parameters[TIVX_KERNEL_LAPLACIAN_PYRAMID_OUTPUT_IDX])
    )
    {
        status = VX_ERROR_INVALID_PARAMETERS;
        VX_PRINT(VX_ZONE_ERROR, "One or more REQUIRED parameters are set to NULL\n");
    }

    if (VX_SUCCESS == status)
    {
        input = (vx_image)parameters[TIVX_KERNEL_LAPLACIAN_PYRAMID_INPUT_IDX];
        laplacian = (vx_pyramid)parameters[TIVX_KERNEL_LAPLACIAN_PYRAMID_LAPLACIAN_IDX];
        output = (vx_image)parameters[TIVX_KERNEL_LAPLACIAN_PYRAMID_OUTPUT_IDX];
    }


    /* PARAMETER ATTRIBUTE FETCH */

    if (VX_SUCCESS == status)
    {
        tivxCheckStatus(&status, vxQueryImage(input, VX_IMAGE_FORMAT, &input_fmt, sizeof(input_fmt)));
        tivxCheckStatus(&status, vxQueryImage(input, VX_IMAGE_WIDTH, &input_w, sizeof(input_w)));
        tivxCheckStatus(&status, vxQueryImage(input, VX_IMAGE_HEIGHT, &input_h, sizeof(input_h)));

        tivxCheckStatus(&status, vxQueryPyramid(laplacian, VX_PYRAMID_FORMAT, &laplacian_fmt, sizeof(laplacian_fmt)));
        tivxCheckStatus(&status, vxQueryPyramid(laplacian, VX_PYRAMID_WIDTH, &laplacian_w, sizeof(laplacian_w)));
        tivxCheckStatus(&status, vxQueryPyramid(laplacian, VX_PYRAMID_HEIGHT, &laplacian_h, sizeof(laplacian_h)));
        tivxCheckStatus(&status, vxQueryPyramid(laplacian, VX_PYRAMID_SCALE, &laplacian_scale, sizeof(laplacian_scale)));
        tivxCheckStatus(&status, vxQueryPyramid(laplacian, VX_PYRAMID_LEVELS, &laplacian_levels, sizeof(laplacian_levels)));

        tivxCheckStatus(&status, vxQueryImage(output, VX_IMAGE_FORMAT, &output_fmt, sizeof(output_fmt)));
        tivxCheckStatus(&status, vxQueryImage(output, VX_IMAGE_WIDTH, &output_w, sizeof(output_w)));
        tivxCheckStatus(&status, vxQueryImage(output, VX_IMAGE_HEIGHT, &output_h, sizeof(output_h)));

        tivxCheckStatus(&status, vxQueryNode(node, VX_NODE_BORDER, &border, sizeof(border)));

#if 1

        laplacian_is_virtual = tivxIsReferenceVirtual((vx_reference)laplacian);
        output_is_virtual = tivxIsReferenceVirtual((vx_reference)output);

#endif

    }


    /* PARAMETER CHECKING */

    if (VX_SUCCESS == status)
    {
        if (VX_DF_IMAGE_U8 != input_fmt)
        {
            status = VX_ERROR_INVALID_PARAMETERS;
            VX_PRINT(VX_ZONE_ERROR, "'input' should be an image of type:\n VX_DF_IMAGE_U8 \n");
        }

        if (vx_false_e == laplacian_is_virtual)
        {
            if (VX_DF_IMAGE_S16 != laplacian_fmt)
            {
                status = VX_ERROR_INVALID_PARAMETERS;
                VX_PRINT(VX_ZONE_ERROR, "'laplacian' should be a pyramid of type:\n VX_DF_IMAGE_S16 \n");
            }
        }

        if (vx_false_e == output_is_virtual)
        {
            if (VX_DF_IMAGE_U8 != output_fmt)
            {
                status = VX_ERROR_INVALID_PARAMETERS;
                VX_PRINT(VX_ZONE_ERROR, "'output' should be an image of type:\n VX_DF_IMAGE_U8 \n");
            }
        }
    }


    /* PARAMETER RELATIONSHIP CHECKING */

    if (VX_SUCCESS == status)
    {
        w = input_w;
        h = input_h;

        for (i = 0U; i < laplacian_levels; i++)
        {
            w = (vx_uint32)ceilf(w * laplacian_scale);
            h = (vx_uint32)ceilf(h * laplacian_scale);
        }

        if (vx_false_e == laplacian_is_virtual)
        {
            if (input_w != laplacian_w)
            {
                status = VX_ERROR_INVALID_PARAMETERS;
                VX_PRINT(VX_ZONE_ERROR, "'input' and 'laplacian' should have the same value for 'width' \n");
            }

            if (input_h != laplacian_h)
            {
                status = VX_ERROR_INVALID_PARAMETERS;
                VX_PRINT(VX_ZONE_ERROR, "'input' and 'laplacian' should have the same value for 'height' \n");
            }
        }

        if (vx_false_e == output_is_virtual)
        {
            if (w != output_w)
            {
                status = VX_ERROR_INVALID_PARAMETERS;
                VX_PRINT(VX_ZONE_ERROR, "Invalid value of 'width' for 'output' with given number of levels and value 'width' for 'input' \n");
            }

            if (h != output_h)
            {
                status = VX_ERROR_INVALID_PARAMETERS;
                VX_PRINT(VX_ZONE_ERROR, "Invalid value of 'height' for 'output' with given number of levels and value 'height' for 'input' \n");
            }
        }
    }


    /* CUSTOM PARAMETER CHECKING */

    if (VX_SUCCESS == status)
    {
        if (VX_SCALE_PYRAMID_HALF != laplacian_scale)
        {
            status = VX_ERROR_INVALID_PARAMETERS;
            VX_PRINT(VX_ZONE_ERROR, "'laplacian' should have 'scale' of type:\n VX_SCALE_PYRAMID_HALF \n");
        }
    }

    if (VX_SUCCESS == status)
    {
        if ((VX_BORDER_UNDEFINED != border.mode) &&
            (VX_BORDER_REPLICATE != border.mode))
        {
            status = VX_ERROR_NOT_SUPPORTED;
            VX_PRINT(VX_ZONE_ERROR, "Only undefined and replicate border mode is supported for laplacian pyramid \n");
        }
    }

#if 1

    if (VX_SUCCESS == status)
    {
        laplacian_fmt = VX_DF_IMAGE_S16;
        output_fmt = VX_DF_IMAGE_U8;

        vxSetMetaFormatAttribute(metas[TIVX_KERNEL_LAPLACIAN_PYRAMID_LAPLACIAN_IDX], VX_PYRAMID_WIDTH, &input_w, sizeof(input_w));
        vxSetMetaFormatAttribute(metas[TIVX_KERNEL_LAPLACIAN_PYRAMID_LAPLACIAN_IDX], VX_PYRAMID_HEIGHT, &input_h, sizeof(input_h));
        vxSetMetaFormatAttribute(metas[TIVX_KERNEL_LAPLACIAN_PYRAMID_LAPLACIAN_IDX], VX_PYRAMID_FORMAT, &laplacian_fmt, sizeof(laplacian_fmt));
        vxSetMetaFormatAttribute(metas[TIVX_KERNEL_LAPLACIAN_PYRAMID_LAPLACIAN_IDX], VX_PYRAMID_LEVELS, &laplacian_levels, sizeof(laplacian_levels));
        vxSetMetaFormatAttribute(metas[TIVX_KERNEL_LAPLACIAN_PYRAMID_LAPLACIAN_IDX], VX_PYRAMID_SCALE, &laplacian_scale, sizeof(laplacian_scale));
        vxSetMetaFormatAttribute(metas[TIVX_KERNEL_LAPLACIAN_PYRAMID_OUTPUT_IDX], VX_IMAGE_FORMAT, &output_fmt, sizeof(output_fmt));
        vxSetMetaFormatAttribute(metas[TIVX_KERNEL_LAPLACIAN_PYRAMID_OUTPUT_IDX], VX_IMAGE_WIDTH, &w, sizeof(w));
        vxSetMetaFormatAttribute(metas[TIVX_KERNEL_LAPLACIAN_PYRAMID_OUTPUT_IDX], VX_IMAGE_HEIGHT, &h, sizeof(h));
    }

#endif

    return status;
}

static vx_status VX_CALLBACK tivxAddKernelLaplacianPyramidInitialize(vx_node node,
            const vx_reference parameters[ ],
            vx_uint32 num_params)
{
    vx_status status = VX_SUCCESS;
    tivxKernelValidRectParams prms;

    vx_pyramid laplacian;
    vx_size laplacian_levels;

    vx_image img, in_img, out_img;
    vx_uint32 i;

    if ( (num_params != TIVX_KERNEL_LAPLACIAN_PYRAMID_MAX_PARAMS)
        || (NULL == parameters[TIVX_KERNEL_LAPLACIAN_PYRAMID_INPUT_IDX])
        || (NULL == parameters[TIVX_KERNEL_LAPLACIAN_PYRAMID_LAPLACIAN_IDX])
        || (NULL == parameters[TIVX_KERNEL_LAPLACIAN_PYRAMID_OUTPUT_IDX])
    )
    {
        status = VX_ERROR_INVALID_PARAMETERS;
        VX_PRINT(VX_ZONE_ERROR, "One or more REQUIRED parameters are set to NULL\n");
    }

    if (VX_SUCCESS == status)
    {
        laplacian = (vx_pyramid)parameters[TIVX_KERNEL_LAPLACIAN_PYRAMID_LAPLACIAN_IDX];
    }

    if (VX_SUCCESS == status)
    {
        tivxCheckStatus(&status, vxQueryPyramid(laplacian, VX_PYRAMID_LEVELS, &laplacian_levels, sizeof(laplacian_levels)));
    }

    if (VX_SUCCESS == status)
    {
        tivxKernelValidRectParams_init(&prms);
        prms.in_img[0U] = (vx_image)parameters[TIVX_KERNEL_LAPLACIAN_PYRAMID_INPUT_IDX];
        img = vxGetPyramidLevel(laplacian, 0U);
        prms.out_img[0U] = img;

        prms.num_input_images = 1U;
        prms.num_output_images = 1U;

        prms.top_pad = 0U;
        prms.bot_pad = 0U;
        prms.left_pad = 0U;
        prms.right_pad = 0U;

        prms.border_mode = VX_BORDER_UNDEFINED;

        tivxCheckStatus(&status, tivxKernelConfigValidRect(&prms));
        tivxCheckStatus(&status, vxReleaseImage(&img));
    }

    if (VX_SUCCESS == status)
    {
        for (i = 1U; i < laplacian_levels; i++)
        {
            in_img = vxGetPyramidLevel(laplacian, i - 1U);

            out_img = vxGetPyramidLevel(laplacian, i);

            prms.in_img[0U] = in_img;
            prms.out_img[0U] = out_img;

            prms.num_input_images = 1U;
            prms.num_output_images = 1U;

            prms.top_pad = 0U;
            prms.bot_pad = 0U;
            prms.left_pad = 0U;
            prms.right_pad = 0U;

            prms.border_mode = VX_BORDER_UNDEFINED;

            tivxCheckStatus(&status, tivxKernelConfigValidRect(&prms));

            tivxCheckStatus(&status, vxReleaseImage(&out_img));
            tivxCheckStatus(&status, vxReleaseImage(&in_img));
        }
    }

    if (VX_SUCCESS == status)
    {
        in_img = vxGetPyramidLevel(laplacian, laplacian_levels - 1U);
        prms.in_img[0U] = in_img;
        prms.out_img[0U] = (vx_image)parameters[TIVX_KERNEL_LAPLACIAN_PYRAMID_OUTPUT_IDX];

        prms.num_input_images = 1U;
        prms.num_output_images = 1U;

        prms.top_pad = 0U;
        prms.bot_pad = 0U;
        prms.left_pad = 0U;
        prms.right_pad = 0U;

        prms.border_mode = VX_BORDER_UNDEFINED;

        tivxCheckStatus(&status, tivxKernelConfigValidRect(&prms));
        tivxCheckStatus(&status, vxReleaseImage(&in_img));
    }

    return status;
}

vx_status tivxAddKernelLaplacianPyramid(vx_context context)
{
    vx_kernel kernel;
    vx_status status;
    uint32_t index;
    vx_enum kernel_id;

    status = vxAllocateUserKernelId(context, &kernel_id);
    if(status != VX_SUCCESS)
    {
        VX_PRINT(VX_ZONE_ERROR, "Unable to allocate user kernel ID\n");
    }

    if (status == VX_SUCCESS)
    {
        kernel = vxAddUserKernel(
                    context,
                    "org.khronos.openvx.laplacian_pyramid",
                    VX_KERNEL_LAPLACIAN_PYRAMID,
                    NULL,
                    TIVX_KERNEL_LAPLACIAN_PYRAMID_MAX_PARAMS,
                    tivxAddKernelLaplacianPyramidValidate,
                    tivxAddKernelLaplacianPyramidInitialize,
                    NULL);

        status = vxGetStatus((vx_reference)kernel);
    }
    if (status == VX_SUCCESS)
    {
        index = 0;

        {
            status = vxAddParameterToKernel(kernel,
                        index,
                        VX_INPUT,
                        VX_TYPE_IMAGE,
                        VX_PARAMETER_STATE_REQUIRED
            );
            index++;
        }
        if (status == VX_SUCCESS)
        {
            status = vxAddParameterToKernel(kernel,
                        index,
                        VX_OUTPUT,
                        VX_TYPE_PYRAMID,
                        VX_PARAMETER_STATE_REQUIRED
            );
            index++;
        }
        if (status == VX_SUCCESS)
        {
            status = vxAddParameterToKernel(kernel,
                        index,
                        VX_OUTPUT,
                        VX_TYPE_IMAGE,
                        VX_PARAMETER_STATE_REQUIRED
            );
        }
        if (status == VX_SUCCESS)
        {
            /* add supported target's */
            tivxAddKernelTarget(kernel, TIVX_TARGET_DSP1);
            tivxAddKernelTarget(kernel, TIVX_TARGET_DSP2);
        }
        if (status == VX_SUCCESS)
        {
            status = vxFinalizeKernel(kernel);
        }
        if (status != VX_SUCCESS)
        {
            vxReleaseKernel(&kernel);
            kernel = NULL;
        }
    }
    else
    {
        kernel = NULL;
    }
    vx_laplacian_pyramid_kernel = kernel;

    return status;
}

vx_status tivxRemoveKernelLaplacianPyramid(vx_context context)
{
    vx_status status;
    vx_kernel kernel = vx_laplacian_pyramid_kernel;

    status = vxRemoveKernel(kernel);
    vx_laplacian_pyramid_kernel = NULL;

    return status;
}


