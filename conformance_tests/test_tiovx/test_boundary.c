/* 
 * Copyright (c) 2012-2017 The Khronos Group Inc.
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
 * Copyright (C) 2017 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 *******************************************************************************
 */

#include "test_tiovx.h"
#include <VX/vx.h>
#include <VX/vxu.h>
#include <string.h>

#include "shared_functions.h"

#define MAX_POINTS 100

TESTCASE(tivxBoundary, CT_VXContext, ct_setup_vx_context, 0)
TESTCASE(tivxNegativeBoundary, CT_VXContext, ct_setup_vx_context, 0)

typedef struct {
    const char* testName;
    CT_Image (*generator)(const char* fileName, int width, int height);
    const char* src1_fileName;
    const char* src2_fileName;
    const char* points_fileName;
    vx_size winSize;
    int useReferencePyramid;
} Arg;


#define PARAMETERS \
    ARG("case1/Boundary", NULL, "optflow_00.bmp", "optflow_01.bmp", "optflow_pyrlk_5x5.txt", 5, 1)

TEST_WITH_ARG(tivxBoundary, testImageBoundary, Arg,
    PARAMETERS
)
{
    vx_context context = context_->vx_context_;
    vx_image   src_image[384];
    int i;

    for (i = 0; i < 384; i++)
    {
        ASSERT_VX_OBJECT(src_image[i] = vxCreateImage(context, 16, 16, VX_DF_IMAGE_U8), VX_TYPE_IMAGE);
    }

    for (i = 0; i < 384; i++)
    {
        VX_CALL(vxReleaseImage(&src_image[i]));
    }
}

TEST_WITH_ARG(tivxBoundary, testPyramidBoundary, Arg,
    PARAMETERS
)
{
    vx_context context = context_->vx_context_;
    vx_pyramid   src_pyr[48];
    int i;

    for (i = 0; i < 48; i++)
    {
        ASSERT_VX_OBJECT(src_pyr[i] = vxCreatePyramid(context, 4, VX_SCALE_PYRAMID_HALF, 16, 16, VX_DF_IMAGE_U8), VX_TYPE_PYRAMID);
    }

    for (i = 0; i < 48; i++)
    {
        VX_CALL(vxReleasePyramid(&src_pyr[i]));
    }
}

TEST_WITH_ARG(tivxBoundary, testPyramidLevelBoundary, Arg,
    PARAMETERS
)
{
    vx_context context = context_->vx_context_;
    vx_pyramid   src_pyr;

    ASSERT_VX_OBJECT(src_pyr = vxCreatePyramid(context, 32, VX_SCALE_PYRAMID_HALF, 16, 16, VX_DF_IMAGE_U8), VX_TYPE_PYRAMID);
    VX_CALL(vxReleasePyramid(&src_pyr));
}

TEST_WITH_ARG(tivxBoundary, testArrayBoundary, Arg,
    PARAMETERS
)
{
    vx_context context = context_->vx_context_;
    vx_array   src_array[48];
    int i;

    for (i = 0; i < 48; i++)
    {
        ASSERT_VX_OBJECT(src_array[i] = vxCreateArray(context, VX_TYPE_KEYPOINT, 4), VX_TYPE_ARRAY);
    }

    for (i = 0; i < 48; i++)
    {
        VX_CALL(vxReleaseArray(&src_array[i]));
    }
}

TEST_WITH_ARG(tivxBoundary, testConvolutionBoundary, Arg,
    PARAMETERS
)
{
    vx_context context = context_->vx_context_;
    vx_convolution   src_conv[48];
    int i;

    for (i = 0; i < 48; i++)
    {
        ASSERT_VX_OBJECT(src_conv[i] = vxCreateConvolution(context, 3, 3), VX_TYPE_CONVOLUTION);
    }

    for (i = 0; i < 48; i++)
    {
        VX_CALL(vxReleaseConvolution(&src_conv[i]));
    }
}

TEST_WITH_ARG(tivxBoundary, testDistributionBoundary, Arg,
    PARAMETERS
)
{
    vx_context context = context_->vx_context_;
    vx_distribution   src_dist[48];
    int i;

    for (i = 0; i < 48; i++)
    {
        ASSERT_VX_OBJECT(src_dist[i] = vxCreateDistribution(context, 100, 5, 200), VX_TYPE_DISTRIBUTION);
    }

    for (i = 0; i < 48; i++)
    {
        VX_CALL(vxReleaseDistribution(&src_dist[i]));
    }
}

