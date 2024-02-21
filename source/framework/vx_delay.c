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



#include <vx_internal.h>

#define tivxIsValidDelay(d) (ownIsValidSpecificReference((vx_reference)(d), (vx_enum)VX_TYPE_DELAY) == (vx_bool)vx_true_e)
#define tivxIsValidGraph(g) (ownIsValidSpecificReference((vx_reference)(g), (vx_enum)VX_TYPE_GRAPH) == (vx_bool)vx_true_e)

static vx_bool ownIsValidObject(vx_enum type);
static void ownResetDelayPrmPool(vx_delay delay);
static tivx_delay_param_t *ownAllocDelayPrm(vx_delay delay);
static void ownFreeDelayPrm(tivx_delay_param_t *prm);
static vx_status ownAddRefToDelay(vx_delay delay, vx_reference ref, uint32_t i);
static void ownReleaseRefFromDelay(vx_delay delay, uint32_t num_items);
static vx_status ownDestructDelay(vx_reference ref);
static vx_status ownAllocDelayBuffer(vx_reference delay_ref);
static void ownDelayInit(vx_delay delay, vx_size count, vx_enum type);
static vx_status ownAgeDelay(vx_delay delay);


static vx_bool ownIsValidObject(vx_enum type)
{
    vx_bool status = (vx_bool)vx_false_e;

    if (((vx_enum)VX_TYPE_IMAGE == type) ||
        ((vx_enum)VX_TYPE_ARRAY == type) ||
        ((vx_enum)VX_TYPE_SCALAR == type) ||
        ((vx_enum)VX_TYPE_DISTRIBUTION == type) ||
        ((vx_enum)VX_TYPE_THRESHOLD == type) ||
        ((vx_enum)VX_TYPE_PYRAMID == type) ||
        ((vx_enum)VX_TYPE_MATRIX == type) ||
        ((vx_enum)VX_TYPE_REMAP == type)  ||
        ((vx_enum)VX_TYPE_LUT == type) ||
        ((vx_enum)VX_TYPE_OBJECT_ARRAY == type) ||
        ((vx_enum)VX_TYPE_CONVOLUTION == type) ||
        (VX_TYPE_USER_DATA_OBJECT == type) ||
        (TIVX_TYPE_RAW_IMAGE == type)
        )
    {
        status = (vx_bool)vx_true_e;
    }

    return (status);
}

static void ownResetDelayPrmPool(vx_delay delay)
{
    vx_uint32 i;
    for(i=0; i<TIVX_DELAY_MAX_PRM_OBJECT; i++)
    {
        delay->prm_pool[i].next = NULL;
        delay->prm_pool[i].node = NULL;
        delay->prm_pool[i].index = 0;
    }
}

static tivx_delay_param_t *ownAllocDelayPrm(vx_delay delay)
{
    tivx_delay_param_t *prm = NULL;
    vx_uint32 i;

    for(i=0; i<TIVX_DELAY_MAX_PRM_OBJECT; i++)
    {
        if(delay->prm_pool[i].node==NULL)
        {
            prm = &delay->prm_pool[i];
            prm->next = NULL;
            prm->index = 0;
            ownLogSetResourceUsedValue("TIVX_DELAY_MAX_PRM_OBJECT", (uint16_t)i+1U);
            break;
        }
    }

    if (prm == NULL)
    {
        VX_PRINT(VX_ZONE_ERROR, "May need to increase the value of TIVX_DELAY_MAX_PRM_OBJECT in tiovx/include/TI/tivx_config.h\n");
    }

    return prm;
}

static void ownFreeDelayPrm(tivx_delay_param_t *prm)
{
    prm->node = NULL;
    prm->next = NULL;
    prm->index = 0;
}

