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



/**
 * \file vx_tutorial_tidl.c Executes the inference of a deep learning network.
 *   It first reads the configuration file 'tidl/tidl_infer.cfg' located in directory test_data and that contains the following information:
 *
 * - path to network model's parameter file generated by the import tools
 *
 * - path to network's file generated by the import tools
 *
 * - path to the input file, usually a grayscale or color image
 *
 * - path to the output file, that will contain the output from the last layer. Not used in the current version of the tutorial.
 *
 * - mode of operation (0:classifier or 1:object detection). Only taken into account for formatting the display on the console window.
 *   Currently only 0:classifier is supported.
 *
 * - processing_core_mode: Specify how the network will be processed if multiple processing cores exist in the system.
 *   0 (default): all cores can be utilized according to each layer's groupID. If a layer's group ID is 1 then it will run on EVE1. If it is 2, it will run on DSP1.
 *   1: The entire network will run on EVE1, even the layers which have group ID 2 (DSP layers).
 *   2: The entire network will run on DSP1, even the layers which have group ID 1 (EVE layers).
 *
 *
 *   All paths are relative to the test_data folder
 *
 *   Using the parameters from the configuration file, vx_tutorial_tidl() will then apply the network model on the input data
 *   and display the result on the console window, which consists of the classification top-5 results.
 *
 *   In this tutorial we learn the below concepts:
 * - How to create OpenVX context, OpenVX user data object and OpenVX tensor objects.
 * - How to read a data file and load the values into the user data object
 * - How to read a data file and load the values into a tensor object
 * - How to create OpenVX node and associate it with previously created graph
 * - How to schedule OpenVX graph for execution then execute the graph
 * - How to cleanup all created resources and exit the OpenVX application
 *
 * To include OpenVX interfaces include below file
 * \code
 * #include <VX/vx.h>
 * \endcode
 *
 * Follow the comments in the function vx_tutorial_tidl()
 * to understand this tutorial
 *
 */

#include <TI/tivx.h>
#include <tivx_utils_file_rd_wr.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <float.h>
#include <math.h>

#include "../../../common/xdais_types.h" /* In TIDL_PATH directory */
#include "sTIDL_IOBufDesc.h"
#include "tivx_tidl_utils.h"

#include "itidl_ti.h"
#include "vx_tutorial_tidl.h"
#include "test_engine/test_utils.h"

#define MAX(_a,_b) (((_a) > (_b)) ? (_a) : (_b))

#define CFG_FILE_NAME       "tivx/tidl/tidl_infer.cfg"

typedef struct {
  char tidl_params_file_path[VX_TUTORIAL_MAX_FILE_PATH];
  char tidl_network_file_path[VX_TUTORIAL_MAX_FILE_PATH];
  char input_file_path[VX_TUTORIAL_MAX_FILE_PATH];
  char output_file_path[VX_TUTORIAL_MAX_FILE_PATH];
  uint32_t operation_mode;
  uint32_t processing_core_mode;
} VxTutorialTidl_CfgObj;

VxTutorialTidl_CfgObj gCfgObj;

static vx_status parse_cfg_file(VxTutorialTidl_CfgObj *obj, char *cfg_file_name);
static vx_status createInputTensors(vx_context context, vx_user_data_object config, vx_tensor *input_tensors);
static vx_status createOutputTensor(vx_context context, vx_user_data_object config, vx_tensor *output_tensors);
static vx_status readInput(vx_context context, vx_user_data_object config, vx_tensor *input_tensors, char *input_file, uint32_t operation_mode);
static void displayOutput(void *bmp_context, vx_df_image df_image, void *data_ptr, vx_uint32 img_width, vx_uint32 img_height, vx_uint32 img_stride, vx_user_data_object config, vx_tensor *output_tensors, char *output_file, uint32_t operation_mode);

