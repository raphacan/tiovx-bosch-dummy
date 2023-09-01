/*
 * Copyright (c) 2012-2016 The Khronos Group Inc.
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


/*!
 * \file
 * \brief The Graph Mode Interface for all Base Kernels.
 * \author Erik Rainey <erik.rainey@gmail.com>
 */

#include <vx_internal.h>

static vx_node vxCreateNodeByStructure(vx_graph graph,
                                vx_kernel kernel,
                                vx_enum kernelenum,
                                vx_reference params[],
                                vx_uint32 num);

/* Note: with the current sample implementation structure, we have no other choice than
returning 0 in case of errors not due to vxCreateGenericNode, because vxGetErrorObject
is internal to another library and is not exported. This is not an issue since vxGetStatus
correctly manages a ref == 0 */
static vx_node vxCreateNodeByStructure(vx_graph graph,
                                vx_kernel kernel,
                                vx_enum kernelenum,
                                vx_reference params[],
                                vx_uint32 num)
{
    vx_status status = (vx_status)VX_SUCCESS;
    vx_node node = NULL;
    vx_uint32 release_kernel = 0;
    vx_context context = vxGetContext((vx_reference)graph);

    if(kernel==NULL)
    {
        kernel = vxGetKernelByEnum(context, kernelenum);
        release_kernel = 1;
    }
    if (kernel != NULL)
    {
        node = vxCreateGenericNode(graph, kernel);
        if (vxGetStatus((vx_reference)node) == (vx_status)VX_SUCCESS)
        {
            vx_uint32 p = 0;
            for (p = 0; p < num; p++)
            {
                status = vxSetParameterByIndex(node, p, params[p]);
                if (status != (vx_status)VX_SUCCESS)
                {
                    vxAddLogEntry((vx_reference)graph, status, "Kernel %d Parameter %u is invalid.\n", kernelenum, p);
                    status = vxReleaseNode(&node);
                    if((vx_status)VX_SUCCESS != status)
                    {
                        VX_PRINT(VX_ZONE_ERROR,"Failed to release reference to node, node might not be a vx_node\n");
                    }
                    node = NULL;
                    break;
                }
            }
        }
        else
        {
            vxAddLogEntry((vx_reference)graph, (vx_status)VX_ERROR_INVALID_PARAMETERS, "Failed to create node with kernel enum %d\n", kernelenum);
            VX_PRINT(VX_ZONE_ERROR, "Failed to create node with kernel enum %d\n", kernelenum);
        }
        if (release_kernel != 0U)
        {
            status = vxReleaseKernel(&kernel);
            if((vx_status)VX_SUCCESS != status)
            {
                VX_PRINT(VX_ZONE_ERROR,"Failed to release reference to kernel, kernel might not be a vx_kernel\n");
            }
        }
    }
    else
    {
        vxAddLogEntry((vx_reference)graph, (vx_status)VX_ERROR_INVALID_PARAMETERS, "failed to retrieve kernel enum %d\n", kernelenum);
        VX_PRINT(VX_ZONE_ERROR, "failed to retrieve kernel enum %d\n", kernelenum);
    }
    return node;
}

vx_node tivxCreateNodeByKernelEnum(vx_graph graph,
                                vx_enum kernelenum,
                                vx_reference params[],
                                vx_uint32 num)
{
    return vxCreateNodeByStructure(graph, NULL, kernelenum, params, num);
}

vx_node tivxCreateNodeByKernelRef(vx_graph graph,
                                vx_kernel kernel,
                                vx_reference params[],
                                vx_uint32 num)
{
    return vxCreateNodeByStructure(graph, kernel, 0, params, num);
}

