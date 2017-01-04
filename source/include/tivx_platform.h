/*
 *******************************************************************************
 *
 * Copyright (C) 2016 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 *******************************************************************************
 */


#ifndef _TIVX_PLATFORM_H_
#define _TIVX_PLATFORM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <tivx_obj_desc_priv.h>

/*!
 * \file
 * \brief Platform APIs
 */

/*!
 * \brief Types of system level locks
 *
 * \ingroup group_tivx_platform
 */
typedef enum {

    /*! \brief Lock the shared object descriptor table */
    TIVX_PLATFORM_LOCK_OBJ_DESC_TABLE = 0,

    /*! \brief Max number of locks */
    TIVX_PLATFORM_LOCK_MAX

} tivx_platform_lock_type_e;

/*!
 * \brief Convert a target name to a specific target ID
 *
 * \param target_name [in] Target name
 *
 * \return target ID
 *
 * \ingroup group_tivx_platform
 */
vx_enum tivxPlatformGetTargetId(const char *target_name);

/*!
 * \brief Match a user specified target_string with kernel suported target name
 *
 * \param kernel_target_name [in] Kernel supported target name
 * \param target_string [in] user specified target string
 *
 * \return vx_true_e if match found, else vx_false_e
 *
 * \ingroup group_tivx_platform
 */
vx_bool tivxPlatformTargetMatch(const char *kernel_target_name, const char *target_string);


/*!
 * \brief Return shared memory info which holds the object descriptors
 *
 *        This is platform APIs since method of specifying shared memory,
 *        number of object descriptors is platform dependant
 *
 * \ingroup group_tivx_platform
 */
void tivxPlatformGetObjDescTableInfo(tivx_obj_desc_table_info_t *table_info);

/*!
 * \brief Take a system level lock
 *
 *        This locks is taken across all targets to mutual exclusion
 *        across targets
 *
 * \ingroup group_tivx_platform
 */
void tivxPlatformSystemLock(vx_enum lock_id);

/*!
 * \brief Release system level lock
 *
 * \ingroup group_tivx_platform
 */
void tivxPlatformSystemUnlock(vx_enum lock_id);

/*!
 * \brief Get the time in micro seconds
 *
 * \ingroup group_tivx_platform
 */
uint64_t tivxPlatformGetTimeInUsecs();

/*!
 * \brief Init Platform module
 *
 * \ingroup group_tivx_platform
 */
vx_status tivxPlatformInit();

/*!
 * \brief DeInit Platform module
 *
 * \ingroup group_tivx_platform
 */
void tivxPlatformDeInit();

/*!
 * \brief Print given string
 *
 * \ingroup group_tivx_platform
 */
void tivxPlatformPrintf(const char *format);

#ifdef __cplusplus
}
#endif

#endif
