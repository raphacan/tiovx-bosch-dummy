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

#include <VX/vx.h>
#include <TI/tivx.h>
#include <TI/tda4x.h>
#include "test_engine/test.h"
#include <TI/tivx_config.h>
#include <string.h>
#include "tivx_utils_file_rd_wr.h"
#include <TI/tivx_task.h>
#include "math.h"
#include <limits.h>
#include "test_tiovx/test_tiovx.h"
#include "tivx_utils_file_rd_wr.h"

#define MAX_NUM_BUF        (8u)
#define MAX_ABS_FILENAME   (1024u)

static const vx_char user_data_object_name[] = "tivx_capture_params_t";

static void make_filename(char *abs_filename, char *filename)
{
    snprintf(abs_filename, MAX_ABS_FILENAME, "%s/%s",
        ct_get_test_file_path(), filename);
}

/*
 * Utility API to set number of buffers at a node parameter
 * The parameter MUST be a output or bidirectonal parameter for the setting
 * to take effect
 */
static vx_status set_num_buf_by_node_index(vx_node node, vx_uint32 node_parameter_index, vx_uint32 num_buf)
{
    return tivxSetNodeParameterNumBufByIndex(node, node_parameter_index, num_buf);
}

/*
 * Utility API used to add a graph parameter from a node, node parameter index
 */
static void add_graph_parameter_by_node_index(vx_graph graph, vx_node node, vx_uint32 node_parameter_index)
{
    vx_parameter parameter = vxGetParameterByIndex(node, node_parameter_index);

    vxAddParameterToGraph(graph, parameter);
    vxReleaseParameter(&parameter);
}

/*
 * Utility API to set pipeline depth for a graph
 */
static vx_status set_graph_pipeline_depth(vx_graph graph, vx_uint32 pipeline_depth)
{
    return tivxSetGraphPipelineDepth(graph, pipeline_depth);
}

/*
 * Utility API to set trigger node for a graph
 */
static vx_status set_graph_trigger_node(vx_graph graph, vx_node node)
{
    return tivxEnableGraphStreaming(graph, node);
}

/*
 * Utility API to export graph information to file for debug and visualization
 */
static vx_status export_graph_to_file(vx_graph graph, char *filename_prefix)
{
    return tivxExportGraphToDot(graph, ct_get_test_file_path(), filename_prefix);
}

static void printGraphPipelinePerformance(vx_graph graph,
            vx_node nodes[], uint32_t num_nodes,
            uint64_t exe_time, uint32_t loop_cnt, uint32_t numPixels)
{
    #define MAX_TEST_NAME (8u)

    vx_perf_t perf_ref;
    char ref_name[MAX_TEST_NAME];
    uint32_t i;
    uint64_t avg_exe_time;

    avg_exe_time = exe_time / loop_cnt;

    for(i=0; i<num_nodes; i++)
    {
         vxQueryNode(nodes[i], VX_NODE_PERFORMANCE, &perf_ref, sizeof(perf_ref));
         snprintf(ref_name,MAX_TEST_NAME, "N%d ", i);
         printPerformance(perf_ref, numPixels, ref_name);
    }

    #if 0
    vxQueryGraph(graph, VX_GRAPH_PERFORMANCE, &perf_ref, sizeof(perf_ref));
    snprintf(ref_name,MAX_TEST_NAME, "G0 ");
    printPerformance(perf_ref, numPixels, ref_name);
    #endif

    printf("[ SYS ] Execution time (avg = %4d.%03d ms, sum = %4d.%03d ms, num = %d)\n",
        (uint32_t)(avg_exe_time/1000u), (uint32_t)(avg_exe_time%1000u),
        (uint32_t)(exe_time/1000u), (uint32_t)(exe_time%1000u),
        loop_cnt
        );
}

/*
 * Utility API to log graph run-time trace
 */
static vx_status log_graph_rt_trace(vx_graph graph)
{
    vx_status status = VX_SUCCESS;

    #if LOG_RT_TRACE_ENABLE
    status = tivxLogRtTrace(graph);
    #endif
    return status;
}