TEST_WITH_ARG(tivxBoundary, testLUTBoundary, Arg,
    PARAMETERS
)
{
    vx_context context = context_->vx_context_;
    vx_lut   src_lut[48];
    int i;

    for (i = 0; i < 48; i++)
    {
        ASSERT_VX_OBJECT(src_lut[i] = vxCreateLUT(context, VX_TYPE_UINT8, 256), VX_TYPE_LUT);
    }

    for (i = 0; i < 48; i++)
    {
        VX_CALL(vxReleaseLUT(&src_lut[i]));
    }
}

TEST_WITH_ARG(tivxBoundary, testDelayBoundary, Arg,
    PARAMETERS
)
{
    vx_context context = context_->vx_context_;
    vx_delay   src_delay[48];
    vx_image   image;
    int i;

    ASSERT_VX_OBJECT(image = vxCreateImage(context, 16, 16, VX_DF_IMAGE_U8), VX_TYPE_IMAGE);

    for (i = 0; i < 48; i++)
    {
        ASSERT_VX_OBJECT(src_delay[i] = vxCreateDelay(context, (vx_reference)image, 2), VX_TYPE_DELAY);
    }

    for (i = 0; i < 48; i++)
    {
        VX_CALL(vxReleaseDelay(&src_delay[i]));
    }

    VX_CALL(vxReleaseImage(&image));
}

TEST_WITH_ARG(tivxBoundary, testMatrixBoundary, Arg,
    PARAMETERS
)
{
    vx_context context = context_->vx_context_;
    vx_matrix   src_matrix[48];
    int i;

    for (i = 0; i < 48; i++)
    {
        ASSERT_VX_OBJECT(src_matrix[i] = vxCreateMatrix(context, VX_TYPE_FLOAT32, 3, 3), VX_TYPE_MATRIX);
    }

    for (i = 0; i < 48; i++)
    {
        VX_CALL(vxReleaseMatrix(&src_matrix[i]));
    }
}

TEST_WITH_ARG(tivxBoundary, testRemapBoundary, Arg,
    PARAMETERS
)
{
    vx_context context = context_->vx_context_;
    vx_remap   src_remap[48];
    int i;

    for (i = 0; i < 48; i++)
    {
        ASSERT_VX_OBJECT(src_remap[i] = vxCreateRemap(context, 32, 24, 16, 12), VX_TYPE_REMAP);
    }

    for (i = 0; i < 48; i++)
    {
        VX_CALL(vxReleaseRemap(&src_remap[i]));
    }
}

TEST_WITH_ARG(tivxBoundary, testScalarBoundary, Arg,
    PARAMETERS
)
{
    vx_context context = context_->vx_context_;
    vx_scalar   src_scalar[48];
    vx_int32 tmp;
    int i;

    for (i = 0; i < 48; i++)
    {
        ASSERT_VX_OBJECT(src_scalar[i] = vxCreateScalar(context, VX_TYPE_INT32, &tmp), VX_TYPE_SCALAR);
    }

    for (i = 0; i < 48; i++)
    {
        VX_CALL(vxReleaseScalar(&src_scalar[i]));
    }
}

TEST_WITH_ARG(tivxBoundary, testThresholdBoundary, Arg,
    PARAMETERS
)
{
    vx_context context = context_->vx_context_;
    vx_threshold   src_threshold[48];
    int i;

    for (i = 0; i < 48; i++)
    {
        ASSERT_VX_OBJECT(src_threshold[i] = vxCreateThreshold(context, VX_THRESHOLD_TYPE_RANGE, VX_TYPE_UINT8), VX_TYPE_THRESHOLD);
    }

    for (i = 0; i < 48; i++)
    {
        VX_CALL(vxReleaseThreshold(&src_threshold[i]));
    }
}

TEST_WITH_ARG(tivxBoundary, testKernelBoundary, Arg,
    PARAMETERS
)
{
    vx_context context = context_->vx_context_;
    vx_kernel   src_kernel[64];
    int i;

    for (i = 0; i < 64; i++)
    {
        ASSERT_VX_OBJECT(src_kernel[i] = vxGetKernelByEnum(context, VX_KERNEL_BOX_3x3), VX_TYPE_KERNEL);
    }

    for (i = 0; i < 64; i++)
    {
        VX_CALL(vxReleaseKernel(&src_kernel[i]));
    }
}

