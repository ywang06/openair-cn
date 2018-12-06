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


/*! \file mme_app_pdn_context.c
  \brief
  \author Lionel Gauthier
  \company Eurecom
  \email: lionel.gauthier@eurecom.fr
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#include "bstrlib.h"

#include "dynamic_memory_check.h"
#include "log.h"
#include "msc.h"
#include "assertions.h"
#include "conversions.h"
#include "common_types.h"
#include "intertask_interface.h"
#include "mme_config.h"
#include "mme_app_extern.h"
#include "mme_app_ue_context.h"
#include "mme_app_defs.h"
#include "common_defs.h"
#include "mme_app_pdn_context.h"
#include "mme_app_apn_selection.h"

static void mme_app_pdn_context_init(ue_context_t * const ue_context, pdn_context_t *const  pdn_context);

//------------------------------------------------------------------------------
void mme_app_free_pdn_context (pdn_context_t ** const pdn_context)
{
  if ((*pdn_context)->apn_in_use) {
    bdestroy_wrapper(&(*pdn_context)->apn_in_use);
  }
  if ((*pdn_context)->apn_subscribed) {
    bdestroy_wrapper(&(*pdn_context)->apn_subscribed);
  }
  if ((*pdn_context)->apn_oi_replacement) {
    bdestroy_wrapper(&(*pdn_context)->apn_oi_replacement);
  }
  if ((*pdn_context)->pco) {
    free_protocol_configuration_options(&(*pdn_context)->pco);
  }
  // todo: free PAA
  if((*pdn_context)->paa){
    free_wrapper((void**)&((*pdn_context)->paa));
  }

  memset(&(*pdn_context)->esm_data, 0, sizeof((*pdn_context)->esm_data));

  free_wrapper((void**)pdn_context);
}

//------------------------------------------------------------------------------
void mme_app_get_pdn_context (mme_ue_s1ap_id_t ue_id, pdn_cid_t const context_id, ebi_t const default_ebi, bstring const apn_subscribed, pdn_context_t **pdn_ctx)
{
  ue_context_t * ue_context = NULL;
  /** Checking for valid fields inside the search comparision function. */
  pdn_context_t pdn_context_key = {.apn_subscribed = apn_subscribed, .default_ebi = default_ebi, .context_identifier = context_id};
  pdn_context_t * pdn_ctx_p = RB_FIND(PdnContexts, &ue_context->pdn_contexts, &pdn_context_key);
  *pdn_ctx = pdn_ctx_p;
  if(!pdn_ctx_p && apn_subscribed){
    /** Could not find the PDN context, search again using the APN. */

    RB_FOREACH (pdn_ctx_p, PdnContexts, &ue_context->pdn_contexts) {
      DevAssert(pdn_ctx_p);
      if(!bstricmp (pdn_ctx_p->apn_subscribed, apn_subscribed))
          *pdn_ctx = pdn_ctx_p;
    }
  }
}

//------------------------------------------------------------------------------
static void mme_app_pdn_context_init(ue_context_t * const ue_context, pdn_context_t *const  pdn_context)
{
  if ((pdn_context) && (ue_context)) {
    memset(pdn_context, 0, sizeof(*pdn_context));
    pdn_context->is_active = false;
    /** Initialize the session bearers map. */
    RB_INIT(&pdn_context->session_bearers);
  }
}

/*
 * Method to generate bearer contexts to be created.
 */