vx_bool ownAddAssociationToDelay(vx_reference value,
                                vx_node n, vx_uint32 i)
{
    vx_delay delay = value->delay;
    vx_int32 delay_index = value->delay_slot_index;
    vx_bool status = (vx_bool)vx_true_e;

    vx_int32 idx = ((vx_int32)delay->index + (vx_int32)abs(delay_index)) % (vx_int32)delay->count;

    if (delay->set[idx].node == NULL) /* head is empty */
    {
        delay->set[idx].node = n;
        delay->set[idx].index = i;
    }
    else
    {
        tivx_delay_param_t **ptr = &delay->set[idx].next;
        for(;;)
        {
            if (*ptr == NULL)
            {
                *ptr = ownAllocDelayPrm(delay);
                if (*ptr != NULL)
                {
                    (*ptr)->node = n;
                    (*ptr)->index = i;
                }
                else
                {
                    status = (vx_bool)vx_false_e;
                }
                break;
            }
#ifdef LDRA_UNTESTABLE_CODE
            else
            {
                ptr = &((*ptr)->next);
            }
#endif
        } 
    }

    if(status == (vx_bool)vx_true_e)
    {
        /* Increment a reference to the delay */
        (void)ownIncrementReference((vx_reference)delay, (vx_enum)VX_INTERNAL);
    }

    return status;
}


vx_bool ownRemoveAssociationToDelay(vx_reference value,
                                   vx_node n, vx_uint32 i)
{
    vx_delay delay = value->delay;
    vx_int32 delay_index = value->delay_slot_index;
    vx_bool do_break;

    vx_int32 idx = delay_index;
    vx_bool check_status = (vx_bool)vx_true_e;
    vx_status status = (vx_status)VX_SUCCESS;

    if ( (delay->set[idx].node == n) && (delay->set[idx].index == i) ) /* head is a match */
    {
        delay->set[idx].node = NULL;
        delay->set[idx].index = 0;
    }
    else
    {
        tivx_delay_param_t **ptr = &delay->set[idx].next;
        tivx_delay_param_t *next = NULL;
        do_break = (vx_bool)vx_false_e;
        for(;;)
        {
            if (*ptr != NULL)
            {
                if ( ((*ptr)->node == n) && ((*ptr)->index == i) )
                {
                    next = (*ptr)->next;
                    ownFreeDelayPrm(*ptr);
                    *ptr = next;
                    do_break = (vx_bool)vx_true_e;
                }
#ifdef LDRA_UNTESTABLE_CODE
                else
                {
                   ptr = &((*ptr)->next);
                }
#endif
            }
            else
            {
                check_status = (vx_bool)vx_false_e;
                do_break = (vx_bool)vx_true_e;
            }

            if ((vx_bool)vx_true_e == do_break)
            {
                break;
            }
        } 
    }


    if (check_status == (vx_bool)vx_true_e) /* Release the delay */
    {
        vx_reference ref=(vx_reference)delay;
        status = ownReleaseReferenceInt(&ref, (vx_enum)VX_TYPE_DELAY, (vx_enum)VX_INTERNAL, NULL);
        if((vx_status)VX_SUCCESS != status)
        {
            VX_PRINT(VX_ZONE_ERROR,"Failed to destroy delay reference\n");
            check_status = (vx_bool)vx_false_e;
        }
    }
    return check_status;
}

static vx_status ownAddRefToDelay(vx_delay delay, vx_reference ref, uint32_t i)
{
    vx_status status = (vx_status)VX_SUCCESS;

    delay->refs[i] = ref;

    /* set the object as a delay element */
    ownInitReferenceForDelay(ref, delay, (vx_int32)i);

    /* increment the internal counter on the image, not the
       external one */
    (void)ownIncrementReference(ref, (vx_enum)VX_INTERNAL);

    return status;
}

