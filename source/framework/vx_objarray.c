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
 * Copyright (C) 2016 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 *******************************************************************************
 */


#include <vx_internal.h>

static vx_status ownDestructObjArray(vx_reference ref);
static vx_status ownInitObjArrayFromObject(
    vx_context context, vx_object_array objarr, vx_reference exemplar);
static vx_status ownInitObjArrayWithImage(
    vx_context context, vx_object_array objarr, vx_image exemplar);
static vx_status ownInitObjArrayWithArray(
    vx_context context, vx_object_array objarr, vx_array exemplar);
static vx_status ownInitObjArrayWithScalar(
    vx_context context, vx_object_array objarr, vx_scalar exemplar);
static vx_status ownInitObjArrayWithDistribution(
    vx_context context, vx_object_array objarr, vx_distribution exemplar);
static vx_status ownInitObjArrayWithThreshold(
    vx_context context, vx_object_array objarr, vx_threshold exemplar);
static vx_status ownInitObjArrayWithPyramid(
    vx_context context, vx_object_array objarr, vx_pyramid exemplar);
static vx_status ownInitObjArrayWithMatrix(
    vx_context context, vx_object_array objarr, vx_matrix exemplar);
static vx_status ownInitObjArrayWithRemap(
    vx_context context, vx_object_array objarr, vx_remap exemplar);
static vx_status ownInitObjArrayWithLut(
    vx_context context, vx_object_array objarr, vx_lut exemplar);

static vx_bool ownIsValidObject(vx_enum type)
{
    vx_bool status = vx_false_e;

    if ((VX_TYPE_IMAGE == type) ||
        (VX_TYPE_ARRAY == type) ||
        (VX_TYPE_SCALAR == type) ||
        (VX_TYPE_DISTRIBUTION == type) ||
        (VX_TYPE_THRESHOLD == type) ||
        (VX_TYPE_PYRAMID == type) ||
        (VX_TYPE_MATRIX == type) ||
        (VX_TYPE_REMAP == type)  ||
        (VX_TYPE_LUT == type))
    {
        status = vx_true_e;
    }

    return (status);
}


VX_API_ENTRY vx_status VX_API_CALL vxReleaseObjectArray(vx_object_array *objarr)
{
    return (ownReleaseReferenceInt(
        (vx_reference*)objarr, VX_TYPE_OBJECT_ARRAY, VX_EXTERNAL, NULL));
}

vx_object_array VX_API_CALL vxCreateObjectArray(
    vx_context context, vx_reference exemplar, vx_size count)
{
    vx_object_array objarr = NULL;
    vx_status status = VX_SUCCESS;

    if ((ownIsValidContext(context) == vx_true_e) &&
        (NULL != exemplar))
    {
        if ((vx_true_e == ownIsValidObject(exemplar->type)) &&
            (count < TIVX_OBJECT_ARRAY_MAX_OBJECT))
        {
            objarr = (vx_object_array)ownCreateReference(
                context, VX_TYPE_OBJECT_ARRAY, VX_EXTERNAL, &context->base);

            if ((vxGetStatus((vx_reference)objarr) == VX_SUCCESS) &&
                (objarr->base.type == VX_TYPE_OBJECT_ARRAY))
            {
                /* assign refernce type specific callback's */
                objarr->base.destructor_callback = ownDestructObjArray;
                objarr->base.mem_alloc_callback = NULL;
                objarr->base.release_callback =
                    (tivx_reference_release_callback_f)vxReleaseObjectArray;

                objarr->base.obj_desc = tivxObjDescAlloc(
                    TIVX_OBJ_DESC_OBJARRAY);
                if(objarr->base.obj_desc==NULL)
                {
                    vxReleaseObjectArray(&objarr);

                    vxAddLogEntry(&context->base, VX_ERROR_NO_RESOURCES,
                        "Could not allocate objarr object descriptor\n");
                    objarr = (vx_object_array)ownGetErrorObject(
                        context, VX_ERROR_NO_RESOURCES);
                }
                else
                {
                    tivx_obj_desc_objarray_t *obj_desc =
                        (tivx_obj_desc_objarray_t *)objarr->base.obj_desc;

                    obj_desc->item_type = exemplar->type;
                    obj_desc->num_items = count;

                    status = ownInitObjArrayFromObject(context, objarr, exemplar);

                    if(status != VX_SUCCESS)
                    {
                        vxReleaseObjectArray(&objarr);

                        vxAddLogEntry(&context->base, VX_ERROR_NO_RESOURCES,
                            "Could not allocate objarr object descriptor\n");
                        objarr = (vx_object_array)ownGetErrorObject(
                            context, VX_ERROR_NO_RESOURCES);
                    }
                }
            }
        }
    }

    return (objarr);
}

