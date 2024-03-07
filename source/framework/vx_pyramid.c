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
#include <math.h>

static vx_status ownDestructPyramid(vx_reference ref);
static vx_status ownAllocPyramidBuffer(vx_reference ref);
static vx_status ownInitPyramid(vx_pyramid prmd);
static vx_status isPyramidCopyable(vx_pyramid input, vx_pyramid output);
static vx_status VX_CALLBACK pyramidKernelCallback(vx_enum kernel_enum, vx_bool validate_only, vx_enum optimization, const vx_reference params[], vx_uint32 num_params);

static const vx_float32 gOrbScaleFactor
    [TIVX_PYRAMID_MAX_LEVELS_ORB] =
{
    1.0f,
    0.8408964152537146f,
    0.7071067811865476f,
    0.5946035575013605f,
    0.5f,
    0.4204482076268573f,
    0.3535533905932737f,
    0.2973017787506803f,
    0.25f,
    0.2102241038134287f,
    0.1767766952966369f,
    0.1486508893753401f,
    0.125f,
    0.1051120519067143f,
    0.08838834764831843f,
    0.07432544468767006f,
    0.0625f
};

/*! \brief check to see if input pyramid can be copied to output.
 * Number of levels and scale must always be equal; for non-virtual
 * output the width, height and format must also agree.
 * Same function is used to validate for swapping.
*/
static vx_status isPyramidCopyable(vx_pyramid input, vx_pyramid output)
{
    vx_status status = (vx_status)VX_SUCCESS;
    tivx_obj_desc_pyramid_t *in_objd = (tivx_obj_desc_pyramid_t *)input->base.obj_desc;
    tivx_obj_desc_pyramid_t *out_objd = (tivx_obj_desc_pyramid_t *)output->base.obj_desc;
    if ((vx_false_e == ownIsValidSpecificReference(&input->base, (vx_enum)VX_TYPE_PYRAMID)) ||
        (vx_false_e == ownIsValidSpecificReference(&output->base, (vx_enum)VX_TYPE_PYRAMID)) ||
        (input == output))
    {
        status = (vx_status)VX_ERROR_NOT_COMPATIBLE;
    }
    else if ((in_objd->num_levels != out_objd->num_levels) ||
             (in_objd->scale != out_objd->scale))
    {
        status = (vx_status)VX_ERROR_NOT_COMPATIBLE;
    }
    else if ((in_objd->width != out_objd->width) &&
        ((0 != out_objd->width) ||
         (vx_false_e == output->base.is_virtual)))
    {
        /* output width must be the same as input width unless output is virtual with width zero */
        status = (vx_status)VX_ERROR_NOT_COMPATIBLE;
    }
    else if ((in_objd->height != out_objd->height) &&
             ((0 != out_objd->height) ||
              (vx_false_e == output->base.is_virtual)))
    {
        /* output height must be the same as input height unless output is virtual with height zero */
        status = (vx_status)VX_ERROR_NOT_COMPATIBLE;
    }
    else if ((in_objd->format != out_objd->format) &&
             ((VX_DF_IMAGE_VIRT != out_objd->format) ||
              (vx_false_e == output->base.is_virtual)))
    {
        /* Output format must be the same as input format unless output is virtual with virtual format */
        status = (vx_status)VX_ERROR_NOT_COMPATIBLE;
    }
    else
    {
        /* All OK, so we propagate metadata */
        out_objd->format = in_objd->format;
        out_objd->height = in_objd->height;
        out_objd->width = in_objd->width;
        /* Q: do we have to do anything for the sub-images?
              elsewhere in the framework this is not done for pyramid
              metadata, but what about the valid region?
              it seems like an omission, but probably
              beyond the scope at present...*/
    }
    return status;
}