vx_node tivxCreateNodeByKernelName(vx_graph graph,
                                const char *kernel_name,
                                vx_reference params[],
                                vx_uint32 num)
{
    vx_node node = NULL;
    vx_kernel kernel;
    vx_context context = vxGetContext((vx_reference)graph);
    vx_status status = (vx_status)VX_SUCCESS;
    kernel = vxGetKernelByName(context, kernel_name);
    if(kernel!=NULL)
    {
        /* kernel is released inside vxCreateNodeByStructure */
        node =  vxCreateNodeByStructure(graph, kernel, 0, params, num);
        status = vxReleaseKernel(&kernel);
        if((vx_status)VX_SUCCESS != status)
        {
            VX_PRINT(VX_ZONE_ERROR,"Failed to release reference to kernel, kernel might not be a vx_kernel\n");
        }
    }
    else
    {
        VX_PRINT(VX_ZONE_ERROR, "Call to vxGetKernelByName failed; kernel may not be registered\n");
    }
    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxColorConvertNode(vx_graph graph, vx_image input, vx_image output)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)output,
    };
    return tivxCreateNodeByKernelEnum(graph, (vx_enum)VX_KERNEL_COLOR_CONVERT, params, dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxChannelExtractNode(vx_graph graph,
                             vx_image input,
                             vx_enum channelNum,
                             vx_image output)
{
    vx_context context = vxGetContext((vx_reference)graph);
    vx_status status = (vx_status)VX_SUCCESS;
    vx_scalar scalar = vxCreateScalar(context, (vx_enum)VX_TYPE_ENUM, &channelNum);
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)scalar,
        (vx_reference)output,
    };
    vx_node node = tivxCreateNodeByKernelEnum(graph,
                                           (vx_enum)VX_KERNEL_CHANNEL_EXTRACT,
                                           params,
                                           dimof(params));
    status = vxReleaseScalar(&scalar); /* node hold reference */
    if((vx_status)VX_SUCCESS != status)
    {
        VX_PRINT(VX_ZONE_ERROR,"Failed to release reference to scalar, scalar might not be a vx_scalar\n");
    }
    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxChannelCombineNode(vx_graph graph,
                             vx_image plane0,
                             vx_image plane1,
                             vx_image plane2,
                             vx_image plane3,
                             vx_image output)
{
    vx_reference params[] = {
       (vx_reference)plane0,
       (vx_reference)plane1,
       (vx_reference)plane2,
       (vx_reference)plane3,
       (vx_reference)output,
    };
    return tivxCreateNodeByKernelEnum(graph,
                                   (vx_enum)VX_KERNEL_CHANNEL_COMBINE,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxSobel3x3Node(vx_graph graph, vx_image input, vx_image output_x, vx_image output_y)
{
    vx_reference params[] = {
       (vx_reference)input,
       (vx_reference)output_x,
       (vx_reference)output_y,
    };
    return tivxCreateNodeByKernelEnum(graph,
                                   (vx_enum)VX_KERNEL_SOBEL_3x3,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxMagnitudeNode(vx_graph graph, vx_image grad_x, vx_image grad_y, vx_image mag)
{
    vx_reference params[] = {
       (vx_reference)grad_x,
       (vx_reference)grad_y,
       (vx_reference)mag,
    };
    return tivxCreateNodeByKernelEnum(graph,
                                   (vx_enum)VX_KERNEL_MAGNITUDE,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxPhaseNode(vx_graph graph, vx_image grad_x, vx_image grad_y, vx_image orientation)
{
    vx_reference params[] = {
       (vx_reference)grad_x,
       (vx_reference)grad_y,
       (vx_reference)orientation,
    };
    return tivxCreateNodeByKernelEnum(graph,
                                   (vx_enum)VX_KERNEL_PHASE,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxScaleImageNode(vx_graph graph, vx_image src, vx_image dst, vx_enum type)
{
    vx_context context = vxGetContext((vx_reference)graph);
    vx_status status = (vx_status)VX_SUCCESS;
    vx_scalar stype = vxCreateScalar(context, (vx_enum)VX_TYPE_ENUM, &type);
    vx_reference params[] = {
        (vx_reference)src,
        (vx_reference)dst,
        (vx_reference)stype,
    };
    vx_node node = tivxCreateNodeByKernelEnum(graph,
                                           (vx_enum)VX_KERNEL_SCALE_IMAGE,
                                           params,
                                           dimof(params));
    status = vxReleaseScalar(&stype);
    if((vx_status)VX_SUCCESS != status)
    {
        VX_PRINT(VX_ZONE_ERROR,"Failed to release reference to scalar, stype might not be a vx_scalar\n");
    }
    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxTableLookupNode(vx_graph graph, vx_image input, vx_lut lut, vx_image output)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)lut,
        (vx_reference)output,
    };
    return tivxCreateNodeByKernelEnum(graph,
                                   (vx_enum)VX_KERNEL_TABLE_LOOKUP,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxHistogramNode(vx_graph graph, vx_image input, vx_distribution distribution)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)distribution,
    };
    return tivxCreateNodeByKernelEnum(graph,
                                   (vx_enum)VX_KERNEL_HISTOGRAM,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxEqualizeHistNode(vx_graph graph, vx_image input, vx_image output)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)output,
    };
    return tivxCreateNodeByKernelEnum(graph,
                                   (vx_enum)VX_KERNEL_EQUALIZE_HISTOGRAM,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxAbsDiffNode(vx_graph graph, vx_image in1, vx_image in2, vx_image out)
{
    vx_reference params[] = {
       (vx_reference)in1,
       (vx_reference)in2,
       (vx_reference)out,
    };
    return tivxCreateNodeByKernelEnum(graph,
                                   (vx_enum)VX_KERNEL_ABSDIFF,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxMeanStdDevNode(vx_graph graph, vx_image input, vx_scalar mean, vx_scalar stddev)
{
    vx_reference params[] = {
       (vx_reference)input,
       (vx_reference)mean,
       (vx_reference)stddev,
    };
    return tivxCreateNodeByKernelEnum(graph,
                                   (vx_enum)VX_KERNEL_MEAN_STDDEV,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxThresholdNode(vx_graph graph, vx_image input, vx_threshold thesh, vx_image output)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)thesh,
        (vx_reference)output,
    };
    return tivxCreateNodeByKernelEnum(graph,
                                   (vx_enum)VX_KERNEL_THRESHOLD,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxIntegralImageNode(vx_graph graph, vx_image input, vx_image output)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)output,
    };
    return tivxCreateNodeByKernelEnum(graph,
                                   (vx_enum)VX_KERNEL_INTEGRAL_IMAGE,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxErode3x3Node(vx_graph graph, vx_image input, vx_image output)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)output,
    };
    return tivxCreateNodeByKernelEnum(graph,
                                   (vx_enum)VX_KERNEL_ERODE_3x3,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxDilate3x3Node(vx_graph graph, vx_image input, vx_image output)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)output,
    };
    return tivxCreateNodeByKernelEnum(graph,
                                   (vx_enum)VX_KERNEL_DILATE_3x3,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxMedian3x3Node(vx_graph graph, vx_image input, vx_image output)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)output,
    };
    return tivxCreateNodeByKernelEnum(graph,
                                   (vx_enum)VX_KERNEL_MEDIAN_3x3,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxBox3x3Node(vx_graph graph, vx_image input, vx_image output)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)output,
    };
    return tivxCreateNodeByKernelEnum(graph,
                                   (vx_enum)VX_KERNEL_BOX_3x3,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxGaussian3x3Node(vx_graph graph, vx_image input, vx_image output)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)output,
    };
    return tivxCreateNodeByKernelEnum(graph,
                                   (vx_enum)VX_KERNEL_GAUSSIAN_3x3,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxNonLinearFilterNode(vx_graph graph, vx_enum function, vx_image input, vx_matrix mask, vx_image output)
{
    vx_scalar func = vxCreateScalar(vxGetContext((vx_reference)graph), (vx_enum)VX_TYPE_ENUM, &function);
    vx_status status = (vx_status)VX_SUCCESS;
    vx_reference params[] = {
        (vx_reference)func,
        (vx_reference)input,
        (vx_reference)mask,
        (vx_reference)output,
    };

    vx_node node = tivxCreateNodeByKernelEnum(graph,
        (vx_enum)VX_KERNEL_NON_LINEAR_FILTER,
        params,
        dimof(params));

    status = vxReleaseScalar(&func);
    if((vx_status)VX_SUCCESS != status)
    {
        VX_PRINT(VX_ZONE_ERROR,"Failed to release reference to scalar, func might not be a vx_scalar\n");
    }
    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxConvolveNode(vx_graph graph, vx_image input, vx_convolution conv, vx_image output)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)conv,
        (vx_reference)output,
    };
    return tivxCreateNodeByKernelEnum(graph,
                                   (vx_enum)VX_KERNEL_CUSTOM_CONVOLUTION,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxGaussianPyramidNode(vx_graph graph, vx_image input, vx_pyramid gaussian)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)gaussian,
    };
    return tivxCreateNodeByKernelEnum(graph,
                                   (vx_enum)VX_KERNEL_GAUSSIAN_PYRAMID,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxLaplacianPyramidNode(vx_graph graph, vx_image input, vx_pyramid laplacian, vx_image output)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)laplacian,
        (vx_reference)output,
    };
    return tivxCreateNodeByKernelEnum(graph,
                                   (vx_enum)VX_KERNEL_LAPLACIAN_PYRAMID,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxLaplacianReconstructNode(vx_graph graph, vx_pyramid laplacian, vx_image input,
                                       vx_image output)
{
    vx_reference params[] = {
        (vx_reference)laplacian,
        (vx_reference)input,
        (vx_reference)output,
    };
    return tivxCreateNodeByKernelEnum(graph,
                                   (vx_enum)VX_KERNEL_LAPLACIAN_RECONSTRUCT,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxAccumulateImageNode(vx_graph graph, vx_image input, vx_image accum)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)accum,
    };
    return tivxCreateNodeByKernelEnum(graph,
                                   (vx_enum)VX_KERNEL_ACCUMULATE,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxAccumulateWeightedImageNode(vx_graph graph, vx_image input, vx_scalar alpha, vx_image accum)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)alpha,
        (vx_reference)accum,
    };
    return tivxCreateNodeByKernelEnum(graph,
                                   (vx_enum)VX_KERNEL_ACCUMULATE_WEIGHTED,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxAccumulateWeightedImageNodeX(vx_graph graph, vx_image input, vx_float32 alpha, vx_image accum)
{
    vx_scalar salpha = vxCreateScalar(vxGetContext((vx_reference)graph), (vx_enum)VX_TYPE_FLOAT32, &alpha);
    vx_node node = vxAccumulateWeightedImageNode(graph, input, salpha, accum);
    vxReleaseScalar(&salpha);
    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxAccumulateSquareImageNode(vx_graph graph, vx_image input, vx_scalar scalar, vx_image accum)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)scalar,
        (vx_reference)accum,
    };
    return tivxCreateNodeByKernelEnum(graph,
                                   (vx_enum)VX_KERNEL_ACCUMULATE_SQUARE,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxAccumulateSquareImageNodeX(vx_graph graph, vx_image input, vx_uint32 shift, vx_image accum)
{
    vx_scalar scalar = vxCreateScalar(vxGetContext((vx_reference)graph), (vx_enum)VX_TYPE_UINT32, &shift);
    vx_node node = vxAccumulateSquareImageNode(graph, input, scalar, accum);
    vxReleaseScalar(&scalar);
    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxMinMaxLocNode(vx_graph graph,
                        vx_image input,
                        vx_scalar minVal, vx_scalar maxVal,
                        vx_array minLoc, vx_array maxLoc,
                        vx_scalar minCount, vx_scalar maxCount)
{
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)minVal,
        (vx_reference)maxVal,
        (vx_reference)minLoc,
        (vx_reference)maxLoc,
        (vx_reference)minCount,
        (vx_reference)maxCount,
    };
    return tivxCreateNodeByKernelEnum(graph,
                                   (vx_enum)VX_KERNEL_MINMAXLOC,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxConvertDepthNode(vx_graph graph, vx_image input, vx_image output, vx_enum policy, vx_scalar shift)
{
    vx_status status = (vx_status)VX_SUCCESS;
    vx_scalar pol = vxCreateScalar(vxGetContext((vx_reference)graph), (vx_enum)VX_TYPE_ENUM, &policy);
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)output,
        (vx_reference)pol,
        (vx_reference)shift,
    };
    vx_node node = tivxCreateNodeByKernelEnum(graph,
                                   (vx_enum)VX_KERNEL_CONVERTDEPTH,
                                   params,
                                   dimof(params));
    status = vxReleaseScalar(&pol);
    if((vx_status)VX_SUCCESS != status)
    {
        VX_PRINT(VX_ZONE_ERROR,"Failed to release reference to scalar, pol might not be a vx_scalar\n");
    }
    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxCannyEdgeDetectorNode(vx_graph graph, vx_image input, vx_threshold hyst,
                                vx_int32 gradient_size, vx_enum norm_type,
                                vx_image output)
{
    vx_status status = (vx_status)VX_SUCCESS;
    vx_scalar gs = vxCreateScalar(vxGetContext((vx_reference)graph), (vx_enum)VX_TYPE_INT32, &gradient_size);
    vx_scalar nt = vxCreateScalar(vxGetContext((vx_reference)graph), (vx_enum)VX_TYPE_ENUM, &norm_type);
    vx_reference params[] = {
        (vx_reference)input,
        (vx_reference)hyst,
        (vx_reference)gs,
        (vx_reference)nt,
        (vx_reference)output,
    };
    vx_node node = tivxCreateNodeByKernelEnum(graph,
                                           (vx_enum)VX_KERNEL_CANNY_EDGE_DETECTOR,
                                           params,
                                           dimof(params));
    status = vxReleaseScalar(&gs);
    if((vx_status)VX_SUCCESS != status)
    {
        VX_PRINT(VX_ZONE_ERROR,"Failed to release reference to scalar, gs might not be a vx_scalar\n");
    }
    status = vxReleaseScalar(&nt);
    if((vx_status)VX_SUCCESS != status)
    {
        VX_PRINT(VX_ZONE_ERROR,"Failed to release reference to scalar, nt might not be a vx_scalar\n");
    }
    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxAndNode(vx_graph graph, vx_image in1, vx_image in2, vx_image out)
{
    vx_reference params[] = {
       (vx_reference)in1,
       (vx_reference)in2,
       (vx_reference)out,
    };
    return tivxCreateNodeByKernelEnum(graph,
                                   (vx_enum)VX_KERNEL_AND,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxOrNode(vx_graph graph, vx_image in1, vx_image in2, vx_image out)
{
    vx_reference params[] = {
       (vx_reference)in1,
       (vx_reference)in2,
       (vx_reference)out,
    };
    return tivxCreateNodeByKernelEnum(graph,
                                   (vx_enum)VX_KERNEL_OR,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxXorNode(vx_graph graph, vx_image in1, vx_image in2, vx_image out)
{
    vx_reference params[] = {
       (vx_reference)in1,
       (vx_reference)in2,
       (vx_reference)out,
    };
    return tivxCreateNodeByKernelEnum(graph,
                                   (vx_enum)VX_KERNEL_XOR,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxNotNode(vx_graph graph, vx_image input, vx_image output)
{
    vx_reference params[] = {
       (vx_reference)input,
       (vx_reference)output,
    };
    return tivxCreateNodeByKernelEnum(graph,
                                   (vx_enum)VX_KERNEL_NOT,
                                   params,
                                   dimof(params));
}

VX_API_ENTRY vx_node VX_API_CALL vxMultiplyNode(vx_graph graph, vx_image in1, vx_image in2, vx_scalar scale, vx_enum overflow_policy, vx_enum rounding_policy, vx_image out)
{
    vx_status status = (vx_status)VX_SUCCESS;
    vx_context context = vxGetContext((vx_reference)graph);
    vx_scalar spolicy = vxCreateScalar(context, (vx_enum)VX_TYPE_ENUM, &overflow_policy);
    vx_scalar rpolicy = vxCreateScalar(context, (vx_enum)VX_TYPE_ENUM, &rounding_policy);
    vx_reference params[] = {
       (vx_reference)in1,
       (vx_reference)in2,
       (vx_reference)scale,
       (vx_reference)spolicy,
       (vx_reference)rpolicy,
       (vx_reference)out,
    };
    vx_node node = tivxCreateNodeByKernelEnum(graph,
                                           (vx_enum)VX_KERNEL_MULTIPLY,
                                           params,
                                           dimof(params));
    status = vxReleaseScalar(&spolicy);
    if((vx_status)VX_SUCCESS != status)
    {
        VX_PRINT(VX_ZONE_ERROR,"Failed to release reference to scalar, spolicy might not be a vx_scalar\n");
    }
    status = vxReleaseScalar(&rpolicy);
    if((vx_status)VX_SUCCESS != status)
    {
        VX_PRINT(VX_ZONE_ERROR,"Failed to release reference to scalar, rpolicy might not be a vx_scalar\n");
    }
    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxAddNode(vx_graph graph, vx_image in1, vx_image in2, vx_enum policy, vx_image out)
{
    vx_status status = (vx_status)VX_SUCCESS;
    vx_context context = vxGetContext((vx_reference)graph);
    vx_scalar spolicy = vxCreateScalar(context, (vx_enum)VX_TYPE_ENUM, &policy);
    vx_reference params[] = {
       (vx_reference)in1,
       (vx_reference)in2,
       (vx_reference)spolicy,
       (vx_reference)out,
    };
    vx_node node = tivxCreateNodeByKernelEnum(graph,
                                           (vx_enum)VX_KERNEL_ADD,
                                           params,
                                           dimof(params));
    status = vxReleaseScalar(&spolicy);
    if((vx_status)VX_SUCCESS != status)
    {
        VX_PRINT(VX_ZONE_ERROR,"Failed to release reference to scalar, spolicy might not be a vx_scalar\n");
    }
    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxSubtractNode(vx_graph graph, vx_image in1, vx_image in2, vx_enum policy, vx_image out)
{
    vx_status status = (vx_status)VX_SUCCESS;
    vx_context context = vxGetContext((vx_reference)graph);
    vx_scalar spolicy = vxCreateScalar(context, (vx_enum)VX_TYPE_ENUM, &policy);
    vx_reference params[] = {
       (vx_reference)in1,
       (vx_reference)in2,
       (vx_reference)spolicy,
       (vx_reference)out,
    };
    vx_node node = tivxCreateNodeByKernelEnum(graph,
                                           (vx_enum)VX_KERNEL_SUBTRACT,
                                           params,
                                           dimof(params));
    status = vxReleaseScalar(&spolicy);
    if((vx_status)VX_SUCCESS != status)
    {
        VX_PRINT(VX_ZONE_ERROR,"Failed to release reference to scalar, spolicy might not be a vx_scalar\n");
    }
    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxWarpAffineNode(vx_graph graph, vx_image input, vx_matrix matrix, vx_enum type, vx_image output)
{
    vx_status status = (vx_status)VX_SUCCESS;
    vx_context context = vxGetContext((vx_reference)graph);
    vx_scalar stype = vxCreateScalar(context, (vx_enum)VX_TYPE_ENUM, &type);
    vx_reference params[] = {
            (vx_reference)input,
            (vx_reference)matrix,
            (vx_reference)stype,
            (vx_reference)output,
    };
    vx_node node = tivxCreateNodeByKernelEnum(graph,
                                           (vx_enum)VX_KERNEL_WARP_AFFINE,
                                           params,
                                           dimof(params));
    status = vxReleaseScalar(&stype);
    if((vx_status)VX_SUCCESS != status)
    {
        VX_PRINT(VX_ZONE_ERROR,"Failed to release reference to scalar, stype might not be a vx_scalar\n");
    }
    if (vxGetStatus((vx_reference)node) == (vx_status)VX_SUCCESS)
    {
        /* default value for Warp node */
        /* change node attribute as kernel attributes alreay copied to node */
        /* in tivxCreateNodeByKernelEnum() */
        status = ownSetNodeAttributeValidRectReset(node, (vx_bool)vx_true_e);
        if((vx_status)VX_SUCCESS != status)
        {
            VX_PRINT(VX_ZONE_ERROR,"Failed to set the attribute VX_NODE_VALID_RECT_RESET in node\n");
        }
    }

    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxWarpPerspectiveNode(vx_graph graph, vx_image input, vx_matrix matrix, vx_enum type, vx_image output)
{
    vx_status status = (vx_status)VX_SUCCESS;
    vx_context context = vxGetContext((vx_reference)graph);
    vx_scalar stype = vxCreateScalar(context, (vx_enum)VX_TYPE_ENUM, &type);
    vx_reference params[] = {
            (vx_reference)input,
            (vx_reference)matrix,
            (vx_reference)stype,
            (vx_reference)output,
    };
    vx_node node = tivxCreateNodeByKernelEnum(graph,
                                           (vx_enum)VX_KERNEL_WARP_PERSPECTIVE,
                                           params,
                                           dimof(params));
    status = vxReleaseScalar(&stype);
    if((vx_status)VX_SUCCESS != status)
    {
        VX_PRINT(VX_ZONE_ERROR,"Failed to release reference to scalar, stype might not be a vx_scalar\n");
    }
    if (vxGetStatus((vx_reference)node) == (vx_status)VX_SUCCESS)
    {
        /* default value for Warp node */
        /* change node attribute as kernel attributes alreay copied to node */
        /* in tivxCreateNodeByKernelEnum() */
        status = ownSetNodeAttributeValidRectReset(node, (vx_bool)vx_true_e);
        if((vx_status)VX_SUCCESS != status)
        {
            VX_PRINT(VX_ZONE_ERROR,"Failed to set the attribute VX_NODE_VALID_RECT_RESET in node\n");
        }
    }

    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxHarrisCornersNode(vx_graph graph,
                            vx_image input,
                            vx_scalar strength_thresh,
                            vx_scalar min_distance,
                            vx_scalar sensitivity,
                            vx_int32 gradient_size,
                            vx_int32 block_size,
                            vx_array corners,
                            vx_scalar num_corners)
{
    vx_status status = (vx_status)VX_SUCCESS;
    vx_scalar win = vxCreateScalar(vxGetContext((vx_reference)graph), (vx_enum)VX_TYPE_INT32, &gradient_size);
    vx_scalar blk = vxCreateScalar(vxGetContext((vx_reference)graph), (vx_enum)VX_TYPE_INT32, &block_size);
    vx_reference params[] = {
            (vx_reference)input,
            (vx_reference)strength_thresh,
            (vx_reference)min_distance,
            (vx_reference)sensitivity,
            (vx_reference)win,
            (vx_reference)blk,
            (vx_reference)corners,
            (vx_reference)num_corners,
    };
    vx_node node = tivxCreateNodeByKernelEnum(graph,
                                           (vx_enum)VX_KERNEL_HARRIS_CORNERS,
                                           params,
                                           dimof(params));
    status = vxReleaseScalar(&win);
    if((vx_status)VX_SUCCESS != status)
    {
        VX_PRINT(VX_ZONE_ERROR,"Failed to release reference to scalar, win might not be a vx_scalar\n");
    }
    status = vxReleaseScalar(&blk);
    if((vx_status)VX_SUCCESS != status)
    {
        VX_PRINT(VX_ZONE_ERROR,"Failed to release reference to scalar, blk might not be a vx_scalar\n");
    }
    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxFastCornersNode(vx_graph graph, vx_image input, vx_scalar strength_thresh, vx_bool nonmax_suppression, vx_array corners, vx_scalar num_corners)
{
    vx_status status = (vx_status)VX_SUCCESS;
    vx_scalar nonmax = vxCreateScalar(vxGetContext((vx_reference)graph),(vx_enum)VX_TYPE_BOOL, &nonmax_suppression);
    vx_reference params[] = {
            (vx_reference)input,
            (vx_reference)strength_thresh,
            (vx_reference)nonmax,
            (vx_reference)corners,
            (vx_reference)num_corners,
    };
    vx_node node = tivxCreateNodeByKernelEnum(graph,
                                           (vx_enum)VX_KERNEL_FAST_CORNERS,
                                           params,
                                           dimof(params));
    status = vxReleaseScalar(&nonmax);
    if((vx_status)VX_SUCCESS != status)
    {
        VX_PRINT(VX_ZONE_ERROR,"Failed to release reference to scalar, nonmax might not be a vx_scalar\n");
    }
    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxOpticalFlowPyrLKNode(vx_graph graph,
                               vx_pyramid old_images,
                               vx_pyramid new_images,
                               vx_array old_points,
                               vx_array new_points_estimates,
                               vx_array new_points,
                               vx_enum termination,
                               vx_scalar epsilon,
                               vx_scalar num_iterations,
                               vx_scalar use_initial_estimate,
                               vx_size window_dimension)
{
    vx_status status = (vx_status)VX_SUCCESS;
    vx_scalar term = vxCreateScalar(vxGetContext((vx_reference)graph), (vx_enum)VX_TYPE_ENUM, &termination);
    vx_scalar winsize = vxCreateScalar(vxGetContext((vx_reference)graph), (vx_enum)VX_TYPE_SIZE, &window_dimension);
    vx_reference params[] = {
            (vx_reference)old_images,
            (vx_reference)new_images,
            (vx_reference)old_points,
            (vx_reference)new_points_estimates,
            (vx_reference)new_points,
            (vx_reference)term,
            (vx_reference)epsilon,
            (vx_reference)num_iterations,
            (vx_reference)use_initial_estimate,
            (vx_reference)winsize,
    };
    vx_node node = tivxCreateNodeByKernelEnum(graph,
                                           (vx_enum)VX_KERNEL_OPTICAL_FLOW_PYR_LK,
                                           params,
                                           dimof(params));
    status = vxReleaseScalar(&term);
    if((vx_status)VX_SUCCESS != status)
    {
        VX_PRINT(VX_ZONE_ERROR,"Failed to release reference to scalar, term might not be a vx_scalar\n");
    }
    status = vxReleaseScalar(&winsize);
    if((vx_status)VX_SUCCESS != status)
    {
        VX_PRINT(VX_ZONE_ERROR,"Failed to release reference to scalar, winsize might not be a vx_scalar\n");
    }
    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxRemapNode(vx_graph graph,
                    vx_image input,
                    vx_remap table,
                    vx_enum policy,
                    vx_image output)
{
    vx_status status = (vx_status)VX_SUCCESS;
    vx_scalar spolicy = vxCreateScalar(vxGetContext((vx_reference)graph), (vx_enum)VX_TYPE_ENUM, &policy);
    vx_reference params[] = {
            (vx_reference)input,
            (vx_reference)table,
            (vx_reference)spolicy,
            (vx_reference)output,
    };
    vx_node node = tivxCreateNodeByKernelEnum(graph,
                                           (vx_enum)VX_KERNEL_REMAP,
                                           params,
                                           dimof(params));
    status = vxReleaseScalar(&spolicy);
    if((vx_status)VX_SUCCESS != status)
    {
        VX_PRINT(VX_ZONE_ERROR,"Failed to release reference to scalar, spolicy might not be a vx_scalar\n");
    }
    if (vxGetStatus((vx_reference)node) == (vx_status)VX_SUCCESS)
    {
        /* default value for Remap node */
        /* change node attribute as kernel attributes alreay copied to node */
        /* in tivxCreateNodeByKernelEnum() */
        status = ownSetNodeAttributeValidRectReset(node, (vx_bool)vx_true_e);
        if((vx_status)VX_SUCCESS != status)
        {
            VX_PRINT(VX_ZONE_ERROR,"Failed to set the attribute VX_NODE_VALID_RECT_RESET in node\n");
        }
    }

    return node;
}

VX_API_ENTRY vx_node VX_API_CALL vxHalfScaleGaussianNode(vx_graph graph, vx_image input, vx_image output, vx_int32 kernel_size)
{
    vx_status status = (vx_status)VX_SUCCESS;
    vx_scalar ksize = vxCreateScalar(vxGetContext((vx_reference)graph), (vx_enum)VX_TYPE_INT32, &kernel_size);
    vx_reference params[] = {
            (vx_reference)input,
            (vx_reference)output,
            (vx_reference)ksize,
    };
    vx_node node = tivxCreateNodeByKernelEnum(graph,
                                           (vx_enum)VX_KERNEL_HALFSCALE_GAUSSIAN,
                                           params,
                                           dimof(params));
    status = vxReleaseScalar(&ksize);
    if((vx_status)VX_SUCCESS != status)
    {
        VX_PRINT(VX_ZONE_ERROR,"Failed to release reference to scalar, ksize might not be a vx_scalar\n");
    }
    return node;
}