//------------------------------------------------------------------------------
void mme_app_get_bearer_contexts_to_be_created(pdn_context_t * pdn_context, bearer_contexts_to_be_created_t *bc_tbc, mme_app_bearer_state_t bc_state){

  OAILOG_FUNC_IN (LOG_MME_APP);

  bearer_context_t * bearer_context_to_setup  = NULL;

  RB_FOREACH (bearer_context_to_setup, SessionBearers, &pdn_context->session_bearers) {
    DevAssert(bearer_context_to_setup);
    // todo: make it selective for multi PDN!
    /*
     * Set the bearer of the pdn context to establish.
     * Set regardless of GBR/NON-GBR QCI the MBR/GBR values.
     * They are already set to zero if non-gbr in the registration of the bearer contexts.
     */
    /** EBI. */
    bc_tbc->bearer_contexts[bc_tbc->num_bearer_context].eps_bearer_id              = bearer_context_to_setup->ebi;
    /** MBR/GBR values (setting indep. of QCI level). */
    bc_tbc->bearer_contexts[bc_tbc->num_bearer_context].bearer_level_qos.gbr.br_ul = bearer_context_to_setup->esm_ebr_context.gbr_ul;
    bc_tbc->bearer_contexts[bc_tbc->num_bearer_context].bearer_level_qos.gbr.br_dl = bearer_context_to_setup->esm_ebr_context.gbr_dl;
    bc_tbc->bearer_contexts[bc_tbc->num_bearer_context].bearer_level_qos.mbr.br_ul = bearer_context_to_setup->esm_ebr_context.mbr_ul;
    bc_tbc->bearer_contexts[bc_tbc->num_bearer_context].bearer_level_qos.mbr.br_dl = bearer_context_to_setup->esm_ebr_context.mbr_dl;
    /** QCI. */
    bc_tbc->bearer_contexts[bc_tbc->num_bearer_context].bearer_level_qos.qci       = bearer_context_to_setup->qci;
    bc_tbc->bearer_contexts[bc_tbc->num_bearer_context].bearer_level_qos.pvi       = bearer_context_to_setup->preemption_vulnerability;
    bc_tbc->bearer_contexts[bc_tbc->num_bearer_context].bearer_level_qos.pci       = bearer_context_to_setup->preemption_capability;
    bc_tbc->bearer_contexts[bc_tbc->num_bearer_context].bearer_level_qos.pl        = bearer_context_to_setup->priority_level;
    /** Set the S1U SAE-GW FTEID. */
    bc_tbc->bearer_contexts[bc_tbc->num_bearer_context].s1u_sgw_fteid.ipv4                = bearer_context_to_setup->s_gw_fteid_s1u.ipv4;
    bc_tbc->bearer_contexts[bc_tbc->num_bearer_context].s1u_sgw_fteid.interface_type      = bearer_context_to_setup->s_gw_fteid_s1u.interface_type;
    bc_tbc->bearer_contexts[bc_tbc->num_bearer_context].s1u_sgw_fteid.ipv4_address.s_addr = bearer_context_to_setup->s_gw_fteid_s1u.ipv4_address.s_addr;
    bc_tbc->bearer_contexts[bc_tbc->num_bearer_context].s1u_sgw_fteid.teid                = bearer_context_to_setup->s_gw_fteid_s1u.teid;
    // todo: ipv6, other interfaces!


    bc_tbc->num_bearer_context++;
    /*
     * Update the bearer state.
     * Setting the bearer state as MME_CREATED when successful CSResp is received and as ACTIVE after successful MBResp is received!!
     */
    if(bc_state != BEARER_STATE_NULL){
      bearer_context_to_setup->bearer_state   |= bc_state;
    }
    // todo: set TFTs for dedicated bearers (currently PCRF triggered).
  }
  OAILOG_FUNC_OUT(LOG_MME_APP);
}

//------------------------------------------------------------------------------
int
mme_app_esm_create_pdn_context(mme_ue_s1ap_id_t ue_id, const apn_configuration_t *apn_configuration, const bstring apn, pdn_cid_t pdn_cid, const pdn_context_t **pdn_context_pp)
{
  OAILOG_FUNC_IN (LOG_MME_APP);
  ue_context_t * ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, ue_id);
  if(!ue_context){
    OAILOG_ERROR (LOG_MME_APP, "No MME_APP UE context could be found for UE: " MME_UE_S1AP_ID_FMT ". \n", ue_id);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }
//  LOCK_UE_CONTEXT(ue_context);
  (*pdn_context_pp) = calloc(1, sizeof(*pdn_context_t));
  if (!(*pdn_context_pp)) {
    OAILOG_CRITICAL(LOG_MME_APP, "Error creating PDN context for UE: " MME_UE_S1AP_ID_FMT ". \n", ue_context->mme_ue_s1ap_id);
   // todo: optimal locks UNLOCK_UE_CONTEXT(ue_context);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }
  /*
   * Check if an APN configuration exists. If so, use it to update the fields.
   */
  mme_app_pdn_context_init(ue_context, (*pdn_context_pp));
  /** Get the default bearer context directly. */
  bearer_context_t * free_bearer = RB_MIN(BearerPool, &ue_context->bearer_pool);
  DevAssert(free_bearer); // todo: else, the pdn context needs to be removed..
  bearer_context_t * bearer_context_registered = NULL;
  mme_app_register_bearer_context(ue_context, free_bearer->ebi, (*pdn_context_pp), &bearer_context_registered);
  (*pdn_context_pp)->default_ebi = bearer_context_registered->ebi;
  bearer_context_registered->linked_ebi = bearer_context_registered->ebi;
  if (apn_configuration) {
    (*pdn_context_pp)->subscribed_apn_ambr          = apn_configuration->ambr; /**< APN received from the HSS APN configuration, to be updated with the PCRF. */
    (*pdn_context_pp)->context_identifier           = apn_configuration->context_identifier;
    (*pdn_context_pp)->pdn_type                     = apn_configuration->pdn_type;
    /** Set the subscribed APN-AMBR from the default profile. */
    (*pdn_context_pp)->subscribed_apn_ambr.br_dl    = apn_configuration->ambr.br_dl;
    (*pdn_context_pp)->subscribed_apn_ambr.br_ul    = apn_configuration->ambr.br_ul;
#ifdef APN_CONFIG_IP_ADDR
    if (apn_configuration->nb_ip_address) { // todo: later
      *pdn_context_pp->paa = calloc(1, sizeof(paa_t));
      *pdn_context_pp->paa->pdn_type           = apn_configuration->ip_address[0].pdn_type;// TODO check this later...
      *pdn_context_pp->paa->ipv4_address       = apn_configuration->ip_address[0].address.ipv4_address;
      if (2 == apn_configuration->nb_ip_address) {
        *pdn_context_pp->paa->ipv6_address       = apn_configuration->ip_address[1].address.ipv6_address;
        *pdn_context_pp->paa->ipv6_prefix_length = 64;
      }
    }
#endif
    /** Set the default QoS values. */
    memcpy (&(*pdn_context_pp)->default_bearer_eps_subscribed_qos_profile, &apn_configuration->subscribed_qos, sizeof(eps_subscribed_qos_profile_t));
    (*pdn_context_pp)->apn_subscribed = blk2bstr(apn_configuration->service_selection, apn_configuration->service_selection_length);  /**< Set the APN-NI from the service selection. */
  } else {
    (*pdn_context_pp)->apn_subscribed = apn;
    (*pdn_context_pp)->context_identifier = pdn_cid + (free_bearer->ebi - 5); /**< Add the temporary context ID from the ebi offset. */
    OAILOG_WARNING(LOG_MME_APP, "No APN configuration exists for UE " MME_UE_S1AP_ID_FMT ". Subscribed APN \"%s.\" \n",
        ue_context->mme_ue_s1ap_id, bdata((*pdn_context_pp)->apn_subscribed));
    /** The subscribed APN should already exist from handover (forward relocation request). */
  }
  // todo: PCOs
  //     * Set the emergency bearer services indicator
  //     */
  //    (*pdn_context_pP)->esm_data.is_emergency = is_emergency;
  //      if (pco) {
  //        if (!(*pdn_context_pP)->pco) {
  //          (*pdn_context_pP)->pco = calloc(1, sizeof(protocol_configuration_options_t));
  //        } else {
  //          clear_protocol_configuration_options((*pdn_context_pP)->pco);
  //        }
  //        copy_protocol_configuration_options((*pdn_context_pP)->pco, pco);
  //      }

  /** Insert the PDN context into the map of PDN contexts. */
  DevAssert(!RB_INSERT (PdnContexts, &ue_context->pdn_contexts, (*pdn_context_pp)));
  // UNLOCK_UE_CONTEXT
  MSC_LOG_EVENT (MSC_NAS_ESM_MME, "0 Create PDN cid %u APN %s", (*pdn_context_pp)->context_identifier, (*pdn_context_pp)->apn_in_use);
  OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);
}

//------------------------------------------------------------------------------
static int
mme_app_esm_update_bearer_context(bearer_context_t * bearer_context, bearer_qos_t * bearer_level_qos, esm_ebr_state esm_ebr_state, traffic_flow_template_t * bearer_tft){
  OAILOG_FUNC_IN (LOG_MME_APP);
  /*
   * Setup the EPS bearer data with verified TFT and QoS.
   */
  bearer_context->qci = bearer_level_qos->qci;
  bearer_context->preemption_capability    = bearer_level_qos->pci == 0 ? 1 : 0;
  bearer_context->preemption_vulnerability = bearer_level_qos->pvi == 0 ? 1 : 0;
  bearer_context->priority_level           = bearer_level_qos->pl;
  DevAssert(!bearer_context->esm_ebr_context.tft);

  bearer_context->esm_ebr_context.status = esm_ebr_state;
  if(bearer_tft){
    if(bearer_context->esm_ebr_context.tft)
      free_traffic_flow_template(&bearer_context->esm_ebr_context.tft);
    bearer_context->esm_ebr_context.tft = bearer_tft;
    bearer_context->esm_ebr_context.status = esm_ebr_state;
  }
  /*
   * Return the EPS bearer identity of the default EPS bearer
   * * * * associated to the new EPS bearer context
   */
  OAILOG_FUNC_RETURN(LOG_MME_APP, RETURNok);
}

