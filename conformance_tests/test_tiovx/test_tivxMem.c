/*

 * Copyright (c) 2012-2020 The Khronos Group Inc.
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

/*
 *
 * Copyright (c) 2020 Texas Instruments Incorporated
 *
 */

#include <TI/tivx_obj_desc.h>
#include "test_tiovx.h"

#define TIVX_TEST_FAIL_CLEANUP(x) {(x) = 1; goto cleanup;}
#define TIVX_TEST_UPDATE_STATUS(x) {if ((x)) CT_RecordFailure();}

#define TIVX_TEST_MAX_NUM_ADDR  (8U)

#define TIVX_TEST_SUCCESS                           (0)
#define TIVX_TEST_ERROR_STATUS_CHECK_FAILED         (-1)
#define TIVX_TEST_ERROR_POINTER_CHECK_FAILED        (-2)

typedef struct
{
    const char *name;
    vx_enum     type;
    uint32_t    aux;
} TestArg;

#define TIVX_TEST_ENTRY(X, v)   CT_GENERATE_PARAMETERS(#X, ARG, X, (v))

#define TEST_PARAMS \
    TIVX_TEST_ENTRY(VX_TYPE_IMAGE, VX_DF_IMAGE_RGB), \
    TIVX_TEST_ENTRY(VX_TYPE_IMAGE, VX_DF_IMAGE_RGBX), \
    TIVX_TEST_ENTRY(VX_TYPE_IMAGE, VX_DF_IMAGE_NV12), \
    TIVX_TEST_ENTRY(VX_TYPE_IMAGE, VX_DF_IMAGE_NV21), \
    TIVX_TEST_ENTRY(VX_TYPE_IMAGE, VX_DF_IMAGE_UYVY), \
    TIVX_TEST_ENTRY(VX_TYPE_IMAGE, VX_DF_IMAGE_IYUV), \
    TIVX_TEST_ENTRY(VX_TYPE_IMAGE, VX_DF_IMAGE_U8), \
    TIVX_TEST_ENTRY(VX_TYPE_TENSOR, 1), \
    TIVX_TEST_ENTRY(VX_TYPE_TENSOR, 2), \
    TIVX_TEST_ENTRY(VX_TYPE_TENSOR, 3), \
    TIVX_TEST_ENTRY(VX_TYPE_TENSOR, 4), \
    TIVX_TEST_ENTRY(VX_TYPE_USER_DATA_OBJECT, 1024), \
    TIVX_TEST_ENTRY(VX_TYPE_ARRAY, 100), \
    TIVX_TEST_ENTRY(VX_TYPE_CONVOLUTION, 9), \
    TIVX_TEST_ENTRY(VX_TYPE_MATRIX, 8), \
    TIVX_TEST_ENTRY(VX_TYPE_DISTRIBUTION, 100), \

TESTCASE(tivxMem, CT_VXContext, ct_setup_vx_context, 0)

static int32_t checkTranslation(const void *ptr, int32_t expectedStatus)
{
    void       *phyAddr;
    void       *virtAddr;
    uint64_t    dmaBufFd;
    uint32_t    size;
    uint32_t    testFail = 0;
    vx_enum     region;
    vx_status   vxStatus;
    int32_t     status;

    status = TIVX_TEST_SUCCESS;

    /* Translate 'ptr' which should hold virtual address, to the associated
     * 'fd' and 'phy' address.
     */
    vxStatus = tivxMemTranslateVirtAddr(ptr, &dmaBufFd, &phyAddr);

    if (vxStatus != expectedStatus)
    {
        VX_PRINT(VX_ZONE_ERROR,
                 "tivxMemTranslateVirtAddr() failed. Expecting status [%d] "
                 "but got [%d]\n", expectedStatus, vxStatus);
        status = TIVX_TEST_ERROR_STATUS_CHECK_FAILED;
    }

    if (vxStatus == (vx_status)VX_SUCCESS)
    {
        /* From the 'dmaBufFd', query the 'virtAddr' and check it against the
         * original 'ptr' we have allocated.
         */
        vxStatus = tivxMemTranslateFd(dmaBufFd, 0, &virtAddr, &phyAddr);

        if (vxStatus != expectedStatus)
        {
            VX_PRINT(VX_ZONE_ERROR,
                     "tivxMemTranslateFd() failed. Expecting status [%d] "
                     "but got [%d]\n", expectedStatus, vxStatus);
            status = TIVX_TEST_ERROR_STATUS_CHECK_FAILED;
        }
    }

    /* Compate 'ptr' and 'virtAddr'. These should be the same. */
    if ((vxStatus == (vx_status)VX_SUCCESS) && (virtAddr != ptr))
    {
        VX_PRINT(VX_ZONE_ERROR,
                 "The retrieved virtAddr does not match the original "
                 "allocated buffer address.\n");
        status = TIVX_TEST_ERROR_POINTER_CHECK_FAILED;
    }

    return status;
}

