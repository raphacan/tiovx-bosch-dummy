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

#include "TI/tivx.h"
#include "TI/j7.h"
#include "tivx_hwa_kernels.h"
#include "tivx_kernel_video_encoder.h"
#include "TI/tivx_target_kernel.h"
#include "tivx_hwa_host_priv.h"

static vx_kernel vx_video_encoder_kernel = NULL;

static vx_status VX_CALLBACK tivxAddKernelVideoEncoderValidate(vx_node node,
            const vx_reference parameters[ ],
            vx_uint32 num,
            vx_meta_format metas[]);
static vx_status VX_CALLBACK tivxAddKernelVideoEncoderInitialize(vx_node node,
            const vx_reference parameters[ ],
            vx_uint32 num_params);

static vx_status VX_CALLBACK tivxAddKernelVideoEncoderValidate(vx_node node,
            const vx_reference parameters[ ],
            vx_uint32 num,
            vx_meta_format metas[])
{
    vx_status status = VX_SUCCESS;

    vx_user_data_object configuration = NULL;
    vx_char configuration_name[VX_MAX_REFERENCE_NAME];
    vx_size configuration_size;
    tivx_video_encoder_params_t configuration_value;

    vx_image input_image = NULL;
    vx_uint32 input_image_w;
    vx_uint32 input_image_h;
    vx_df_image input_image_fmt;

    vx_user_data_object output_bitstream = NULL;
    vx_char output_bitstream_name[VX_MAX_REFERENCE_NAME];
    vx_size output_bitstream_size;

    if ( (num != TIVX_KERNEL_VIDEO_ENCODER_MAX_PARAMS)
        || (NULL == parameters[TIVX_KERNEL_VIDEO_ENCODER_CONFIGURATION_IDX])
        || (NULL == parameters[TIVX_KERNEL_VIDEO_ENCODER_INPUT_IMAGE_IDX])
        || (NULL == parameters[TIVX_KERNEL_VIDEO_ENCODER_OUTPUT_BITSTREAM_IDX])
    )
    {
        status = VX_ERROR_INVALID_PARAMETERS;
        VX_PRINT(VX_ZONE_ERROR, "One or more REQUIRED parameters are set to NULL\n");
    }

    if (VX_SUCCESS == status)
    {
        configuration = (vx_user_data_object)parameters[TIVX_KERNEL_VIDEO_ENCODER_CONFIGURATION_IDX];
        input_image = (vx_image)parameters[TIVX_KERNEL_VIDEO_ENCODER_INPUT_IMAGE_IDX];
        output_bitstream = (vx_user_data_object)parameters[TIVX_KERNEL_VIDEO_ENCODER_OUTPUT_BITSTREAM_IDX];
    }


    /* PARAMETER ATTRIBUTE FETCH */

    if (VX_SUCCESS == status)
    {
        tivxCheckStatus(&status, vxQueryUserDataObject(configuration, VX_USER_DATA_OBJECT_NAME, &configuration_name, sizeof(configuration_name)));
        tivxCheckStatus(&status, vxQueryUserDataObject(configuration, VX_USER_DATA_OBJECT_SIZE, &configuration_size, sizeof(configuration_size)));
        tivxCheckStatus(&status, vxCopyUserDataObject(configuration, 0, sizeof(tivx_video_encoder_params_t), &configuration_value, VX_READ_ONLY, VX_MEMORY_TYPE_HOST));

        tivxCheckStatus(&status, vxQueryImage(input_image, VX_IMAGE_FORMAT, &input_image_fmt, sizeof(input_image_fmt)));
        tivxCheckStatus(&status, vxQueryImage(input_image, VX_IMAGE_WIDTH, &input_image_w, sizeof(input_image_w)));
        tivxCheckStatus(&status, vxQueryImage(input_image, VX_IMAGE_HEIGHT, &input_image_h, sizeof(input_image_h)));

        tivxCheckStatus(&status, vxQueryUserDataObject(output_bitstream, VX_USER_DATA_OBJECT_NAME, &output_bitstream_name, sizeof(output_bitstream_name)));
        tivxCheckStatus(&status, vxQueryUserDataObject(output_bitstream, VX_USER_DATA_OBJECT_SIZE, &output_bitstream_size, sizeof(output_bitstream_size)));
    }

    /* PARAMETER CHECKING */

    if (VX_SUCCESS == status)
    {
        if ((configuration_size != sizeof(tivx_video_encoder_params_t)) ||
            (strncmp(configuration_name, "tivx_video_encoder_params_t", sizeof(configuration_name)) != 0))
        {
            status = VX_ERROR_INVALID_PARAMETERS;
            VX_PRINT(VX_ZONE_ERROR, "'configuration' should be a user_data_object of type:\n tivx_video_encoder_params_t \n");
        }

        if (VX_DF_IMAGE_NV12 != input_image_fmt)
        {
            status = VX_ERROR_INVALID_PARAMETERS;
            VX_PRINT(VX_ZONE_ERROR, "'input_image' should be an image of type:\n VX_DF_IMAGE_NV12 \n");
        }

        if (strncmp(output_bitstream_name, "tivx_video_bitstream_t", sizeof(output_bitstream_name)) != 0)
        {
            status = VX_ERROR_INVALID_PARAMETERS;
            VX_PRINT(VX_ZONE_ERROR, "'output_bitstream' should be a user_data_object of type:\n tivx_video_bitstream_t \n");
        }
    }

    /* CUSTOM PARAMETER CHECKING */

    if (VX_SUCCESS == status)
    {
        if (TIVX_BITSTREAM_FORMAT_H264 != configuration_value.bitstream_format)
        {
            status = VX_ERROR_INVALID_PARAMETERS;
            VX_PRINT(VX_ZONE_ERROR, "Encoder param 'bitstream_format' should be:\n TIVX_BITSTREAM_FORMAT_H264 \n");
        }
    }
    VX_PRINT(VX_ZONE_INFO, "Exit\n");

    return status;
}