void vx_tutorial_tidl()
{

  vx_context context;
  vx_user_data_object  config1, config2, realConfig;
  vx_user_data_object  network;
  vx_tensor input_tensors[VX_TUTORIAL_MAX_TENSORS];
  vx_tensor output_tensors1[VX_TUTORIAL_MAX_TENSORS];
  vx_tensor output_tensors2[VX_TUTORIAL_MAX_TENSORS];
  vx_array intermDataQ, inDataQ1, outDataQ2;
  vx_tensor *real_output_tensors;
  vx_perf_t perf_graph, perf_node1, perf_node2;
  int32_t i;

  size_t sizeFilePath;
  char filePath[MAXPATHLENGTH];
  const char *targetCore1, *targetCore2;
  vx_enum targetCpuId1, targetCpuId2;

  vx_status status = VX_SUCCESS;

  VxTutorialTidl_CfgObj *obj = &gCfgObj;

  vx_graph graph = 0;
  vx_node node1 = 0;
  vx_node node2 = 0;
  vx_kernel kernel1 = 0;
  vx_kernel kernel2 = 0;

  uint32_t num_input_tensors  = 0;
  uint32_t num_output_tensors1 = 0;
  uint32_t num_output_tensors2 = 0;

  printf(" vx_tutorial_tidl: Tutorial Started !!! \n");

  context = vxCreateContext();
  VX_TUTORIAL_ASSERT_VALID_REF(context);

  vxDirective((vx_reference)context, VX_DIRECTIVE_ENABLE_PERFORMANCE);

  sizeFilePath = snprintf(filePath, MAXPATHLENGTH, "%s/%s", ct_get_test_file_path(), CFG_FILE_NAME);

  if (sizeFilePath > MAXPATHLENGTH) {
    printf("Error: path of config gile too long to fit in string\n");
    goto exit;
  }

  printf(" Reading config file %s ...\n", filePath);

  status= parse_cfg_file(obj, filePath);
  if (status!=VX_SUCCESS) {
    goto exit;
  }


  printf(" Reading network file %s ...\n", obj->tidl_network_file_path);

  network = vx_tidl_utils_readNetwork(context, &obj->tidl_network_file_path[0]);
  VX_TUTORIAL_ASSERT_VALID_REF(network)

  /*
   *   Processing_core_mode: Specify how the network will be processed if multiple processing cores exist in the system.
   *   0 (default): all cores can be utilized according to each layer's groupID. If a layer's group ID is 1 then it will run on EVE1. If it is 2, it will run on DSP1.
   *   1: The entire network will run on EVE1, even the layers which have group ID 2 (DSP layers).
   *   2: The entire network will run on DSP1, even the layers which have group ID 1 (EVE layers).
   *
   */
#ifdef HOST_EMULATION
  /* In host emulation on PC, it is not possible to test for processing_core_mode=2
   * but we can test test processing_core_mode=1, which gives a high confidence that
   * processing_core_mode=2 works as well.
   * For a definitive testing of processing_core_mode=2, test on target.
   */
  if (obj->processing_core_mode== 2) {
    obj->processing_core_mode= 1;
  }
#endif

  if (obj->processing_core_mode== 0){

    /*
     * In case the network has only one group of layer, assign the first core and disable the second core
     * In case there are 2 groups of layers. 1st group is always assigned to EVE and second group always assigned to DSP
     * */
    int32_t layersGroupCount[TIVX_CPU_ID_MAX];
    int32_t numLayersGroup= vx_tidl_utils_countLayersGroup(network, layersGroupCount);

    if (numLayersGroup== 1) {
      if (layersGroupCount[1]!=0) {
        targetCore1= TIVX_TARGET_EVE1;
        targetCpuId1= TIVX_CPU_ID_EVE1;
      }
      else if (layersGroupCount[2]!=0) {
        targetCore1= TIVX_TARGET_DSP1;
        targetCpuId1= TIVX_CPU_ID_DSP1;
      }
      else {
        printf(" Invalid layer group ID detected, exiting ...\n");
        goto exit;
      }
      targetCore2= NULL;
      targetCpuId2= TIVX_INVALID_CPU_ID;
    }
    else if (numLayersGroup== 2) {
      targetCore1= TIVX_TARGET_EVE1;
      targetCpuId1= TIVX_CPU_ID_EVE1;
      targetCore2= TIVX_TARGET_DSP1;
      targetCpuId2= TIVX_CPU_ID_DSP1;
    }
    else {
      printf(" Invalid number of groups of layers, exiting ...\n");
      goto exit;
    }

  }
  else if (obj->processing_core_mode== 1) {
    targetCore1= TIVX_TARGET_EVE1;
    targetCpuId1= TIVX_CPU_ID_EVE1;
    targetCore2= NULL;
    targetCpuId2= TIVX_INVALID_CPU_ID;
  }
  else if (obj->processing_core_mode== 2) {
    targetCore1= TIVX_TARGET_DSP1;
    targetCpuId1= TIVX_CPU_ID_DSP1;
    targetCore2= NULL;
    targetCpuId2= TIVX_INVALID_CPU_ID;
  }
  else {
    printf("Invalid processing core mode, exiting ...\n");
    goto exit;
  }

  /* If processing_core_mode is not 0, update each layer's group ID so that the entire network runs either on EVE or DSP*/
  if (obj->processing_core_mode!= 0) {
    vx_tidl_utils_updateLayersGroup(network, targetCpuId1);
  }

  config1 = vx_tidl_utils_getConfig(context, network, &num_input_tensors, &num_output_tensors1, targetCpuId1);

  /* In case the network runs on one CPU, set num_output_tensors2 to 0 */
  if (targetCpuId2== TIVX_INVALID_CPU_ID) {
    num_output_tensors2= 0;
  }
  else {
    int32_t num_interm_tensors= num_output_tensors1;

    config2 = vx_tidl_utils_getConfig(context, network, &num_output_tensors1, &num_output_tensors2, targetCpuId2);

    if (num_interm_tensors != num_output_tensors1) {
      printf("Number of output tensors from first group of layers not equal to the number of input tensors from second group of layers. Exiting ...\n");
      goto exit;
    }
  }

  printf(" Reading network params file %s ...\n", obj->tidl_params_file_path);

  status= vx_tidl_utils_readParams(network, &obj->tidl_params_file_path[0]);
  VX_TUTORIAL_ASSERT(status==VX_SUCCESS);

  kernel1 = tivxAddKernelTIDL(context, num_input_tensors, num_output_tensors1);
  VX_TUTORIAL_ASSERT_VALID_REF(kernel1)

  if (targetCpuId2!= TIVX_INVALID_CPU_ID) {
    kernel2 = tivxAddKernelTIDL(context, num_output_tensors1, num_output_tensors2);
    VX_TUTORIAL_ASSERT_VALID_REF(kernel2)
  }

  printf(" Create graph ... \n");

  /* Create OpenVx Graph */
  graph = vxCreateGraph(context);
  VX_TUTORIAL_ASSERT_VALID_REF(graph)

  printf(" Create input and output tensors for node 1... \n");
  /* Create array of input tensors for the first node */
  status= createInputTensors(context, config1, input_tensors);
  VX_TUTORIAL_ASSERT(status==VX_SUCCESS);

  /* Create array of output tensors for the first node, which is also the input tensors for the second node */
  status= createOutputTensor(context, config1, output_tensors1);
  VX_TUTORIAL_ASSERT(status==VX_SUCCESS);

  /* A intermDataQ must be passed as input and output parameters
   * Each element of the array represents the scaling factor in Q8 format
   * by which each 8-bits element of an input or output tensor needs to be divided with in order
   * to obtain its real value.
   * As input, intermDataQ is only used if the network that is executed takes its input tensor from a previous network.
   * As output dataQ array will return the scaling factor to be applied to each output tensor.
   * If a graph is composed of multiple TI-DL nodes that executes a sequence of networks in which each network's input corresponds to the output of the immediately previous network
   * then the same intermDataQ should be passed as parameter to each TI_DL node in order to ensure that the intermediate outputs are properly scaled.
   */
  intermDataQ = vxCreateArray(context, VX_TYPE_INT32, VX_TUTORIAL_MAX_TENSORS);
  inDataQ1 = vxCreateArray(context, VX_TYPE_INT32, VX_TUTORIAL_MAX_TENSORS);
  outDataQ2 = vxCreateArray(context, VX_TYPE_INT32, VX_TUTORIAL_MAX_TENSORS);

  printf(" Create node 1... \n");

  node1 = tivxTIDLNode(graph, kernel1, config1, network,
      input_tensors, inDataQ1,
      output_tensors1, intermDataQ
      );
  VX_TUTORIAL_ASSERT_VALID_REF(node1)

  /* Set target node to targetCore1 (EVE1 or DSP1)*/
  vxSetNodeTarget(node1, VX_TARGET_STRING, targetCore1);

  if (targetCpuId2== TIVX_CPU_ID_DSP1) {
    printf(" Create output tensors for node 2... \n");

    /* Create array of output tensors for the second node */
    status= createOutputTensor(context, config2, output_tensors2);
    VX_TUTORIAL_ASSERT(status==VX_SUCCESS);

    printf(" Create node 2... \n");

    node2 = tivxTIDLNode(graph, kernel2, config2, network,
        output_tensors1, intermDataQ,
        output_tensors2, outDataQ2
        );
    VX_TUTORIAL_ASSERT_VALID_REF(node2)

    /* Set target node to targetCore1 (EVE1 or DSP1)*/
    vxSetNodeTarget(node2, VX_TARGET_STRING, targetCore2);
  }

  printf(" Verify graph ... \n");
  /* Verify the TIDL Graph
   * When executed in host emulation on PC, the version of TI-DL library linked displays information about each layer of the network.
   * In target execution, such display is disabled in the library.
   * */
  status = vxVerifyGraph(graph);
  VX_TUTORIAL_ASSERT(status==VX_SUCCESS);

  if(VX_SUCCESS == status) {

    /* Read input from file and populate the input tensor #0, we assume here that only one input tensor is used */
    status= readInput(context, config1, &input_tensors[0], &obj->input_file_path[0], obj->operation_mode);
    VX_TUTORIAL_ASSERT(status==VX_SUCCESS);

    if (status!=VX_SUCCESS) {
      goto exit;
    }

    printf(" Execute graph ... \n");
    /* Execute the network */
    status = vxProcessGraph(graph);
    VX_TUTORIAL_ASSERT(status==VX_SUCCESS);

    /* Display the output_tensors1 if graph runs 1 cores */
    if (targetCpuId2== TIVX_INVALID_CPU_ID) {
      real_output_tensors= &output_tensors1[0];
      realConfig= config1;
    }
    else { /* Display the output_tensors2 if graph runs 2 cores */
      real_output_tensors= &output_tensors2[0];
      realConfig= config2;
    }

    displayOutput(NULL, (vx_df_image)NULL, NULL, 0, 0, 0, realConfig, real_output_tensors, &obj->output_file_path[0], obj->operation_mode);

  }

  vxQueryNode(node1, VX_NODE_PERFORMANCE, &perf_node1, sizeof(perf_node1));
  printf("\n---- Node 1 (%s) Execution time: %4.6f ms\n", (targetCpuId1== TIVX_CPU_ID_EVE1) ? "EVE" : "DSP" , perf_node1.min/1000000.0);

  if(node2 != 0) {
    vxQueryNode(node2, VX_NODE_PERFORMANCE, &perf_node2, sizeof(perf_node2));
    printf("---- Node 2 (%s) Execution time: %4.6f ms\n", (targetCpuId2== TIVX_CPU_ID_EVE1) ? "EVE" : "DSP" , perf_node2.min/1000000.0);
  }

  vxQueryGraph(graph, VX_GRAPH_PERFORMANCE, &perf_graph, sizeof(perf_graph));
  printf("\n---- Total Graph Execution time: %4.6f ms\n", perf_graph.min/1000000.0);

  vxReleaseNode(&node1);

  if (node2 !=0 ){
    vxReleaseNode(&node2);
    vxReleaseUserDataObject(&config2);
  }

  vxReleaseGraph(&graph);

  vxReleaseUserDataObject(&config1);

  vxReleaseUserDataObject(&network);

  for (i= 0; i < num_input_tensors; i++) {
    vxReleaseTensor(&input_tensors[i]);
  }

  for (i= 0; i < num_output_tensors1; i++) {
    vxReleaseTensor(&output_tensors1[i]);
  }

  for (i= 0; i < num_output_tensors2; i++) {
    vxReleaseTensor(&output_tensors2[i]);
  }

  vxReleaseArray(&intermDataQ);
  vxReleaseArray(&inDataQ1);
  vxReleaseArray(&outDataQ2);

  vxRemoveKernel(kernel1);
  if (kernel2!=0){
    vxRemoveKernel(kernel2);
  }

  exit:
  printf("\n vx_tutorial_tidl: Tutorial Done !!! \n");
  printf(" \n");

  vxReleaseContext(&context);

}

