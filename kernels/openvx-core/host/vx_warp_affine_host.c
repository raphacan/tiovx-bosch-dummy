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
#include <tivx_openvx_core_kernels.h>
#include <tivx_kernel_warp_affine.h>
#include <TI/tivx_target_kernel.h>

static vx_kernel vx_warp_affine_kernel = NULL;

static vx_status VX_CALLBACK tivxAddKernelWarpAffineValidate(vx_node node,
            const vx_reference parameters[ ],
            vx_uint32 num,
            vx_meta_format metas[]);

static vx_status VX_CALLBACK tivxAddKernelWarpAffineInitialize(vx_node node,
            const vx_reference parameters[ ],
            vx_uint32 num_params);

static vx_status VX_CALLBACK tivxAddKernelWarpAffineValidate(vx_node node,
            const vx_reference parameters[ ],
            vx_uint32 num,
            vx_meta_format metas[])
{
    vx_status status = VX_SUCCESS;
    vx_image img[2U];
    vx_matrix matrix;
    vx_uint32 w[2U], h[2U], i;
    vx_size mat_w, mat_h;
    vx_df_image fmt[2U];
    vx_enum matrix_type = 0;
    vx_enum interpolation_type = 0;
    vx_df_image out_fmt;
    vx_border_t border;

    for (i = 0U; i < TIVX_KERNEL_WARP_AFFINE_MAX_PARAMS; i ++)
    {
        /* Check for NULL */
        if (NULL == parameters[i])
        {
            status = VX_ERROR_NO_MEMORY;
            break;
        }
    }
    if (VX_SUCCESS == status)
    {
        img[0U] = (vx_image)parameters[TIVX_KERNEL_WARP_AFFINE_IN0_IMG_IDX];
        img[1U] = (vx_image)parameters[TIVX_KERNEL_WARP_AFFINE_OUT_IMG_IDX];

        /* Get the image width/heigh and format */
        status = vxQueryImage(img[0U], VX_IMAGE_FORMAT, &fmt[0U],
            sizeof(fmt[0U]));

        status |= vxQueryImage(img[0U], VX_IMAGE_WIDTH, &w[0U], sizeof(w[0U]));
        status |= vxQueryImage(img[0U], VX_IMAGE_HEIGHT, &h[0U], sizeof(h[0U]));

        if (VX_SUCCESS == status)
        {
            /* Check for validity of data format */
            if (VX_DF_IMAGE_U8 != fmt[0U])
            {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
        }
    }

    if (VX_SUCCESS == status)
    {
        matrix = (vx_matrix)parameters[TIVX_KERNEL_WARP_AFFINE_IN0_MAT_IDX];

        /* Get the image width/heigh and format */
        status = vxQueryMatrix(matrix, VX_MATRIX_TYPE, &matrix_type, sizeof(matrix_type));
        status |= vxQueryMatrix(matrix, VX_MATRIX_COLUMNS, &mat_w, sizeof(mat_w));
        status |= vxQueryMatrix(matrix, VX_MATRIX_ROWS, &mat_h, sizeof(mat_h));

        if (VX_SUCCESS == status)
        {
            /* Check for validity of data format */
            if (VX_TYPE_FLOAT32 != matrix_type)
            {
                status = VX_ERROR_INVALID_PARAMETERS;
            }

            /* Check for frame sizes */
            if ((mat_w != 2U) || (mat_h != 3U))
            {
                status = VX_ERROR_INVALID_PARAMETERS;
            }
        }
    }

    if (VX_SUCCESS == status)
    {
        vx_enum stype = 0;
        vx_scalar scalar = (vx_scalar)parameters[TIVX_KERNEL_WARP_AFFINE_IN0_SC_IDX];

        status = vxQueryScalar(scalar, VX_SCALAR_TYPE, &stype,
            sizeof(stype));

        if ((VX_SUCCESS == status) && (stype == VX_TYPE_ENUM))
        {
            status = vxCopyScalar(scalar, &interpolation_type, VX_READ_ONLY,
                VX_MEMORY_TYPE_HOST);

            if (VX_SUCCESS == status)
            {
                if((interpolation_type == VX_INTERPOLATION_NEAREST_NEIGHBOR) ||
                   (interpolation_type == VX_INTERPOLATION_BILINEAR))
                {
                    status = VX_SUCCESS;
                }
                else
                {
                    status = VX_ERROR_INVALID_VALUE;
                }
            }
        }
        else
        {
            status = VX_ERROR_INVALID_TYPE;
        }
    }

    if (VX_SUCCESS == status)
    {
        /* Get the image width/heigh and format */
        status = vxQueryImage(img[1U], VX_IMAGE_FORMAT, &fmt[1U],
            sizeof(fmt[1U]));
        status |= vxQueryImage(img[1U], VX_IMAGE_WIDTH, &w[1U], sizeof(w[1U]));
        status |= vxQueryImage(img[1U], VX_IMAGE_HEIGHT, &h[1U], sizeof(h[1U]));

        /* Check for frame sizes */
        /* If output is a virtual image, it must not have a size of 0, 0 */
        if ((w[1U] <= 0) || (h[1U] <= 0))
        {
            status = VX_ERROR_INVALID_PARAMETERS;
        }

        /* Check for format */
        /* If output is a virtual image, the format must be specified */
        if (VX_DF_IMAGE_U8 != fmt[1U])
        {
            status = VX_ERROR_INVALID_PARAMETERS;
        }
    }

    if (VX_SUCCESS == status)
    {
        status = vxQueryNode(node, VX_NODE_BORDER, &border, sizeof(border));
        if (VX_SUCCESS == status)
        {
            if ((border.mode != VX_BORDER_UNDEFINED) &&
                (border.mode != VX_BORDER_CONSTANT))
            {
                status = VX_ERROR_NOT_SUPPORTED;
                VX_PRINT(VX_ZONE_ERROR, "Only undefined and constant border mode is supported for warp affine\n");
            }
        }
    }

#if 1
    if (VX_SUCCESS == status)
    {
        out_fmt = VX_DF_IMAGE_U8;
        for (i = 0U; i < TIVX_KERNEL_WARP_AFFINE_MAX_PARAMS; i ++)
        {
            if (NULL != metas[i])
            {
                vx_enum type = 0;
                vxQueryReference(parameters[i], VX_REFERENCE_TYPE, &type, sizeof(type));
                if (VX_TYPE_IMAGE == type)
                {
                    vxSetMetaFormatAttribute(metas[i], VX_IMAGE_FORMAT, &out_fmt,
                        sizeof(out_fmt));
                    vxSetMetaFormatAttribute(metas[i], VX_IMAGE_WIDTH, &w[1U],
                        sizeof(w[1U]));
                    vxSetMetaFormatAttribute(metas[i], VX_IMAGE_HEIGHT, &h[1U],
                        sizeof(h[1U]));
                }
            }
        }
    }
#endif
    return status;
}


static vx_status VX_CALLBACK tivxAddKernelWarpAffineInitialize(vx_node node,
            const vx_reference parameters[ ],
            vx_uint32 num_params)
{
    vx_status status = VX_SUCCESS;
    vx_uint32 i;
    vx_image out_image;
    vx_rectangle_t out_rect;
    vx_uint32 width, height;

    if (num_params != TIVX_KERNEL_WARP_AFFINE_MAX_PARAMS)
    {
        status = VX_ERROR_INVALID_PARAMETERS;
    }

    for (i = 0U; (i < TIVX_KERNEL_WARP_AFFINE_MAX_PARAMS) &&
            (VX_SUCCESS == status); i ++)
    {
        /* Check for NULL */
        if (NULL == parameters[i])
        {
            status = VX_ERROR_NO_MEMORY;
            break;
        }
    }

    if (VX_SUCCESS == status)
    {
        out_image = (vx_image)parameters[TIVX_KERNEL_WARP_AFFINE_OUT_IMG_IDX];

        status |= vxQueryImage(out_image, VX_IMAGE_WIDTH, &width, sizeof(width));
        status |= vxQueryImage(out_image, VX_IMAGE_HEIGHT, &height, sizeof(height));

        out_rect.start_x = out_rect.start_y = 0;
        out_rect.end_x = width;
        out_rect.end_y = height;

        status |= vxSetImageValidRectangle(out_image, &out_rect);
    }

    return status;
}

vx_status tivxAddKernelWarpAffine(vx_context context)
{
    vx_kernel kernel;
    vx_status status;
    uint32_t index;

    kernel = vxAddUserKernel(
                            context,
                            "org.khronos.openvx.warp_affine",
                            VX_KERNEL_WARP_AFFINE,
                            NULL,
                            TIVX_KERNEL_WARP_AFFINE_MAX_PARAMS,
                            tivxAddKernelWarpAffineValidate,
                            tivxAddKernelWarpAffineInitialize,
                            NULL);

    status = vxGetStatus((vx_reference)kernel);

    if ( status == VX_SUCCESS)
    {
        index = 0;

        if ( status == VX_SUCCESS)
        {
            status = vxAddParameterToKernel(kernel,
                index,
                VX_INPUT,
                VX_TYPE_IMAGE,
                VX_PARAMETER_STATE_REQUIRED
                );
            index++;
        }
        if ( status == VX_SUCCESS)
        {
            status = vxAddParameterToKernel(kernel,
                index,
                VX_INPUT,
                VX_TYPE_MATRIX,
                VX_PARAMETER_STATE_REQUIRED
                );
            index++;
        }
        if ( status == VX_SUCCESS)
        {
            status = vxAddParameterToKernel(kernel,
                index,
                VX_INPUT,
                VX_TYPE_SCALAR,
                VX_PARAMETER_STATE_REQUIRED
                );
            index++;
        }
        if ( status == VX_SUCCESS)
        {
            status = vxAddParameterToKernel(kernel,
                index,
                VX_OUTPUT,
                VX_TYPE_IMAGE,
                VX_PARAMETER_STATE_REQUIRED
                );
            index++;
        }
        if ( status == VX_SUCCESS)
        {
            /* add supported target's */
            tivxAddKernelTarget(kernel, TIVX_TARGET_DSP1);
            tivxAddKernelTarget(kernel, TIVX_TARGET_DSP2);
        }

        if ( status == VX_SUCCESS)
        {
            status = vxFinalizeKernel(kernel);
        }
        if( status != VX_SUCCESS)
        {
            vxReleaseKernel(&kernel);
            kernel = NULL;
        }
    }
    else
    {
        kernel = NULL;
    }

    vx_warp_affine_kernel = kernel;

    return status;
}

vx_status tivxRemoveKernelWarpAffine(vx_context context)
{
    vx_status status;
    vx_kernel kernel = vx_warp_affine_kernel;

    /* Kernel is released as part of Remove Kernel */
    status = vxRemoveKernel(kernel);

    vx_warp_affine_kernel = NULL;

    return status;
}





