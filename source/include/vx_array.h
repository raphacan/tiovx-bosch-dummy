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


#ifndef _VX_ARRAY_H_
#define _VX_ARRAY_H_


#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \file
 * \brief Implementation of Array object
 */

/*!
 * \brief Max possible mapping via vxMapArray supported
 *
 * \ingroup group_vx_array_cfg
 */
#define TIVX_ARRAY_MAX_MAPS     (16u)

/*!
 * \brief Information about a array mapping
 *
 * \ingroup group_vx_array
 */
typedef struct _tivx_array_map_info_t
{
    /*! \brief Address mapped via vxMapArray() */
    uint8_t *map_addr;
    /*! \brief Size of memory region mapped via vxMapArray() */
    vx_size  map_size;
    /*! \brief Type of access being done by user, see \ref vx_accessor_e */
    vx_enum usage;
} tivx_array_map_info_t;

/*!
 * \brief Array object internal state
 *
 * \ingroup group_vx_array
 */
typedef struct _vx_array
{
    /*! \brief reference object */
    tivx_reference_t base;

    /*! \brief Mapping done via vxMapArray() */
    tivx_array_map_info_t maps[TIVX_ARRAY_MAX_MAPS];
} tivx_array_t;



/*!
 * \brief function to initialize virtual array parameters
 *
 * \param arr       [in] virtual array reference
 * \param item_type [in] type of array items
 * \param capacity  [in] array size
 *
 * \return VX_SUCCESS on success
 *
 * \ingroup group_tivx_array
 */
vx_status ownInitVirtualArray(
    vx_array arr, vx_enum item_type, vx_size capacity);

#ifdef __cplusplus
}
#endif

#endif