TEST_WITH_ARG(tivxBoundary, testNodeBoundary, Arg,
    PARAMETERS
)
{
    vx_context context = context_->vx_context_;
    vx_kernel   src_kernel;
    vx_node   src_node[32];
    int i;
    vx_graph graph = 0;

    ASSERT_VX_OBJECT(graph = vxCreateGraph(context), VX_TYPE_GRAPH);

    ASSERT_VX_OBJECT(src_kernel = vxGetKernelByEnum(context, VX_KERNEL_BOX_3x3), VX_TYPE_KERNEL);

    for (i = 0; i < 32; i++)
    {
        ASSERT_VX_OBJECT(src_node[i] = vxCreateGenericNode(graph, src_kernel), VX_TYPE_NODE);
    }

    for (i = 0; i < 32; i++)
    {
        VX_CALL(vxReleaseNode(&src_node[i]));
    }

    VX_CALL(vxReleaseKernel(&src_kernel));
    VX_CALL(vxReleaseGraph(&graph));
}

TEST_WITH_ARG(tivxBoundary, testParameterBoundary, Arg,
    PARAMETERS
)
{
    vx_context context = context_->vx_context_;
    vx_kernel   src_kernel;
    vx_node   src_node[32];
    vx_parameter src_parameter[48];
    int i;
    vx_graph graph = 0;

    ASSERT_VX_OBJECT(graph = vxCreateGraph(context), VX_TYPE_GRAPH);

    ASSERT_VX_OBJECT(src_kernel = vxGetKernelByEnum(context, VX_KERNEL_CHANNEL_EXTRACT), VX_TYPE_KERNEL);

    for (i = 0; i < 32; i++)
    {
        ASSERT_VX_OBJECT(src_node[i] = vxCreateGenericNode(graph, src_kernel), VX_TYPE_NODE);
    }

    for (i = 0; i < 48; i++)
    {
        ASSERT_VX_OBJECT(src_parameter[i] = vxGetParameterByIndex(src_node[0], 0), VX_TYPE_PARAMETER);
    }

    for (i = 0; i < 48; i++)
    {
        VX_CALL(vxReleaseParameter(&src_parameter[i]));
    }

    for (i = 0; i < 32; i++)
    {
        VX_CALL(vxReleaseNode(&src_node[i]));
    }

    VX_CALL(vxReleaseKernel(&src_kernel));
    VX_CALL(vxReleaseGraph(&graph));
}

TEST_WITH_ARG(tivxBoundary, testGraphBoundary, Arg,
    PARAMETERS
)
{
    vx_context context = context_->vx_context_;
    vx_graph   src_graph[8];
    int i;

    for (i = 0; i < 8; i++)
    {
        ASSERT_VX_OBJECT(src_graph[i] = vxCreateGraph(context), VX_TYPE_GRAPH);
    }

    for (i = 0; i < 8; i++)
    {
        VX_CALL(vxReleaseGraph(&src_graph[i]));
    }
}

TEST_WITH_ARG(tivxBoundary, testObjectArray, Arg,
    PARAMETERS
)
{
    vx_context context = context_->vx_context_;
    vx_object_array src_object_array[48];
    int i;
    vx_image image = 0;
    ASSERT_VX_OBJECT(image = vxCreateImage(context, 16, 16, VX_DF_IMAGE_U8), VX_TYPE_IMAGE);

    for (i = 0; i < 48; i++)
    {
        ASSERT_VX_OBJECT(src_object_array[i] = vxCreateObjectArray(context, (vx_reference)image, 2), VX_TYPE_OBJECT_ARRAY);
    }

    for (i = 0; i < 48; i++)
    {
        VX_CALL(vxReleaseObjectArray(&src_object_array[i]));
    }

    VX_CALL(vxReleaseImage(&image));
}