static vx_status VX_CALLBACK tivxAddKernelVideoEncoderInitialize(vx_node node,
            const vx_reference parameters[ ],
            vx_uint32 num_params)
{
    vx_status status = VX_SUCCESS;
    tivxKernelValidRectParams prms;
    VX_PRINT(VX_ZONE_INFO, "tivxAddKernelVideoEncoderInitialize entry\n");

    if ( (num_params != TIVX_KERNEL_VIDEO_ENCODER_MAX_PARAMS)
        || (NULL == parameters[TIVX_KERNEL_VIDEO_ENCODER_CONFIGURATION_IDX])
        || (NULL == parameters[TIVX_KERNEL_VIDEO_ENCODER_INPUT_IMAGE_IDX])
        || (NULL == parameters[TIVX_KERNEL_VIDEO_ENCODER_OUTPUT_BITSTREAM_IDX])
    )
    {
        status = VX_ERROR_INVALID_PARAMETERS;
        VX_PRINT(VX_ZONE_ERROR, "One or more REQUIRED parameters are set to NULL\n");
    }
    if (VX_SUCCESS == status)
    {
        tivxKernelValidRectParams_init(&prms);

        prms.in_img[0U] = (vx_image)parameters[TIVX_KERNEL_VIDEO_ENCODER_INPUT_IMAGE_IDX];

        prms.num_input_images = 1;
        prms.num_output_images = 0;

        prms.top_pad = 0;
        prms.bot_pad = 0;
        prms.left_pad = 0;
        prms.right_pad = 0;
        prms.border_mode = VX_BORDER_UNDEFINED;

        status = tivxKernelConfigValidRect(&prms);
    }
    VX_PRINT(VX_ZONE_INFO, "tivxAddKernelVideoEncoderInitialize exit\n");

    return status;
}

vx_status tivxAddKernelVideoEncoder(vx_context context)
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
                    TIVX_KERNEL_VIDEO_ENCODER_NAME,
                    kernel_id,
                    NULL,
                    TIVX_KERNEL_VIDEO_ENCODER_MAX_PARAMS,
                    tivxAddKernelVideoEncoderValidate,
                    tivxAddKernelVideoEncoderInitialize,
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
                        VX_TYPE_USER_DATA_OBJECT,
                        VX_PARAMETER_STATE_REQUIRED
            );
            index++;
        }
        if (status == VX_SUCCESS)
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
                        VX_TYPE_USER_DATA_OBJECT,
                        VX_PARAMETER_STATE_REQUIRED
            );
        }
        if (status == VX_SUCCESS)
        {
            tivxAddKernelTarget(kernel, TIVX_TARGET_VENC1);
            tivxAddKernelTarget(kernel, TIVX_TARGET_VENC2);
	}
        if (status == (vx_status)VX_SUCCESS)
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
    vx_video_encoder_kernel = kernel;

    return status;
}

vx_status tivxRemoveKernelVideoEncoder(vx_context context)
{
    vx_status status;
    vx_kernel kernel = vx_video_encoder_kernel;

    status = vxRemoveKernel(kernel);
    vx_video_encoder_kernel = NULL;

    return status;
}

void tivx_video_encoder_params_init(tivx_video_encoder_params_t *prms)
{
    if (NULL != prms)
    {
        memset(prms, 0x0, sizeof(tivx_video_encoder_params_t));

        prms->bitstream_format = TIVX_BITSTREAM_FORMAT_H264;
        prms->features = TIVX_ENC_FEATURE_CABAC | TIVX_ENC_FEATURE_8x8;
        prms->rcmode = TIVX_ENC_SVBR;
        prms->idr_period = 1; /* for I-only encode, set to 1 */
        prms->i_period = 1; /* for I-only encode, set to 1 */
        prms->bitrate = 10*1000000;
        prms->framerate = 30;
        prms->crop_left = 0;
        prms->crop_right = 0;
        prms->crop_top = 0;
        prms->crop_bottom = 0;
        prms->nslices = 1;
        prms->base_pipe = 0;
        prms->initial_qp_i = 0;
        prms->initial_qp_p = 0;
        prms->initial_qp_b = 0;
        prms->min_qp = 0;
        prms->max_qp = 0;
    }
}
