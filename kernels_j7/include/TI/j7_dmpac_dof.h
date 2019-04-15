/*
 *
 * Copyright (c) 2019 Texas Instruments Incorporated
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

#ifndef J7_DMPAC_DOF_H_
#define J7_DMPAC_DOF_H_

#include <VX/vx.h>
#include <VX/vx_kernels.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \file
 * \brief The DOF kernels in this kernel extension.
 */

/*! \brief dmpac_dof kernel name
 *  \ingroup group_vision_function_dmpac_dof
 */
#define TIVX_KERNEL_DMPAC_DOF_NAME     "com.ti.hwa.dmpac_dof"


/*********************************
 *      DMPAC_DOF Control Commands
 *********************************/

/*! \brief Control Command to set DOF HTS Bandwidth limit parameters
 *         User data object tivx_dmpac_dof_hts_bw_limit_params_t is passed
 *         as argument with this control command.
 *
 *  \ingroup group_vision_function_dmpac_dof
 */
#define TIVX_NODE_DMPAC_DOF_SET_HTS_BW_LIMIT_PARAMS        (0x10000000u)

/*! \brief Control Command to set DOF confidence score parameters
 *         User data object tivx_dmpac_dof_cs_tree_params_t is passed
 *         as argument with this control command.
 *
 *  \ingroup group_vision_function_dmpac_dof
 */
#define TIVX_NODE_DMPAC_DOF_CS_PARAMS                      (0x10000001u)

/*! \brief Control Command to get the error status
 *         Returns the error status of the last processed frame.
 *         Reference to vx_scalar is passed as argument with
 *         this control command.
 *         Node returns bit-mask of error status in u32 variable of vx_scalar.
 *
 *  \ingroup group_vision_function_dmpac_dof
 */
#define TIVX_NODE_DMPAC_DOF_GET_ERR_STATUS                 (0x10000002u)


/*********************************
 *      DMPAC_DOF PREDICTORS
 *********************************/
/** \brief No predictor used */
#define TIVX_NODE_DMPAC_DOF_PREDICTOR_NONE                  (0U)

 /** \brief Delayed left predictor */
#define TIVX_NODE_DMPAC_DOF_PREDICTOR_DELEY_LEFT            (1U)

/** \brief Temporal predictor, Need flow vector output from previous image pair
           as Temporal input, can only be set for base layer
 */
#define TIVX_NODE_DMPAC_DOF_PREDICTOR_TEMPORAL              (2U)

/** \brief Pyramidal left predictor, can be set for base and intermediate layers
 */
#define TIVX_NODE_DMPAC_DOF_PREDICTOR_PYR_LEFT              (3U)

/** \brief Pyramidal colocated predictor, can be set for base and intermediate
           layers
 */
#define TIVX_NODE_DMPAC_DOF_PREDICTOR_PYR_COLOCATED         (4U)


/*********************************
 *      DMPAC_DOF Defines
 *********************************/

/*! Error status for DOF VBUSM Read interface error */
#define TIVX_NODE_DMPAC_DOF_RD_ERR                         (0x04U)
/*! Error status for DOF VBUSM Write interface error */
#define TIVX_NODE_DMPAC_DOF_WR_ERR                         (0x08U)
/*! Error status for MP0 read error */
#define TIVX_NODE_DMPAC_DOF_MP0_RD_STATUS_ERR              (0x10U)
/*! Error status for FOCO SL2 VBSUM write interface error */
#define TIVX_NODE_DMPAC_DOF_FOCO0_SL2_WR_ERR               (0x400000U)
/*! Error status for FOCO SL2 VBSUM read interface error */
#define TIVX_NODE_DMPAC_DOF_FOCO0_VBUSM_RD_ERR             (0x200000U)

/*********************************
 *      DMPAC_DOF STRUCTURES
 *********************************/

/*!
 * \brief The configuration data structure used by the
          TIVX_KERNEL_DMPAC_DOF kernel.
 *
 * \ingroup group_vision_function_dmpac_dof
 */
typedef struct {
    /*! Range [0, 62]   Recommended = 48, 48
        if horizontal_search_range is 191 then
        (vertical_search_range[0] (upward) +
            vertical_search_range[1] (downward) <= 112)
    */
    uint16_t  vertical_search_range[2];
    /*! Range [0, 191] Recommended value = 191
        if vertical_search_range[0] = vertical_search_range[1] = 62
        then horizontal_search_range <= 170
    */
    uint16_t  horizontal_search_range;
    /*! 0: Disabled; 1: Enable post-processing median filter [recommended] */
    uint16_t  median_filter_enable;
    /*! Range [0, 31]   recommended = 24 */
    uint16_t  motion_smoothness_factor;
    /*! 0 = Motion neutral, 5x5 Census Transform
        1 = Forward motion
        2 = Reverse motion
        3 = Motion neutral, 7x Census Transform */
    uint16_t  motion_direction;
    /*! Predictor for the base image */
    uint16_t  base_predictor[2];
    /*! Predictor for the intermediate pyramid level  */
    uint16_t  inter_predictor[2];
    /*! IIR filter alpha value recommended = 0x66 */
    uint16_t  iir_filter_alpha;
    /*! 0 = Disable LK for Base pyramid level, 1 = Enable LK for base
        disabling LK for base will generate 2 byte per pixel flow vector
        recommended = 1 (enable)
    */
    uint16_t  enable_lk;
    /*! Maximum possible number of enabled pixel in row if sparse optical flow
        is enabled. Flow vector is generated for only sof_max_pix_in_row pixel
        per row
    */
    uint16_t  sof_max_pix_in_row;
    /*! Valid only if sparse optical flow is enabled.
        Number of paxel row with at least one enabled pixel. Paxel row is pair
        of image row (eg. row 1 & 2, row 3 & 4 etc).
    */
    uint16_t  sof_fv_height;
} tivx_dmpac_dof_params_t;