static vx_status parse_cfg_file(VxTutorialTidl_CfgObj *obj, char *cfg_file_name)
{
  FILE *fp = fopen(cfg_file_name, "r");
  char line_str[1024];
  char *token;
  size_t sizeFilePath;
  char filePath[MAXPATHLENGTH];
  vx_status status = VX_SUCCESS;

  /* Set processing_core_mode to 0, which means network can be partitioned accross all cores */
  obj->processing_core_mode= 0;

  if(fp==NULL)
  {
    printf("# ERROR: Unable to open config file [%s]\n", cfg_file_name);
#ifdef HOST_EMULATION
    printf("# ERROR: Please make sure that the environment variable VX_TEST_DATA_PATH is set to .../conformance_tests/test_data\n");
#endif
    status= VX_FAILURE;
    goto exit;
  }

  while(fgets(line_str, sizeof(line_str), fp)!=NULL)
  {
    char s[]=" \t";

    if (strchr(line_str, '#'))
    {
      continue;
    }

    /* get the first token */
    token = strtok(line_str, s);

    if(strcmp(token, "tidl_params_file_path")==0)
    {
      token = strtok(NULL, s);
      token[strlen(token)-1]=0;
      sizeFilePath = snprintf(filePath, MAXPATHLENGTH, "%s/%s", ct_get_test_file_path(), token);
      if (sizeFilePath > MAXPATHLENGTH) {
        printf("Error in parse_cfg_file, path too long to fit in string\n");
      }
      else {
        strcpy(obj->tidl_params_file_path, filePath);
      }
    }
    else
      if(strcmp(token, "tidl_network_file_path")==0)
      {
        token = strtok(NULL, s);
        token[strlen(token)-1]=0;
        sizeFilePath = snprintf(filePath, MAXPATHLENGTH, "%s/%s", ct_get_test_file_path(), token);
        if (sizeFilePath > MAXPATHLENGTH) {
          printf("Error in parse_cfg_file, path too long to fit in string\n");
        }
        else {
          strcpy(obj->tidl_network_file_path, filePath);
        }
      }
      else
        if(strcmp(token, "input_file_path")==0)
        {
          token = strtok(NULL, s);
          token[strlen(token)-1]=0;
          sizeFilePath = snprintf(filePath, MAXPATHLENGTH, "%s/%s", ct_get_test_file_path(), token);
          if (sizeFilePath > MAXPATHLENGTH) {
            printf("Error in parse_cfg_file, path too long to fit in string\n");
          }
          else {
            strcpy(obj->input_file_path, filePath);
          }
        }
        else
          if(strcmp(token, "output_file_path")==0)
          {
            token = strtok(NULL, s);
            token[strlen(token)-1]=0;
            sizeFilePath = snprintf(filePath, MAXPATHLENGTH, "%s/%s", ct_get_test_file_path(), token);
            if (sizeFilePath > MAXPATHLENGTH) {
              printf("Error in parse_cfg_file, path too long to fit in string\n");
            }
            else {
              strcpy(obj->output_file_path, filePath);
            }
          }
          else
            if(strcmp(token, "operation_mode")==0)
            {
              token = strtok(NULL, s);
              obj->operation_mode = atoi(token);
            }
            else
              if(strcmp(token, "processing_core_mode")==0)
              {
                token = strtok(NULL, s);
                obj->processing_core_mode = atoi(token);
              }
  }

  fclose(fp);

  exit:
  return status;
}

