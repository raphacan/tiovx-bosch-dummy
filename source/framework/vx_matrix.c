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

static vx_matrix ownCreateMatrix(vx_reference scope, vx_enum data_type, vx_size columns, vx_size rows, vx_bool is_virtual);
static vx_status isMatrixCopyable(vx_matrix input, vx_matrix output);
static vx_status copyMatrix(vx_matrix input, vx_matrix output);
static vx_status swapMatrix(vx_matrix input, vx_matrix output);
static vx_status VX_CALLBACK matrixKernelCallback(vx_enum kernel_enum, vx_bool validate_only, vx_enum optimization, const vx_reference params[], vx_uint32 num_params);

/*! \brief This function is called to find out if it is OK to copy the input to the output.
 * Columns, rows, data type, pattern, origin_x and origin_y must be the same
 * \returns VX_SUCCESS if it is, otherwise another error code.
 *
 */
static vx_status isMatrixCopyable(vx_matrix input, vx_matrix output)
{
    tivx_obj_desc_matrix_t *ip_obj_desc = (tivx_obj_desc_matrix_t *)input->base.obj_desc;
    tivx_obj_desc_matrix_t *op_obj_desc = (tivx_obj_desc_matrix_t *)output->base.obj_desc;
    if ((input != output) &&
        (ownIsValidSpecificReference(&input->base, (vx_enum)VX_TYPE_MATRIX) == (vx_bool)vx_true_e) &&
        (op_obj_desc != NULL) &&
        (ownIsValidSpecificReference(&output->base, (vx_enum)VX_TYPE_MATRIX) == (vx_bool)vx_true_e) &&
        (op_obj_desc != NULL) &&
        (ip_obj_desc->columns == op_obj_desc->columns) &&
        (ip_obj_desc->rows == op_obj_desc->rows) &&
        (ip_obj_desc->data_type == op_obj_desc->data_type) &&
        (ip_obj_desc->pattern == op_obj_desc->pattern) &&
        (ip_obj_desc->origin_x == op_obj_desc->origin_x) &&
        (ip_obj_desc->origin_y == op_obj_desc->origin_y)
        )
    {
        return VX_SUCCESS;
    }
    else
    {
        return VX_ERROR_NOT_COMPATIBLE;
    }
}

/*! \brief Copy input to output
 * The input must be copyable to the output; checks done already.
 * Note that locking a reference actually locks the context, so we only lock
 * one reference!

 */
static vx_status copyMatrix(vx_matrix input, vx_matrix output)
{
    return (ownCopyReferenceGeneric((vx_reference)input, (vx_reference)output));
}

/*! \brief swap input and output pointers
 * Input and output must be swappable; checks done already.
 */
static vx_status swapMatrix(vx_matrix input, vx_matrix output)
{
    return ownSwapReferenceGeneric((vx_reference)input, (vx_reference)output);
}

/* Call back function that handles the copy, swap and move kernels */
static vx_status VX_CALLBACK matrixKernelCallback(vx_enum kernel_enum, vx_bool validate_only, vx_enum optimization, const vx_reference params[], vx_uint32 num_params)
{
    /*
        Decode the kernel operation - simple version!
    */
    vx_matrix input = (vx_matrix)params[0];
    vx_matrix output = (vx_matrix)params[1];
    switch (kernel_enum)
    {
        case VX_KERNEL_COPY:    return validate_only ? isMatrixCopyable(input, output) : copyMatrix(input, output);
        case VX_KERNEL_SWAP:    /* Swap and move do exactly the same */
        case VX_KERNEL_MOVE:     return validate_only ? isMatrixCopyable(input, output) : swapMatrix(input, output);
        default:                return VX_ERROR_NOT_SUPPORTED;
    }
}

