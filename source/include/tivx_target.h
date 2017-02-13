/*
 *******************************************************************************
 *
 * Copyright (C) 2016 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 *******************************************************************************
 */


#ifndef TIVX_TARGET_H_
#define TIVX_TARGET_H_


#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \file
 * \brief Implementation of Target Object APIs
 */

/*!
 * \brief Max number of Target that can exist on a CPU
 * \ingroup group_tivx_target_cfg
 */
#define TIVX_TARGET_MAX_TARGETS_IN_CPU  (16u)

/*!
 * \brief Max depth of queue associated with target
 * \ingroup group_tivx_target_cfg
 */
#define TIVX_TARGET_MAX_JOB_QUEUE_DEPTH         (32u)

/*! \brief Target ID bit mask
 * \ingroup group_tivx_target
 */
#define TIVX_TARGET_INST_MASK (0xFu)

/*! \brief Target ID bit mask
 * \ingroup group_tivx_target
 */
#define TIVX_TARGET_INST_SHIFT (0x0u)

/*! \brief CPU ID bit mask
 * \ingroup group_tivx_target
 */
#define TIVX_CPU_ID_MASK    (0xFu)

/*! \brief CPU ID bit shift
 * \ingroup group_tivx_target
 */
#define TIVX_CPU_ID_SHIFT   (0x4)

/*! \brief Target ID bit mask
 * \ingroup group_tivx_target
 */
#define TIVX_TARGET_ID_MASK     (0xFFu)

/*! \brief Target ID bit shift
 * \ingroup group_tivx_target
 */
#define TIVX_TARGET_ID_SHIFT    (0x0u)

/*! \brief Make target based on CPU ID
 *
 *         Currently it assumes there are max 16 CPUs in system
 *         and max 16 targets on a given CPU in system
 *
 * \ingroup group_tivx_target
 */
#define TIVX_MAKE_TARGET_ID(cpu, target_inst)  ((vx_enum)(((((uint32_t)cpu)&TIVX_CPU_ID_MASK) << TIVX_CPU_ID_SHIFT) |((((uint32_t)target_inst)&TIVX_TARGET_INST_MASK) << TIVX_TARGET_INST_SHIFT)))

/*! \brief Get CPU Id from given target
 *
 *         CPU Id is stored in upper nibble of the lower 8bits of the target
 *
 * \ingroup group_tivx_target
 */
#define TIVX_GET_CPU_ID(target)                ((vx_enum)(((target)>>TIVX_CPU_ID_SHIFT)&TIVX_CPU_ID_MASK))

/*! \brief Get the target instances from target
 *
 *         Target Id is stored in lower nibble of 8bit target
 *
 * \ingroup group_tivx_target
 */
#define TIVX_GET_TARGET_INST(target)           ((uint16_t)(((target)>>TIVX_TARGET_INST_SHIFT)) & TIVX_TARGET_INST_MASK)


/*! \brief Const to denote invalid ID's
 * \ingroup group_tivx_target
 */
#define TIVX_TARGET_ID_INVALID      (0xFFFFFFFFu)

/*! \brief Const to denote invalid ID's
 * \ingroup group_tivx_target
 */
#define TIVX_CPU_ID_INVALID         (0xFFFFFFFFu)

/*! \brief Command ID for commands that can be send to a target
 * \ingroup group_tivx_target
 */
typedef enum _tivx_target_cmd_e {

    /*! \brief Command to create a node on target */
    TIVX_CMD_NODE_CREATE  = 0x00000001u,
    /*! \brief Command to delete a node on target */
    TIVX_CMD_NODE_DELETE  = 0x00000002u,
    /*! \brief Command to control a node on target */
    TIVX_CMD_NODE_CONTROL  = 0x00000003u,
    /*! \brief Command to call a user callback, on node execution complete */
    TIVX_CMD_NODE_USER_CALLBACK  = 0x00000004u,

} tivx_target_cmd_e;

/*! \brief Target Object */
typedef struct _tivx_target *tivx_target;

/*!
 * \brief Target object internal state
 *
 * \ingroup group_tivx_target
 */