//------------------------------------------------------------------------------
int
mme_app_esm_update_pdn_context(mme_ue_s1ap_id_t ue_id, const bstring apn, pdn_cid_t pdn_cid, ebi_t linked_ebi, esm_ebr_state esm_ebr_state, const ambr_t apn_ambr, const bearer_qos_t *bearer_qos, protocol_configuration_options_t *pcos){
  ue_context_t              *ue_context         = NULL;
  pdn_context_t             *pdn_context        = NULL;
  int                        rc                 = RETURNok;

  OAILOG_FUNC_IN (LOG_MME_APP);
  ue_context_t * ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, ue_id);
  if(!ue_context){
    OAILOG_ERROR (LOG_MME_APP, "No MME_APP UE context could be found for UE: " MME_UE_S1AP_ID_FMT ". \n", ue_id);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }
  pdn_context_t * pdn_context = NULL;
  mme_app_get_pdn_context(ue_id, pdn_cid, linked_ebi, apn, &pdn_context);
  if(!pdn_context){
    OAILOG_ERROR (LOG_MME_APP, "No PDN context for could be found for APN \"%s\" for UE: " MME_UE_S1AP_ID_FMT ". \n", bdata(apn), ue_id);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }
  //  LOCK_UE_CONTEXT(ue_context);
  /** Update the authorized APN ambr of the pdn context. */
  // todo: check if UE apn-ambr reached, if so, just add the remaining. (todo: inform sae/pcrf)

  /** Get the default bearer context directly. */
  bearer_context_t * default_bearer = RB_MIN(SessionBearers, &pdn_context->session_bearers);
  rc = mme_app_update_bearer_context(default_bearer, bearer_qos, esm_ebr_state);
  //    if(pco){
  //      if (bearer_context->esm_ebr_context.pco) {
  //        free_protocol_configuration_options(&bearer_context->esm_ebr_context.pco);
  //      }
  //      /** Make it with memcpy, don't use bearer.. */
  //      bearer_context->esm_ebr_context.pco = (protocol_configuration_options_t *) calloc (1, sizeof (protocol_configuration_options_t));
  //      memcpy(bearer_context->esm_ebr_context.pco, pco, sizeof (protocol_configuration_options_t));  /**< Should have the processed bitmap in the validation . */
  //    }
  //  UNLOCK_UE_CONTEXT(ue_context);
  OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNok);
}

//------------------------------------------------------------------------------
int
mme_app_cn_update_bearer_context(mme_ue_s1ap_id_t ue_id, const ebi_t ebi, struct fteid_set_s * fteid_set){
  ue_context_t * ue_context         = NULL;
  bearer_context_t * bearer_context = NULL;
  int rc                            = RETURNok;

  OAILOG_FUNC_IN (LOG_MME_APP);
  ue_context_t * ue_context = mme_ue_context_exists_mme_ue_s1ap_id(&mme_app_desc.mme_ue_contexts, ue_id);
  if(!ue_context){
    OAILOG_ERROR (LOG_MME_APP, "No MME_APP UE context could be found for UE: " MME_UE_S1AP_ID_FMT ". \n", ue_id);
    OAILOG_FUNC_RETURN (LOG_MME_APP, RETURNerror);
  }
  //  LOCK_UE_CONTEXT(ue_context);
  /** Update the FTEIDs and the bearers CN state. */
  mme_app_get_session_bearer_context_from_all(ue_id, ebi, &bearer_context);
  if(bearer_context){
    /** We can set the FTEIDs right before the CBResp is set. */
    if(fteid_set){
      memcpy((void*)&bearer_context->s_gw_fteid_s1u , fteid_set->s1u_fteid, sizeof(fteid_t));
      memcpy((void*)&bearer_context->p_gw_fteid_s5_s8_up , fteid_set->s5_fteid, sizeof(fteid_t));
      bearer_context->bearer_state |= BEARER_STATE_SGW_CREATED;
    }
    /** Set the MME_APP states (todo: may be with Activate Dedicated Bearer Response). */
    bearer_context->bearer_state   |= BEARER_STATE_SGW_CREATED;
    bearer_context->bearer_state   |= BEARER_STATE_MME_CREATED;
    OAILOG_ERROR (LOG_MME_APP, "No MME_APP UE context could be found for UE: " MME_UE_S1AP_ID_FMT ". \n", ue_id);
    rc = RETURNok;
  }else{
    OAILOG_ERROR (LOG_MME_APP, "No MME_APP UE context could be found for UE: " MME_UE_S1AP_ID_FMT ". \n", ue_id);
    rc = RETURNerror;
  }
  //  UNLOCK_UE_CONTEXT(ue_context);
  OAILOG_FUNC_RETURN (LOG_MME_APP, rc);
}