static vx_status VX_CALLBACK pyramidKernelCallback(vx_enum kernel_enum, vx_bool validate_only, vx_enum optimization, const vx_reference params[], vx_uint32 num_params)
{
    vx_status status = (vx_status)VX_SUCCESS;
    if (validate_only)
    {
        return isPyramidCopyable((vx_pyramid)params[0], (vx_pyramid)params[1]);
    }
    else    /* dispatch to each sub-image in turn */
    {
        vx_uint32 lvl;
        for (lvl = 0; lvl < ((tivx_obj_desc_pyramid_t *)params[0]->obj_desc)->num_levels && VX_SUCCESS == status; ++lvl)
        {
            vx_reference p2[2] = {&((vx_pyramid)params[0])->img[lvl]->base, &((vx_pyramid)params[1])->img[lvl]->base};
            vx_kernel_callback_rb_f kf = p2[0]->kernel_callback;
            if (kf)
            {
                status = (*kf)(kernel_enum, vx_false_e, optimization, p2, 2);
            }
            else
            {
                status = (vx_status)VX_ERROR_NOT_SUPPORTED;
            }
        }
    }
    return status;
}


VX_API_ENTRY vx_status VX_API_CALL vxReleasePyramid(vx_pyramid *prmd)
{
    return (ownReleaseReferenceInt(
        (vx_reference*)prmd, (vx_enum)VX_TYPE_PYRAMID, (vx_enum)VX_EXTERNAL, NULL));
}