// Passes w/ 31 not w/ 32
TEST_WITH_ARG(tivxBoundary, testObjectArrayItems, Arg,
    PARAMETERS
)
{
    vx_context context = context_->vx_context_;
    vx_object_array src_object_array;
    vx_image image = 0;
    ASSERT_VX_OBJECT(image = vxCreateImage(context, 16, 16, VX_DF_IMAGE_U8), VX_TYPE_IMAGE);

    ASSERT_VX_OBJECT(src_object_array = vxCreateObjectArray(context, (vx_reference)image, 32), VX_TYPE_OBJECT_ARRAY); // grep for max array items to see how it's used

    VX_CALL(vxReleaseObjectArray(&src_object_array));

    VX_CALL(vxReleaseImage(&image));
}

// Doesn't fail
TEST_WITH_ARG(tivxBoundary, testContext, Arg,
    PARAMETERS
)
{
    vx_context context = context_->vx_context_;

    vx_context context2 = vxCreateContext(); // Might be a build option, might return a reference to the single context

    vx_context context3 = vxCreateContext();
}

TEST_WITH_ARG(tivxNegativeBoundary, negativeTestObjectArrayItems, Arg,
    PARAMETERS
)
{
    vx_context context = context_->vx_context_;
    vx_object_array src_object_array;
    vx_lut lut = 0;
    ASSERT_VX_OBJECT(lut = vxCreateLUT(context, VX_TYPE_UINT8, 256), VX_TYPE_LUT);

    EXPECT_VX_ERROR(src_object_array = vxCreateObjectArray(context, (vx_reference)lut, 33), VX_ERROR_NO_RESOURCES);

    VX_CALL(vxReleaseLUT(&lut));
}

TEST_WITH_ARG(tivxNegativeBoundary, negativeTestObjectArray, Arg,
    PARAMETERS
)
{
    vx_context context = context_->vx_context_;
    vx_object_array src_object_array[49];
    int i;
    vx_image image = 0;
    ASSERT_VX_OBJECT(image = vxCreateImage(context, 16, 16, VX_DF_IMAGE_U8), VX_TYPE_IMAGE);

    for (i = 0; i < 48; i++)
    {
        ASSERT_VX_OBJECT(src_object_array[i] = vxCreateObjectArray(context, (vx_reference)image, 1), VX_TYPE_OBJECT_ARRAY);
    }

    EXPECT_VX_ERROR(src_object_array[48] = vxCreateObjectArray(context, (vx_reference)image, 1), VX_ERROR_NO_RESOURCES);

    for (i = 0; i < 48; i++)
    {
        VX_CALL(vxReleaseObjectArray(&src_object_array[i]));
    }

    VX_CALL(vxReleaseImage(&image));
}

TEST_WITH_ARG(tivxNegativeBoundary, negativeTestParameterBoundary, Arg,
    PARAMETERS
)
{
    vx_context context = context_->vx_context_;
    vx_kernel   src_kernel;
    vx_node   src_node[32];
    vx_parameter src_parameter[49];
    int i;
    vx_graph graph = 0;

    ASSERT_VX_OBJECT(graph = vxCreateGraph(context), VX_TYPE_GRAPH);

    ASSERT_VX_OBJECT(src_kernel = vxGetKernelByEnum(context, VX_KERNEL_CHANNEL_EXTRACT), VX_TYPE_KERNEL);

    for (i = 0; i < 32; i++)
    {
        ASSERT_VX_OBJECT(src_node[i] = vxCreateGenericNode(graph, src_kernel), VX_TYPE_NODE);
    }

    for (i = 0; i < 49; i++)
    {
        ASSERT_VX_OBJECT(src_parameter[i] = vxGetParameterByIndex(src_node[0], 0), VX_TYPE_PARAMETER);
    }

    for (i = 0; i < 49; i++)
    {
        VX_CALL(vxReleaseParameter(&src_parameter[i]));
    }

    for (i = 0; i < 32; i++)
    {
        VX_CALL(vxReleaseNode(&src_node[i]));
    }

    VX_CALL(vxReleaseKernel(&src_kernel));
    VX_CALL(vxReleaseGraph(&graph));
}

TEST_WITH_ARG(tivxNegativeBoundary, negativeTestGraphBoundary, Arg,
    PARAMETERS
)
{
    vx_context context = context_->vx_context_;
    vx_graph   src_graph[9];
    int i;

    for (i = 0; i < 8; i++)
    {
        ASSERT_VX_OBJECT(src_graph[i] = vxCreateGraph(context), VX_TYPE_GRAPH);
    }

    EXPECT_VX_ERROR(src_graph[8] = vxCreateGraph(context), VX_ERROR_NO_RESOURCES);

    for (i = 0; i < 8; i++)
    {
        VX_CALL(vxReleaseGraph(&src_graph[i]));
    }
}

