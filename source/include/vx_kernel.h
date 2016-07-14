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


#ifndef _VX_KERNEL_H_
#define _VX_KERNEL_H_


#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \file
 * \brief Implementation of Kernel object
 */


/*! \brief Maximum targets a kernel can run on */
#define TIVX_MAX_TARGETS_PER_KERNEL     (8u)

/*! \brief The internal representation of the attributes associated with a run-time parameter.
 * \ingroup group_vx_kernel
 */
typedef struct _tivx_signature_t {
    /*! \brief The array of directions */
    vx_enum        directions[TIVX_KERNEL_MAX_PARAMS];
    /*! \brief The array of types */
    vx_enum        types[TIVX_KERNEL_MAX_PARAMS];
    /*! \brief The array of states */
    vx_enum        states[TIVX_KERNEL_MAX_PARAMS];
    /*! \brief The number of items in both \ref tivx_signature_t::directions and \ref tivx_signature_t::types. */
    vx_uint32      num_parameters;
} tivx_signature_t;


/*!
 * \brief Kernel object internal state
 *
 * \ingroup group_vx_kernel
 */
typedef struct _vx_kernel
{
    /*! \brief reference object */
    tivx_reference_t base;

    /*! \brief name of kernel */
    vx_char        name[VX_MAX_KERNEL_NAME];
    /*! \brief enum associated with this kernel */
    vx_enum        enumeration;
    /*! \brief parameter signature of this kernel */
    tivx_signature_t signature;
    /*! \brief The parameters validator */
    vx_kernel_validate_f    validate;
    /*! \brief The initialization function */
    vx_kernel_initialize_f initialize;
    /*! \brief The deinitialization function */
    vx_kernel_deinitialize_f deinitialize;
    /*! \brief The pointer to the function to execute the kernel */
    vx_kernel_f             function;
    /*! \brief number of supported targets */
    vx_uint32               num_targets;
    /*! \brief target names, index 0 is the default or preferred target for this kernel */
    char                    target_name[TIVX_MAX_TARGETS_PER_KERNEL][TIVX_MAX_TARGET_NAME];

} tivx_kernel_t;


/*!
 * \brief Get default target ID associated with this kernel
 *
 * \return TIVX_TARGET_ID_INVALID if no target or invalid target associated with this kernel
 *
 * \ingroup group_vx_kernel
 */
tivx_target_id_e ownKernelGetDefaultTarget(vx_kernel kernel);

/*!
 * \brief Match user provided target with supported targets
 *        and return target ID on which to run this kernel
 *
 * \return TIVX_TARGET_ID_INVALID if valid target match not found
 *
 * \ingroup group_vx_kernel
 */
tivx_target_id_e ownKernelGetTarget(vx_kernel kernel, const char *target_string);


#ifdef __cplusplus
}
#endif

#endif
