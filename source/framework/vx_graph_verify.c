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



/*
 * Since graph verify is involved logic, putting it in a different file.
 */

#include <vx_internal.h>

static vx_status ownGraphValidRectCallback(
    vx_graph graph, vx_node node, vx_meta_format meta[]);
static vx_status ownGraphInitVirtualNode(
    vx_graph graph, vx_node node, vx_meta_format meta[]);
static vx_status ownGraphNodeKernelValidate(
    vx_graph graph, vx_meta_format meta[]);
static vx_status ownGraphNodeKernelInit(vx_graph graph);
static vx_status ownGraphNodeKernelDeinit(vx_graph graph);
static vx_bool ownGraphIsRefMatch(vx_graph graph, vx_reference ref1, vx_reference ref2);
static vx_status ownGraphCalcInAndOutNodes(vx_graph graph);
static vx_status ownGraphCalcHeadAndLeafNodes(vx_graph graph);
static vx_status ownGraphAllocateDataObjects(vx_graph graph);
static vx_status ownGraphNodeCreateCompletionEvents(vx_graph graph);
static vx_status ownGraphCreateNodeCallbackCommands(vx_graph graph);


static vx_status ownGraphValidRectCallback(
    vx_graph graph, vx_node node, vx_meta_format meta[])
{
    vx_status status = VX_SUCCESS;
    vx_uint32 num_in_image = 0, num_params, i;
    vx_reference ref;
    vx_meta_format mf;

    num_params = node->kernel->signature.num_parameters;
    for (i = 0; i < num_params; i ++)
    {
        ref = node->parameters[i];

        if( (node->kernel->signature.directions[i] == VX_INPUT)
            &&
            (NULL != ref)
            &&
            (ref->type == VX_TYPE_IMAGE)
            )
        {
            graph->in_valid_rect_ptr[num_in_image] = &graph->in_valid_rect[num_in_image];

            status |= vxGetValidRegionImage((vx_image)ref, &graph->in_valid_rect[num_in_image]);

            num_in_image++;
        }
    }

    if(status == VX_SUCCESS)
    {
        for (i = 0; i < num_params; i ++)
        {
            ref = node->parameters[i];
            mf = meta[i];

            if( (node->kernel->signature.directions[i] == VX_OUTPUT)
                &&
                (NULL != ref)
                &&
                (NULL != mf)
                &&
                (NULL != mf->valid_rect_callback)
                )
            {
                if(ref->type == VX_TYPE_IMAGE)
                {
                    vx_status tmp_status;

                    graph->out_valid_rect_ptr[0] = &graph->out_valid_rect[0];

                    tmp_status = mf->valid_rect_callback(node, i,
                                    (const vx_rectangle_t* const*)graph->in_valid_rect_ptr,
                                    graph->out_valid_rect_ptr);
                    if(tmp_status==VX_SUCCESS)
                    {
                        status |= vxSetImageValidRectangle((vx_image)ref, &graph->out_valid_rect[0]);
                    }
                }
                else
                if(ref->type == VX_TYPE_PYRAMID)
                {
                    vx_status tmp_status;
                    vx_size levels, k;

                    status |= vxQueryPyramid((vx_pyramid)ref, VX_PYRAMID_LEVELS, &levels, sizeof(levels));

                    if(status==VX_SUCCESS)
                    {
                        for(k=0; k<levels; k++)
                        {
                            graph->out_valid_rect_ptr[k] = &graph->out_valid_rect[k];
                        }

                        tmp_status = mf->valid_rect_callback(node, i,
                                        (const vx_rectangle_t* const*)graph->in_valid_rect_ptr,
                                        graph->out_valid_rect_ptr);
                        if(tmp_status==VX_SUCCESS)
                        {
                            for(k=0; k<levels; k++)
                            {
                                vx_image img = vxGetPyramidLevel((vx_pyramid)ref, k);

                                status |= vxSetImageValidRectangle(img, &graph->out_valid_rect[k]);

                                vxReleaseImage(&img);
                            }
                        }
                    }
                }
                else
                {
                    /* not supported for other references */
                    status = VX_FAILURE;
                    VX_PRINT(VX_ZONE_ERROR,"not supported for references other than image or pyramid\n");
                }
            }
        }
    }
    return status;
}

