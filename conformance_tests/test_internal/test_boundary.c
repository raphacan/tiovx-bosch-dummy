/*

 * Copyright (c) 2012-2017 The Khronos Group Inc.
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

#include "test_tiovx.h"
#include <TI/tivx_obj_desc.h>
#include <TI/tivx_test_kernels.h>
#include <TI/tivx_config.h>
#include <TI/tivx_capture.h>
#include <VX/vx.h>
#include <VX/vxu.h>
#include <string.h>
#include <math.h>
#include <TI/tivx_mutex.h>
#include <TI/tivx_queue.h>

/* The below include files are used for TIVX_TEST_WAIVER_COMPLEXITY_AND_MAINTENANCE_COST_001
 * described below */
#include <tivx_event_queue.h>
#include <tivx_obj_desc_priv.h>
#include <vx_reference.h>
#include <vx_context.h>
#include <tivx_data_ref_queue.h>
#include <vx_node.h>

#include "shared_functions.h"

#define MAX_POINTS 100

TESTCASE(tivxObjDescBoundary, CT_VXContext, ct_setup_vx_context, 0)

TEST(tivxObjDescBoundary, negativeTestObjDescBoundary)
{
    extern tivx_obj_desc_t *ownObjDescAlloc(vx_enum type, vx_reference ref);
    extern vx_status ownObjDescFree(tivx_obj_desc_t **obj_desc);

    vx_context context = context_->vx_context_;
    int i, j;
    tivx_obj_desc_t *obj_desc[TIVX_PLATFORM_MAX_OBJ_DESC_SHM_INST] = {NULL};
    vx_image img = NULL;

    img = (vx_image)ownCreateReference(context, (vx_enum)VX_TYPE_IMAGE, (vx_enum)VX_EXTERNAL, &context->base);

    for (i = 0; i < TIVX_PLATFORM_MAX_OBJ_DESC_SHM_INST; i++)
    {
        obj_desc[i] = (tivx_obj_desc_t *)ownObjDescAlloc((vx_enum)TIVX_OBJ_DESC_IMAGE, (vx_reference)img);
        if (NULL != obj_desc[i])
        {
            break;
        }
    }

     for (j = 0; j < i; j++)
    {
        if (NULL == obj_desc[j])
        {
            break;
        }
        VX_CALL(ownObjDescFree((tivx_obj_desc_t**)&obj_desc[j]));
    }

    VX_CALL(ownReleaseReferenceInt((vx_reference*)&img, (vx_enum)VX_TYPE_IMAGE, (vx_enum)VX_EXTERNAL, NULL));
}