static vx_status createInputTensors(vx_context context, vx_user_data_object config, vx_tensor *input_tensors)
{
  vx_size   input_sizes[VX_TUTORIAL_MAX_TENSOR_DIMS];
  vx_map_id map_id_config;
  sTIDL_IOBufDesc_t *ioBufDesc;
  uint32_t id;
  vx_status status = VX_SUCCESS;

  vxMapUserDataObject(config, 0, sizeof(sTIDL_IOBufDesc_t), &map_id_config,
      (void **)&ioBufDesc, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);

  if (ioBufDesc->numInputBuf < VX_TUTORIAL_MAX_TENSORS) {

    for(id = 0; id < ioBufDesc->numInputBuf; id++) {
      input_sizes[0] = ioBufDesc->inWidth[id]  + ioBufDesc->inPadL[id] + ioBufDesc->inPadR[id];
      input_sizes[1] = ioBufDesc->inHeight[id] + ioBufDesc->inPadT[id] + ioBufDesc->inPadB[id];
      input_sizes[2] = ioBufDesc->inNumChannels[id];

      input_tensors[id] = vxCreateTensor(context, 3, input_sizes, VX_TYPE_UINT8, 0);
    }

  }
  else {
    status= VX_FAILURE;
  }

  vxUnmapUserDataObject(config, map_id_config);

  return status;
}