static vx_status ownGraphInitVirtualNode(
    vx_graph graph, vx_node node, vx_meta_format meta[])
{
    vx_status status = VX_SUCCESS;
    uint32_t i, num_params;
    vx_reference ref;
    vx_meta_format mf;
    vx_pyramid pmd;
    vx_size levels;
    vx_enum type;

    num_params = node->kernel->signature.num_parameters;
    for (i = 0; i < num_params; i ++)
    {
        ref = node->parameters[i];
        mf = meta[i];

        if( (ref != NULL) && (mf != NULL) )
        {
            if ((node->kernel->signature.directions[i] == VX_OUTPUT) &&
                (vx_true_e == ref->is_virtual))
            {
                if ((ref->scope->type == VX_TYPE_GRAPH) && (ref->scope != (vx_reference)graph))
                {
                    status = VX_ERROR_INVALID_SCOPE;
                    VX_PRINT(VX_ZONE_ERROR,"invalid scope\n");
                }

                if (VX_SUCCESS == status)
                {
                    switch (mf->type)
                    {
                        case VX_TYPE_SCALAR:
                            status = vxQueryScalar((vx_scalar)ref,
                                VX_SCALAR_TYPE, &type, sizeof(type));
                            /* For scalar, just check if type is correct or
                                not */
                            if (VX_SUCCESS == status)
                            {
                                if (type != mf->sc.type)
                                {
                                    status = VX_ERROR_INVALID_TYPE;
                                    VX_PRINT(VX_ZONE_ERROR,"invalid scalar type\n");
                                }
                            }
                            break;
                        case VX_TYPE_IMAGE:
                            if (0 == mf->img.width)
                            {
                                status = VX_ERROR_INVALID_VALUE;
                                VX_PRINT(VX_ZONE_ERROR,"image width value equal to zero\n");
                            }
                            else if (0 == mf->img.height)
                            {
                                status = VX_ERROR_INVALID_VALUE;
                                VX_PRINT(VX_ZONE_ERROR,"image height value equal to zero\n");
                            }
                            else
                            {
                                status = ownInitVirtualImage((vx_image)ref,
                                    mf->img.width, mf->img.height,
                                    mf->img.format);
                            }
                            break;
                        case VX_TYPE_ARRAY:
                            status = ownInitVirtualArray(
                                (vx_array)ref, mf->arr.item_type,
                                mf->arr.capacity);
                            break;
                        case VX_TYPE_PYRAMID:
                            pmd = (vx_pyramid)ref;

                            status = vxQueryPyramid(
                                pmd, VX_PYRAMID_LEVELS, &levels,
                                sizeof(levels));

                            if (VX_SUCCESS == status)
                            {
                                /* Levels must be same even in this case */
                                if (levels != mf->pmd.levels)
                                {
                                    status = VX_ERROR_INVALID_VALUE;
                                    VX_PRINT(VX_ZONE_ERROR,"pyramid levels incorrect\n");
                                }
                                else if (0 == mf->pmd.width)
                                {
                                    status = VX_ERROR_INVALID_VALUE;
                                    VX_PRINT(VX_ZONE_ERROR,"pyramid width equal to zero\n");
                                }
                                else if (0 == mf->pmd.height)
                                {
                                    status = VX_ERROR_INVALID_VALUE;
                                    VX_PRINT(VX_ZONE_ERROR,"pyramid height equal to zero\n");
                                }
                                else
                                {
                                    status = ownInitVirtualPyramid(pmd,
                                        mf->pmd.width, mf->pmd.height,
                                        mf->pmd.format);
                                }
                            }
                            break;
                        default:
                            break;
                    }
                }

                if (status != VX_SUCCESS)
                {
                    break;
                }
            }
        }
    }

    return (status);
}