vx_object_array VX_API_CALL vxCreateVirtualObjectArray(
    vx_graph graph, vx_reference exemplar, vx_size count)
{
    vx_object_array objarr = NULL;
    vx_context context;

    if ((ownIsValidSpecificReference(&graph->base, VX_TYPE_GRAPH) ==
                vx_true_e) &&
        (NULL != exemplar))
    {
        context = graph->base.context;

        if ((vx_true_e == ownIsValidObject(exemplar->type)) &&
            (count < TIVX_OBJECT_ARRAY_MAX_OBJECT))
        {
            objarr = (vx_object_array)ownCreateReference(
                context, VX_TYPE_OBJECT_ARRAY, VX_EXTERNAL, &context->base);

            if ((vxGetStatus((vx_reference)objarr) == VX_SUCCESS) &&
                (objarr->base.type == VX_TYPE_OBJECT_ARRAY))
            {
                /* assign refernce type specific callback's */
                objarr->base.destructor_callback = ownDestructObjArray;
                objarr->base.mem_alloc_callback = NULL;
                objarr->base.release_callback =
                    (tivx_reference_release_callback_f)vxReleaseObjectArray;

                objarr->base.obj_desc = tivxObjDescAlloc(
                    TIVX_OBJ_DESC_OBJARRAY);
                if(objarr->base.obj_desc==NULL)
                {
                    vxReleaseObjectArray(&objarr);

                    vxAddLogEntry(&context->base, VX_ERROR_NO_RESOURCES,
                        "Could not allocate objarr object descriptor\n");
                    objarr = (vx_object_array)ownGetErrorObject(
                        context, VX_ERROR_NO_RESOURCES);
                }
                else
                {
                    tivx_obj_desc_objarray_t *obj_desc =
                        (tivx_obj_desc_objarray_t *)objarr->base.obj_desc;

                    obj_desc->item_type = exemplar->type;
                    obj_desc->num_items = count;

                    ownInitObjArrayFromObject(context, objarr, exemplar);

                    objarr->base.is_virtual = vx_true_e;
                    objarr->base.scope = (vx_reference)graph;
                }
            }
        }
    }

    return (objarr);
}

vx_reference VX_API_CALL vxGetObjectArrayItem(
    vx_object_array objarr, vx_uint32 index)
{
    vx_reference ref = NULL;
    tivx_obj_desc_objarray_t *obj_desc =
        (tivx_obj_desc_objarray_t *)objarr->base.obj_desc;

    if ((ownIsValidSpecificReference(&objarr->base, VX_TYPE_OBJECT_ARRAY) ==
            vx_true_e) && (obj_desc != NULL) &&
        (index < obj_desc->num_items) &&
        (objarr->base.is_virtual == vx_false_e))
    {
        ref = objarr->ref[index];
    }

    return (ref);
}