static vx_status createOutputTensor(vx_context context, vx_user_data_object config, vx_tensor *output_tensors)
{
  vx_size    output_sizes[VX_TUTORIAL_MAX_TENSOR_DIMS];
  vx_map_id map_id_config;
  uint32_t id;
  sTIDL_IOBufDesc_t *ioBufDesc;
  vx_status status = VX_SUCCESS;

  vxMapUserDataObject(config, 0, sizeof(sTIDL_IOBufDesc_t), &map_id_config,
      (void **)&ioBufDesc, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);

  if (ioBufDesc->numOutputBuf < VX_TUTORIAL_MAX_TENSORS) {

    for(id = 0; id < ioBufDesc->numOutputBuf; id++) {
      output_sizes[0] = ioBufDesc->outWidth[id]  + ioBufDesc->outPadL[id] + ioBufDesc->outPadR[id];
      output_sizes[1] = ioBufDesc->outHeight[id] + ioBufDesc->outPadT[id] + ioBufDesc->outPadB[id];
      output_sizes[2] = ioBufDesc->outNumChannels[id];

      output_tensors[id] = vxCreateTensor(context, 3, output_sizes, VX_TYPE_FLOAT32, 0);
    }

  }
  else {
    status= VX_FAILURE;
  }

  vxUnmapUserDataObject(config, map_id_config);

  return status;
}