static vx_reference testTivxMemAllocObject(vx_context context, vx_enum type, uint32_t  aux)
{
    vx_status       vxStatus;
    vx_reference    ref;

    ref      = NULL;
    vxStatus = (vx_status)VX_SUCCESS;

    if (type == (vx_enum)VX_TYPE_IMAGE)
    {
        vx_image    image;

        image = vxCreateImage(context, 64, 48, aux);

        if (image == NULL)
        {
            VX_PRINT(VX_ZONE_ERROR, "vxCreateImage() failed.\n");
        }

        ref = (vx_reference)image;
    }
    else if (type == (vx_enum)VX_TYPE_TENSOR)
    {
        vx_tensor   tensor;
        vx_size     dims[TIVX_CONTEXT_MAX_TENSOR_DIMS];
        uint32_t    i;

        for (i = 0; i < aux; i++)
        {
            dims[i] = 100;
        }

        tensor = vxCreateTensor(context, aux, dims, VX_TYPE_UINT8, 0);

        if (tensor == NULL)
        {
            VX_PRINT(VX_ZONE_ERROR, "vxCreateTensor() failed.\n");
        }

        ref = (vx_reference)tensor;
    }
    else if (type == (vx_enum)VX_TYPE_USER_DATA_OBJECT)
    {
        vx_user_data_object obj;

        obj = vxCreateUserDataObject(context, NULL, aux, NULL);

        if (obj == NULL)
        {
            VX_PRINT(VX_ZONE_ERROR, "vxCreateUserDataObject() failed.\n");
        }

        ref = (vx_reference)obj;
    }
    else if (type == (vx_enum)VX_TYPE_ARRAY)
    {
        vx_array    array;

        array = vxCreateArray(context, VX_TYPE_COORDINATES3D, aux);

        if (array == NULL)
        {
            VX_PRINT(VX_ZONE_ERROR, "vxCreateArray() failed.\n");
        }

        ref = (vx_reference)array;
    }
    else if (type == (vx_enum)VX_TYPE_CONVOLUTION)
    {
        vx_convolution  conv;

        conv = vxCreateConvolution(context, aux, aux);

        if (conv == NULL)
        {
            VX_PRINT(VX_ZONE_ERROR, "vxCreateConvolution() failed.\n");
        }

        ref = (vx_reference)conv;
    }
    else if (type == (vx_enum)VX_TYPE_MATRIX)
    {
        vx_matrix   matrix;

        matrix = vxCreateMatrix(context, VX_TYPE_UINT32, aux, aux);

        if (matrix == NULL)
        {
            VX_PRINT(VX_ZONE_ERROR, "vxCreateMatrix() failed.\n");
        }

        ref = (vx_reference)matrix;
    }
    else if (type == (vx_enum)VX_TYPE_DISTRIBUTION)
    {
        vx_distribution  dist;

        dist = vxCreateDistribution(context, aux, 5, 200);

        if (dist == NULL)
        {
            VX_PRINT(VX_ZONE_ERROR, "vxCreateDistribution() failed.\n");
        }

        ref = (vx_reference)dist;
    }
    else
    {
        VX_PRINT(VX_ZONE_ERROR, "Unsupported type [%d].\n", type);
    }

    return ref;
}