static vx_matrix ownCreateMatrix(vx_reference scope, vx_enum data_type, vx_size columns, vx_size rows, vx_bool is_virtual)
{
    vx_matrix matrix = NULL;
    vx_reference ref = NULL;
    vx_size dim = 0U;
    tivx_obj_desc_matrix_t *obj_desc = NULL;
	vx_status status = (vx_status)VX_SUCCESS;
    vx_context context;

    if (ownIsValidSpecificReference(scope, (vx_enum)VX_TYPE_GRAPH) == (vx_bool)vx_true_e)
    {
        context = vxGetContext(scope);
    }
    else
    {
        context = (vx_context)scope;
    }

    if(ownIsValidContext(context) == (vx_bool)vx_true_e)
    {
        if ((data_type == (vx_enum)VX_TYPE_INT8) || (data_type == (vx_enum)VX_TYPE_UINT8))
        {
            dim = sizeof(vx_uint8);
        }
        else if ((data_type == (vx_enum)VX_TYPE_INT16) ||
                 (data_type == (vx_enum)VX_TYPE_UINT16))
        {
            dim = sizeof(vx_uint16);
        }
        else if ((data_type == (vx_enum)VX_TYPE_INT32) ||
                 (data_type == (vx_enum)VX_TYPE_UINT32) ||
                 (data_type == (vx_enum)VX_TYPE_FLOAT32))
        {
            dim = sizeof(vx_uint32);
        }
        else if ((data_type == (vx_enum)VX_TYPE_INT64) ||
                 (data_type == (vx_enum)VX_TYPE_UINT64) ||
                 (data_type == (vx_enum)VX_TYPE_FLOAT64))
        {
            dim = sizeof(vx_uint64);
        }
        else
        {
            dim = 0U;
        }

        if ((rows != 0U) && (columns != 0U) && (dim != 0UL))
        {
            ref = ownCreateReference(context, (vx_enum)VX_TYPE_MATRIX,
                (vx_enum)VX_EXTERNAL, &context->base);

            if ((vxGetStatus(ref) == (vx_status)VX_SUCCESS) &&
                (ref->type == (vx_enum)VX_TYPE_MATRIX))
            {
                /* status set to NULL due to preceding type check */
                matrix = vxCastRefAsMatrix(ref,NULL);
                /* assign refernce type specific callback's */
                matrix->base.destructor_callback = &ownDestructReferenceGeneric;
                matrix->base.mem_alloc_callback = &ownAllocReferenceBufferGeneric;
                matrix->base.release_callback = &ownReleaseReferenceBufferGeneric;
                matrix->base.kernel_callback = &matrixKernelCallback;
                obj_desc = (tivx_obj_desc_matrix_t*)ownObjDescAlloc(
                    (vx_enum)TIVX_OBJ_DESC_MATRIX, vxCastRefFromMatrix(matrix));
                if(obj_desc==NULL)
                {
                    status = vxReleaseMatrix(&matrix);
                    if((vx_status)VX_SUCCESS != status)
                    {
                        VX_PRINT(VX_ZONE_ERROR,"Failed to release reference to matrix object\n");
                    }

                    vxAddLogEntry(&context->base, (vx_status)VX_ERROR_NO_RESOURCES,
                        "Could not allocate matrix object descriptor\n");
                    matrix = (vx_matrix)ownGetErrorObject(
                        context, (vx_status)VX_ERROR_NO_RESOURCES);
                    VX_PRINT(VX_ZONE_ERROR, "Could not allocate matrix object descriptor\n");
                    VX_PRINT(VX_ZONE_ERROR, "Exceeded max object descriptors available. Increase TIVX_PLATFORM_MAX_OBJ_DESC_SHM_INST value\n");
                    VX_PRINT(VX_ZONE_ERROR, "Increase TIVX_PLATFORM_MAX_OBJ_DESC_SHM_INST value in source/platform/psdk_j7/common/soc/tivx_platform_psdk_<soc>.h\n");
                }
                else
                {
                    obj_desc->data_type = data_type;
                    obj_desc->columns = (uint32_t)columns;
                    obj_desc->rows = (uint32_t)rows;
                    obj_desc->origin_x = (uint32_t)columns/2U;
                    obj_desc->origin_y = (uint32_t)rows/2U;
                    obj_desc->pattern = (vx_enum)VX_PATTERN_OTHER;
                    obj_desc->mem_size = (uint32_t)columns*(uint32_t)rows*(uint32_t)dim;
                    obj_desc->mem_ptr.host_ptr = (uint64_t)0;
                    obj_desc->mem_ptr.shared_ptr = (uint64_t)0;
                    obj_desc->mem_ptr.mem_heap_region = (vx_enum)TIVX_MEM_EXTERNAL;
                    matrix->base.obj_desc = (tivx_obj_desc_t *)obj_desc;
                }
            }
        }
    }

    return (matrix);
}

vx_matrix VX_API_CALL vxCreateMatrix(vx_context context, vx_enum data_type, vx_size columns, vx_size rows)
{
    return ownCreateMatrix((vx_reference)context, data_type, columns, rows, vx_false_e);
}