TEST_WITH_ARG(tivxNegativeBoundary, negativeTestNodeBoundary, Arg,
    PARAMETERS
)
{
    vx_context context = context_->vx_context_;
    vx_kernel   src_kernel;
    vx_node   src_node[33];
    int i;
    vx_graph graph = 0;

    ASSERT_VX_OBJECT(graph = vxCreateGraph(context), VX_TYPE_GRAPH);

    ASSERT_VX_OBJECT(src_kernel = vxGetKernelByEnum(context, VX_KERNEL_BOX_3x3), VX_TYPE_KERNEL);

    for (i = 0; i < 32; i++)
    {
        ASSERT_VX_OBJECT(src_node[i] = vxCreateGenericNode(graph, src_kernel), VX_TYPE_NODE);
    }

    EXPECT_VX_ERROR(src_node[32] = vxCreateGenericNode(graph, src_kernel), VX_ERROR_NO_RESOURCES);

    for (i = 0; i < 32; i++)
    {
        VX_CALL(vxReleaseNode(&src_node[i]));
    }

    VX_CALL(vxReleaseKernel(&src_kernel));
    VX_CALL(vxReleaseGraph(&graph));
}

// Note: should fail but doesn't
TEST_WITH_ARG(tivxNegativeBoundary, negativeTestKernelBoundary, Arg,
    PARAMETERS
)
{
    vx_context context = context_->vx_context_;
    vx_kernel   src_kernel[65];
    int i;

    for (i = 0; i < 64; i++)
    {
        ASSERT_VX_OBJECT(src_kernel[i] = vxGetKernelByEnum(context, VX_KERNEL_BOX_3x3), VX_TYPE_KERNEL);
    }

    EXPECT_VX_ERROR(src_kernel[64] = vxGetKernelByEnum(context, VX_KERNEL_BOX_3x3), VX_ERROR_NO_RESOURCES);

    for (i = 0; i < 64; i++)
    {
        VX_CALL(vxReleaseKernel(&src_kernel[i]));
    }
}

TEST_WITH_ARG(tivxNegativeBoundary, negativeTestThresholdBoundary, Arg,
    PARAMETERS
)
{
    vx_context context = context_->vx_context_;
    vx_threshold   src_threshold[49];
    int i;

    for (i = 0; i < 48; i++)
    {
        ASSERT_VX_OBJECT(src_threshold[i] = vxCreateThreshold(context, VX_THRESHOLD_TYPE_RANGE, VX_TYPE_UINT8), VX_TYPE_THRESHOLD);
    }

    EXPECT_VX_ERROR(src_threshold[48] = vxCreateThreshold(context, VX_THRESHOLD_TYPE_RANGE, VX_TYPE_UINT8), VX_ERROR_NO_RESOURCES);

    for (i = 0; i < 48; i++)
    {
        VX_CALL(vxReleaseThreshold(&src_threshold[i]));
    }
}

TEST_WITH_ARG(tivxNegativeBoundary, negativeTestScalarBoundary, Arg,
    PARAMETERS
)
{
    vx_context context = context_->vx_context_;
    vx_scalar   src_scalar[49];
    vx_int32 tmp;
    int i;

    for (i = 0; i < 48; i++)
    {
        ASSERT_VX_OBJECT(src_scalar[i] = vxCreateScalar(context, VX_TYPE_INT32, &tmp), VX_TYPE_SCALAR);
    }

    EXPECT_VX_ERROR(src_scalar[48] = vxCreateScalar(context, VX_TYPE_INT32, &tmp), VX_ERROR_NO_RESOURCES);

    for (i = 0; i < 48; i++)
    {
        VX_CALL(vxReleaseScalar(&src_scalar[i]));
    }
}

TEST_WITH_ARG(tivxNegativeBoundary, negativeTestRemapBoundary, Arg,
    PARAMETERS
)
{
    vx_context context = context_->vx_context_;
    vx_remap   src_remap[49];
    int i;

    for (i = 0; i < 48; i++)
    {
        ASSERT_VX_OBJECT(src_remap[i] = vxCreateRemap(context, 32, 24, 16, 12), VX_TYPE_REMAP);
    }

    EXPECT_VX_ERROR(src_remap[48] = vxCreateRemap(context, 32, 24, 16, 12), VX_ERROR_NO_RESOURCES);

    for (i = 0; i < 48; i++)
    {
        VX_CALL(vxReleaseRemap(&src_remap[i]));
    }
}

