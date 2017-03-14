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


#include <VX/vx.h>

#include "test_tiovx.h"

#define VALID_SHIFT_MIN 0
// #define VALID_SHIFT_MIN -64
#define VALID_SHIFT_MAX 7

#define CT_EXECUTE_ASYNC

static void referenceConvertDepth(CT_Image src, CT_Image dst, int shift, vx_enum policy)
{
    uint32_t i, j;

    ASSERT(src && dst);
    ASSERT(src->width == dst->width);
    ASSERT(src->height == dst->height);
    ASSERT((src->format == VX_DF_IMAGE_U8 && dst->format == VX_DF_IMAGE_S16) || (src->format == VX_DF_IMAGE_S16 && dst->format == VX_DF_IMAGE_U8));
    ASSERT(policy == VX_CONVERT_POLICY_WRAP || policy == VX_CONVERT_POLICY_SATURATE);

    if (shift > 16) shift = 16;
    if (shift < -16) shift = -16;

    if (src->format == VX_DF_IMAGE_U8)
    {
        // according to spec the policy is ignored
        // if (policy == VX_CONVERT_POLICY_WRAP)
        {
            // up-conversion + wrap
            if (shift < 0)
            {
                for (i = 0; i < dst->height; ++i)
                    for (j = 0; j < dst->width; ++j)
                        dst->data.s16[i * dst->stride + j] = ((unsigned)src->data.y[i * src->stride + j]) >> (-shift);
            }
            else
            {
                for (i = 0; i < dst->height; ++i)
                    for (j = 0; j < dst->width; ++j)
                        dst->data.s16[i * dst->stride + j] = ((unsigned)src->data.y[i * src->stride + j]) << shift;
            }
        }
        // else if (VX_CONVERT_POLICY_SATURATE)
        // {
        //     // up-conversion + saturate
        //     if (shift < 0)
        //     {
        //         for (i = 0; i < dst->height; ++i)
        //             for (j = 0; j < dst->width; ++j)
        //                 dst->data.s16[i * dst->stride + j] = ((unsigned)src->data.y[i * src->stride + j]) >> (-shift);
        //     }
        //     else
        //     {
        //         for (i = 0; i < dst->height; ++i)
        //             for (j = 0; j < dst->width; ++j)
        //             {
        //                 unsigned v = ((unsigned)src->data.y[i * src->stride + j]) << shift;
        //                 if (v > 32767) v = 32767;
        //                 dst->data.s16[i * dst->stride + j] = v;
        //             }
        //     }
        // }
    }
    else if (policy == VX_CONVERT_POLICY_WRAP)
    {
        // down-conversion + wrap
        if (shift < 0)
        {
            for (i = 0; i < dst->height; ++i)
                for (j = 0; j < dst->width; ++j)
                    dst->data.y[i * dst->stride + j] = src->data.s16[i * src->stride + j] << (-shift);
        }
        else
        {
            for (i = 0; i < dst->height; ++i)
                for (j = 0; j < dst->width; ++j)
                    dst->data.y[i * dst->stride + j] = src->data.s16[i * src->stride + j] >> shift;
        }
    }
    else if (policy == VX_CONVERT_POLICY_SATURATE)
    {
        // down-conversion + saturate
        if (shift < 0)
        {
            for (i = 0; i < dst->height; ++i)
                for (j = 0; j < dst->width; ++j)
                {
                    int32_t v = src->data.s16[i * src->stride + j] << (-shift);
                    if (v > 255) v = 255;
                    if (v < 0) v = 0;
                    dst->data.y[i * dst->stride + j] = v;
                }
        }
        else
        {
            for (i = 0; i < dst->height; ++i)
                for (j = 0; j < dst->width; ++j)
                {
                    int32_t v = src->data.s16[i * src->stride + j] >> shift;
                    if (v > 255) v = 255;
                    if (v < 0) v = 0;
                    dst->data.y[i * dst->stride + j] = v;
                }
        }
    }
}

static void referenceSequentialConvertDepth(CT_Image src, CT_Image virt, CT_Image dst, int shift, vx_enum policy)
{
    referenceConvertDepth(src, virt, shift, policy);
    referenceConvertDepth(virt, dst, shift, policy);
}

static void fillSquence(CT_Image dst, uint32_t seq_init)
{
    uint32_t i, j;
    uint32_t val = seq_init;

    ASSERT(dst);
    ASSERT(dst->format == VX_DF_IMAGE_U8 || dst->format == VX_DF_IMAGE_S16);

    if (dst->format == VX_DF_IMAGE_U8)
    {
        for (i = 0; i < dst->height; ++i)
            for (j = 0; j < dst->width; ++j)
                dst->data.y[i * dst->stride + j] = ++val;
    }
    else
    {
        for (i = 0; i < dst->height; ++i)
            for (j = 0; j < dst->width; ++j)
                dst->data.s16[i * dst->stride + j] = ++val;
    }
}

TESTCASE(tivxConvertDepth,  CT_VXContext, ct_setup_vx_context, 0)