vx_pyramid VX_API_CALL vxCreatePyramid(
    vx_context context, vx_size levels, vx_float32 scale, vx_uint32 width,
    vx_uint32 height, vx_df_image format)
{
    vx_status status = (vx_status)VX_SUCCESS;
    vx_pyramid prmd = NULL;
    vx_uint32 i;
    tivx_obj_desc_pyramid_t *obj_desc = NULL;

    if(ownIsValidContext(context) == (vx_bool)vx_true_e)
    {
        if (width == 0U)
        {
            VX_PRINT(VX_ZONE_ERROR, "Width is equal to 0\n");
            status = (vx_status)VX_FAILURE;
        }
        if (height == 0U)
        {
            VX_PRINT(VX_ZONE_ERROR, "Height is equal to 0\n");
            status = (vx_status)VX_FAILURE;
        }
        if (levels == 0U)
        {
            VX_PRINT(VX_ZONE_ERROR, "Levels is equal to 0\n");
            status = (vx_status)VX_FAILURE;
        }
        if ((scale >= 1.0f) ||
            (scale < 0.25f))
        {
            VX_PRINT(VX_ZONE_ERROR, "Invalid scale value\n");
            status = (vx_status)VX_FAILURE;
        }
        if (levels > TIVX_PYRAMID_MAX_LEVEL_OBJECTS)
        {
            VX_PRINT(VX_ZONE_ERROR, "Levels greater than max allowable\n");
            VX_PRINT(VX_ZONE_ERROR, "May need to increase the value of TIVX_PYRAMID_MAX_LEVEL_OBJECTS in tiovx/include/TI/tivx_config.h\n");
            status = (vx_status)VX_FAILURE;
        }
        if ((scale == VX_SCALE_PYRAMID_ORB) &&
            (levels > TIVX_PYRAMID_MAX_LEVELS_ORB))
        {
            VX_PRINT(VX_ZONE_ERROR, "Orb levels are greater than max allowable\n");
            VX_PRINT(VX_ZONE_ERROR, "May need to increase the value of TIVX_PYRAMID_MAX_LEVELS_ORB in tiovx/include/TI/tivx_config.h\n");
            status = (vx_status)VX_FAILURE;
        }

        if (((vx_status)VX_SUCCESS == status) &&
            (scale == VX_SCALE_PYRAMID_ORB))
        {
            ownLogSetResourceUsedValue("TIVX_PYRAMID_MAX_LEVELS_ORB", (uint16_t)levels);
        }

        if ((vx_status)VX_SUCCESS == status)
        {
            ownLogSetResourceUsedValue("TIVX_PYRAMID_MAX_LEVEL_OBJECTS", (uint16_t)levels);
        }

        if ((vx_status)VX_SUCCESS == status)
        {
            prmd = (vx_pyramid)ownCreateReference(context, (vx_enum)VX_TYPE_PYRAMID,
                (vx_enum)VX_EXTERNAL, &context->base);

            if ((vxGetStatus((vx_reference)prmd) == (vx_status)VX_SUCCESS) &&
                (prmd->base.type == (vx_enum)VX_TYPE_PYRAMID))
            {
                /* assign refernce type specific callback's */
                prmd->base.destructor_callback = &ownDestructPyramid;
                prmd->base.mem_alloc_callback = &ownAllocPyramidBuffer;
                prmd->base.release_callback =
                    &ownReleaseReferenceBufferGeneric;
				prmd->base.kernel_callback = &pyramidKernelCallback;
                obj_desc = (tivx_obj_desc_pyramid_t*)ownObjDescAlloc(
                    (vx_enum)TIVX_OBJ_DESC_PYRAMID, (vx_reference)prmd);
                if(obj_desc==NULL)
                {
                    if((vx_status)VX_SUCCESS != vxReleasePyramid(&prmd))
                    {
                        VX_PRINT(VX_ZONE_ERROR,"Failed to release reference to pyramid object\n");
                    }
                    vxAddLogEntry(&context->base, (vx_status)VX_ERROR_NO_RESOURCES,
                        "Could not allocate prmd object descriptor\n");
                    prmd = (vx_pyramid)ownGetErrorObject(
                        context, (vx_status)VX_ERROR_NO_RESOURCES);
                    VX_PRINT(VX_ZONE_ERROR, "Could not allocate pyramid object descriptor\n");
                    VX_PRINT(VX_ZONE_ERROR, "Exceeded max object descriptors available. Increase TIVX_PLATFORM_MAX_OBJ_DESC_SHM_INST value\n");
                    VX_PRINT(VX_ZONE_ERROR, "Increase TIVX_PLATFORM_MAX_OBJ_DESC_SHM_INST value in source/platform/psdk_j7/common/soc/tivx_platform_psdk_<soc>.h\n");
                }
                else
                {
                    obj_desc->num_levels = (uint32_t)levels;
                    obj_desc->width = width;
                    obj_desc->height = height;
                    obj_desc->scale = scale;
                    obj_desc->format = format;
                    prmd->base.obj_desc = (tivx_obj_desc_t *)obj_desc;

                    for (i = 0u; i < TIVX_PYRAMID_MAX_LEVEL_OBJECTS; i ++)
                    {
                        prmd->img[i] = NULL;
                    }

                    status = ownInitPyramid(prmd);

                    if ((vx_status)VX_SUCCESS != status)
                    {
                        if((vx_status)VX_SUCCESS != vxReleasePyramid(&prmd))
                        {
                            VX_PRINT(VX_ZONE_ERROR,"Failed to release reference to pyramid object\n");
                        }
                    }
                }
            }
        }
    }

    return (prmd);
}

