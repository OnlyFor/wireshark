/* packet-ilp.c
 * Routines for OMA Internal Location Protocol packet dissection
 * Copyright 2006, e.yimjia <jy.m12.0@gmail.com>
 * Copyright 2019, Pascal Quantin <pascal@wireshark.org>
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * ref OMA-TS-ILP-V2_0_4-20181213-A
 * http://www.openmobilealliance.org
 */

#include "config.h"

#include <epan/packet.h>
#include <epan/prefs.h>
#include <epan/asn1.h>
#include <wsutil/array.h>

#include "packet-per.h"
#include "packet-tcp.h"
#include "packet-gsm_map.h"
#include "packet-e164.h"
#include "packet-e212.h"

#define PNAME  "OMA Internal Location Protocol"
#define PSNAME "ILP"
#define PFNAME "ilp"

void proto_register_ilp(void);

static dissector_handle_t rrlp_handle;
static dissector_handle_t lpp_handle;
static dissector_handle_t ilp_tcp_handle;


/* IANA Registered Ports
 * oma-ilp         7276/tcp    OMA Internal Location
 */
#define ILP_TCP_PORT    7276

/* Initialize the protocol and registered fields */
static int proto_ilp;


#define ILP_HEADER_SIZE 2

static bool ilp_desegment = true;

#include "packet-ilp-hf.c"
static int hf_ilp_mobile_directory_number;

/* Initialize the subtree pointers */
static int ett_ilp;
static int ett_ilp_setid;
#include "packet-ilp-ett.c"

/* Include constants */
#include "packet-ilp-val.h"


#include "packet-ilp-fn.c"


static unsigned
get_ilp_pdu_len(packet_info *pinfo _U_, tvbuff_t *tvb, int offset, void *data _U_)
{
  /* PDU length = Message length */
  return tvb_get_ntohs(tvb,offset);
}

static int
dissect_ilp_tcp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data)
{
  tcp_dissect_pdus(tvb, pinfo, tree, ilp_desegment, ILP_HEADER_SIZE,
                   get_ilp_pdu_len, dissect_ILP_PDU_PDU, data);
  return tvb_captured_length(tvb);
}

void proto_reg_handoff_ilp(void);

/*--- proto_register_ilp -------------------------------------------*/
void proto_register_ilp(void) {

  /* List of fields */
  static hf_register_info hf[] = {

#include "packet-ilp-hfarr.c"
    { &hf_ilp_mobile_directory_number,
      { "Mobile Directory Number", "ilp.mobile_directory_number",
        FT_STRING, BASE_NONE, NULL, 0,
        NULL, HFILL }},
  };

  /* List of subtrees */
  static int *ett[] = {
    &ett_ilp,
    &ett_ilp_setid,
#include "packet-ilp-ettarr.c"
  };

  module_t *ilp_module;


  /* Register protocol */
  proto_ilp = proto_register_protocol(PNAME, PSNAME, PFNAME);
  ilp_tcp_handle = register_dissector("ilp", dissect_ilp_tcp, proto_ilp);

  /* Register fields and subtrees */
  proto_register_field_array(proto_ilp, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));

  ilp_module = prefs_register_protocol(proto_ilp, NULL);

  prefs_register_bool_preference(ilp_module, "desegment_ilp_messages",
        "Reassemble ILP messages spanning multiple TCP segments",
        "Whether the ILP dissector should reassemble messages spanning multiple TCP segments."
        " To use this option, you must also enable \"Allow subdissectors to reassemble TCP streams\" in the TCP protocol settings.",
        &ilp_desegment);
}


/*--- proto_reg_handoff_ilp ---------------------------------------*/
void
proto_reg_handoff_ilp(void)
{
  dissector_handle_t ilp_pdu_handle;

  ilp_pdu_handle = create_dissector_handle(dissect_ILP_PDU_PDU, proto_ilp);
  rrlp_handle = find_dissector_add_dependency("rrlp", proto_ilp);
  lpp_handle = find_dissector_add_dependency("lpp", proto_ilp);

  dissector_add_string("media_type","application/oma-supl-ilp", ilp_pdu_handle);
  dissector_add_uint_with_preference("tcp.port", ILP_TCP_PORT, ilp_tcp_handle);
}