typedef struct _tivx_target {

    /*! \brief ID of this target */
    vx_enum target_id;

    /*! \brief Handle to underlying task associate with this target */
    tivx_task task_handle;

    /*! \brief Parameters of task associated with this target
     */
    tivx_task_create_params_t task_params;

    /*! \brief handle to job queue associated with this target */
    tivx_queue job_queue_handle;

    /*! \brief queue memory */
    uint32_t job_queue_memory[TIVX_TARGET_MAX_JOB_QUEUE_DEPTH];

    /*! \brief target main */
    tivx_task_main_f target_main;

    /*! \brief Flag to request target to exit main so that it can be deleted */
    vx_bool targetExitRequest;

    /*! \brief Flag to indicate target exited its processing loop and is about to exit
     *   its main function
     */
    vx_bool targetExitDone;

} tivx_target_t;

/*!
 * \brief Parameters that can be set during target creation
 *
 * \ingroup group_tivx_target
 */
typedef struct _tivx_target_create_params
{
    /*! \brief Pointer to task stack, if NULL then task stack
     *         is allcoated by OS and not supplied by user
     */
    uint8_t *task_stack_ptr;

    /*! \brief Task stack size, if 0, OS allocates stack with default size
     */
    uint32_t task_stack_size;

    /*! \brief If task runs on a SMP CPU then this value tells the affinity
     *         of task to a given core,
     *         Valid values are 0 .. max cores in the SMP CPU.
     *         when TIVX_TASK_AFFINITY_ANY is used OS decides the task affinity.
     */
    uint32_t task_core_affinity;

    /*! \brief task priority for task associated with this target
     *         TIVX_TASK_PRI_HIGHEST is highest priority,
     *         TIVX_TASK_PRI_LOWEST is lowest priority
     */
    uint32_t task_priority;

} tivx_target_create_params_t;

/*!
 * \brief Get CPU ID associated with given target ID
 *
 * \param target_id [in] Target ID
 *
 * \return CPU ID if target found on some CPU
 * \return TIVX_CPU_ID_INVALID if target not found any CPU
 *
 * \ingroup group_tivx_target
 */
vx_enum tivxTargetGetCpuId(vx_enum target_id);

/*!
 * \brief Queue object descriptor to a given target
 *
 * \param target_id [in] Target ID
 * \param obj_desc_id [in] object descriptor ID
 *
 * \return VX_SUCCESS on success
 * \return VX_ERROR_NO_RESOURCES if queue is full
 * \return VX_FAILURE on other errors
 *
 * \ingroup group_tivx_target
 */
vx_status tivxTargetQueueObjDesc(vx_enum target_id, uint16_t obj_desc_id);

/*!
 * \brief Used to set default target create parameters in parameter structure
 *
 * \param params [out] Params to use for target creation
 *
 * \ingroup group_tivx_target
 */
void tivxTargetSetDefaultCreateParams(tivx_target_create_params_t *params);

/*!
 * \brief Create a target object and associate with given target ID
 *
 *        During system init, this API needs to be called for all targets local to this CPU
 *        Make sure two different CPUs do not associate against the same target ID
 *
 * \param target_id [in] Target ID
 * \param params [in] Params to use for target creation
 *
 * \ingroup group_tivx_target
 */
vx_status tivxTargetCreate(vx_enum target_id, tivx_target_create_params_t *params);


/*!
 * \brief Delete a target object
 *
 *        During system deinit, this API needs to be called for all targets local to this CPU
 *
 * \param target_id [in] Target ID
 *
 * \ingroup group_tivx_target
 */
vx_status tivxTargetDelete(vx_enum target_id);

/*!
 * \brief Init global state of target module
 *
 *        Also inits target kernel, target kernel instance
 *
 * \ingroup group_tivx_target
 */
void tivxTargetInit(void);

/*!
 * \brief De-Init global state of target module
 *
 *        Also de-inits target kernel, target kernel instance
 *
 * \ingroup group_tivx_target
 */
void tivxTargetDeInit(void);


#ifdef __cplusplus
}
#endif

#endif