vx_image VX_API_CALL vxGetPyramidLevel(vx_pyramid prmd, vx_uint32 index)
{
    vx_image img = NULL;

    if (ownIsValidSpecificReference((vx_reference)prmd, (vx_enum)VX_TYPE_PYRAMID) == (vx_bool)vx_false_e)
    {
        vx_context context = ownGetContext();
        if (ownIsValidContext(context) == (vx_bool)vx_true_e)
        {
            vxAddLogEntry(&context->base, (vx_status)VX_ERROR_NO_RESOURCES,
                "Invalid pyramid reference\n");
            VX_PRINT(VX_ZONE_ERROR, "Invalid pyramid reference\n");
            img = (vx_image)ownGetErrorObject(
                context, (vx_status)VX_ERROR_NO_RESOURCES);
            VX_PRINT(VX_ZONE_ERROR, "Invalid pyramid reference\n");
        }
        else
        {
            VX_PRINT(VX_ZONE_ERROR, "Invalid pyramid reference\n");
            VX_PRINT(VX_ZONE_ERROR, "Context has not yet been created, returning NULL for vx_reference\n");
        }
    }
    else
    {
        if ((ownIsValidSpecificReference((vx_reference)prmd, (vx_enum)VX_TYPE_PYRAMID) ==
                (vx_bool)vx_true_e) && (prmd->base.obj_desc != NULL) &&
            (index < ((tivx_obj_desc_pyramid_t *)prmd->base.obj_desc)->
                num_levels))
        {
            img = prmd->img[index];

            /* Should increment the reference count,
             * To release this image, app should explicitely call ReleaseImage.
             * Setting it as void since return value 'count' not used further
             */
            (void)ownIncrementReference(&img->base, (vx_enum)VX_EXTERNAL);

            /* setting is_array_element flag */
            img->base.is_array_element = (vx_bool)vx_true_e;
        }
        else
        {
            vxAddLogEntry(&prmd->base.context->base, (vx_status)VX_ERROR_NO_RESOURCES,
                "Invalid pyramid reference\n");
            img = (vx_image)ownGetErrorObject(prmd->base.context,
                (vx_status)VX_ERROR_INVALID_PARAMETERS);
            VX_PRINT(VX_ZONE_ERROR, "Invalid pyramid reference\n");
        }
    }

    return (img);
}

vx_pyramid VX_API_CALL vxCreateVirtualPyramid(
    vx_graph graph, vx_size levels, vx_float32 scale, vx_uint32 width,
    vx_uint32 height, vx_df_image format)
{
    vx_pyramid prmd = NULL;
    vx_context context;
    vx_uint32 i;
    tivx_obj_desc_pyramid_t *obj_desc = NULL;

    /* levels can not be 0 even in virtual prmd */
    if ((ownIsValidSpecificReference((vx_reference)graph, (vx_enum)VX_TYPE_GRAPH) ==
            (vx_bool)vx_true_e) &&
        (levels > 0U) && (levels <= TIVX_PYRAMID_MAX_LEVEL_OBJECTS) &&
        ( ((scale == VX_SCALE_PYRAMID_ORB) && (levels <= TIVX_PYRAMID_MAX_LEVELS_ORB)) ||
           (scale == VX_SCALE_PYRAMID_HALF) ) )
    {
        context = graph->base.context;

        /* frame size and format can be unspecified in virtual prmd */

        prmd = (vx_pyramid)ownCreateReference(context, (vx_enum)VX_TYPE_PYRAMID,
            (vx_enum)VX_EXTERNAL, &context->base);

        if ((vxGetStatus((vx_reference)prmd) == (vx_status)VX_SUCCESS) &&
            (prmd->base.type == (vx_enum)VX_TYPE_PYRAMID))
        {
            /* assign refernce type specific callback's */
            prmd->base.destructor_callback = &ownDestructPyramid;
            prmd->base.mem_alloc_callback = &ownAllocPyramidBuffer;
            prmd->base.release_callback =
                &ownReleaseReferenceBufferGeneric;
			prmd->base.kernel_callback = &pyramidKernelCallback;
            obj_desc = (tivx_obj_desc_pyramid_t*)ownObjDescAlloc(
                (vx_enum)TIVX_OBJ_DESC_PYRAMID, (vx_reference)prmd);
            if(obj_desc==NULL)
            {
                if((vx_status)VX_SUCCESS != vxReleasePyramid(&prmd))
                {
                    VX_PRINT(VX_ZONE_ERROR,"Failed to release reference to pyramid object\n");
                }

                vxAddLogEntry(&context->base, (vx_status)VX_ERROR_NO_RESOURCES,
                    "Could not allocate prmd object descriptor\n");
                prmd = (vx_pyramid)ownGetErrorObject(
                    context, (vx_status)VX_ERROR_NO_RESOURCES);
                VX_PRINT(VX_ZONE_ERROR, "Could not allocate pyramid object descriptor\n");
                VX_PRINT(VX_ZONE_ERROR, "Exceeded max object descriptors available. Increase TIVX_PLATFORM_MAX_OBJ_DESC_SHM_INST value\n");
                VX_PRINT(VX_ZONE_ERROR, "Increase TIVX_PLATFORM_MAX_OBJ_DESC_SHM_INST value in source/platform/psdk_j7/common/soc/tivx_platform_psdk_<soc>.h\n");
            }
            else
            {
                obj_desc->num_levels = (uint32_t)levels;
                obj_desc->width = width;
                obj_desc->height = height;
                obj_desc->scale = scale;
                obj_desc->format = format;

                prmd->base.is_virtual = (vx_bool)vx_true_e;
                ownReferenceSetScope(&prmd->base, &graph->base);

                prmd->base.obj_desc = (tivx_obj_desc_t *)obj_desc;

                for (i = 0u; i < TIVX_PYRAMID_MAX_LEVEL_OBJECTS; i ++)
                {
                    prmd->img[i] = NULL;
                }

                if (scale == VX_SCALE_PYRAMID_ORB)
                {
                    ownLogSetResourceUsedValue("TIVX_PYRAMID_MAX_LEVELS_ORB", (uint16_t)levels);
                }

                ownLogSetResourceUsedValue("TIVX_PYRAMID_MAX_LEVEL_OBJECTS", (uint16_t)levels);
            }
        }
    }
    else
    {
        VX_PRINT(VX_ZONE_WARNING, "May need to increase the value of TIVX_PYRAMID_MAX_LEVELS_ORB or TIVX_PYRAMID_MAX_LEVEL_OBJECTS in tiovx/include/TI/tivx_config.h\n");
    }

    return (prmd);
}