TEST(tivxObjDescBoundary, negativeBoundaryThreshold)
{
    extern tivx_obj_desc_t *ownObjDescAlloc(vx_enum type, vx_reference ref);
    extern vx_status ownObjDescFree(tivx_obj_desc_t **obj_desc);

    vx_context context = context_->vx_context_;
    int i, j;
    tivx_obj_desc_t *obj_desc[TIVX_PLATFORM_MAX_OBJ_DESC_SHM_INST] = {NULL};
    vx_image img;
    vx_image img1;
    vx_threshold vxt=NULL;
    vx_object_array vxoa = NULL;
    vx_node node = NULL;
    tivx_shared_mem_ptr_t tsmp;
    uint32_t size = 1024U;
    vx_object_array src_object_array;

    vx_convolution conv = NULL;
    vx_size rows = 3, cols = 3;

    vx_distribution dist = NULL;
    int32_t udata[256];
    vx_enum usage = VX_WRITE_ONLY, user_mem_type = VX_MEMORY_TYPE_HOST;
    vx_size num_bins = 1;
    vx_int32 offset = 1;
    vx_uint32 range = 5;
    vx_uint8  scalar_val = 0;
    vx_scalar scalar = NULL;

    vx_matrix matrix = NULL;
    vx_enum data_type = VX_TYPE_INT8;

    // For Tensor
    vx_size nod = TIVX_CONTEXT_MAX_TENSOR_DIMS;
    vx_size dims[TIVX_CONTEXT_MAX_TENSOR_DIMS] = {0};
    vx_enum dt = VX_TYPE_UINT8;
    vx_int8 fpp = 0;

    vx_array array = NULL;

    vx_lut lut = NULL;
    vx_size count = 0;

    vx_user_data_object user_data_object = 0;
    vx_uint32 usrdata = 0;

    vx_remap remap = NULL;
    vx_uint32 src_width = 1, src_height = 1, dst_width = 1, dst_height = 1;

    vx_pyramid pymd = NULL, pymd1 = NULL;
    vx_uint32 width = 3, height = 3;
    vx_size levels = 1;
    vx_float32 scale = 0.9f;
    vx_df_image format = VX_DF_IMAGE_U8;

    tivx_raw_image raw_image;
    tivx_raw_image_create_params_t params;
    params.width = 128;
    params.height = 128;
    params.num_exposures = 3;
    params.line_interleaved = vx_true_e;
    params.format[0].pixel_container = TIVX_RAW_IMAGE_16_BIT;
    params.format[0].msb = 12;
    params.format[1].pixel_container = TIVX_RAW_IMAGE_8_BIT;
    params.format[1].msb = 7;
    params.format[2].pixel_container = TIVX_RAW_IMAGE_P12_BIT;
    params.format[2].msb = 11;
    params.meta_height_before = 5;
    params.meta_height_after = 0;

    ASSERT_VX_OBJECT(conv = vxCreateConvolution(context, cols, rows), VX_TYPE_CONVOLUTION);
    ASSERT_EQ_VX_STATUS(VX_SUCCESS, ownAllocReferenceBufferGeneric((vx_reference)conv));
    VX_CALL(vxReleaseConvolution(&conv));

    vx_graph graph = NULL;
    vx_tensor tensor;
    ASSERT_VX_OBJECT(graph = vxCreateGraph(context), VX_TYPE_GRAPH);

    img = (vx_image)ownCreateReference(context, (vx_enum)VX_TYPE_IMAGE, (vx_enum)VX_EXTERNAL, &context->base);
    for (i = 0; i < TIVX_PLATFORM_MAX_OBJ_DESC_SHM_INST-1; i++)
    {
        obj_desc[i] = (tivx_obj_desc_t *)ownObjDescAlloc((vx_enum)TIVX_OBJ_DESC_IMAGE, (vx_reference)img);
        if (NULL == obj_desc[i])
        {
            break;
        }
    }

    EXPECT_VX_ERROR(vxt = vxCreateThreshold(context, VX_THRESHOLD_TYPE_RANGE, VX_TYPE_UINT8), VX_ERROR_NO_RESOURCES);
    EXPECT_VX_ERROR(vxoa = vxCreateObjectArray(context, (vx_reference)img, 2), VX_ERROR_NO_RESOURCES);
    EXPECT_VX_ERROR(src_object_array = vxCreateVirtualObjectArray(graph, (vx_reference)img, 32), VX_ERROR_NO_RESOURCES);
    EXPECT_VX_ERROR(img1 = vxCreateImage(context, 640, 480, VX_DF_IMAGE_U8), VX_ERROR_NO_RESOURCES);
    EXPECT_VX_ERROR(dist = vxCreateDistribution(context, num_bins, offset, range), VX_ERROR_NO_RESOURCES);
    EXPECT_VX_ERROR(matrix = vxCreateMatrix(context, data_type, cols, rows), VX_ERROR_NO_RESOURCES);
    EXPECT_VX_ERROR(matrix = vxCreateMatrixFromPattern(context, VX_PATTERN_OTHER, cols, rows), VX_ERROR_NO_RESOURCES);
    EXPECT_VX_ERROR(scalar = vxCreateScalar(context, VX_TYPE_UINT8, &scalar_val), VX_ERROR_NO_RESOURCES);
    EXPECT_VX_ERROR(tensor = vxCreateTensor(context, nod, dims, dt, fpp), VX_ERROR_NO_RESOURCES);
    EXPECT_VX_ERROR(array = vxCreateArray(context, VX_TYPE_ARRAY, 1),VX_ERROR_NO_RESOURCES);
    EXPECT_VX_ERROR(array = vxCreateVirtualArray(graph, VX_TYPE_KEYPOINT, 1),VX_ERROR_NO_RESOURCES);
    EXPECT_VX_ERROR(lut = vxCreateLUT(context, data_type, count), VX_ERROR_NO_RESOURCES);
    EXPECT_VX_ERROR(user_data_object = vxCreateUserDataObject(context, NULL, sizeof(vx_uint32), &usrdata), VX_ERROR_NO_RESOURCES);
    EXPECT_VX_ERROR(remap = vxCreateRemap(context, src_width, src_height, dst_width, dst_height),VX_ERROR_NO_RESOURCES);
    EXPECT_VX_ERROR(conv = vxCreateConvolution(context, cols, rows), VX_ERROR_NO_RESOURCES);

    EXPECT_VX_ERROR(pymd = vxCreatePyramid(context, levels, scale, width, height, format), VX_ERROR_NO_RESOURCES);
    EXPECT_VX_ERROR(pymd1 = vxCreateVirtualPyramid(graph, levels, VX_SCALE_PYRAMID_HALF, width, height, format), VX_ERROR_NO_RESOURCES);
    EXPECT_VX_ERROR(raw_image = tivxCreateRawImage(context, &params), VX_ERROR_NO_RESOURCES);
    ASSERT_EQ_VX_STATUS(VX_FAILURE, ownNodeKernelInitKernelName(node));
    ASSERT_EQ_VX_STATUS(VX_ERROR_INVALID_REFERENCE, ownAllocReferenceBufferGeneric((vx_reference)img));
    for (j = 0; j < i-1; j++)
    {
        if (NULL == obj_desc[j])
        {
            break;
        }
        VX_CALL(ownObjDescFree((tivx_obj_desc_t**)&obj_desc[j]));
    }
    if(NULL!=img)
    {
        VX_CALL(ownReleaseReferenceInt((vx_reference*)&img, (vx_enum)VX_TYPE_IMAGE, (vx_enum)VX_EXTERNAL, NULL));
    }
    VX_CALL(vxReleaseGraph(&graph));
}

TESTCASE_TESTS(tivxObjDescBoundary,
        negativeTestObjDescBoundary,
        negativeBoundaryThreshold
        )