static void ownReleaseRefFromDelay(vx_delay delay, uint32_t num_items)
{
    uint32_t i;
    vx_status status;
    if (tivxIsValidDelay(delay) && (delay->type == (vx_enum)VX_TYPE_PYRAMID) && (delay->pyr_num_levels > 0U) )
    {
        /* release pyramid delays */
        for (i = 0; i < delay->pyr_num_levels; i++)
        {
            status = ownReleaseReferenceInt((vx_reference *)&(delay->pyr_delay[i]), (vx_enum)VX_TYPE_DELAY, (vx_enum)VX_INTERNAL, NULL);
            if((vx_status)VX_SUCCESS != status)
            {
                VX_PRINT(VX_ZONE_ERROR,"Failed to destroy pyramid delay reference\n");
            }  
        }
        delay->pyr_num_levels = 0;
    }

    if (tivxIsValidDelay(delay) && (delay->type == (vx_enum)VX_TYPE_OBJECT_ARRAY) && (delay->obj_arr_num_items > 0U) )
    {
        /* release object array delays */
        for (i = 0; i < delay->obj_arr_num_items; i++)
        {
            status = ownReleaseReferenceInt((vx_reference *)&(delay->obj_arr_delay[i]), (vx_enum)VX_TYPE_DELAY, (vx_enum)VX_INTERNAL, NULL);
            if((vx_status)VX_SUCCESS != status)
            {
                VX_PRINT(VX_ZONE_ERROR,"Failed to destroy object array reference\n");
            } 
        }
        delay->obj_arr_num_items = 0;
    }

    for (i = 0; i < num_items; i ++)
    {
        if (NULL != delay->refs[i])
        {
            /* decrement the internal counter on the image, not the
               external one */
            (void)ownDecrementReference(delay->refs[i], (vx_enum)VX_INTERNAL);

            status =  vxReleaseReference(&delay->refs[i]);
            if((vx_status)VX_SUCCESS != status)
            {
                VX_PRINT(VX_ZONE_ERROR,"Failed to release delay reference \n");
            }
        }
    }
}

static vx_status ownDestructDelay(vx_reference ref)
{
    vx_delay delay = (vx_delay)ref;

#ifdef LDRA_UNTESTABLE_CODE
    if(delay->base.type == (vx_enum)VX_TYPE_DELAY)
#endif
    {
        ownReleaseRefFromDelay(delay, delay->count);
    }
    return (vx_status)VX_SUCCESS;
}

static vx_status ownAllocDelayBuffer(vx_reference delay_ref)
{
    vx_status status = (vx_status)VX_SUCCESS;
    vx_uint32 i=0;
    vx_delay delay = (vx_delay)delay_ref;
    vx_reference ref;

#ifdef LDRA_UNTESTABLE_CODE
    if(delay->base.type == (vx_enum)VX_TYPE_DELAY)
#endif
    {
        for (i = 0u; i < delay->count; i++)
        {
            ref = delay->refs[i];

#ifdef LDRA_UNTESTABLE_CODE
            if (ref != NULL)
#endif
            {
                status = (vx_status)VX_SUCCESS;
                if(ref->mem_alloc_callback != NULL)
                {
                    status = ref->mem_alloc_callback(ref);
                }

                if ((vx_status)VX_SUCCESS != status)
                {
                    break;
                }
            }
#ifdef LDRA_UNTESTABLE_CODE
            else
            {
                VX_PRINT(VX_ZONE_ERROR, "delay reference %d is null\n", i);
                status = (vx_status)VX_ERROR_INVALID_VALUE;
            }
#endif
        }
    }
#ifdef LDRA_UNTESTABLE_CODE
    else
    {
        VX_PRINT(VX_ZONE_ERROR, "reference type is not delay\n");
        status = (vx_status)VX_ERROR_INVALID_REFERENCE;
    }
#endif

    return status;
}

static void ownDelayInit(vx_delay delay, vx_size count, vx_enum type)
{
    vx_size i;

    delay->type = type;
    delay->count = (uint32_t)count;
    delay->pyr_num_levels = 0;
    delay->obj_arr_num_items = 0;
    delay->index = 0;
    ownResetDelayPrmPool(delay);
    for(i=0; i<count; i++)
    {
        delay->set[i].next = NULL;
        delay->set[i].node = NULL;
        delay->set[i].index = 0;
    }

    /* assign refernce type specific callback's */
    delay->base.destructor_callback = &ownDestructDelay;
    delay->base.mem_alloc_callback = &ownAllocDelayBuffer;
    delay->base.release_callback =
        &ownReleaseReferenceBufferGeneric;
}