TEST_WITH_ARG(tivxNegativeBoundary, negativeTestMatrixBoundary, Arg,
    PARAMETERS
)
{
    vx_context context = context_->vx_context_;
    vx_matrix   src_matrix[49];
    int i;

    for (i = 0; i < 48; i++)
    {
        ASSERT_VX_OBJECT(src_matrix[i] = vxCreateMatrix(context, VX_TYPE_FLOAT32, 3, 3), VX_TYPE_MATRIX);
    }

    EXPECT_VX_ERROR(src_matrix[48] = vxCreateMatrix(context, VX_TYPE_FLOAT32, 3, 3), VX_ERROR_NO_RESOURCES);

    for (i = 0; i < 48; i++)
    {
        VX_CALL(vxReleaseMatrix(&src_matrix[i]));
    }
}

TEST_WITH_ARG(tivxNegativeBoundary, negativeTestDelayBoundary, Arg,
    PARAMETERS
)
{
    vx_context context = context_->vx_context_;
    vx_delay   src_delay[49];
    vx_image   image;
    int i;

    ASSERT_VX_OBJECT(image = vxCreateImage(context, 16, 16, VX_DF_IMAGE_U8), VX_TYPE_IMAGE);

    for (i = 0; i < 48; i++)
    {
        ASSERT_VX_OBJECT(src_delay[i] = vxCreateDelay(context, (vx_reference)image, 2), VX_TYPE_DELAY);
    }

    EXPECT_VX_ERROR(src_delay[48] = vxCreateDelay(context, (vx_reference)image, 2), VX_ERROR_NO_RESOURCES);

    for (i = 0; i < 48; i++)
    {
        VX_CALL(vxReleaseDelay(&src_delay[i]));
    }

    VX_CALL(vxReleaseImage(&image));
}

TEST_WITH_ARG(tivxNegativeBoundary, negativeTestLUTBoundary, Arg,
    PARAMETERS
)
{
    vx_context context = context_->vx_context_;
    vx_lut   src_lut[49];
    int i;

    for (i = 0; i < 48; i++)
    {
        ASSERT_VX_OBJECT(src_lut[i] = vxCreateLUT(context, VX_TYPE_UINT8, 256), VX_TYPE_LUT);
    }

    EXPECT_VX_ERROR(src_lut[48] = vxCreateLUT(context, VX_TYPE_UINT8, 256), VX_ERROR_NO_RESOURCES);

    for (i = 0; i < 48; i++)
    {
        VX_CALL(vxReleaseLUT(&src_lut[i]));
    }
}

TEST_WITH_ARG(tivxNegativeBoundary, negativeTestDistributionBoundary, Arg,
    PARAMETERS
)
{
    vx_context context = context_->vx_context_;
    vx_distribution   src_dist[49];
    int i;

    for (i = 0; i < 48; i++)
    {
        ASSERT_VX_OBJECT(src_dist[i] = vxCreateDistribution(context, 100, 5, 200), VX_TYPE_DISTRIBUTION);
    }

    EXPECT_VX_ERROR(src_dist[48] = vxCreateDistribution(context, 100, 5, 200), VX_ERROR_NO_RESOURCES);

    for (i = 0; i < 48; i++)
    {
        VX_CALL(vxReleaseDistribution(&src_dist[i]));
    }
}

TEST_WITH_ARG(tivxNegativeBoundary, negativeTestArrayBoundary, Arg,
    PARAMETERS
)
{
    vx_context context = context_->vx_context_;
    vx_array   src_array[49];
    int i;

    for (i = 0; i < 48; i++)
    {
        ASSERT_VX_OBJECT(src_array[i] = vxCreateArray(context, VX_TYPE_KEYPOINT, 4), VX_TYPE_ARRAY);
    }

    EXPECT_VX_ERROR(src_array[48] = vxCreateArray(context, VX_TYPE_KEYPOINT, 4), VX_ERROR_NO_RESOURCES);

    for (i = 0; i < 48; i++)
    {
        VX_CALL(vxReleaseArray(&src_array[i]));
    }
}

