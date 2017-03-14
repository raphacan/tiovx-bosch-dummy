/* 
 * Copyright (c) 2012-2016 The Khronos Group Inc.
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

#include <math.h>
#include <float.h>
#include <string.h>
#include <VX/vx.h>
#include <VX/vxu.h>
#include "test_tiovx.h"

#ifndef M_PI
#define M_PIF   3.14159265358979323846f
#else
#define M_PIF   (vx_float32)M_PI
#endif

static CT_Image own_generate_random(const char* fileName, int width, int height)
{
    CT_Image image;

    ASSERT_NO_FAILURE_(return 0,
            image = ct_allocate_ct_image_random(width, height, VX_DF_IMAGE_U8, &CT()->seed_, 0, 256));

    return image;
}


TESTCASE(WarpPerspective, CT_VXContext, ct_setup_vx_context, 0)


enum CT_PerspectiveMatrixType {
    VX_MATRIX_IDENT = 0,
    VX_MATRIX_SCALE,
    VX_MATRIX_SCALE_ROTATE,
    VX_MATRIX_RANDOM
};

#define VX_NN_AREA_SIZE         1.5
#define VX_BILINEAR_TOLERANCE   1

static CT_Image warp_perspective_read_image_8u(const char* fileName, int width, int height)
{
    CT_Image image = NULL;

    image = ct_read_image(fileName, 1);
    ASSERT_(return 0, image);
    ASSERT_(return 0, image->format == VX_DF_IMAGE_U8);

    return image;
}

static CT_Image warp_perspective_generate_random(const char* fileName, int width, int height)
{
    CT_Image image;

    ASSERT_NO_FAILURE_(return 0,
            image = ct_allocate_ct_image_random(width, height, VX_DF_IMAGE_U8, &CT()->seed_, 0, 256));

    return image;
}

#define RND_FLT(low, high)      (vx_float32)CT_RNG_NEXT_REAL(CT()->seed_, low, high);
static void warp_perspective_generate_matrix(vx_float32 *m, int src_width, int src_height, int dst_width, int dst_height, int type)
{
    vx_float32 mat[3][3];
    vx_float32 angle, scale_x, scale_y, cos_a, sin_a;
    if (VX_MATRIX_IDENT == type)
    {
        mat[0][0] = 1.f;
        mat[0][1] = 0.f;
        mat[0][2] = 0.f;

        mat[1][0] = 0.f;
        mat[1][1] = 1.f;
        mat[1][2] = 0.f;

        mat[2][0] = 0.f;
        mat[2][1] = 0.f;
        mat[2][2] = 1.f;
    }
    else if (VX_MATRIX_SCALE == type)
    {
        scale_x = src_width / (vx_float32)dst_width;
        scale_y = src_height / (vx_float32)dst_height;

        mat[0][0] = scale_x;
        mat[0][1] = 0.f;
        mat[0][2] = 0.f;

        mat[1][0] = 0.f;
        mat[1][1] = scale_y;
        mat[1][2] = 0.f;

        mat[2][0] = 0.f;
        mat[2][1] = 0.f;
        mat[2][2] = 1.f;
    }
    else if (VX_MATRIX_SCALE_ROTATE == type)
    {
        angle = M_PIF / RND_FLT(3.f, 6.f);
        scale_x = src_width / (vx_float32)dst_width;
        scale_y = src_height / (vx_float32)dst_height;
        cos_a = cosf(angle);
        sin_a = sinf(angle);

        mat[0][0] = cos_a * scale_x;
        mat[0][1] = sin_a * scale_y;
        mat[0][2] = 0.f;

        mat[1][0] = -sin_a * scale_x;
        mat[1][1] = cos_a  * scale_y;
        mat[1][2] = 0.f;

        mat[2][0] = 0.f;
        mat[2][1] = 0.f;
        mat[2][2] = 1.f;
    }
    else// if (VX_MATRIX_RANDOM == type)
    {
        angle = M_PIF / RND_FLT(3.f, 6.f);
        scale_x = src_width / (vx_float32)dst_width;
        scale_y = src_height / (vx_float32)dst_height;
        cos_a = cosf(angle);
        sin_a = sinf(angle);

        mat[0][0] = cos_a * RND_FLT(scale_x / 2.f, scale_x);
        mat[0][1] = sin_a * RND_FLT(scale_y / 2.f, scale_y);
        mat[0][2] = RND_FLT(0.f, 0.1f);

        mat[1][0] = -sin_a * RND_FLT(scale_y / 2.f, scale_y);
        mat[1][1] = cos_a  * RND_FLT(scale_x / 2.f, scale_x);
        mat[1][2] = RND_FLT(0.f, 0.1f);

        mat[2][0] = src_width  / 5.f * RND_FLT(-1.f, 1.f);
        mat[2][1] = src_height / 5.f * RND_FLT(-1.f, 1.f);
        mat[2][2] = 1.f;
    }
    memcpy(m, mat, sizeof(mat));
}

static vx_matrix warp_perspective_create_matrix(vx_context context, vx_float32 *m)
{
    vx_matrix matrix;
    matrix = vxCreateMatrix(context, VX_TYPE_FLOAT32, 3, 3);
    if (vxGetStatus((vx_reference)matrix) == VX_SUCCESS)
    {
        if (VX_SUCCESS != vxCopyMatrix(matrix, m, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST))
        {
            VX_CALL_(return 0, vxReleaseMatrix(&matrix));
        }
    }
    return matrix;
}

static int warp_perspective_check_pixel(CT_Image input, CT_Image output, int x, int y, vx_enum interp_type, vx_border_t border, vx_float32 *m)
{
    vx_float64 x0, y0, z0, xlower, ylower, s, t;
    vx_int32 xi, yi;
    int candidate;
    vx_uint8 res = *CT_IMAGE_DATA_PTR_8U(output, x, y);

    x0 = (vx_float64)m[3 * 0 + 0] * (vx_float64)x + (vx_float64)m[3 * 1 + 0] * (vx_float64)y + (vx_float64)m[3 * 2 + 0];
    y0 = (vx_float64)m[3 * 0 + 1] * (vx_float64)x + (vx_float64)m[3 * 1 + 1] * (vx_float64)y + (vx_float64)m[3 * 2 + 1];
    z0 = (vx_float64)m[3 * 0 + 2] * (vx_float64)x + (vx_float64)m[3 * 1 + 2] * (vx_float64)y + (vx_float64)m[3 * 2 + 2];
    if (fabs(z0) < DBL_MIN)
        return 0;

    x0 = x0 / z0;
    y0 = y0 / z0;
    if (VX_INTERPOLATION_NEAREST_NEIGHBOR == interp_type)
    {
        for (yi = (vx_int32)ceil(y0 - VX_NN_AREA_SIZE); (vx_float64)yi <= y0 + VX_NN_AREA_SIZE; yi++)
        {
            for (xi = (vx_int32)ceil(x0 - VX_NN_AREA_SIZE); (vx_float64)xi <= x0 + VX_NN_AREA_SIZE; xi++)
            {
                if (0 <= xi && 0 <= yi && xi < (vx_int32)input->width && yi < (vx_int32)input->height)
                {
                    candidate = *CT_IMAGE_DATA_PTR_8U(input, xi, yi);
                }
                else if (VX_BORDER_CONSTANT == border.mode)
                {
                    candidate = border.constant_value.U8;
                }
                else
                {
                    candidate = -1;
                }
                if (candidate == -1 || candidate == res)
                    return 0;
            }
        }
        CT_FAIL_(return 1, "Check failed for pixel (%d, %d): %d", x, y, (int)res);
    }
    else if (VX_INTERPOLATION_BILINEAR == interp_type)
    {
        xlower = floor(x0);
        ylower = floor(y0);

        s = x0 - xlower;
        t = y0 - ylower;

        xi = (vx_int32)xlower;
        yi = (vx_int32)ylower;

        candidate = -1;
        if (VX_BORDER_UNDEFINED == border.mode)
        {
            if (xi >= 0 && yi >= 0 && xi < (vx_int32)input->width - 1 && yi < (vx_int32)input->height - 1)
            {
                candidate = (int)((1. - s) * (1. - t) * (vx_float64) *CT_IMAGE_DATA_PTR_8U(input, xi    , yi    ) +
                                        s  * (1. - t) * (vx_float64) *CT_IMAGE_DATA_PTR_8U(input, xi + 1, yi    ) +
                                  (1. - s) *       t  * (vx_float64) *CT_IMAGE_DATA_PTR_8U(input, xi    , yi + 1) +
                                        s  *       t  * (vx_float64) *CT_IMAGE_DATA_PTR_8U(input, xi + 1, yi + 1));
            }
        }
        else if (VX_BORDER_CONSTANT == border.mode)
        {
            candidate = (int)((1. - s) * (1. - t) * (vx_float32)CT_IMAGE_DATA_CONSTANT_8U(input, xi    , yi    , border.constant_value.U8) +
                                    s  * (1. - t) * (vx_float32)CT_IMAGE_DATA_CONSTANT_8U(input, xi + 1, yi    , border.constant_value.U8) +
                              (1. - s) *       t  * (vx_float32)CT_IMAGE_DATA_CONSTANT_8U(input, xi    , yi + 1, border.constant_value.U8) +
                                    s  *       t  * (vx_float32)CT_IMAGE_DATA_CONSTANT_8U(input, xi + 1, yi + 1, border.constant_value.U8));
        }
        if (candidate == -1 || (abs(candidate - res) <= VX_BILINEAR_TOLERANCE))
            return 0;
        return 1;
    }
    CT_FAIL_(return 1, "Interpolation type undefined");
}

static void warp_perspective_validate(CT_Image input, CT_Image output, vx_enum interp_type, vx_border_t border, vx_float32 *m)
{
    vx_uint32 err_count = 0;

    CT_FILL_IMAGE_8U(, output,
            {
                ASSERT_NO_FAILURE(err_count += warp_perspective_check_pixel(input, output, x, y, interp_type, border, m));
            });
    if (10 * err_count > output->width * output->height)
        CT_FAIL_(return, "Check failed for %d pixels", err_count);
}

static void warp_perspective_check(CT_Image input, CT_Image output, vx_enum interp_type, vx_border_t border, vx_float32* m)
{
    ASSERT(input && output);
    ASSERT( (interp_type == VX_INTERPOLATION_NEAREST_NEIGHBOR) ||
            (interp_type == VX_INTERPOLATION_BILINEAR));

    ASSERT( (border.mode == VX_BORDER_UNDEFINED) ||
            (border.mode == VX_BORDER_CONSTANT));

    warp_perspective_validate(input, output, interp_type, border, m);
    if (CT_HasFailure())
    {
        printf("=== INPUT ===\n");
        ct_dump_image_info(input);
        printf("=== OUTPUT ===\n");
        ct_dump_image_info(output);
        printf("Matrix:\n%g %g %g\n%g %g %g\n%g %g %g\n",
                m[0], m[3], m[6],
                m[1], m[4], m[7],
                m[2], m[5], m[8]);
    }
}

static void warp_perspective_sequential_check(CT_Image input, CT_Image virt, CT_Image output, vx_enum interp_type, vx_border_t border, vx_float32* m_node1, vx_float32* m_node2)
{
    ASSERT(input && output);
    ASSERT( (interp_type == VX_INTERPOLATION_NEAREST_NEIGHBOR) ||
            (interp_type == VX_INTERPOLATION_BILINEAR));

    ASSERT( (border.mode == VX_BORDER_UNDEFINED) ||
            (border.mode == VX_BORDER_CONSTANT));

    warp_perspective_validate(input, virt, interp_type, border, m_node1);
    warp_perspective_validate(virt, output, interp_type, border, m_node2);
    if (CT_HasFailure())
    {
        printf("=== INPUT ===\n");
        ct_dump_image_info(input);
        printf("=== OUTPUT ===\n");
        ct_dump_image_info(output);
        printf("Matrix:\n%g %g %g\n%g %g %g\n%g %g %g\n",
                m_node1[0], m_node1[3], m_node1[6],
                m_node1[1], m_node1[4], m_node1[7],
                m_node1[2], m_node1[5], m_node1[8]);
    }
}

typedef struct {
    const char* testName;
    CT_Image(*generator)(const char* fileName, int width, int height);
    const char*      fileName;
    int src_width, src_height;
    int width, height;
    vx_border_t border;
    vx_enum interp_type;
    int matrix_type;
} Arg;

#define ADD_VX_BORDERS_WARP_PERSPECTIVE(testArgName, nextmacro, ...) \
    CT_EXPAND(nextmacro(testArgName "/VX_BORDER_CONSTANT=1", __VA_ARGS__, { VX_BORDER_CONSTANT, {{ 1 }} })), \
    CT_EXPAND(nextmacro(testArgName "/VX_BORDER_CONSTANT=127", __VA_ARGS__, { VX_BORDER_CONSTANT, {{ 127 }} }))

#define ADD_VX_INTERP_TYPE_WARP_PERSPECTIVE(testArgName, nextmacro, ...) \
    CT_EXPAND(nextmacro(testArgName "/VX_INTERPOLATION_NEAREST_NEIGHBOR", __VA_ARGS__, VX_INTERPOLATION_NEAREST_NEIGHBOR)), \
    CT_EXPAND(nextmacro(testArgName "/VX_INTERPOLATION_BILINEAR", __VA_ARGS__, VX_INTERPOLATION_BILINEAR ))

#define ADD_VX_INTERPOLATION_TYPE_NEAREST_NEIGHBOR(testArgName, nextmacro, ...) \
    CT_EXPAND(nextmacro(testArgName "/VX_INTERPOLATION_NEAREST_NEIGHBOR", __VA_ARGS__, VX_INTERPOLATION_NEAREST_NEIGHBOR))

#define ADD_VX_MATRIX_PARAM_WARP_PERSPECTIVE(testArgName, nextmacro, ...) \
    CT_EXPAND(nextmacro(testArgName "/VX_MATRIX_RANDOM", __VA_ARGS__,       VX_MATRIX_RANDOM))


#define PARAMETERS \
    CT_GENERATE_PARAMETERS("random", ADD_SIZE_18x18, ADD_VX_BORDERS_WARP_PERSPECTIVE, ADD_VX_INTERPOLATION_TYPE_NEAREST_NEIGHBOR, ADD_VX_MATRIX_PARAM_WARP_PERSPECTIVE, ARG, own_generate_random, NULL, 128, 128), \
    CT_GENERATE_PARAMETERS("random", ADD_SIZE_644x258, ADD_VX_BORDERS_WARP_PERSPECTIVE, ADD_VX_INTERPOLATION_TYPE_NEAREST_NEIGHBOR, ADD_VX_MATRIX_PARAM_WARP_PERSPECTIVE, ARG, own_generate_random, NULL, 128, 128), \
    CT_GENERATE_PARAMETERS("random", ADD_SIZE_1600x1200, ADD_VX_BORDERS_WARP_PERSPECTIVE, ADD_VX_INTERPOLATION_TYPE_NEAREST_NEIGHBOR, ADD_VX_MATRIX_PARAM_WARP_PERSPECTIVE, ARG, own_generate_random, NULL, 128, 128)

TEST_WITH_ARG(WarpPerspective, testGraphProcessing, Arg,
    PARAMETERS
)
{
    vx_context context = context_->vx_context_;
    vx_graph graph1 = 0, graph2 = 0;
    vx_node node1 = 0, node2 = 0;
    vx_node node1_graph2 = 0, node2_graph2 = 0;
    vx_image input_image = 0, input_image_graph2 = 0, output_image = 0, output_image_graph2 = 0, virt = 0, int_image = 0;
    vx_matrix matrix_node1 = 0, matrix_node2 = 0;
    vx_float32 m_node1[9], m_node2[9];
    vx_perf_t perf_node1, perf_node2, perf_graph1;
    vx_perf_t perf_node1_graph2, perf_node2_graph2, perf_graph2;

    CT_Image input = NULL, output = NULL, output_graph2 = NULL, int_cimage = NULL;

    vx_border_t border = arg_->border;

    ASSERT_NO_FAILURE(input = arg_->generator(arg_->fileName, arg_->src_width, arg_->src_height));
    ASSERT_NO_FAILURE(output = ct_allocate_image(arg_->width, arg_->height, VX_DF_IMAGE_U8));
    ASSERT_NO_FAILURE(output_graph2 = ct_allocate_image(arg_->width, arg_->height, VX_DF_IMAGE_U8));
    ASSERT_NO_FAILURE(int_cimage = ct_allocate_image(arg_->width, arg_->height, VX_DF_IMAGE_U8));

    ASSERT_VX_OBJECT(input_image = ct_image_to_vx_image(input, context), VX_TYPE_IMAGE);
    ASSERT_VX_OBJECT(output_image = ct_image_to_vx_image(output, context), VX_TYPE_IMAGE);
    ASSERT_VX_OBJECT(input_image_graph2 = ct_image_to_vx_image(input, context), VX_TYPE_IMAGE);
    ASSERT_VX_OBJECT(output_image_graph2 = ct_image_to_vx_image(output_graph2, context), VX_TYPE_IMAGE);
    ASSERT_VX_OBJECT(int_image = ct_image_to_vx_image(int_cimage, context), VX_TYPE_IMAGE);
    ASSERT_NO_FAILURE(warp_perspective_generate_matrix(m_node1, input->width, input->height, arg_->width, arg_->height, arg_->matrix_type));
    ASSERT_VX_OBJECT(matrix_node1 = warp_perspective_create_matrix(context, m_node1), VX_TYPE_MATRIX);
    ASSERT_NO_FAILURE(warp_perspective_generate_matrix(m_node2, input->width, input->height, arg_->width, arg_->height, arg_->matrix_type));
    ASSERT_VX_OBJECT(matrix_node2 = warp_perspective_create_matrix(context, m_node2), VX_TYPE_MATRIX);

    ASSERT_VX_OBJECT(graph1 = vxCreateGraph(context), VX_TYPE_GRAPH);

    ASSERT_VX_OBJECT(graph2 = vxCreateGraph(context), VX_TYPE_GRAPH);

    ASSERT_VX_OBJECT(virt = vxCreateImage(context, arg_->width, arg_->height, VX_DF_IMAGE_U8), VX_TYPE_IMAGE);

    ASSERT_VX_OBJECT(node1 = vxWarpPerspectiveNode(graph1, input_image, matrix_node1, arg_->interp_type, virt), VX_TYPE_NODE);

    ASSERT_VX_OBJECT(node2 = vxWarpPerspectiveNode(graph1, virt, matrix_node2, arg_->interp_type, output_image), VX_TYPE_NODE);

    ASSERT_VX_OBJECT(node1_graph2 = vxWarpPerspectiveNode(graph2, input_image_graph2, matrix_node1, arg_->interp_type, int_image), VX_TYPE_NODE);

    ASSERT_VX_OBJECT(node2_graph2 = vxWarpPerspectiveNode(graph2, int_image, matrix_node2, arg_->interp_type, output_image_graph2), VX_TYPE_NODE);

    VX_CALL(vxSetNodeAttribute(node1, VX_NODE_BORDER, &border, sizeof(border)));

    VX_CALL(vxSetNodeAttribute(node2, VX_NODE_BORDER, &border, sizeof(border)));

    VX_CALL(vxSetNodeAttribute(node1_graph2, VX_NODE_BORDER, &border, sizeof(border)));

    VX_CALL(vxSetNodeAttribute(node2_graph2, VX_NODE_BORDER, &border, sizeof(border)));

    VX_CALL(vxVerifyGraph(graph1));
    VX_CALL(vxProcessGraph(graph1));

    VX_CALL(vxVerifyGraph(graph2));
    VX_CALL(vxProcessGraph(graph2));

    vxQueryNode(node1, VX_NODE_PERFORMANCE, &perf_node1, sizeof(perf_node1));
    vxQueryNode(node2, VX_NODE_PERFORMANCE, &perf_node2, sizeof(perf_node2));
    vxQueryGraph(graph1, VX_GRAPH_PERFORMANCE, &perf_graph1, sizeof(perf_graph1));
    vxQueryNode(node1_graph2, VX_NODE_PERFORMANCE, &perf_node1_graph2, sizeof(perf_node1_graph2));
    vxQueryNode(node2_graph2, VX_NODE_PERFORMANCE, &perf_node2_graph2, sizeof(perf_node2_graph2));
    vxQueryGraph(graph2, VX_GRAPH_PERFORMANCE, &perf_graph2, sizeof(perf_graph2));

    ASSERT_NO_FAILURE(output = ct_image_from_vx_image(output_image));
    ASSERT_NO_FAILURE(output_graph2 = ct_image_from_vx_image(output_image_graph2));
    ASSERT_NO_FAILURE(int_cimage = ct_image_from_vx_image(int_image));
    ASSERT_NO_FAILURE(warp_perspective_sequential_check(input, int_cimage, output_graph2, arg_->interp_type, arg_->border, m_node1, m_node2));

    ASSERT_EQ_CTIMAGE(output_graph2, output);

    VX_CALL(vxReleaseNode(&node1));
    VX_CALL(vxReleaseNode(&node2));
    VX_CALL(vxReleaseNode(&node1_graph2));
    VX_CALL(vxReleaseNode(&node2_graph2));
    VX_CALL(vxReleaseGraph(&graph1));
    VX_CALL(vxReleaseGraph(&graph2));
    VX_CALL(vxReleaseMatrix(&matrix_node1));
    VX_CALL(vxReleaseMatrix(&matrix_node2));
    VX_CALL(vxReleaseImage(&int_image));
    VX_CALL(vxReleaseImage(&virt));
    VX_CALL(vxReleaseImage(&output_image));
    VX_CALL(vxReleaseImage(&input_image));
    VX_CALL(vxReleaseImage(&output_image_graph2));
    VX_CALL(vxReleaseImage(&input_image_graph2));

    ASSERT(node1 == 0);
    ASSERT(node2 == 0);
    ASSERT(node1_graph2 == 0);
    ASSERT(node2_graph2 == 0);
    ASSERT(graph1 == 0);
    ASSERT(graph2 == 0);
    ASSERT(matrix_node1 == 0);
    ASSERT(matrix_node2 == 0);
    ASSERT(output_image == 0);
    ASSERT(input_image == 0);

    printPerformance(perf_node1, arg_->width*arg_->height, "N1");
    printPerformance(perf_node2, arg_->width*arg_->height, "N2");
    printPerformance(perf_graph1, arg_->width*arg_->height, "G1");

    printPerformance(perf_node1_graph2, arg_->width*arg_->height, "N1");
    printPerformance(perf_node2_graph2, arg_->width*arg_->height, "N2");
    printPerformance(perf_graph2, arg_->width*arg_->height, "G2");
}

TESTCASE_TESTS(WarpPerspective,
        testGraphProcessing
)