vx_matrix VX_API_CALL vxCreateVirtualMatrix(vx_graph graph, vx_enum data_type, vx_size columns, vx_size rows)
{
    return ownCreateMatrix((vx_reference)graph, data_type, columns, rows, vx_true_e);
}


vx_matrix VX_API_CALL vxCreateMatrixFromPattern(
    vx_context context, vx_enum pattern, vx_size columns, vx_size rows)
{
    vx_status status = (vx_status)VX_SUCCESS;
    vx_matrix matrix = NULL;
    vx_size dim = 0U, i, j;
    vx_uint8 *pTempDataPtr;
    tivx_obj_desc_matrix_t *obj_desc = NULL;

    /* Check for errors */
    if(ownIsValidContext(context) != (vx_bool)vx_true_e)
    {
        VX_PRINT(VX_ZONE_ERROR, "invalid context\n");
        status = (vx_status)VX_FAILURE;
    }

    if (rows == 0U)
    {
        VX_PRINT(VX_ZONE_ERROR, "rows value is equal to zero\n");
        status = (vx_status)VX_FAILURE;
    }

    if (columns == 0U)
    {
        VX_PRINT(VX_ZONE_ERROR, "columns value is equal to zero\n");
        status = (vx_status)VX_FAILURE;
    }

    if (((vx_enum)VX_PATTERN_BOX != pattern) && ((vx_enum)VX_PATTERN_CROSS != pattern) &&
             ((vx_enum)VX_PATTERN_OTHER != pattern) && ((vx_enum)VX_PATTERN_DISK != pattern))
    {
        VX_PRINT(VX_ZONE_ERROR, "invalid pattern value\n");
        status = (vx_status)VX_FAILURE;
    }

    /* For Cross pattern, rows and columns must be odd */
    if (((vx_enum)VX_PATTERN_CROSS == pattern) && (((rows%2U) == 0U) || ((columns%2U) == 0U)))
    {
        VX_PRINT(VX_ZONE_ERROR, "cross pattern rows and columns are not odd\n");
        status = (vx_status)VX_FAILURE;
    }

    /* For Disk pattern, rows and columns must be equal */
    if (((vx_enum)VX_PATTERN_DISK == pattern) && ( rows != columns ))
    {
        VX_PRINT(VX_ZONE_ERROR, "disk pattern rows and columns are not equal\n");
        status = (vx_status)VX_FAILURE;
    }

    if ((vx_status)VX_SUCCESS == status)
    {
        dim = sizeof(vx_uint8);

        vx_reference matrix_ref = ownCreateReference(context, (vx_enum)VX_TYPE_MATRIX,
            (vx_enum)VX_EXTERNAL, &context->base);

        if ((vxGetStatus(matrix_ref) == (vx_status)VX_SUCCESS) &&
            (matrix_ref->type == (vx_enum)VX_TYPE_MATRIX))
        {
            /* status set to NULL due to preceding type check */
            matrix = vxCastRefAsMatrix(matrix_ref,NULL);
            /* assign refernce type specific callback's */
            matrix->base.destructor_callback = &ownDestructReferenceGeneric;
            matrix->base.mem_alloc_callback = &ownAllocReferenceBufferGeneric;
            matrix->base.release_callback = &ownReleaseReferenceBufferGeneric;
            matrix->base.kernel_callback = &matrixKernelCallback;
            obj_desc = (tivx_obj_desc_matrix_t*)ownObjDescAlloc(
                (vx_enum)TIVX_OBJ_DESC_MATRIX, vxCastRefFromMatrix(matrix));
            if(obj_desc==NULL)
            {
                status = vxReleaseMatrix(&matrix);
                if((vx_status)VX_SUCCESS != status)
                {
                    VX_PRINT(VX_ZONE_ERROR,"Failed to release reference to matrix object\n");
                }

                vxAddLogEntry(&context->base, (vx_status)VX_ERROR_NO_RESOURCES,
                    "Could not allocate matrix object descriptor\n");
                matrix = (vx_matrix)ownGetErrorObject(
                    context, (vx_status)VX_ERROR_NO_RESOURCES);
                VX_PRINT(VX_ZONE_ERROR, "Could not allocate matrix object descriptor\n");
                VX_PRINT(VX_ZONE_ERROR, "Exceeded max object descriptors available. Increase TIVX_PLATFORM_MAX_OBJ_DESC_SHM_INST value\n");
                VX_PRINT(VX_ZONE_ERROR, "Increase TIVX_PLATFORM_MAX_OBJ_DESC_SHM_INST value in source/platform/psdk_j7/common/soc/tivx_platform_psdk_<soc>.h\n");
                status = (vx_status)VX_FAILURE;
            }
            else
            {
                /* Initialize descriptor object */
                obj_desc->data_type = (vx_enum)VX_TYPE_UINT8;
                obj_desc->columns = (uint32_t)columns;
                obj_desc->rows = (uint32_t)rows;
                obj_desc->origin_x = (uint32_t)columns/2U;
                obj_desc->origin_y = (uint32_t)rows/2U;
                obj_desc->pattern = pattern;
                obj_desc->mem_size = (uint32_t)columns*(uint32_t)rows*(uint32_t)dim;
                obj_desc->mem_ptr.mem_heap_region = (vx_enum)TIVX_MEM_EXTERNAL;
                matrix->base.obj_desc = (tivx_obj_desc_t *)obj_desc;

                obj_desc->mem_ptr.host_ptr = (uint64_t)0;
                obj_desc->mem_ptr.shared_ptr = (uint64_t)0;

                /* Allocate memory for matrix since matrix need to be
                   filled up with a pattern  */
                status = ownAllocReferenceBufferGeneric(&matrix->base);

                if ((vx_status)VX_SUCCESS != status)
                {
                    /* Free up memory allocated for matrix */
                    /* Error status check is not done as it
                     * is already done in the previous status
                     * check of ownAllocReferenceBufferGeneric
                     */
                    (void)ownDestructReferenceGeneric(&matrix->base);

                    /* Release matrix */
                    status = vxReleaseMatrix(&matrix);
                    if((vx_status)VX_SUCCESS != status)
                    {
                        VX_PRINT(VX_ZONE_ERROR,"Failed to release reference to matrix object\n");
                    }
                    vxAddLogEntry(&context->base, (vx_status)VX_ERROR_NO_RESOURCES,
                        "Could not allocate matrix object descriptor\n");
                    matrix = (vx_matrix)ownGetErrorObject(
                        context, (vx_status)VX_ERROR_NO_RESOURCES);
                    VX_PRINT(VX_ZONE_ERROR, "Could not allocate matrix object descriptor\n");
                }
                else
                {
                    obj_desc->mem_ptr.shared_ptr =
                        tivxMemHost2SharedPtr(
                            obj_desc->mem_ptr.host_ptr,
                            (vx_enum)TIVX_MEM_EXTERNAL);
                }
            }
        }
        else
        {
            VX_PRINT(VX_ZONE_ERROR, "Invalid matrix reference\n");
            status = (vx_status)VX_FAILURE;
        }
    }

    if (((vx_status)VX_SUCCESS == status) && ((uint64_t)0 != obj_desc->mem_ptr.host_ptr))
    {
        tivxCheckStatus(&status, tivxMemBufferMap((void*)(uintptr_t)obj_desc->mem_ptr.host_ptr,
            obj_desc->mem_size, (vx_enum)VX_MEMORY_TYPE_HOST,
            (vx_enum)VX_WRITE_ONLY));

        pTempDataPtr = (vx_uint8 *)(uintptr_t)obj_desc->mem_ptr.host_ptr;
        if ((vx_enum)VX_PATTERN_BOX == pattern)
        {
            for (i = 0U; i < rows; i ++)
            {
                for (j = 0U; j < columns; j ++)
                {
                    *pTempDataPtr = 255U;
                    pTempDataPtr ++;
                }
            }
        }
        else if ((vx_enum)VX_PATTERN_CROSS == pattern)
        {
            for (i = 0U; i < rows; i ++)
            {
                for (j = 0U; j < columns; j ++)
                {
                    *pTempDataPtr = 0;
                    pTempDataPtr ++;
                }
            }
            /* Set data values in the centre row and column to 255  */
            pTempDataPtr = (void*)(uintptr_t)obj_desc->mem_ptr.host_ptr;
            pTempDataPtr = &(pTempDataPtr[((rows/2U))*columns]);
            for (i = 0U; i < columns; i ++)
            {
                *pTempDataPtr = 255;
                pTempDataPtr ++;
            }
            pTempDataPtr = (void*)(uintptr_t)obj_desc->mem_ptr.host_ptr;
            pTempDataPtr = &(pTempDataPtr[(columns/2U)]);
            for (i = 0U; i < rows; i ++)
            {
                *pTempDataPtr = 255;
                pTempDataPtr = &(pTempDataPtr[columns]);
            }
        }
        else if ((vx_enum)VX_PATTERN_DISK == pattern)
        {
            vx_uint8* mask = (vx_uint8*)(uintptr_t)obj_desc->mem_ptr.host_ptr;
            vx_int16 ref;

            for (i = 0U; i < rows; i ++)
            {
                for (j = 0U; j < columns; j ++)
                {
                    ref = (((((((vx_float32)i - ((vx_float32)rows / 2.0f)) + 0.5f) * (((vx_float32)i - ((vx_float32)rows / 2.0f) ) + 0.5f)) / (((vx_float32)rows / 2.0f) * ((vx_float32)rows / 2.0f))) +
                        (((((vx_float32)j - ((vx_float32)columns / 2.0f)) + 0.5f) * (((vx_float32)j - ((vx_float32)columns / 2.0f)) + 0.5f)) / (((vx_float32)columns / 2.0f) * ((vx_float32)columns / 2.0f))))
                        <= 1.0f) ? 255 : 0;

                    mask[j + (i * columns)] = (vx_uint8)ref;
                }
            }
        }
        else /* VS_PATTERN_OTHER */
        {
            for (i = 0U; i < rows; i ++)
            {
                for (j = 0U; j < columns; j ++)
                {
                    *pTempDataPtr = 0;
                    pTempDataPtr ++;
                }
            }
        }

        tivxCheckStatus(&status, tivxMemBufferUnmap((void*)(uintptr_t)obj_desc->mem_ptr.host_ptr,
            obj_desc->mem_size, (vx_enum)VX_MEMORY_TYPE_HOST,
            (vx_enum)VX_WRITE_ONLY));
    }

    return (matrix);
}