TESTCASE(tivxHwaCapture, CT_VXContext, ct_setup_vx_context, 0)

typedef struct {
    const char* name;
    int stream_time;
    int measure_perf;
} Arg_Capture;

#define STREAMING_PARAMETERS \
    CT_GENERATE_PARAMETERS("streaming", ARG, 5, 0), \
    CT_GENERATE_PARAMETERS("streaming", ARG, 5, 1), \
    CT_GENERATE_PARAMETERS("streaming", ARG, 5, 1), \
    CT_GENERATE_PARAMETERS("streaming", ARG, 5, 1), \
    CT_GENERATE_PARAMETERS("streaming", ARG, 5, 1), \
    CT_GENERATE_PARAMETERS("streaming", ARG, 5, 1), \
    CT_GENERATE_PARAMETERS("streaming", ARG, 5, 1), \
    CT_GENERATE_PARAMETERS("streaming", ARG, 5, 1), \
    CT_GENERATE_PARAMETERS("streaming", ARG, 5, 1), \
    
    #if 0
    CT_GENERATE_PARAMETERS("streaming", ARG, 100, 1), \
    
    #endif

TEST_WITH_ARG(tivxHwaCapture, testGraphProcessing, Arg_Capture, STREAMING_PARAMETERS)
{
    vx_context context = context_->vx_context_;
    vx_graph graph;
    vx_node n0;
    vx_object_array capture_frames[MAX_NUM_BUF];
    vx_user_data_object capture_config;
    tivx_capture_params_t local_capture_config;
    vx_image img_exemplar;
    uint32_t width = 320, height = 240;
    uint32_t objarr_idx, num_capture_frames = 1; /* TODO: eventually move to 4, but use 1 for now */
    uint32_t buf_id, loop_id, loop_cnt, num_buf, loopCnt, frameIdx;
    CT_Image tst_img;
    vx_graph_parameter_queue_params_t graph_parameters_queue_params_list[1];
    uint64_t exe_time; /* TODO: Add in profiling info */
    char filename[MAX_ABS_FILENAME];

    /* Setting to num buf of capture node */
    num_buf = 3;
    loop_cnt = arg_->stream_time;

    tivxHwaLoadKernels(context);

    tivx_clr_debug_zone(VX_ZONE_INFO);

    ASSERT_VX_OBJECT(graph = vxCreateGraph(context), VX_TYPE_GRAPH);

    /* Hardcoding since reading from sample image */
    ASSERT_VX_OBJECT(img_exemplar = vxCreateImage(context, width, height, VX_DF_IMAGE_RGBX), VX_TYPE_IMAGE);

    /* allocate Input and Output refs, multiple refs created to allow pipelining of graph */
    for(buf_id=0; buf_id<num_buf; buf_id++)
    {
        ASSERT_VX_OBJECT(capture_frames[buf_id] = vxCreateObjectArray(context, (vx_reference)img_exemplar, num_capture_frames), VX_TYPE_OBJECT_ARRAY);
    }

    /* Config initialization */
    local_capture_config.enableCsiv2p0Support = (uint32_t)vx_true_e;
    local_capture_config.isRawCapture = (uint32_t)vx_false_e;
    local_capture_config.numDataLanes = 4U;
    for (loopCnt = 0U ;
         loopCnt < local_capture_config.numDataLanes ;
         loopCnt++)
    {
        local_capture_config.dataLanesMap[loopCnt] = loopCnt;
    }

    ASSERT_VX_OBJECT(capture_config = vxCreateUserDataObject(context, user_data_object_name, sizeof(tivx_capture_params_t), &local_capture_config), (enum vx_type_e)VX_TYPE_USER_DATA_OBJECT);

    ASSERT_VX_OBJECT(n0 = tivxCaptureNode(graph, capture_config, capture_frames[0]), VX_TYPE_NODE);

    /* input @ node index 0, becomes graph parameter 1 */
    add_graph_parameter_by_node_index(graph, n0, 1);

    /* set graph schedule config such that graph parameter @ index 0 and 1 are enqueuable */
    graph_parameters_queue_params_list[0].graph_parameter_index = 0;
    graph_parameters_queue_params_list[0].refs_list_size = num_buf;
    graph_parameters_queue_params_list[0].refs_list = (vx_reference*)&capture_frames[0];

    /* Schedule mode auto is used, here we dont need to call vxScheduleGraph
     * Graph gets scheduled automatically as refs are enqueued to it
     */
    vxSetGraphScheduleConfig(graph,
            VX_GRAPH_SCHEDULE_MODE_QUEUE_AUTO,
            1,
            graph_parameters_queue_params_list
            );


    VX_CALL(vxSetNodeTarget(n0, VX_TARGET_STRING, TIVX_TARGET_CAPTURE1));
    ASSERT_EQ_VX_STATUS(VX_SUCCESS, vxVerifyGraph(graph));

    /*export_graph_to_file(graph, "test_capture_node");
    log_graph_rt_trace(graph);*/

    exe_time = tivxPlatformGetTimeInUsecs();

    /* enqueue buf for pipeup but dont trigger graph execution */
    for(buf_id=0; buf_id<num_buf-1; buf_id++)
    {
        tivxGraphParameterEnqueueReadyRef(graph, 0, (vx_reference*)&capture_frames[buf_id], 1, TIVX_GRAPH_PARAMETER_ENQUEUE_FLAG_PIPEUP);
    }

    /* after pipeup, now enqueue a buffer to trigger graph scheduling */
    vxGraphParameterEnqueueReadyRef(graph, 0, (vx_reference*)&capture_frames[buf_id], 1);

    /* wait for graph instances to complete, compare output and recycle data buffers, schedule again */
    for(loop_id=0; loop_id<(loop_cnt+num_buf); loop_id++)
    {
        uint32_t num_refs;
        vx_object_array out_capture_frames;

        /* Get output reference, waits until a reference is available */
        vxGraphParameterDequeueDoneRef(graph, 0, (vx_reference*)&out_capture_frames, 1, &num_refs);

        if(arg_->measure_perf==0)
        {
            for (frameIdx = 0; frameIdx < num_capture_frames; frameIdx++)
            {
                vx_image out_img;

                ASSERT_VX_OBJECT(out_img = (vx_image)vxGetObjectArrayItem(out_capture_frames, frameIdx), VX_TYPE_IMAGE);

                ASSERT_NO_FAILURE({
                    tst_img = ct_image_from_vx_image(out_img);
                });

                /* test to make sure it contains data */
                ASSERT(tst_img->data.y[0] != 0x0);

                VX_CALL(vxReleaseImage(&out_img));
            }
        }

        vxGraphParameterEnqueueReadyRef(graph, 0, (vx_reference*)&out_capture_frames, 1);

    }

    /* ensure all graph processing is complete */
    vxWaitGraph(graph);

    exe_time = tivxPlatformGetTimeInUsecs() - exe_time;

    /*if(arg_->measure_perf==1)
    {
        vx_node nodes[] = { n0 };

        printGraphPipelinePerformance(graph, nodes, 1, exe_time, loop_cnt+num_buf, width*height);
    }*/

    VX_CALL(vxReleaseNode(&n0));
    VX_CALL(vxReleaseGraph(&graph));

    VX_CALL(vxReleaseImage(&img_exemplar));
    for(buf_id=0; buf_id<num_buf; buf_id++)
    {
        VX_CALL(vxReleaseObjectArray(&capture_frames[buf_id]));
    }
    VX_CALL(vxReleaseUserDataObject(&capture_config));

    tivxHwaUnLoadKernels(context);

    tivx_clr_debug_zone(VX_ZONE_INFO);
}


TESTCASE_TESTS(tivxHwaCapture, testGraphProcessing)