vx_status VX_API_CALL vxQueryObjectArray(
    vx_object_array objarr, vx_enum attribute, void *ptr, vx_size size)
{
    vx_status status = VX_SUCCESS;

    if (ownIsValidSpecificReference(&objarr->base, VX_TYPE_OBJECT_ARRAY) == vx_false_e
        ||
        objarr->base.obj_desc == NULL
        )
    {
        status = VX_ERROR_INVALID_REFERENCE;
    }
    else
    {
        switch (attribute)
        {
            case VX_OBJECT_ARRAY_ITEMTYPE:
                if (VX_CHECK_PARAM(ptr, size, vx_enum, 0x3))
                {
                    tivx_obj_desc_objarray_t *obj_desc =
                        (tivx_obj_desc_objarray_t *)objarr->base.obj_desc;

                    *(vx_enum *)ptr = obj_desc->item_type;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case VX_OBJECT_ARRAY_NUMITEMS:
                if (VX_CHECK_PARAM(ptr, size, vx_size, 0x3))
                {
                    tivx_obj_desc_objarray_t *obj_desc =
                        (tivx_obj_desc_objarray_t *)objarr->base.obj_desc;

                    *(vx_size *)ptr = obj_desc->num_items;
                }
                else
                {
                    status = VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            default:
                status = VX_ERROR_NOT_SUPPORTED;
                break;
        }
    }

    return status;
}

static vx_status ownInitObjArrayFromObject(
    vx_context context, vx_object_array objarr, vx_reference exemplar)
{
    vx_status status = VX_SUCCESS;

    switch (exemplar->type)
    {
        case VX_TYPE_LUT:
            status = ownInitObjArrayWithLut(
                context, objarr, (vx_lut)exemplar);
            break;
        case VX_TYPE_REMAP:
            status = ownInitObjArrayWithRemap(
                context, objarr, (vx_remap)exemplar);
            break;
        case VX_TYPE_MATRIX:
            status = ownInitObjArrayWithMatrix(
                context, objarr, (vx_matrix)exemplar);
            break;
        case VX_TYPE_PYRAMID:
            status = ownInitObjArrayWithPyramid(
                context, objarr, (vx_pyramid)exemplar);
            break;
        case VX_TYPE_IMAGE:
            status = ownInitObjArrayWithImage(
                context, objarr, (vx_image)exemplar);
            break;
        case VX_TYPE_ARRAY:
            status = ownInitObjArrayWithArray(
                context, objarr, (vx_array)exemplar);
            break;
        case VX_TYPE_SCALAR:
            status = ownInitObjArrayWithScalar(
                context, objarr, (vx_scalar)exemplar);
            break;
        case VX_TYPE_DISTRIBUTION:
            status = ownInitObjArrayWithDistribution(
                context, objarr, (vx_distribution)exemplar);
            break;
        case VX_TYPE_THRESHOLD:
            status = ownInitObjArrayWithThreshold(
                context, objarr, (vx_threshold)exemplar);
            break;
    }

    return (status);
}

static vx_status ownInitObjArrayWithLut(
    vx_context context, vx_object_array objarr, vx_lut exemplar)
{
    vx_status status = VX_SUCCESS;
    vx_enum data_type;
    vx_size count;
    vx_lut lut;
    vx_uint32 i, j, num_items;

    status |= vxQueryLUT(exemplar, VX_LUT_TYPE, &data_type,
        sizeof(data_type));
    status |= vxQueryLUT(exemplar, VX_LUT_COUNT, &count,
        sizeof(count));

    if (VX_SUCCESS == status)
    {
        tivx_obj_desc_objarray_t *obj_desc =
            (tivx_obj_desc_objarray_t *)objarr->base.obj_desc;

        num_items = obj_desc->num_items;
        for (i = 0; i < num_items; i ++)
        {
            lut = vxCreateLUT(context, data_type, count);

            if (vxGetStatus((vx_reference)lut) == VX_SUCCESS)
            {
                objarr->ref[i] = (vx_reference)lut;
                obj_desc->obj_desc_id[i] =
                    objarr->ref[i]->obj_desc->obj_desc_id;
            }
            else
            {
                status = VX_FAILURE;
                vxAddLogEntry(&context->base, VX_ERROR_NO_RESOURCES,
                   "Could not allocate image object descriptor\n");
                break;
           }
        }

        if (VX_SUCCESS != status)
        {
            for (j = 0; j < i; j ++)
            {
                if (NULL != objarr->ref[j])
                {
                    vxReleaseLUT((vx_lut*)&objarr->ref[j]);
                }
            }
        }
    }

    return (status);
}

static vx_status ownInitObjArrayWithRemap(
    vx_context context, vx_object_array objarr, vx_remap exemplar)
{
    vx_status status = VX_SUCCESS;
    vx_uint32 src_width, src_height, dst_width, dst_height, i, num_items, j;
    vx_remap rem;

    status |= vxQueryRemap(exemplar, VX_REMAP_SOURCE_WIDTH, &src_width,
        sizeof(src_width));
    status |= vxQueryRemap(exemplar, VX_REMAP_SOURCE_HEIGHT, &src_height,
        sizeof(src_height));
    status |= vxQueryRemap(exemplar, VX_REMAP_DESTINATION_WIDTH, &dst_width,
        sizeof(dst_width));
    status |= vxQueryRemap(exemplar, VX_REMAP_DESTINATION_HEIGHT, &dst_height,
        sizeof(dst_height));

    if (VX_SUCCESS == status)
    {
        tivx_obj_desc_objarray_t *obj_desc =
            (tivx_obj_desc_objarray_t *)objarr->base.obj_desc;

        num_items = obj_desc->num_items;
        for (i = 0; i < num_items; i ++)
        {
            rem = vxCreateRemap(context, src_width, src_height, dst_width,
                dst_height);

            if (vxGetStatus((vx_reference)rem) == VX_SUCCESS)
            {
                objarr->ref[i] = (vx_reference)rem;
                obj_desc->obj_desc_id[i] =
                    objarr->ref[i]->obj_desc->obj_desc_id;
            }
            else
            {
                status = VX_FAILURE;
                vxAddLogEntry(&context->base, VX_ERROR_NO_RESOURCES,
                   "Could not allocate image object descriptor\n");
                break;
           }
        }

        if (VX_SUCCESS != status)
        {
            for (j = 0; j < i; j ++)
            {
                if (NULL != objarr->ref[j])
                {
                    vxReleaseRemap((vx_remap*)&objarr->ref[j]);
                }
            }
        }
    }

    return (status);
}

static vx_status ownInitObjArrayWithMatrix(
    vx_context context, vx_object_array objarr, vx_matrix exemplar)
{
    vx_status status = VX_SUCCESS;
    vx_size rows, columns;
    vx_uint32 i, num_items, j;
    vx_enum type;
    vx_matrix mat;

    status |= vxQueryMatrix(exemplar, VX_MATRIX_TYPE, &type, sizeof(type));
    status |= vxQueryMatrix(exemplar, VX_MATRIX_ROWS, &rows, sizeof(rows));
    status |= vxQueryMatrix(exemplar, VX_MATRIX_COLUMNS, &columns,
        sizeof(columns));

    if (VX_SUCCESS == status)
    {
        tivx_obj_desc_objarray_t *obj_desc =
            (tivx_obj_desc_objarray_t *)objarr->base.obj_desc;

        num_items = obj_desc->num_items;
        for (i = 0; i < num_items; i ++)
        {
            mat = vxCreateMatrix(context, type, columns, rows);

            if (vxGetStatus((vx_reference)mat) == VX_SUCCESS)
            {
                objarr->ref[i] = (vx_reference)mat;
                obj_desc->obj_desc_id[i] =
                    objarr->ref[i]->obj_desc->obj_desc_id;
            }
            else
            {
                status = VX_FAILURE;
                vxAddLogEntry(&context->base, VX_ERROR_NO_RESOURCES,
                   "Could not allocate image object descriptor\n");
                break;
           }
        }

        if (VX_SUCCESS != status)
        {
            for (j = 0; j < i; j ++)
            {
                if (NULL != objarr->ref[j])
                {
                    vxReleaseMatrix((vx_matrix*)&objarr->ref[j]);
                }
            }
        }
    }

    return (status);
}

static vx_status ownInitObjArrayWithPyramid(
    vx_context context, vx_object_array objarr, vx_pyramid exemplar)
{
    vx_status status = VX_SUCCESS;
    vx_size levels;
    vx_float32 scale;
    vx_uint32 width, height, i, num_items, j;
    vx_df_image format;
    vx_pyramid pmd;

    status |= vxQueryPyramid(exemplar, VX_PYRAMID_LEVELS, &levels, sizeof(width));
    status |= vxQueryPyramid(exemplar, VX_PYRAMID_SCALE, &scale, sizeof(width));
    status |= vxQueryPyramid(exemplar, VX_PYRAMID_WIDTH, &width, sizeof(width));
    status |= vxQueryPyramid(exemplar, VX_PYRAMID_HEIGHT, &height, sizeof(height));
    status |= vxQueryPyramid(exemplar, VX_PYRAMID_FORMAT, &format, sizeof(format));

    if (VX_SUCCESS == status)
    {
        tivx_obj_desc_objarray_t *obj_desc =
            (tivx_obj_desc_objarray_t *)objarr->base.obj_desc;

        num_items = obj_desc->num_items;
        for (i = 0; i < num_items; i ++)
        {
            pmd = vxCreatePyramid(context, levels, scale, width, height,
                format);

            if (vxGetStatus((vx_reference)pmd) == VX_SUCCESS)
            {
                objarr->ref[i] = (vx_reference)pmd;
                obj_desc->obj_desc_id[i] =
                    objarr->ref[i]->obj_desc->obj_desc_id;
            }
            else
            {
                status = VX_FAILURE;
                vxAddLogEntry(&context->base, VX_ERROR_NO_RESOURCES,
                   "Could not allocate image object descriptor\n");
                break;
           }
        }

        if (VX_SUCCESS != status)
        {
            for (j = 0; j < i; j ++)
            {
                if(objarr->ref[j])
                {
                    vxReleasePyramid((vx_pyramid*)&objarr->ref[j]);
                }
            }
        }
    }

    return (status);
}

static vx_status ownInitObjArrayWithImage(
    vx_context context, vx_object_array objarr, vx_image exemplar)
{
    vx_status status = VX_SUCCESS;
    vx_uint32 width, height, i, num_items, j;
    vx_df_image format;
    vx_image img;

    status |= vxQueryImage(exemplar, VX_IMAGE_WIDTH, &width, sizeof(width));
    status |= vxQueryImage(exemplar, VX_IMAGE_HEIGHT, &height, sizeof(height));
    status |= vxQueryImage(exemplar, VX_IMAGE_FORMAT, &format, sizeof(format));

    if (VX_SUCCESS == status)
    {
        tivx_obj_desc_objarray_t *obj_desc =
            (tivx_obj_desc_objarray_t *)objarr->base.obj_desc;

        num_items = obj_desc->num_items;
        for (i = 0; i < num_items; i ++)
        {
            img = vxCreateImage(context, width, height, format);

            if (vxGetStatus((vx_reference)img) == VX_SUCCESS)
            {
                objarr->ref[i] = (vx_reference)img;
                obj_desc->obj_desc_id[i] =
                    objarr->ref[i]->obj_desc->obj_desc_id;
            }
            else
            {
                status = VX_FAILURE;
                vxAddLogEntry(&context->base, VX_ERROR_NO_RESOURCES,
                   "Could not allocate image object descriptor\n");
                break;
           }
        }

        if (VX_SUCCESS != status)
        {
            for (j = 0; j < i; j ++)
            {
                if (NULL != objarr->ref[j])
                {
                    vxReleaseImage((vx_image*)&objarr->ref[j]);
                }
            }
        }
    }

    return (status);
}

static vx_status ownInitObjArrayWithArray(
    vx_context context, vx_object_array objarr, vx_array exemplar)
{
    vx_status status = VX_SUCCESS;
    vx_enum type;
    vx_size capacity;
    vx_array arr;
    vx_uint32 num_items, i, j;

    status |= vxQueryArray(exemplar, VX_ARRAY_ITEMTYPE, &type, sizeof(type));
    status |= vxQueryArray(exemplar, VX_ARRAY_CAPACITY, &capacity, sizeof(capacity));

    if (VX_SUCCESS == status)
    {
        tivx_obj_desc_objarray_t *obj_desc =
            (tivx_obj_desc_objarray_t *)objarr->base.obj_desc;



        num_items = obj_desc->num_items;

        for (i = 0; i < num_items; i ++)
        {
            arr = vxCreateArray(context, type, capacity);

            if (vxGetStatus((vx_reference)arr) == VX_SUCCESS)
            {
                objarr->ref[i] = (vx_reference)arr;
                obj_desc->obj_desc_id[i] = ((vx_reference)arr)->
                    obj_desc->obj_desc_id;
            }
            else
            {
                status = VX_FAILURE;
                vxAddLogEntry(&context->base, VX_ERROR_NO_RESOURCES,
                   "Could not allocate array object descriptor\n");
                break;
           }
        }

        if (VX_SUCCESS != status)
        {
            for (j = 0; j < i; j ++)
            {
                if (NULL != objarr->ref[j])
                {
                    vxReleaseArray((vx_array*)&objarr->ref[j]);
                }
            }
        }
    }

    return (status);
}

static vx_status ownInitObjArrayWithScalar(
    vx_context context, vx_object_array objarr, vx_scalar exemplar)
{
    vx_status status = VX_SUCCESS;
    vx_enum type;
    vx_scalar sc;
    vx_uint32 num_items, i, j;

    status |= vxQueryScalar(exemplar, VX_SCALAR_TYPE, &type, sizeof(type));

    if (VX_SUCCESS == status)
    {
        tivx_obj_desc_objarray_t *obj_desc =
            (tivx_obj_desc_objarray_t *)objarr->base.obj_desc;

        num_items = obj_desc->num_items;
        for (i = 0; i < num_items; i ++)
        {
            /* TODO: How to get the internal data pointer */
            sc = vxCreateScalar(context, type, NULL);

            if (vxGetStatus((vx_reference)sc) == VX_SUCCESS)
            {
                objarr->ref[i] = (vx_reference)sc;
                obj_desc->obj_desc_id[i] =
                    objarr->ref[i]->obj_desc->obj_desc_id;
            }
            else
            {
                status = VX_FAILURE;
                vxAddLogEntry(&context->base, VX_ERROR_NO_RESOURCES,
                   "Could not allocate array object descriptor\n");
                break;
           }
        }

        if (VX_SUCCESS != status)
        {
            for (j = 0; j < i; j ++)
            {
                if (NULL != objarr->ref[j])
                {
                    vxReleaseScalar((vx_scalar*)&objarr->ref[j]);
                }
            }
        }
    }

    return (status);
}
static vx_status ownInitObjArrayWithDistribution(
    vx_context context, vx_object_array objarr, vx_distribution exemplar)
{
    vx_status status = VX_SUCCESS;
    vx_size num_bins;
    vx_int32 offset;
    vx_uint32 num_items, i, j, range;
    vx_distribution dist;

    status |= vxQueryDistribution(exemplar, VX_DISTRIBUTION_OFFSET, &offset, sizeof(offset));
    status |= vxQueryDistribution(exemplar, VX_DISTRIBUTION_RANGE, &range, sizeof(range));
    status |= vxQueryDistribution(exemplar, VX_DISTRIBUTION_BINS, &num_bins, sizeof(num_bins));

    if (VX_SUCCESS == status)
    {
        tivx_obj_desc_objarray_t *obj_desc =
            (tivx_obj_desc_objarray_t *)objarr->base.obj_desc;
        num_items = obj_desc->num_items;
        for (i = 0; i < num_items; i ++)
        {
            dist = vxCreateDistribution(context, num_bins, offset, range);

            if (vxGetStatus((vx_reference)dist) == VX_SUCCESS)
            {
                objarr->ref[i] = (vx_reference)dist;
                obj_desc->obj_desc_id[i] =
                    objarr->ref[i]->obj_desc->obj_desc_id;
            }
            else
            {
                status = VX_FAILURE;
                vxAddLogEntry(&context->base, VX_ERROR_NO_RESOURCES,
                   "Could not allocate distribution object descriptor\n");
                break;
           }
        }

        if (VX_SUCCESS != status)
        {
            for (j = 0; j < i; j ++)
            {
                if (NULL != objarr->ref[j])
                {
                    vxReleaseDistribution((vx_distribution*)&objarr->ref[j]);
                }
            }
        }
    }

    return (status);
}

static vx_status ownInitObjArrayWithThreshold(
    vx_context context, vx_object_array objarr, vx_threshold exemplar)
{
    vx_status status = VX_SUCCESS;
    vx_enum thr_type;
    vx_enum data_type;
    vx_threshold thr;
    vx_uint32 num_items, i, j;

    status |= vxQueryThreshold(exemplar, VX_THRESHOLD_DATA_TYPE, &data_type,
        sizeof(data_type));
    status |= vxQueryThreshold(exemplar, VX_THRESHOLD_TYPE, &thr_type,
        sizeof(thr_type));

    if (VX_SUCCESS == status)
    {
        tivx_obj_desc_objarray_t *obj_desc =
            (tivx_obj_desc_objarray_t *)objarr->base.obj_desc;
        num_items = obj_desc->num_items;
        for (i = 0; i < num_items; i ++)
        {
            thr = vxCreateThreshold(context, thr_type, data_type);

            if (vxGetStatus((vx_reference)thr) == VX_SUCCESS)
            {
                objarr->ref[i] = (vx_reference)thr;
                obj_desc->obj_desc_id[i] =
                    objarr->ref[i]->obj_desc->obj_desc_id;
            }
            else
            {
                status = VX_FAILURE;
                vxAddLogEntry(&context->base, VX_ERROR_NO_RESOURCES,
                   "Could not allocate threshold object descriptor\n");
                break;
           }
        }

        if (VX_SUCCESS != status)
        {
            for (j = 0; j < i; j ++)
            {
                if (NULL != objarr->ref[j])
                {
                    vxReleaseThreshold((vx_threshold*)&objarr->ref[j]);
                }
            }
        }
    }

    return (status);
}

static vx_status ownDestructObjArray(vx_reference ref)
{
    vx_object_array objarr = (vx_object_array)ref;

    if(objarr->base.type == VX_TYPE_OBJECT_ARRAY)
    {
        if(objarr->base.obj_desc!=NULL)
        {
            tivx_obj_desc_objarray_t *obj_desc =
                (tivx_obj_desc_objarray_t *)objarr->base.obj_desc;

            vx_uint32 num_items, i;

            num_items = obj_desc->num_items;

            for (i = 0; i < num_items; i ++)
            {
                if(objarr->ref[i])
                {
                    vxReleaseReference(&objarr->ref[i]);
                }
            }

            tivxObjDescFree(&objarr->base.obj_desc);
        }
    }
    return VX_SUCCESS;
}