TEST_WITH_ARG(tivxNegativeBoundary, negativeTestConvolutionBoundary, Arg,
    PARAMETERS
)
{
    vx_context context = context_->vx_context_;
    vx_convolution   src_conv[49];
    int i;

    for (i = 0; i < 48; i++)
    {
        ASSERT_VX_OBJECT(src_conv[i] = vxCreateConvolution(context, 3, 3), VX_TYPE_CONVOLUTION);
    }

    EXPECT_VX_ERROR(src_conv[48] = vxCreateConvolution(context, 3, 3), VX_ERROR_NO_RESOURCES);

    for (i = 0; i < 48; i++)
    {
        VX_CALL(vxReleaseConvolution(&src_conv[i]));
    }
}

TEST_WITH_ARG(tivxNegativeBoundary, negativeTestPyramidBoundary, Arg,
    PARAMETERS
)
{
    vx_context context = context_->vx_context_;
    vx_pyramid   src_pyr[49];
    int i;

    for (i = 0; i < 48; i++)
    {
        ASSERT_VX_OBJECT(src_pyr[i] = vxCreatePyramid(context, 4, VX_SCALE_PYRAMID_HALF, 16, 16, VX_DF_IMAGE_U8), VX_TYPE_PYRAMID);
    }

    EXPECT_VX_ERROR(src_pyr[48] = vxCreatePyramid(context, 4, VX_SCALE_PYRAMID_HALF, 16, 16, VX_DF_IMAGE_U8), VX_ERROR_NO_RESOURCES);

    for (i = 0; i < 48; i++)
    {
        VX_CALL(vxReleasePyramid(&src_pyr[i]));
    }
}

TEST_WITH_ARG(tivxNegativeBoundary, negativeTestImageBoundary, Arg,
    PARAMETERS
)
{
    vx_context context = context_->vx_context_;
    vx_image   src_image[385];
    int i;

    for (i = 0; i < 384; i++)
    {
        ASSERT_VX_OBJECT(src_image[i] = vxCreateImage(context, 16, 16, VX_DF_IMAGE_U8), VX_TYPE_IMAGE);
    }

     EXPECT_VX_ERROR(src_image[384] = vxCreateImage(context, 16, 16, VX_DF_IMAGE_U8), VX_ERROR_NO_RESOURCES);

    for (i = 0; i < 384; i++)
    {
        VX_CALL(vxReleaseImage(&src_image[i]));
    }
}

TEST_WITH_ARG(tivxNegativeBoundary, negativeTestPyramidLevelBoundary, Arg,
    PARAMETERS
)
{
    vx_context context = context_->vx_context_;
    vx_pyramid   src_pyr;

    EXPECT_VX_ERROR(src_pyr = vxCreatePyramid(context, 33, VX_SCALE_PYRAMID_HALF, 16, 16, VX_DF_IMAGE_U8), VX_ERROR_NO_RESOURCES);
}

TESTCASE_TESTS(tivxBoundary,
        testImageBoundary,
        testPyramidBoundary,
        testPyramidLevelBoundary,
        testArrayBoundary,
        testConvolutionBoundary,
        testDistributionBoundary,
        testLUTBoundary,
        testDelayBoundary,
        testMatrixBoundary,
        testRemapBoundary,
        testScalarBoundary,
        testThresholdBoundary,
        testKernelBoundary, 
        testNodeBoundary,
        testParameterBoundary,
        testGraphBoundary,
        testObjectArray,
        testObjectArrayItems,
        testContext
        )

TESTCASE_TESTS(tivxNegativeBoundary,
        negativeTestObjectArrayItems,
        negativeTestObjectArray,
        /*negativeTestParameterBoundary,*/
        negativeTestGraphBoundary,
        negativeTestNodeBoundary,
        /*negativeTestKernelBoundary,*/
        negativeTestPyramidLevelBoundary,
        negativeTestThresholdBoundary,
        negativeTestScalarBoundary,
        negativeTestRemapBoundary,
        negativeTestMatrixBoundary,
        negativeTestDelayBoundary,
        negativeTestLUTBoundary,
        negativeTestDistributionBoundary,
        negativeTestConvolutionBoundary,
        negativeTestArrayBoundary,
        negativeTestPyramidBoundary,
        negativeTestImageBoundary
        )