static vx_status testTivxMemFreeObject(vx_reference ref, vx_enum type)
{
    vx_status   vxStatus;

    vxStatus = (vx_status)VX_SUCCESS;

    if (ref == NULL)
    {
        VX_PRINT(VX_ZONE_ERROR, "'ref' NULL.\n");
        vxStatus = (vx_status)VX_FAILURE;
    }

    if (vxStatus == (vx_status)VX_SUCCESS)
    {
        if (type == (vx_enum)VX_TYPE_IMAGE)
        {
            vx_image    image = (vx_image)ref;
            vxStatus = vxReleaseImage(&image);
        }
        else if (type == (vx_enum)VX_TYPE_TENSOR)
        {
            vx_tensor   tensor = (vx_tensor)ref;
            vxStatus = vxReleaseTensor(&tensor);
        }
        else if (type == (vx_enum)VX_TYPE_USER_DATA_OBJECT)
        {
            vx_user_data_object obj = (vx_user_data_object)ref;
            vxStatus = vxReleaseUserDataObject(&obj);
        }
        else if (type == (vx_enum)VX_TYPE_ARRAY)
        {
            vx_array    array = (vx_array)ref;
            vxStatus = vxReleaseArray(&array);
        }
        else if (type == (vx_enum)VX_TYPE_CONVOLUTION)
        {
            vx_convolution  conv = (vx_convolution)ref;
            vxStatus = vxReleaseConvolution(&conv);
        }
        else if (type == (vx_enum)VX_TYPE_MATRIX)
        {
            vx_matrix   matrix = (vx_matrix)ref;
            vxStatus = vxReleaseMatrix(&matrix);
        }
        else if (type == (vx_enum)VX_TYPE_DISTRIBUTION)
        {
            vx_distribution dist = (vx_distribution)ref;
            vxStatus = vxReleaseDistribution(&dist);
        }
        else
        {
            VX_PRINT(VX_ZONE_ERROR, "Unsupported type [%d].\n", type);
            vxStatus = (vx_status)VX_FAILURE;
        }
    }

    return vxStatus;
}

TEST(tivxMem, testTranslateAddrMemAlloc)
{
    void       *ptr;
    void       *phyAddr;
    void       *virtAddr;
    uint64_t    dmaBufFd;
    uint32_t    size;
    uint32_t    testFail = 0;
    vx_enum     region;
    int32_t     status;

    size   = 1024;
    region = TIVX_MEM_EXTERNAL;
    ptr = tivxMemAlloc(size, region);

    if (ptr == NULL)
    {
        VX_PRINT(VX_ZONE_ERROR, "tivxMemAlloc() failed.\n");
        TIVX_TEST_FAIL_CLEANUP(testFail);
    }

    /* Check address translation. */
    status = checkTranslation(ptr, VX_SUCCESS);

    if (status != TIVX_TEST_SUCCESS)
    {
        VX_PRINT(VX_ZONE_ERROR, "checkTranslation() failed.\n");
        TIVX_TEST_FAIL_CLEANUP(testFail);
    }

cleanup:
    if (ptr != NULL)
    {
        tivxMemFree(ptr, size, region);
    }

    TIVX_TEST_UPDATE_STATUS(testFail);
}