static vx_status ownGraphNodeKernelValidate(
    vx_graph graph, vx_meta_format meta[])
{
    vx_node node;
    vx_status status = VX_SUCCESS;
    uint32_t i, j;

    for(i=0; i<graph->num_nodes; i++)
    {
        node = graph->nodes[i];

        status = ownNodeKernelValidate(node, meta);
        if(status == VX_SUCCESS)
        {
            status = ownGraphInitVirtualNode(graph, node, meta);
            if(status == VX_SUCCESS)
            {
                status = ownGraphValidRectCallback(graph, node, meta);
                if(status != VX_SUCCESS)
                {
                    VX_PRINT(VX_ZONE_ERROR,"graph valid rectangle callback failed at index %d\n", i);
                }
            }
            else
            {
                VX_PRINT(VX_ZONE_ERROR,"graph init virtual node failed at index %d\n", i);
            }
        }
        else
        {
            VX_PRINT(VX_ZONE_ERROR,"node kernel validate failed at index %d\n", i);
        }

        if(status != VX_SUCCESS)
        {
            break;
        }

        for (j = 0; j < TIVX_KERNEL_MAX_PARAMS; j ++)
        {
            meta[j]->valid_rect_callback = NULL;
        }
    }

    return status;
}

static vx_status ownGraphNodeKernelInit(vx_graph graph)
{
    vx_node node;
    vx_status status = VX_SUCCESS;
    uint32_t i;

    for(i=0; i<graph->num_nodes; i++)
    {
        node = graph->nodes[i];

        status = ownNodeKernelInit(node);
        if(status != VX_SUCCESS )
        {
            VX_PRINT(VX_ZONE_ERROR,"Node kernel init for node at index %d failed\n", i);
            break;
        }
    }

    return status;
}

static vx_status ownGraphNodeKernelDeinit(vx_graph graph)
{
    vx_node node;
    vx_status status = VX_SUCCESS;
    uint32_t i;

    for(i=0; i<graph->num_nodes; i++)
    {
        node = graph->nodes[i];

        status = ownNodeKernelDeinit(node);
        if(status != VX_SUCCESS)
        {
            VX_PRINT(VX_ZONE_ERROR,"Node kernel de-init for node at index %d failed\n", i);
            break;
        }
    }

    return status;
}

static vx_bool ownGraphIsRefMatch(vx_graph graph, vx_reference ref1, vx_reference ref2)
{
    vx_bool is_match = vx_false_e;

    if((NULL != ref1) && (NULL != ref2))
    {
        if(ref1 == ref2)
        {
            is_match = vx_true_e;
        }
        else if (((vx_reference)graph != ref2->scope) &&
            (ref1 == ref2->scope))
        {
            is_match = vx_true_e;
        }
        else
        {
            is_match = vx_false_e;
        }
    }
    return is_match;
}