static vx_status readDataS8(FILE *fp, int8_t *ptr, int32_t n,
    int32_t width, int32_t height, int32_t pitch,
    int32_t chOffset)
{
  int32_t   i0, i1;
  uint32_t readSize;
  vx_status status = VX_SUCCESS;

  for(i0 = 0; i0 < n; i0++)
  {
    for(i1 = 0; i1 < height; i1++)
    {
      readSize= fread(&ptr[i0*chOffset + i1*pitch], 1, width, fp);
      if (readSize != width) {
        status= VX_FAILURE;
        goto exit;
      }
    }
  }

  exit:
  return status;

}


static vx_status readInput(vx_context context, vx_user_data_object config, vx_tensor *input_tensors, char *input_file, uint32_t operation_mode)
{
  vx_status status = VX_SUCCESS;

  int8_t      *input_buffer = NULL;
  uint32_t   id;

  vx_map_id map_id_config;
  vx_map_id map_id_input;

  vx_size    start[VX_TUTORIAL_MAX_TENSOR_DIMS];
  vx_size    input_strides[VX_TUTORIAL_MAX_TENSOR_DIMS];
  vx_size    input_sizes[VX_TUTORIAL_MAX_TENSOR_DIMS];

  sTIDL_IOBufDesc_t *ioBufDesc;

  FILE *fp;

  fp= fopen(input_file, "rb");

  if(fp==NULL)
  {
    printf("# ERROR: Unable to open input file [%s]\n", input_file);
    return(VX_FAILURE);
  }

  vxMapUserDataObject(config, 0, sizeof(sTIDL_IOBufDesc_t), &map_id_config,
      (void **)&ioBufDesc, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);

  for(id = 0; id < ioBufDesc->numInputBuf; id++)
  {
    input_sizes[0] = ioBufDesc->inWidth[id]  + ioBufDesc->inPadL[id] + ioBufDesc->inPadR[id];
    input_sizes[1] = ioBufDesc->inHeight[id] + ioBufDesc->inPadT[id] + ioBufDesc->inPadB[id];
    input_sizes[2] = ioBufDesc->inNumChannels[id];

    start[0] = start[1] = start[2] = 0;

    input_strides[0] = 1;
    input_strides[1] = input_sizes[0];
    input_strides[2] = input_sizes[1] * input_strides[1];

    status = tivxMapTensorPatch(input_tensors[id], 3, start, input_sizes, &map_id_input, input_strides, (void **)&input_buffer, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);

    if (VX_SUCCESS == status)
    {
      status= readDataS8(
          fp,
          &input_buffer[(ioBufDesc->inPadT[id] * input_strides[1]) + ioBufDesc->inPadL[id]],
          ioBufDesc->inNumChannels[id],
          ioBufDesc->inWidth[id],
          ioBufDesc->inHeight[id],
          input_strides[1],
          input_strides[2]);

      tivxUnmapTensorPatch(input_tensors[id], map_id_input);

      if (status== VX_FAILURE) {
        goto exit;
      }
    }
  }

  exit:
  vxUnmapUserDataObject(config, map_id_config);

  fclose(fp);

  return status;
}