VX_API_ENTRY vx_delay VX_API_CALL vxCreateDelay(vx_context context,
                              vx_reference exemplar,
                              vx_size count)
{
    vx_status status = (vx_status)VX_SUCCESS;
    vx_delay delay = NULL;
    vx_reference ref;
    vx_uint32 i;

    if ( ownIsValidContext(context) != 0 )
    {
        if (count <= TIVX_DELAY_MAX_OBJECT)
        {
            if ((ownIsValidReference(exemplar) != (vx_bool)vx_false_e) && (ownIsValidObject(exemplar->type) != (vx_bool)vx_false_e))
            {
                delay = (vx_delay)ownCreateReference(
                                        context, (vx_enum)VX_TYPE_DELAY, (vx_enum)VX_EXTERNAL, &context->base);
                if ( (vxGetStatus((vx_reference)delay) == (vx_status)VX_SUCCESS) && (delay->base.type == (vx_enum)VX_TYPE_DELAY) )
                {
                    ownDelayInit(delay, count, exemplar->type);

                    for(i=0; i<count; i++)
                    {
                        ref = tivxCreateReferenceFromExemplar(context, exemplar);

                        status = vxGetStatus(ref);

                        if(status == (vx_status)VX_SUCCESS)
                        {
                            status = ownAddRefToDelay(delay, ref, i);
                        }
                        else
                        {
                            VX_PRINT(VX_ZONE_ERROR,"tivxCreateReferenceFromExemplar Failed\n");
                            break;
                        }
                    }
                    if(status == (vx_status)VX_SUCCESS)
                    {
                        ownLogSetResourceUsedValue("TIVX_DELAY_MAX_OBJECT", (uint16_t)count);
                    }
                    if ( (status == (vx_status)VX_SUCCESS) && (exemplar->type == (vx_enum)VX_TYPE_OBJECT_ARRAY) )
                    {
                        vx_size num_items, item_idx;
                        vx_enum item_type;
                        vx_delay objarrdelay;

                        status = vxQueryObjectArray((vx_object_array)exemplar, (vx_enum)VX_OBJECT_ARRAY_NUMITEMS, &num_items, sizeof(num_items));
                        delay->obj_arr_num_items = 0;
                        if(status == (vx_status)VX_SUCCESS)
                        {
                            status = vxQueryObjectArray((vx_object_array)exemplar, (vx_enum)VX_OBJECT_ARRAY_ITEMTYPE, &item_type, sizeof(item_type));
                            if ((vx_status)VX_SUCCESS == status)
                            {
                                for (item_idx = 0; item_idx < num_items; item_idx++)
                                {
                                    objarrdelay = (vx_delay)ownCreateReference(context, (vx_enum)VX_TYPE_DELAY, (vx_enum)VX_INTERNAL, (vx_reference)delay);
                                    delay->obj_arr_delay[item_idx] = objarrdelay;
                                    if ( (vxGetStatus((vx_reference)objarrdelay) == (vx_status)VX_SUCCESS) && (objarrdelay->base.type == (vx_enum)VX_TYPE_DELAY) )
                                    {
                                        ownDelayInit(objarrdelay, count, item_type);
                                        for (i = 0; i < count; i++)
                                        {
                                            ref = (vx_reference)vxGetObjectArrayItem((vx_object_array)delay->refs[i], (vx_uint32)item_idx);

                                            if (NULL != ref)
                                            {
                                                status = ownAddRefToDelay(objarrdelay, ref, i);
                                            }
                                            /* removed the status check as the status would be 
                                             * always success from the above conditions 
                                             */
                                            else 
                                            {
                                                VX_PRINT(VX_ZONE_ERROR, "reference was not added to delay\n");
                                                break;
                                            }
                                        }
                                        delay->obj_arr_num_items++;
                                    }
                                }
                            }
                        }
                    }
                    if ( (status == (vx_status)VX_SUCCESS) && (exemplar->type == (vx_enum)VX_TYPE_PYRAMID) )
                    {
                        vx_size levels, level_idx;
                        vx_delay pyrdelay;

                        status = vxQueryPyramid((vx_pyramid)exemplar, (vx_enum)VX_PYRAMID_LEVELS, &levels, sizeof(levels));
                        delay->pyr_num_levels = 0;
                        if(status == (vx_status)VX_SUCCESS)
                        {
                            for (level_idx = 0; level_idx < levels; level_idx++)
                            {
                                pyrdelay = (vx_delay)ownCreateReference(context, (vx_enum)VX_TYPE_DELAY, (vx_enum)VX_INTERNAL, (vx_reference)delay);
                                delay->pyr_delay[level_idx] = pyrdelay;
                                if ( (vxGetStatus((vx_reference)pyrdelay) == (vx_status)VX_SUCCESS) && (pyrdelay->base.type == (vx_enum)VX_TYPE_DELAY) )
                                {
                                    ownDelayInit(pyrdelay, count, (vx_enum)VX_TYPE_IMAGE);
                                    for (i = 0; i < count; i++)
                                    {
                                        ref = (vx_reference)vxGetPyramidLevel((vx_pyramid)delay->refs[i], (vx_uint32)level_idx);
                                        /* removed status always returning success */
                                        (void)ownAddRefToDelay(pyrdelay, ref, i);
                                    }
                                    delay->pyr_num_levels++;
                                }
                            }
                        }
                    }
                    if(status!=(vx_status)VX_SUCCESS)
                    {
                        ownReleaseRefFromDelay(delay, i);
                        status = vxReleaseDelay(&delay);
                        if((vx_status)VX_SUCCESS != status)
                        {
                            VX_PRINT(VX_ZONE_ERROR,"Failed to release a reference to delay object\n");
                        }

                        VX_PRINT(VX_ZONE_ERROR, "Could not allocate delay object descriptor\n");
                        vxAddLogEntry(&context->base, (vx_status)VX_ERROR_NO_RESOURCES,
                            "Could not allocate delay object descriptor\n");
                        delay = (vx_delay)ownGetErrorObject(
                            context, (vx_status)VX_ERROR_NO_RESOURCES);
                    }
                }
                else
                {
                    VX_PRINT(VX_ZONE_ERROR, "Could not create delay reference\n");
                    delay = (vx_delay)ownGetErrorObject(context, (vx_status)VX_ERROR_NO_RESOURCES);
                }
            }
            else
            {
                VX_PRINT(VX_ZONE_ERROR, "invalid reference or reference type\n");
                delay = (vx_delay)ownGetErrorObject(context, (vx_status)VX_ERROR_INVALID_REFERENCE);
            }
        }
        else
        {
            VX_PRINT(VX_ZONE_ERROR, "count > TIVX_DELAY_MAX_OBJECT\n");
            VX_PRINT(VX_ZONE_ERROR, "May need to increase the value of TIVX_DELAY_MAX_OBJECT in tiovx/include/TI/tivx_config.h\n");
            delay = (vx_delay)ownGetErrorObject(context, (vx_status)VX_ERROR_NO_RESOURCES);
        }
    }

    return delay;
}