static vx_status ownGraphCalcInAndOutNodes(vx_graph graph)
{
    vx_node node_cur, node_next;
    uint32_t node_cur_idx, node_next_idx;
    uint32_t prm_cur_idx, prm_next_idx;
    uint32_t prm_cur_dir, prm_next_dir;
    vx_reference ref1, ref2;
    vx_status status = VX_SUCCESS;

    for(node_cur_idx=0; node_cur_idx<graph->num_nodes; node_cur_idx++)
    {
        node_cur = graph->nodes[node_cur_idx];

        for(prm_cur_idx=0; prm_cur_idx<ownNodeGetNumParameters(node_cur); prm_cur_idx++)
        {
            prm_cur_dir = ownNodeGetParameterDir(node_cur, prm_cur_idx);

            ref1 = ownNodeGetParameterRef(node_cur, prm_cur_idx);

            if( (prm_cur_dir == VX_OUTPUT) || (prm_cur_dir == VX_BIDIRECTIONAL) )
            {
                /* for each output, see if it matches any node input data */
                for(node_next_idx=(node_cur_idx+1)%graph->num_nodes;
                    node_next_idx!=node_cur_idx;
                    node_next_idx=(node_next_idx+1)%graph->num_nodes)
                {
                    node_next = graph->nodes[node_next_idx];

                    for(prm_next_idx=0; prm_next_idx < ownNodeGetNumParameters(node_next); prm_next_idx++)
                    {
                        prm_next_dir = ownNodeGetParameterDir(node_next, prm_next_idx);

                        ref2 = ownNodeGetParameterRef(node_next, prm_next_idx);

                        if(ref2)
                        {
                            if( (prm_next_dir == VX_INPUT) || (prm_next_dir == VX_BIDIRECTIONAL) )
                            {
                                /* check if input data reference of next node is equal to
                                   output data reference of current */
                                if(ownGraphIsRefMatch(graph, ref1, ref2))
                                {
                                    /* add node_next as output node for current node if not already added */
                                    status = ownNodeAddOutNode(node_cur, node_next);

                                    if(status == VX_SUCCESS)
                                    {
                                        /* add node_current as input node for next node if not already added */
                                        status = ownNodeAddInNode(node_next, node_cur);
                                        if (status != VX_SUCCESS)
                                        {
                                            VX_PRINT(VX_ZONE_ERROR,"Add in node at index %d failed\n", node_cur_idx);
                                        }
                                    }
                                    else
                                    {
                                        VX_PRINT(VX_ZONE_ERROR,"Add out node at index %d failed\n", node_cur_idx);
                                    }
                                }
                            }
                            else
                            if( prm_next_dir == VX_OUTPUT )
                            {
                                /* check if any output of next node matches current node
                                 * This would mean two nodes output to same data object which is not allowed
                                 */
                                if(ownGraphIsRefMatch(graph, ref1, ref2))
                                {
                                    status = VX_FAILURE;
                                    VX_PRINT(VX_ZONE_ERROR,"Output of next node matches current node at index %d failed\n", node_cur_idx);
                                }
                            }
                            else
                            {
                                /* Do nothing as there is no other
                                   direction possible */
                            }
                        }
                    }
                }
            }
        }
    }

    return status;
}

static vx_status ownGraphCalcHeadAndLeafNodes(vx_graph graph)
{
    vx_node node;
    uint32_t i;
    uint32_t num_in, num_out;

    graph->num_head_nodes = 0;
    graph->num_leaf_nodes = 0;

    for(i=0; i<graph->num_nodes; i++)
    {
        node = graph->nodes[i];

        num_in = ownNodeGetNumInNodes(node);
        num_out = ownNodeGetNumOutNodes(node);

        if(num_in==0)
        {
            graph->head_nodes[graph->num_head_nodes] = node;
            graph->num_head_nodes++;
        }
        if(num_out==0)
        {
            graph->leaf_nodes[graph->num_leaf_nodes] = node;
            graph->num_leaf_nodes++;
        }
    }

    return VX_SUCCESS;
}

static vx_status ownGraphAllocateDataObjects(vx_graph graph)
{
    vx_node node_cur;
    uint32_t node_cur_idx;
    uint32_t prm_cur_idx;
    vx_status status = VX_SUCCESS;
    vx_reference ref;

    for(node_cur_idx=0; node_cur_idx<graph->num_nodes; node_cur_idx++)
    {
        node_cur = graph->nodes[node_cur_idx];

        for(prm_cur_idx=0; prm_cur_idx<ownNodeGetNumParameters(node_cur); prm_cur_idx++)
        {
            ref = ownNodeGetParameterRef(node_cur, prm_cur_idx);

            /* reference could be null for optional parameters */
            if (NULL != ref)
            {
                if(ownNodeIsPrmReplicated(node_cur, prm_cur_idx))
                {
                    /* if this is a replicated node, replicated parameter
                     * then allocate memory for parent object
                     */
                    status = ownReferenceAllocMem(ref->scope);
                    if (status != VX_SUCCESS)
                    {
                        VX_PRINT(VX_ZONE_ERROR,"Memory allocation for replicated parameter parent object at index %d failed\n", node_cur_idx);
                    }
                }
                else
                if(ref->delay != NULL )
                {
                    /* if this is part of delay then allocate memory for all
                     * delay objects
                     */
                    status = ownReferenceAllocMem((vx_reference)ref->delay);
                    if (status != VX_SUCCESS)
                    {
                        VX_PRINT(VX_ZONE_ERROR,"Memory allocation for delay objects at index %d failed\n", node_cur_idx);
                    }
                }
                else
                {
                    /* alloc memory for data reference, if not already allocated
                     * Its ok to call this multiple times for the same reference
                     * memory gets allocated only once
                     */
                    status = ownReferenceAllocMem(ref);
                    if (status != VX_SUCCESS)
                    {
                        VX_PRINT(VX_ZONE_ERROR,"Memory allocation for data reference at index %d failed\n", node_cur_idx);
                    }
                }

                if(status != VX_SUCCESS)
                {
                    break;
                }
            }
        }
        if(status != VX_SUCCESS)
        {
            break;
        }
    }
    return status;
}