TEST(tivxMem, testTranslateAddrMalloc)
{
    void       *ptr;
    void       *phyAddr;
    void       *virtAddr;
    uint64_t    dmaBufFd;
    uint32_t    size;
    uint32_t    testFail = 0;
    vx_enum     region;
    int32_t     status;
    int32_t     expectedStatus;

    /* ALlocate a memory block using malloc(). */
    size   = 1024;
    ptr = malloc(size);

    if (ptr == NULL)
    {
        VX_PRINT(VX_ZONE_ERROR, "malloc() failed.\n");
        TIVX_TEST_FAIL_CLEANUP(testFail);
    }

    /* In PC environment, the memory translation functions always work. */
#if defined(PLATFORM_PC)
    expectedStatus = VX_SUCCESS;
#else
    expectedStatus = VX_FAILURE;
#endif

    /* Check address translation. The translation should fail since the memory
     * has not been allocated using ether tivxMemAlloc() or ion_alloc().
     */
    status = checkTranslation(ptr, expectedStatus);

    if (status != TIVX_TEST_SUCCESS)
    {
        VX_PRINT(VX_ZONE_ERROR, "checkTranslation() failed.\n");
        TIVX_TEST_FAIL_CLEANUP(testFail);
    }

cleanup:
    if (ptr != NULL)
    {
        free(ptr);
    }

    TIVX_TEST_UPDATE_STATUS(testFail);
}

TEST_WITH_ARG(tivxMem, testReferenceImportExport, TestArg, TEST_PARAMS)
{
    vx_context      context = context_->vx_context_;
    void           *virtAddr1[TIVX_TEST_MAX_NUM_ADDR] = {NULL};
    void           *virtAddr2[TIVX_TEST_MAX_NUM_ADDR] = {NULL};
    vx_reference    ref[2] = {NULL};
    uint32_t        testFail = 0;
    vx_enum         type = (vx_enum)arg_->type;
    uint32_t        maxNumAddr;
    uint32_t        numEntries1;
    uint32_t        numEntries2;
    uint32_t        i;
    vx_status       vxStatus;

    /* Allocate objects. Both these objects should not have any
     * internal memory allocated.
     */
    for (i = 0; i < 2; i++)
    {
        ref[i] = testTivxMemAllocObject(context, type, arg_->aux);

        if (ref[i] == NULL)
        {
            VX_PRINT(VX_ZONE_ERROR, "testTivxMemAllocObject(%d) failed.\n", i);
            TIVX_TEST_FAIL_CLEANUP(testFail);
        }
    }

    /* Export the handles from obj[0]. This forces the internal handles to be
     * allocated and returned.
     */
    maxNumAddr = TIVX_TEST_MAX_NUM_ADDR;
    vxStatus = tivxReferenceExportHandle(ref[0],
                                         virtAddr1,
                                         maxNumAddr,
                                         &numEntries1);

    if (vxStatus != (vx_status)VX_SUCCESS)
    {
        VX_PRINT(VX_ZONE_ERROR, "tivxReferenceExportHandle() failed.\n");
        TIVX_TEST_FAIL_CLEANUP(testFail);
    }

    /* Import the handles into obj[1]. */
    vxStatus = tivxReferenceImportHandle(ref[1],
                                         (const void **)virtAddr1,
                                         numEntries1);

    if (vxStatus != (vx_status)VX_SUCCESS)
    {
        VX_PRINT(VX_ZONE_ERROR, "tivxReferenceImportHandle() failed.\n");
        TIVX_TEST_FAIL_CLEANUP(testFail);
    }

    /* Check address translation. */
    for (i = 0; i < numEntries1; i++)
    {
        int32_t     status;

        status = checkTranslation(virtAddr1[i], VX_SUCCESS);

        if (status != TIVX_TEST_SUCCESS)
        {
            VX_PRINT(VX_ZONE_ERROR,
                     "checkTranslation() failed for virtAddr1[%d].\n", i);
            TIVX_TEST_FAIL_CLEANUP(testFail);
        }
    }

    /* Export the handles from obj[1]. */
    maxNumAddr = TIVX_TEST_MAX_NUM_ADDR;
    vxStatus = tivxReferenceExportHandle(ref[1],
                                         virtAddr2,
                                         maxNumAddr,
                                         &numEntries2);

    if (vxStatus != (vx_status)VX_SUCCESS)
    {
        VX_PRINT(VX_ZONE_ERROR, "tivxReferenceExportHandle() failed.\n");
        TIVX_TEST_FAIL_CLEANUP(testFail);
    }

    /* Check the number of entries. These should match. */
    if (numEntries1 != numEntries2)
    {
        VX_PRINT(VX_ZONE_ERROR, "Number of entries do not match.\n");
        TIVX_TEST_FAIL_CLEANUP(testFail);
    }

    /* Compare the addresses exported from the objects. These should match. */
    for (i = 0; i < numEntries1; i++)
    {
        if (virtAddr1[i] != virtAddr2[i])
        {
            VX_PRINT(VX_ZONE_ERROR, "Address entry [%d] mis-match.\n", i);
            TIVX_TEST_FAIL_CLEANUP(testFail);
        }
    }

    /* Two objects have the same handles, which will cause issues when the
     * objects are released. We need to remove the handles from one of the
     * objects. Let us remove them from obj[0]. We do this by importing NULL
     * handles.
     */
    for (i = 0; i < maxNumAddr; i++)
    {
        virtAddr1[i] = NULL;
    }

    /* Import NULL handles into obj[0]. */
    vxStatus = tivxReferenceImportHandle(ref[0],
                                         (const void **)virtAddr1,
                                         numEntries1);

    if (vxStatus != (vx_status)VX_SUCCESS)
    {
        VX_PRINT(VX_ZONE_ERROR, "tivxReferenceImportHandle(NULL) failed.\n");
        TIVX_TEST_FAIL_CLEANUP(testFail);
    }

cleanup:

    /* Free the objects. */
    for (i = 0; i < 2; i++)
    {
        vxStatus = testTivxMemFreeObject(ref[i], type);

        if (vxStatus != (vx_status)VX_SUCCESS)
        {
            VX_PRINT(VX_ZONE_ERROR, "testTivxMemFreeObject(%d) failed.\n", i);
            testFail = 1;
        }
    }

    TIVX_TEST_UPDATE_STATUS(testFail);
}