VX_API_ENTRY vx_reference VX_API_CALL vxGetReferenceFromDelay(
    vx_delay delay, vx_int32 index)
{
    vx_reference ref = NULL;
    if (tivxIsValidDelay(delay))
    {
        if ((vx_uint32)abs(index) < delay->count)
        {
            vx_int32 i = ((vx_int32)delay->index + (vx_int32)abs(index)) % (vx_int32)delay->count;
            ref = delay->refs[i];
        }
        else
        {
            vxAddLogEntry(&delay->base, (vx_status)VX_ERROR_INVALID_PARAMETERS, "Failed to retrieve reference from delay by index %d\n", index);
            ref = (vx_reference)ownGetErrorObject(delay->base.context, (vx_status)VX_ERROR_INVALID_PARAMETERS);
        }
    }
    return ref;
}

VX_API_ENTRY vx_status VX_API_CALL vxQueryDelay(vx_delay delay,
    vx_enum attribute, void *ptr, vx_size size)
{
    vx_status status = (vx_status)VX_SUCCESS;

    if (tivxIsValidDelay(delay))
    {
        switch (attribute)
        {
            case (vx_enum)VX_DELAY_TYPE:
                if (VX_CHECK_PARAM(ptr, size, vx_size, 0x3U))
                {
                    *(vx_enum *)ptr = delay->type;
                }
                else
                {
                    VX_PRINT(VX_ZONE_ERROR, "delay type query failed\n");
                    status = (vx_status)VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case (vx_enum)VX_DELAY_SLOTS:
                if (VX_CHECK_PARAM(ptr, size, vx_size, 0x3U))
                {
                    *(vx_size *)ptr = (vx_size)delay->count;
                }
                else
                {
                    VX_PRINT(VX_ZONE_ERROR, "delay slots query failed\n");
                    status = (vx_status)VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            default:
                VX_PRINT(VX_ZONE_ERROR, "invalid attribute\n");
                status = (vx_status)VX_ERROR_NOT_SUPPORTED;
                break;
        }
    }
    else
    {
        VX_PRINT(VX_ZONE_ERROR, "invalid delay\n");
        status = (vx_status)VX_ERROR_INVALID_REFERENCE;
    }

    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxReleaseDelay(vx_delay *d)
{
    return ownReleaseReferenceInt((vx_reference *)d, (vx_enum)VX_TYPE_DELAY, (vx_enum)VX_EXTERNAL, NULL);
}

VX_API_ENTRY vx_status VX_API_CALL vxAgeDelay(vx_delay delay)
{
    vx_status status = (vx_status)VX_SUCCESS;

    if (tivxIsValidDelay(delay))
    {
        vx_int32 i;

        status = ownAgeDelay(delay);

        switch (delay->type)
        {
            case (vx_enum)VX_TYPE_PYRAMID:
                if (delay->pyr_num_levels > 0U)
                {
                    /* age pyramid levels */
                    vx_int32 numLevels = (vx_int32)delay->pyr_num_levels;
                    for (i = 0; i < numLevels; ++i)
                    {
                        status = ownAgeDelay(delay->pyr_delay[i]);
                    }
                }
                break;

            case (vx_enum)VX_TYPE_OBJECT_ARRAY:
                if (delay->obj_arr_num_items > 0U)
                {
                     /* age object array levels */
                    vx_int32 numLevels = (vx_int32)delay->obj_arr_num_items;
                    for (i = 0; i < numLevels; ++i)
                    {
                        /* Note: Given that object arrays cannot have object array
                         * exemplars, this aging process is valid */
                        status = ownAgeDelay(delay->obj_arr_delay[i]);
                    }
                }
                break;

            default:
                break;

        }
    }
    else
    {
        VX_PRINT(VX_ZONE_ERROR, "invalid delay\n");
        status = (vx_status)VX_ERROR_INVALID_REFERENCE;
    }
    return status;
}

static vx_status ownAgeDelay(vx_delay delay)
{
    vx_status status = (vx_status)VX_SUCCESS;

    if (tivxIsValidDelay(delay))
    {
        vx_int32 i,j;

        /* increment the index */
        delay->index = (delay->index + 1U) % (vx_uint32)delay->count;

        /* then reassign the parameters */
        for (i = 0; i < (vx_int32)delay->count; i++)
        {
            tivx_delay_param_t *param = NULL;

            j = ((vx_int32)delay->index + i) % (vx_int32)delay->count;
            param = &delay->set[i];
            do
            {
                if (param->node != NULL)
                {
                    status = ownNodeSetParameter(param->node,param->index,delay->refs[j]);
                    if ((vx_status)VX_SUCCESS != status)
                    {
                        VX_PRINT(VX_ZONE_ERROR, "Failed to set parameter at node \n");
                    }

                    ownInitReferenceForDelay(delay->refs[j], delay, i);
                }
                param = param->next;
            } while (param != NULL);
        }
    }
    else
    {
        VX_PRINT(VX_ZONE_ERROR, "invalid delay\n");
        status = (vx_status)VX_ERROR_INVALID_REFERENCE;
    }
    return status;
}

