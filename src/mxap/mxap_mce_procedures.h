/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the Apache License, Version 2.0  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*! \file mxap_mce_procedures.h
  \brief
  \author Dincer BEKEN
  \company Blackned GmbH
  \email: dbeken@blackned.de
*/

#ifndef FILE_MXAP_MCE_PROCEDURES_SEEN
#define FILE_MXAP_MCE_PROCEDURES_SEEN

#include "common_defs.h"

/** \brief Handle MBMS Session Start Request from the MCE_APP.
 **/
void
m3ap_handle_mbms_session_start_request (
  const itti_m3ap_mbms_session_start_req_t * const mbms_session_start_req_pP);

/** \brief Handle MBMS Session Stop Request from the MCE_APP.
 **/
void
m3ap_handle_mbms_session_stop_request (
  const itti_m3ap_mbms_session_stop_req_t * const mbms_session_stop_req_pP);

/** \brief Handle MBMS Session Update Request from the MCE_APP.
 **/
void
m3ap_handle_mbms_session_update_request (
  const itti_m3ap_mbms_session_update_req_t * const mbms_session_update_req_pP);

#endif /* FILE_MXAP_MCE_PROCEDURES_SEEN */