static vx_status ownGraphNodeCreateCompletionEvents(vx_graph graph)
{
    vx_node node;
    uint32_t i;
    vx_status status = VX_SUCCESS;

    for(i=0; i<graph->num_leaf_nodes; i++)
    {
        node = graph->leaf_nodes[i];

        status = ownNodeCreateCompletionEvent(node);
        if(status != VX_SUCCESS)
        {
            VX_PRINT(VX_ZONE_ERROR,"Node create completion event at index %d failed\n", i);
            break;
        }
    }

    return status;
}

static vx_status ownGraphCreateNodeCallbackCommands(vx_graph graph)
{
    vx_node node;
    uint32_t i;
    vx_status status = VX_SUCCESS;

    for(i=0; i<graph->num_nodes; i++)
    {
        node = graph->nodes[i];

        status = ownNodeCreateUserCallbackCommand(node);
        if(status != VX_SUCCESS)
        {
            VX_PRINT(VX_ZONE_ERROR,"Node create user callback command at index %d failed\n", i);
            break;
        }
    }

    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxVerifyGraph(vx_graph graph)
{
    uint32_t i;
    vx_status status = VX_SUCCESS;
    vx_meta_format meta[TIVX_KERNEL_MAX_PARAMS] = {NULL};
    vx_bool first_time_verify = vx_true_e;

    if (NULL != graph)
    {
        first_time_verify = ((graph->verified == vx_false_e) &&
            (graph->reverify == vx_false_e)) ? vx_true_e : vx_false_e;
    }
    else
    {
        VX_PRINT(VX_ZONE_ERROR,"Invalid graph reference\n");
        status = VX_ERROR_INVALID_PARAMETERS;
    }

    for (i = 0; (i < TIVX_KERNEL_MAX_PARAMS) && (VX_SUCCESS == status); i ++)
    {
        meta[i] = vxCreateMetaFormat(graph->base.context);

        /* This should not fail at all */
        if (vxGetStatus((vx_reference)meta[i]) != VX_SUCCESS)
        {
            VX_PRINT(VX_ZONE_ERROR,"Unable to create meta format object\n");
            status = VX_ERROR_NO_RESOURCES;
        }
    }

    if((graph) && (ownIsValidSpecificReference((vx_reference)graph, VX_TYPE_GRAPH)) &&
        (VX_SUCCESS == status))
    {
        ownReferenceLock(&graph->base);

        graph->verified = vx_false_e;

        if(first_time_verify == vx_false_e)
        {
            ownGraphNodeKernelDeinit(graph);
        }
        {
            if(status == VX_SUCCESS)
            {
                /* Find out nodes and in nodes for each node in the graph
                 * No resources are allcoated in this step
                 */
                status = ownGraphCalcInAndOutNodes(graph);
                if(status != VX_SUCCESS)
                {
                    VX_PRINT(VX_ZONE_ERROR,"Unable to calculate out nodes and in nodes for each node\n");
                }
            }


            if(status == VX_SUCCESS)
            {
                vx_bool has_cycle;

                ownContextLock(graph->base.context);

                /* Topological sort graph to find cycles
                 */
                ownGraphTopologicalSort(
                            &graph->base.context->graph_sort_context,
                            graph->nodes,
                            graph->num_nodes,
                            &has_cycle);

                ownContextUnlock(graph->base.context);

                if(has_cycle)
                {
                    VX_PRINT(VX_ZONE_ERROR,"Topological sort failed, due to cycles in graph\n");
                    status = VX_FAILURE;
                }
            }

            if(status == VX_SUCCESS)
            {
                /* Call validate function for each node
                 * If validation fails then return with error
                 * No resources are allcoated in this step
                 */
                status = ownGraphNodeKernelValidate(graph, meta);

                if(status != VX_SUCCESS)
                {
                    VX_PRINT(VX_ZONE_ERROR,"Node kernel Validate failed\n");
                }
            }

            if(status == VX_SUCCESS)
            {
                /* Find head nodes and leaf nodes
                 * in graph
                 * No resources are allcoated in this step
                 */
                status = ownGraphCalcHeadAndLeafNodes(graph);
                if(status != VX_SUCCESS)
                {
                    VX_PRINT(VX_ZONE_ERROR,"Find head nodes and leaf nodes failed\n");
                }
            }

            if(status == VX_SUCCESS)
            {
                /* Allocate memory associated with data objects of this graph
                 * Memory resources are allcoated in this step
                 * No need to free them in case of error, since they get free'ed during
                 * data object release
                 */
                status = ownGraphAllocateDataObjects(graph);
                if(status != VX_SUCCESS)
                {
                    VX_PRINT(VX_ZONE_ERROR,"Memory alloc for data objects failed\n");
                }
            }

            if(status == VX_SUCCESS)
            {
                /* Set completion event's with leaf nodes
                 * If case of any error these events gets free'ed during
                 * graph release
                 */
                status = ownGraphNodeCreateCompletionEvents(graph);
                if(status != VX_SUCCESS)
                {
                    VX_PRINT(VX_ZONE_ERROR,"Create completion events for leaf nodes failed\n");
                }
            }

            if(status == VX_SUCCESS)
            {
                /* Set node callback commands
                 * If case of any error these command are free'ed during
                 * graph release
                 */
                status = ownGraphCreateNodeCallbackCommands(graph);
                if(status != VX_SUCCESS)
                {
                    VX_PRINT(VX_ZONE_ERROR,"Create node callback commands failed\n");
                }
            }

            if(status == VX_SUCCESS)
            {
                /* Call target kernel init for each node
                 * This results in message coomunication with target kernels
                 * Memory gets allocated which MUST be free'ed via target kernel
                 * deinit.
                 * kernel deinit called during node release
                 */
                status = ownGraphNodeKernelInit(graph);
                if(status != VX_SUCCESS)
                {
                    VX_PRINT(VX_ZONE_ERROR,"Node kernel init failed\n");
                }
            }

            if(status == VX_SUCCESS)
            {
                /* everything passed, now graph is verified */
                graph->verified = vx_true_e;
                graph->reverify = vx_false_e;
                graph->state = VX_GRAPH_STATE_VERIFIED;
            }

            if(status != VX_SUCCESS)
            {
                if(status != VX_SUCCESS)
                {
                    VX_PRINT(VX_ZONE_ERROR,"Graph verify failed\n");
                }
                /* deinit kernel to recover resources */
                ownGraphNodeKernelDeinit(graph);
            }
        }

        ownReferenceUnlock(&graph->base);
    }
    else
    {
        VX_PRINT(VX_ZONE_ERROR,"Invalid graph reference\n");
        status = VX_ERROR_INVALID_REFERENCE;
    }

    for (i = 0; i < TIVX_KERNEL_MAX_PARAMS; i ++)
    {
        if (NULL != meta[i])
        {
            vxReleaseMetaFormat(&meta[i]);
        }
    }

    return status;
}