/*!
 * \brief Configuration parameters used to calculate Confidence Score used by
          TIVX_KERNEL_DMPAC_DOF kernel.
 *
 * \ingroup group_vision_function_dmpac_dof
 */
typedef struct {
    /*! Confidence score gain parameters */
    uint32_t  cs_gain;
    /*! Index value for Confidence Score Decision Tree */
    uint32_t  decision_tree_index[16][3];
    /*! Threshold value for Confidence Score Decision Tree */
    uint32_t  decision_tree_threshold[16][3];
    /*! Weights value for Confidence Score Decision Tree */
    uint32_t  decision_tree_weight[16][4];
} tivx_dmpac_dof_cs_tree_params_t;

/*!
 * \brief Configuration parameters used to set HTS BW limit used by
          TIVX_KERNEL_DMPAC_DOF kernel.
 *
 * \ingroup group_vision_function_dmpac_dof
 */
typedef struct {
    /*! 0 = Disable HTS BW limiter, 1 = Enable */
    uint32_t  enable_hts_bw_limit;
    /*! Cycle count between each HTS trigger */
    uint32_t  cycle_cnt;
    /*! Count for which cycle_cnt average is calculated */
    uint32_t  token_cnt;
} tivx_dmpac_dof_hts_bw_limit_params_t;

/*********************************
 *      Function Prototypes
 *********************************/

/*!
 * \brief Function to register HWA Kernels on the dmpac_dof Target
 * \ingroup group_vision_function_hwa
 */
void tivxRegisterHwaTargetDmpacDofKernels(void);

/*!
 * \brief Function to un-register HWA Kernels on the dmpac_dof Target
 * \ingroup group_vision_function_hwa
 */
void tivxUnRegisterHwaTargetDmpacDofKernels(void);


/*! \brief [Graph] Creates a DMPAC_DOF Node.
 *
 * - The data format of image within pyramid MUST be <tt>\ref VX_DF_IMAGE_U8</tt>,
 *   <tt>\ref TIVX_DF_IMAGE_U16</tt>, or <tt>\ref TIVX_DF_IMAGE_P12</tt> format.
 * - The pyramid MUST use scale of VX_SCALE_PYRAMID_HALF
 * - The max number of pyramid levels can be 6
 * - The width and height of base level MUST be interger multiple of 2^pyramidlevels
 * - The meta properties of input_current, input_reference MUST be identical
 * - If the optional input_current_base and input_reference_base is used, then
 *   the base size of the respective pyramid paramters should be half the width
 *   and height of the base images.
 *
 * \param [in] graph The reference to the graph.
 * \param [in] configuration The input object of a single params structure of
 *             type <tt>\ref tivx_dmpac_dof_params_t</tt>.
 * \param [in] input_current_base   (optional) Current input base image of pyramid
 *             (if not included in input_current pyramid).
 * \param [in] input_reference_base (optional) Reference input base image of pyramid
 *             (if not included in input_reference pyramid).
 * \param [in] input_current Current input pyramid.
 * \param [in] input_reference Reference input pyramid.
 * \param [in] flow_vector_in (optional) Flow vector from previous execution of DOF.
 *             Size of image is base_width x base_height x 32bpp.
 *             Use <tt>\ref VX_DF_IMAGE_U32 </tt> dataformat.
 * \param [in] sparse_of_map (optional) Sparse OF bit-mask of size
 *             base_width/8 x base_height x 1bpp.
 *             Use <tt>\ref VX_DF_IMAGE_U8 </tt> dataformat.
 * \param [out] flow_vector_out Flow vector output.
 *              Size of image is base_width x base_height x 32bpp.
 *              Use <tt>\ref VX_DF_IMAGE_U32 </tt> dataformat.
 * \param [out] confidence_histogram (optional) Confidence histogram.
 *              Distribution meta properties, num_bins = 16, offset = 0, range = 16.
 * \see <tt>TIVX_KERNEL_DMPAC_DOF_NAME</tt>
 * \ingroup group_vision_function_dmpac_dof
 * \return <tt>\ref vx_node</tt>.
 * \retval vx_node A node reference.
 *         Any possible errors preventing a successful creation should be
 *         checked using <tt>\ref vxGetStatus</tt>
 */
VX_API_ENTRY vx_node VX_API_CALL tivxDmpacDofNode(vx_graph graph,
                                      vx_user_data_object  configuration,
                                      vx_image             input_current_base,
                                      vx_image             input_reference_base,
                                      vx_pyramid           input_current,
                                      vx_pyramid           input_reference,
                                      vx_image             flow_vector_in,
                                      vx_image             sparse_of_map,
                                      vx_image             flow_vector_out,
                                      vx_distribution      confidence_histogram);

/*!
 * \brief Function to initialize DOF parameters with default value
 * \ingroup group_vision_function_dmpac_dof
 */
void tivx_dmpac_dof_params_init(tivx_dmpac_dof_params_t *prms);

/*!
 * \brief Function to initialize DOF HTS bandwidth limit parameters with default value
 * \ingroup group_vision_function_dmpac_dof
 */
void tivx_dmpac_dof_hts_bw_limit_params_init(
                                    tivx_dmpac_dof_hts_bw_limit_params_t *prms);

#ifdef __cplusplus
}
#endif

#endif /* J7_DMPAC_DOF_H_ */