vx_status ownInitVirtualPyramid(
    vx_pyramid prmd, vx_uint32 width, vx_uint32 height, vx_df_image format)
{
    vx_status status = (vx_status)VX_FAILURE;
    tivx_obj_desc_pyramid_t *obj_desc = NULL;

    if ((ownIsValidSpecificReference((vx_reference)prmd, (vx_enum)VX_TYPE_PYRAMID) == (vx_bool)vx_true_e)
        &&
        (prmd->base.obj_desc != NULL))
    {
        obj_desc = (tivx_obj_desc_pyramid_t *)prmd->base.obj_desc;

        if ((width > 0U) &&
            (height > 0U) &&
            (prmd->base.is_virtual == (vx_bool)vx_true_e))
        {
            obj_desc->width = width;
            obj_desc->height = height;
            obj_desc->format = format;

            status = ownInitPyramid(prmd);
        }
    }

    return (status);
}

vx_status VX_API_CALL vxQueryPyramid(
    vx_pyramid prmd, vx_enum attribute, void *ptr, vx_size size)
{
    vx_status status = (vx_status)VX_SUCCESS;
    tivx_obj_desc_pyramid_t *obj_desc = NULL;

    if ((ownIsValidSpecificReference((vx_reference)prmd, (vx_enum)VX_TYPE_PYRAMID) == (vx_bool)vx_false_e)
            || (prmd->base.obj_desc == NULL))
    {
        VX_PRINT(VX_ZONE_ERROR, "Invalid reference\n");
        status = (vx_status)VX_ERROR_INVALID_REFERENCE;
    }
    else
    {
        obj_desc = (tivx_obj_desc_pyramid_t *)prmd->base.obj_desc;
        switch (attribute)
        {
            case (vx_enum)VX_PYRAMID_LEVELS:
                if (VX_CHECK_PARAM(ptr, size, vx_size, 0x3U))
                {
                    *(vx_size *)ptr = obj_desc->num_levels;
                }
                else
                {
                    VX_PRINT(VX_ZONE_ERROR, "Query pyramid levels failed\n");
                    status = (vx_status)VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case (vx_enum)VX_PYRAMID_SCALE:
                if (VX_CHECK_PARAM(ptr, size, vx_float32, 0x3U))
                {
                    *(vx_float32 *)ptr = obj_desc->scale;
                }
                else
                {
                    VX_PRINT(VX_ZONE_ERROR, "Query pyramid scale failed\n");
                    status = (vx_status)VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case (vx_enum)VX_PYRAMID_WIDTH:
                if (VX_CHECK_PARAM(ptr, size, vx_uint32, 0x3U))
                {
                    *(vx_uint32 *)ptr = obj_desc->width;
                }
                else
                {
                    VX_PRINT(VX_ZONE_ERROR, "Query pyramid width failed\n");
                    status = (vx_status)VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case (vx_enum)VX_PYRAMID_HEIGHT:
                if (VX_CHECK_PARAM(ptr, size, vx_uint32, 0x3U))
                {
                    *(vx_uint32 *)ptr = obj_desc->height;
                }
                else
                {
                    VX_PRINT(VX_ZONE_ERROR, "Query pyramid height failed\n");
                    status = (vx_status)VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case (vx_enum)VX_PYRAMID_FORMAT:
                if (VX_CHECK_PARAM(ptr, size, vx_df_image, 0x3U))
                {
                    *(vx_df_image *)ptr = obj_desc->format;
                }
                else
                {
                    status = (vx_status)VX_ERROR_INVALID_PARAMETERS;
                    VX_PRINT(VX_ZONE_ERROR, "Query pyramid format failed\n");
                }
                break;
            default:
                VX_PRINT(VX_ZONE_ERROR, "Invalid attribute\n");
                status = (vx_status)VX_ERROR_NOT_SUPPORTED;
                break;
        }
    }

    return status;
}

static vx_status ownAllocPyramidBuffer(vx_reference ref)
{
    vx_status status = (vx_status)VX_SUCCESS;
    vx_uint32 i=0;
    vx_pyramid prmd = (vx_pyramid)ref;
    tivx_obj_desc_pyramid_t *obj_desc = NULL;
    vx_image img;

    if(prmd->base.type == (vx_enum)VX_TYPE_PYRAMID)
    {
        if(prmd->base.obj_desc != NULL)
        {
            obj_desc = (tivx_obj_desc_pyramid_t *)prmd->base.obj_desc;
            for (i = 0u; i < obj_desc->num_levels; i++)
            {
                img = prmd->img[i];

                if ((NULL != img) && (NULL != img->base.mem_alloc_callback))
                {
                    status = img->base.mem_alloc_callback((vx_reference)img);

                    if ((vx_status)VX_SUCCESS != status)
                    {
                        break;
                    }
                }
                else
                {
                    VX_PRINT(VX_ZONE_ERROR, "Image level %d is NULL\n", i);
                    status = (vx_status)VX_ERROR_INVALID_VALUE;
                }
            }
        }
        else
        {
            VX_PRINT(VX_ZONE_ERROR, "Pyramid base object descriptor is NULL\n");
            status = (vx_status)VX_ERROR_INVALID_VALUE;
        }
    }
    else
    {
        VX_PRINT(VX_ZONE_ERROR, "Reference type is not pyramid\n");
        status = (vx_status)VX_ERROR_INVALID_REFERENCE;
    }

    return status;
}

static vx_status ownDestructPyramid(vx_reference ref)
{
    vx_status status = (vx_status)VX_SUCCESS;
    vx_pyramid prmd = (vx_pyramid)ref;
    vx_uint32 i = 0;
    tivx_obj_desc_pyramid_t *obj_desc = NULL;

    obj_desc = (tivx_obj_desc_pyramid_t *)prmd->base.obj_desc;
    if(obj_desc == NULL)
    {
        status = (vx_status)VX_ERROR_INVALID_REFERENCE;
        VX_PRINT(VX_ZONE_ERROR, "Object descriptor is NULL!\n");
    }
    else
    {

        for (i = 0; i < obj_desc->num_levels; i++)
        {
            if ((NULL != prmd->img[i]) &&
                (vxGetStatus((vx_reference)prmd->img[i]) == (vx_status)VX_SUCCESS))
            {
                /* decrement the internal counter on the image, not the
                * external one. Setting it as void since return value
                * 'count' is not used further.
                */
                (void)ownDecrementReference((vx_reference)prmd->img[i], (vx_enum)VX_INTERNAL);

                status = ownReleaseReferenceInt((vx_reference *)&prmd->img[i],
                        (vx_enum)VX_TYPE_IMAGE, (vx_enum)VX_EXTERNAL, NULL);
                if ((vx_status)VX_SUCCESS != status)
                {
                    VX_PRINT(VX_ZONE_ERROR, "Pyramid level %d release failed\n", i);
                    break;
                }
            }
        }
    }
    if(prmd->base.type == (vx_enum)VX_TYPE_PYRAMID)
    {
        if(prmd->base.obj_desc!=NULL)
        {
            status = ownObjDescFree((tivx_obj_desc_t**)&prmd->base.obj_desc);
            if ((vx_status)VX_SUCCESS != status)
            {
                VX_PRINT(VX_ZONE_ERROR, "Pyramid object descriptor free failed\n");
            }
        }
    }
    return status;
}

static vx_status ownInitPyramid(vx_pyramid prmd)
{
    vx_status status = (vx_status)VX_SUCCESS;
    vx_image img;
    vx_uint32 i, w, h, j;
    vx_float32 t1, scale;
    tivx_obj_desc_pyramid_t *obj_desc = NULL;

    obj_desc = (tivx_obj_desc_pyramid_t *)prmd->base.obj_desc;

    w = obj_desc->width;
    h = obj_desc->height;
    scale = obj_desc->scale;
    t1 = obj_desc->scale;

    for (i = 0; i < obj_desc->num_levels; i++)
    {
        img = vxCreateImage(prmd->base.context, w, h,
            obj_desc->format);

        if (vxGetStatus((vx_reference)img) == (vx_status)VX_SUCCESS)
        {
            prmd->img[i] = img;
            obj_desc->obj_desc_id[i] =
                img->base.obj_desc->obj_desc_id;

            /* Setting the element index so that we can index into object array */
            img->base.obj_desc->element_idx = i;

            /* increment the internal counter on the image, not the
               external one */
            (void)ownIncrementReference((vx_reference)img, (vx_enum)VX_INTERNAL);

            /* remember that the scope of the image is the prmd */
            ownReferenceSetScope(&img->base, &prmd->base);

            if (VX_SCALE_PYRAMID_ORB == scale)
            {
                w = (vx_uint32)ceilf((vx_float32)obj_desc->width *
                    gOrbScaleFactor[i+1U]);
                h = (vx_uint32)ceilf((vx_float32)obj_desc->height *
                    gOrbScaleFactor[i+1U]);
            }
            else
            {
                w = (vx_uint32)ceilf((vx_float32)obj_desc->width * t1);
                h = (vx_uint32)ceilf((vx_float32)obj_desc->height * t1);
                t1 = t1 * scale;
            }
        }
        else
        {
            status = (vx_status)VX_FAILURE;
            vxAddLogEntry(&prmd->base.context->base, (vx_status)VX_ERROR_NO_RESOURCES,
               "Could not allocate image object descriptor\n");
            VX_PRINT(VX_ZONE_ERROR,"Could not allocate image object descriptor\n");
            break;
        }
    }

    if ((vx_status)VX_SUCCESS != status)
    {
        for (j = 0; j < i; j ++)
        {
            if (NULL != prmd->img[j])
            {
                /* increment the internal counter on the image, not the
                   external one */
                (void)ownDecrementReference((vx_reference)prmd->img[j], (vx_enum)VX_INTERNAL);

                status = ownReleaseReferenceInt((vx_reference *)&prmd->img[j],
                    (vx_enum)VX_TYPE_IMAGE, (vx_enum)VX_EXTERNAL, NULL);
                if((vx_status)VX_SUCCESS != status)
                {
                    VX_PRINT(VX_ZONE_ERROR,"Pyramid release failed\n");
                }
            }
        }
    }

    return (status);
}