TEST(tivxMem, testReferenceImportNeg)
{
    vx_context      context = context_->vx_context_;
    void           *virtAddr[TIVX_TEST_MAX_NUM_ADDR] = {NULL};
    vx_reference    ref = NULL;
    uint32_t        testFail = 0;
    vx_enum         type = VX_TYPE_IMAGE;
    vx_df_image     format = VX_DF_IMAGE_YUV4;
    uint32_t        numEntries;
    uint32_t        i;
    vx_status       vxStatus;

    /* VX_DF_IMAGE_YUV4 has 3 planes so expects 3 address entries. */
    numEntries = 3;

    /* Import the handles into obj. Since 'ref' is NULL, the call should
     * fail.
     */
    vxStatus = tivxReferenceImportHandle(ref,
                                         (const void **)virtAddr,
                                         numEntries);

    if (vxStatus != (vx_status)VX_FAILURE)
    {
        VX_PRINT(VX_ZONE_ERROR, "tivxReferenceImportHandle() failed.\n");
        TIVX_TEST_FAIL_CLEANUP(testFail);
    }

    /* Allocate objects. Both these objects should not have any
     * internal memory allocated.
     */
    ref = testTivxMemAllocObject(context, type, format);

    if (ref == NULL)
    {
        VX_PRINT(VX_ZONE_ERROR, "testTivxMemAllocObject() failed.\n");
        TIVX_TEST_FAIL_CLEANUP(testFail);
    }

    /* Import the handles into obj. Since 'virtAddr' is NULL, the call should
     * fail.
     */
    vxStatus = tivxReferenceImportHandle(ref, NULL, numEntries);

    if (vxStatus != (vx_status)VX_FAILURE)
    {
        VX_PRINT(VX_ZONE_ERROR, "tivxReferenceImportHandle() failed.\n");
        TIVX_TEST_FAIL_CLEANUP(testFail);
    }

    /* Run the loop with [0..(numEntries+1)] values. */
    for (i = 0; i < numEntries+2; i++)
    {
        vx_status expected;

        if (i >= numEntries)
        {
            expected = (vx_status)VX_SUCCESS;
        }
        else
        {
            /* Since 'numEntries' is invalid, the call should fail. */
            expected = (vx_status)VX_FAILURE;
        }

        /* Import the handles into obj. */
        vxStatus = tivxReferenceImportHandle(ref, (const void **)virtAddr, i);

        if (vxStatus != (vx_status)expected)
        {
            VX_PRINT(VX_ZONE_ERROR, "tivxReferenceImportHandle() failed.\n");
            TIVX_TEST_FAIL_CLEANUP(testFail);
        }
    }

cleanup:

    /* Free the object. */
    vxStatus = testTivxMemFreeObject(ref, type);

    if (vxStatus != (vx_status)VX_SUCCESS)
    {
        VX_PRINT(VX_ZONE_ERROR, "testTivxMemFreeObject() failed.\n");
        testFail = 1;
    }

    TIVX_TEST_UPDATE_STATUS(testFail);
}