vx_status VX_API_CALL vxQueryMatrix(
    vx_matrix matrix, vx_enum attribute, void *ptr, vx_size size)
{
    vx_status status = (vx_status)VX_SUCCESS;
    tivx_obj_desc_matrix_t *obj_desc = NULL;

    if ((ownIsValidSpecificReference(vxCastRefFromMatrix(matrix), (vx_enum)VX_TYPE_MATRIX) == (vx_bool)vx_false_e)
        ||
        (matrix->base.obj_desc == NULL)
        )
    {
        VX_PRINT(VX_ZONE_ERROR, "Invalid matrix reference\n");
        status = (vx_status)VX_ERROR_INVALID_REFERENCE;
    }
    else
    {
        obj_desc = (tivx_obj_desc_matrix_t *)matrix->base.obj_desc;
        switch (attribute)
        {
            case (vx_enum)VX_MATRIX_TYPE:
                if (VX_CHECK_PARAM(ptr, size, vx_enum, 0x3U))
                {
                    *(vx_enum *)ptr = obj_desc->data_type;
                }
                else
                {
                    VX_PRINT(VX_ZONE_ERROR, "Query matrix type failed\n");
                    status = (vx_status)VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case (vx_enum)VX_MATRIX_COLUMNS:
                if (VX_CHECK_PARAM(ptr, size, vx_size, 0x3U))
                {
                    *(vx_size *)ptr = obj_desc->columns;
                }
                else
                {
                    VX_PRINT(VX_ZONE_ERROR, "Query matrix columns failed\n");
                    status = (vx_status)VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case (vx_enum)(vx_enum)VX_MATRIX_ROWS:
                if (VX_CHECK_PARAM(ptr, size, vx_size, 0x3U))
                {
                    *(vx_size *)ptr = obj_desc->rows;
                }
                else
                {
                    VX_PRINT(VX_ZONE_ERROR, "Query matrix rows failed\n");
                    status = (vx_status)VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case (vx_enum)VX_MATRIX_SIZE:
                if (VX_CHECK_PARAM(ptr, size, vx_size, 0x3U))
                {
                    *(vx_size *)ptr =
                        obj_desc->mem_size;
                }
                else
                {
                    VX_PRINT(VX_ZONE_ERROR, "Query matrix size failed\n");
                    status = (vx_status)VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case (vx_enum)VX_MATRIX_ORIGIN:
                if (VX_CHECK_PARAM(ptr, size, vx_coordinates2d_t, 0x3U))
                {
                    vx_coordinates2d_t *rect = (vx_coordinates2d_t *)ptr;

                    rect->x = obj_desc->origin_x;
                    rect->y = obj_desc->origin_y;
                }
                else
                {
                    VX_PRINT(VX_ZONE_ERROR, "Query matrix origin failed\n");
                    status = (vx_status)VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            case (vx_enum)VX_MATRIX_PATTERN:
                if (VX_CHECK_PARAM(ptr, size, vx_enum, 0x3U))
                {
                    *(vx_enum *)ptr = obj_desc->pattern;
                }
                else
                {
                    VX_PRINT(VX_ZONE_ERROR, "Query matrix pattern failed\n");
                    status = (vx_status)VX_ERROR_INVALID_PARAMETERS;
                }
                break;
            default:
                VX_PRINT(VX_ZONE_ERROR, "Invalid matrix query attribute\n");
                status = (vx_status)VX_ERROR_NOT_SUPPORTED;
                break;
        }
    }

    return status;
}

VX_API_ENTRY vx_status VX_API_CALL vxReleaseMatrix(vx_matrix *matrix)
{
    return (ownReleaseReferenceInt(
        vxCastRefFromMatrixP(matrix), (vx_enum)VX_TYPE_MATRIX, (vx_enum)VX_EXTERNAL, NULL));
}

vx_status VX_API_CALL vxCopyMatrix(
    vx_matrix matrix, void *user_ptr, vx_enum usage, vx_enum user_mem_type)
{
    vx_status status = (vx_status)VX_SUCCESS;
    vx_uint32 size;
    tivx_obj_desc_matrix_t *obj_desc = NULL;

    if ((ownIsValidSpecificReference(vxCastRefFromMatrix(matrix), (vx_enum)VX_TYPE_MATRIX) == (vx_bool)vx_false_e)
        ||
        (matrix->base.obj_desc == NULL)
        )
    {
        VX_PRINT(VX_ZONE_ERROR, "Invalid matrix reference\n");
        status = (vx_status)VX_ERROR_INVALID_REFERENCE;
    }
    else
    {
        obj_desc = (tivx_obj_desc_matrix_t *)matrix->base.obj_desc;
        if ((vx_enum)VX_MEMORY_TYPE_HOST != user_mem_type)
        {
            VX_PRINT(VX_ZONE_ERROR, "user mem type is not equal to VX_MEMORY_TYPE_HOST\n");
            status = (vx_status)VX_ERROR_INVALID_PARAMETERS;
        }

        /* Memory still not allocated */
        if (((vx_enum)VX_READ_ONLY == usage) &&
            ((uint64_t)0 == obj_desc->mem_ptr.host_ptr))
        {
            VX_PRINT(VX_ZONE_ERROR, "Memory is not allocated\n");
            status = (vx_status)VX_ERROR_INVALID_PARAMETERS;
        }

        if (NULL == user_ptr)
        {
            VX_PRINT(VX_ZONE_ERROR, "User pointer is NULL\n");
            status = (vx_status)VX_ERROR_INVALID_PARAMETERS;
        }
    }

    if ((vx_status)VX_SUCCESS == status)
    {
        size = obj_desc->mem_size;

        /* Copy from matrix object to user memory */
        if ((vx_enum)VX_READ_ONLY == usage)
        {
            tivxCheckStatus(&status, tivxMemBufferMap((void*)(uintptr_t)obj_desc->mem_ptr.host_ptr, size,
                (vx_enum)VX_MEMORY_TYPE_HOST, (vx_enum)VX_READ_ONLY));

            (void)memcpy(user_ptr, (void*)(uintptr_t)obj_desc->mem_ptr.host_ptr, size);

            tivxCheckStatus(&status, tivxMemBufferUnmap((void*)(uintptr_t)obj_desc->mem_ptr.host_ptr, size,
                (vx_enum)VX_MEMORY_TYPE_HOST, (vx_enum)VX_READ_ONLY));
        }
        else /* Copy from user memory to matrix object */
        {
            status = ownAllocReferenceBufferGeneric(&matrix->base);

            if ((vx_status)VX_SUCCESS == status)
            {
                tivxCheckStatus(&status, tivxMemBufferMap((void*)(uintptr_t)obj_desc->mem_ptr.host_ptr, size,
                    (vx_enum)VX_MEMORY_TYPE_HOST, (vx_enum)VX_WRITE_ONLY));

                (void)memcpy((void*)(uintptr_t)obj_desc->mem_ptr.host_ptr, user_ptr, size);

                tivxCheckStatus(&status, tivxMemBufferUnmap((void*)(uintptr_t)obj_desc->mem_ptr.host_ptr, size,
                    (vx_enum)VX_MEMORY_TYPE_HOST, (vx_enum)VX_WRITE_ONLY));
            }
        }
    }

    return (status);
}