typedef struct {
    const char* name;
    uint32_t width;
    uint32_t height;
    vx_df_image format_from;
    vx_df_image format_to;
    vx_enum policy;
} cvt_depth_arg;

#define CVT_ARG(w,h,from,to,p) ARG(#p"/"#w"x"#h" "#from"->"#to, w, h, VX_DF_IMAGE_##from, VX_DF_IMAGE_##to, VX_CONVERT_POLICY_##p)

#define PREPEND_SIZE(macro, ...)                \
    CT_EXPAND(macro(18, 18, __VA_ARGS__)),        \
    CT_EXPAND(macro(644, 258, __VA_ARGS__)),      \
    CT_EXPAND(macro(1600, 1200, __VA_ARGS__))

#define CVT_ARGS                                \
    PREPEND_SIZE(CVT_ARG, U8, S16, SATURATE),   \
    PREPEND_SIZE(CVT_ARG, U8, S16, WRAP),       \
    PREPEND_SIZE(CVT_ARG, S16, U8, SATURATE),   \
    PREPEND_SIZE(CVT_ARG, S16, U8, WRAP)

TEST_WITH_ARG(tivxConvertDepth, BitExact, cvt_depth_arg, CVT_ARGS)
{
    vx_image src, dst, virt;
    CT_Image ref_src, refdst, vxdst, virt_ctimage;
    vx_graph graph;
    vx_node node1, node2;
    vx_scalar scalar_shift;
    vx_int32 shift = 2;
    vx_int32 tmp = 0;
    vx_context context = context_->vx_context_;
    vx_perf_t perf_node1, perf_node2, perf_node3, perf_graph;

    ASSERT_NO_FAILURE({
        ref_src = ct_allocate_image(arg_->width, arg_->height, arg_->format_from);
        fillSquence(ref_src, (uint32_t)CT()->seed_);
        src = ct_image_to_vx_image(ref_src, context);
    });

    ASSERT_VX_OBJECT(dst = vxCreateImage(context, arg_->width, arg_->height, arg_->format_from), VX_TYPE_IMAGE);
    ASSERT_VX_OBJECT(scalar_shift = vxCreateScalar(context, VX_TYPE_INT32, &tmp), VX_TYPE_SCALAR);
    ASSERT_VX_OBJECT(graph = vxCreateGraph(context), VX_TYPE_GRAPH);
    ASSERT_VX_OBJECT(virt   = vxCreateVirtualImage(graph, 0, 0, arg_->format_to), VX_TYPE_IMAGE);
    ASSERT_VX_OBJECT(node1 = vxConvertDepthNode(graph, src, virt, arg_->policy, scalar_shift), VX_TYPE_NODE);
    ASSERT_VX_OBJECT(node2 = vxConvertDepthNode(graph, virt, dst, arg_->policy, scalar_shift), VX_TYPE_NODE);

    ASSERT_EQ_VX_STATUS(VX_SUCCESS, vxCopyScalar(scalar_shift, &shift, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST));

    // run graph
#ifdef CT_EXECUTE_ASYNC
    EXPECT_EQ_VX_STATUS(VX_SUCCESS, vxScheduleGraph(graph));
    EXPECT_EQ_VX_STATUS(VX_SUCCESS, vxWaitGraph(graph));
#else
    EXPECT_EQ_VX_STATUS(VX_SUCCESS, vxProcessGraph(graph));
#endif

    vxQueryNode(node1, VX_NODE_PERFORMANCE, &perf_node1, sizeof(perf_node1));
    vxQueryNode(node2, VX_NODE_PERFORMANCE, &perf_node2, sizeof(perf_node2));
    vxQueryGraph(graph, VX_GRAPH_PERFORMANCE, &perf_graph, sizeof(perf_graph));

    ASSERT_NO_FAILURE({
        vxdst = ct_image_from_vx_image(dst);
        virt_ctimage = ct_allocate_image(arg_->width, arg_->height, arg_->format_to);
        refdst = ct_allocate_image(arg_->width, arg_->height, arg_->format_from);
        referenceSequentialConvertDepth(ref_src, virt_ctimage, refdst, shift, arg_->policy);
    });

    EXPECT_EQ_CTIMAGE(refdst, vxdst);

    VX_CALL(vxReleaseImage(&dst));
    VX_CALL(vxReleaseImage(&virt));
    VX_CALL(vxReleaseImage(&src));
    VX_CALL(vxReleaseScalar(&scalar_shift));
    VX_CALL(vxReleaseNode(&node1));
    VX_CALL(vxReleaseNode(&node2));
    VX_CALL(vxReleaseGraph(&graph));

    printPerformance(perf_node1, arg_->width*arg_->height, "N1");
    printPerformance(perf_node2, arg_->width*arg_->height, "N2");
    printPerformance(perf_graph, arg_->width*arg_->height, "G1");
}

TESTCASE_TESTS(tivxConvertDepth, BitExact)