TEST(tivxMem, testReferenceExportNeg)
{
    vx_context      context = context_->vx_context_;
    void           *virtAddr[TIVX_TEST_MAX_NUM_ADDR] = {NULL};
    vx_reference    ref = NULL;
    uint32_t        testFail = 0;
    vx_enum         type = VX_TYPE_IMAGE;
    vx_df_image     format = VX_DF_IMAGE_YUV4;
    uint32_t        maxNumAddr;
    uint32_t        numEntries;
    uint32_t        i;
    vx_status       vxStatus;

    /* VX_DF_IMAGE_YUV4 has 3 planes so expects 3 address entries. */
    maxNumAddr = 3;

    /* Import the handles into obj. Since 'ref' is NULL, the call should
     * fail.
     */
    vxStatus = tivxReferenceExportHandle(ref,
                                         virtAddr,
                                         maxNumAddr,
                                         &numEntries);

    if (vxStatus != (vx_status)VX_FAILURE)
    {
        VX_PRINT(VX_ZONE_ERROR, "tivxReferenceExportHandle() failed.\n");
        TIVX_TEST_FAIL_CLEANUP(testFail);
    }

    /* Allocate objects. Both these objects should not have any
     * internal memory allocated.
     */
    ref = testTivxMemAllocObject(context, type, format);

    if (ref == NULL)
    {
        VX_PRINT(VX_ZONE_ERROR, "testTivxMemAllocObject() failed.\n");
        TIVX_TEST_FAIL_CLEANUP(testFail);
    }

    /* Import the handles into obj. Since 'virtAddr' is NULL, the call should
     * fail.
     */
    vxStatus = tivxReferenceExportHandle(ref, NULL, maxNumAddr, &numEntries);

    if (vxStatus != (vx_status)VX_FAILURE)
    {
        VX_PRINT(VX_ZONE_ERROR, "tivxReferenceExportHandle() failed.\n");
        TIVX_TEST_FAIL_CLEANUP(testFail);
    }

    /* Run the loop with [0..(numEntries+1)] values. */
    for (i = 0; i < numEntries+2; i++)
    {
        vx_status expected;

        if (i >= maxNumAddr)
        {
            expected = (vx_status)VX_SUCCESS;
        }
        else
        {
            /* Since 'maxNumAddr' is invalid, the call should fail. */
            expected = (vx_status)VX_FAILURE;
        }

        /* Import the handles into obj. */
        vxStatus = tivxReferenceExportHandle(ref, virtAddr, i, &numEntries);

        if (vxStatus != (vx_status)expected)
        {
            VX_PRINT(VX_ZONE_ERROR, "tivxReferenceExportHandle() failed.\n");
            TIVX_TEST_FAIL_CLEANUP(testFail);
        }
    }

cleanup:

    /* Free the object. */
    vxStatus = testTivxMemFreeObject(ref, type);

    if (vxStatus != (vx_status)VX_SUCCESS)
    {
        VX_PRINT(VX_ZONE_ERROR, "testTivxMemFreeObject() failed.\n");
        testFail = 1;
    }

    TIVX_TEST_UPDATE_STATUS(testFail);
}

TESTCASE_TESTS(tivxMem,
               testTranslateAddrMemAlloc,
               testTranslateAddrMalloc,
               testReferenceImportExport,
               testReferenceImportNeg,
               testReferenceExportNeg
)