static void displayOutput(void *bmp_context, vx_df_image df_image, void *data_ptr, vx_uint32 img_width, vx_uint32 img_height, vx_uint32 img_stride, vx_user_data_object config, vx_tensor *output_tensors, char *output_file, uint32_t operation_mode)
{
  vx_status status = VX_SUCCESS;

  vx_size output_sizes[VX_TUTORIAL_MAX_TENSOR_DIMS];

  vx_map_id map_id_config;

  int32_t id, i, j;

  sTIDL_IOBufDesc_t *ioBufDesc;

  vxMapUserDataObject(config, 0, sizeof(sTIDL_IOBufDesc_t), &map_id_config,
      (void **)&ioBufDesc, VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);

  for(id = 0; id < 1; id++)
  {
    output_sizes[0] = ioBufDesc->outWidth[id]  + ioBufDesc->outPadL[id] + ioBufDesc->outPadR[id];
    output_sizes[1] = ioBufDesc->outHeight[id] + ioBufDesc->outPadT[id] + ioBufDesc->outPadB[id];
    output_sizes[2] = ioBufDesc->outNumChannels[id];

    status = vxGetStatus((vx_reference)output_tensors[id]);

    if (VX_SUCCESS == status)
    {
      void *output_buffer;

      vx_map_id map_id_output;

      vx_size output_strides[VX_TUTORIAL_MAX_TENSOR_DIMS];
      vx_size start[VX_TUTORIAL_MAX_TENSOR_DIMS];

      start[0] = start[1] = start[2] = start[3] = 0;

      output_strides[0] = 1;
      output_strides[1] = output_sizes[0];
      output_strides[2] = output_sizes[1] * output_strides[1];

      tivxMapTensorPatch(output_tensors[id], 3, start, output_sizes, &map_id_output, output_strides, &output_buffer, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

      if (operation_mode == 0)
      {
        uint8_t *pOut;
        uint8_t score[5];
        vx_uint32 classid[5];

        pOut = (uint8_t *)output_buffer + (ioBufDesc->outPadT[id] * output_sizes[0]) + ioBufDesc->outPadL[id];

        for(i = 0; i < 5; i++)
        {
          score[i] = 0;
          classid[i] = 0xFFFFFFFF;

          for(j = 0; j < output_sizes[0]; j++)
          {
            if(pOut[j] > score[i])
            {
              score[i] = pOut[j];
              classid[i] = j;
            }
          }

          pOut[classid[i]] = 0;
        }

        printf("\nImage classification Top-5 results: \n");

        for(i = 0; i < 5; i++)
        {
          printf(" %s, class-id: %d, score: %u\n", (char *)&imgnet_labels[classid[i]], classid[i], score[i]);
        }
      }
      else
        if (operation_mode== 1)
        {
          typedef struct {
            float objId;
            float label;
            float score;
            float xmin;
            float ymin;
            float xmax;
            float ymax;
          } ODLayerObjInfo;

          /* Display of coordinates of detected objects */
          uint8_t *pOut;
          ODLayerObjInfo *pObjInfo;
          uint32_t numObjs;

          numObjs= 20;

          pOut = (uint8_t *)output_buffer + (ioBufDesc->outPadT[id] * output_sizes[0]) + ioBufDesc->outPadL[id];

          pObjInfo = (ODLayerObjInfo *)pOut;

          printf("\n\nObjId|label|score| xmin| ymin| xmax| ymax|\n");
          printf("------------------------------------------\n");
          for(i = 0; i < numObjs; i++)
          {
            ODLayerObjInfo * pObj = pObjInfo + i;
            if ((int32_t)(pObj->objId)!=-1) {
              printf("%5d|%5d|%5.2f|%5.2f|%5.2f|%5.2f|%5.2f|\n", (int32_t)pObj->objId, (uint32_t)pObj->label, pObj->score, pObj->xmin, pObj->ymin, pObj->xmax, pObj->ymax);
            }
            else {
              break;
            }
            /*
            if(pPSpots->score >= 0.5f)
            {
              drawBox(obj, pObj);
            }
            */
          }
          printf("\nNumber of detected objects: %d\n\n", i);
        }
      tivxUnmapTensorPatch(output_tensors[id], map_id_output);
    }
  }

  vxUnmapUserDataObject(config, map_id_config);
}
