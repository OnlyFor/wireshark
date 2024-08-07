/* packet-esis.c
 * Routines for ISO/OSI End System to Intermediate System
 * Routing Exchange Protocol ISO 9542.
 *
 * Ralf Schneider <Ralf.Schneider@t-online.de>
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <epan/packet.h>
#include <epan/expert.h>
#include <epan/nlpid.h>
#include <epan/unit_strings.h>
#include "packet-osi.h"
#include "packet-osi-options.h"

/* The version we support is 1 */
#define ESIS_REQUIRED_VERSION    1

/* ESIS PDU types */
#define ESIS_ESH_PDU    02
#define ESIS_ISH_PDU    04
#define ESIS_RD_PDU     06

/* The length of the fixed part */
#define ESIS_HDR_FIXED_LENGTH 9

void proto_register_esis(void);
void proto_reg_handoff_esis(void);

/* esis base header */
static int  proto_esis;

static int  hf_esis_nlpi;
static int  hf_esis_length;
static int  hf_esis_version;
static int  hf_esis_reserved;
static int  hf_esis_type;
static int  hf_esis_holdtime;
static int  hf_esis_checksum;
static int  hf_esis_checksum_status;
/* Generated from convert_proto_tree_add_text.pl */
static int hf_esis_dal;
static int hf_esis_number_of_source_addresses;
static int hf_esis_netl;
static int hf_esis_sal;
static int hf_esis_sa;
static int hf_esis_bsnpal;
static int hf_esis_net;
static int hf_esis_da;
static int hf_esis_bsnpa;

static int ett_esis;
static int ett_esis_area_addr;
static int ett_esis_network;
static int ett_esis_dest_addr;
static int ett_esis_subnetwork;


static expert_field ei_esis_version;
static expert_field ei_esis_length;
static expert_field ei_esis_type;
static expert_field ei_esis_checksum;

static dissector_handle_t esis_handle;

static const value_string esis_vals[] = {
  { ESIS_ESH_PDU, "ES HELLO"},
  { ESIS_ISH_PDU, "IS HELLO"},
  { ESIS_RD_PDU,  "RD REQUEST"},
  { 0,             NULL} };

/* ################## Descriptions ###########################################*/
/* Parameters for the ESH PDU
 * Source Address Parameter:
 *
 * Octet:    Length:   Parameter Type:
 *     10          1   Number of Source Addresses ( NSAPs served by this Network
 *     11          1   Source Address Length Indicator ( SAL )     #    Entity )
 * 12-m-1   variable   Source Address ( NSAP )
 *      m              Options, dissected in osi.c
 *
 *
 * Parameter for the ISH PDU:
 * Network Entity Title Parameter:
 *
 * Octet:    Length:   Parameter Type:
 *     10          1   Network Entity Title Length Indicator ( NETL )
 * 11-m-1   variable   Network Entity Title ( NET )
 *      m              Options, dissected in osi.c
 *
 *
 * Parameter for the RD PDU:
 * When re-directed to an IS:
 *
 *  Octet:   Length:   Parameter Type:
 *      10         1   Destination Address Length Indicator ( DAL )
 *  11>m-1  variable   Destination Address ( DA )
 *       m         1   Subnetwork Address Length Indicator ( BSNPAL )
 * m+1>n-1  variable   Subnetwork Address ( BSNPA )
 *       n         1   Network Entity Title Length Indicator ( NETL )
 * n+1>p-1  variable   Network Entity Title ( NET )
 *       p             Options, dissected in osi.c
 *
 *
 * Parameter for the RD PDU:
 * When re-directed to an ES:
 *
 *  Octet:   Length:   Parameter Type:
 *      10         1   Destination Address Length Indicator ( DAL )
 *  11>m-1  variable   Destination Address ( DA )
 *       m         1   Subnetwork Address Length Indicator ( BSNPAL )
 * m+1>n-1  variable   Subnetwork Address ( BSNPA )
 *       n         1   Network Entity Title Length Indicator ( NETL ) == 0
 *     n+1             Options, dissected in osi.c
 *
 */

/* ############################ Tool Functions ############################## */


static void
esis_dissect_esh_pdu( uint8_t len, tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo) {
    proto_tree *esis_area_tree;
    int         offset  = 0;
    unsigned    no_sa, sal;

    proto_item  *ti;

    offset += ESIS_HDR_FIXED_LENGTH;

    ti = proto_tree_add_item_ret_uint( tree, hf_esis_number_of_source_addresses, tvb, offset, 1, ENC_NA, &no_sa);
    len--;
    offset++;

    esis_area_tree = proto_item_add_subtree( ti, ett_esis_area_addr );
    while ( no_sa-- > 0 ) {
      proto_tree_add_item_ret_uint(esis_area_tree, hf_esis_sal, tvb, offset, 1, ENC_NA, &sal);
      offset++;
      proto_tree_add_string(esis_area_tree, hf_esis_sa, tvb, offset, sal, print_nsap_net( pinfo->pool, tvb, offset, sal ) );
      offset += sal;
      len    -= ( sal + 1 );
    }
    dissect_osi_options( len, tvb, offset, tree, pinfo );

} /* esis_dissect_esh_pdu */

static void
esis_dissect_ish_pdu( uint8_t len, tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo) {

    int   offset  = 0;
    int   netl    = 0;
    proto_tree* network_tree;

    offset += ESIS_HDR_FIXED_LENGTH;

    netl = (int) tvb_get_uint8(tvb, offset);
    network_tree = proto_tree_add_subtree( tree, tvb, offset, netl + 1, ett_esis_network, NULL,
                         "### Network Entity Title Section ###");
    proto_tree_add_uint(network_tree, hf_esis_netl, tvb, offset++, 1, netl);
    proto_tree_add_string(network_tree, hf_esis_net, tvb, offset, netl, print_nsap_net( pinfo->pool, tvb, offset, netl ) );
    offset += netl;
    len    -= ( netl + 1 );

    dissect_osi_options( len, tvb, offset, network_tree, pinfo );
}

static void
esis_dissect_redirect_pdu( uint8_t len, tvbuff_t *tvb, proto_tree *tree, packet_info *pinfo) {

    int   offset  = 0;
    int   tmpl    = 0;
    proto_tree *dest_tree, *subnet_tree, *network_tree;

    offset += ESIS_HDR_FIXED_LENGTH;

    tmpl = (int) tvb_get_uint8(tvb, offset);
    dest_tree = proto_tree_add_subtree( tree, tvb, offset, tmpl + 1, ett_esis_dest_addr, NULL,
                         "### Destination Address Section ###" );
    proto_tree_add_uint(dest_tree, hf_esis_dal, tvb, offset++, 1, tmpl);
    proto_tree_add_string( dest_tree, hf_esis_da, tvb, offset, tmpl,
                         print_nsap_net( pinfo->pool, tvb, offset, tmpl ) );
    offset += tmpl;
    len    -= ( tmpl + 1 );
    tmpl    = (int) tvb_get_uint8(tvb, offset);

    subnet_tree = proto_tree_add_subtree( tree, tvb, offset, tmpl + 1, ett_esis_subnetwork, NULL,
                         "###  Subnetwork Address Section ###");
    proto_tree_add_uint(subnet_tree, hf_esis_bsnpal, tvb, offset++, 1, tmpl);
    proto_tree_add_item(subnet_tree, hf_esis_bsnpa, tvb, offset, tmpl, ENC_NA);
    offset += tmpl;
    len    -= ( tmpl + 1 );
    tmpl    = (int) tvb_get_uint8(tvb, offset);

    if ( 0 == tmpl ) {
      network_tree = proto_tree_add_subtree( tree, tvb, offset, 1, ett_esis_network, NULL,
                           "### No Network Entity Title Section ###" );
      offset++;
      len--;
    }
    else {
      network_tree = proto_tree_add_subtree( tree, tvb, offset, 1, ett_esis_network, NULL,
                           "### Network Entity Title Section ###" );
      proto_tree_add_uint(network_tree, hf_esis_netl, tvb, offset++, 1, tmpl);
      proto_tree_add_string( network_tree, hf_esis_net, tvb, offset, tmpl,
                           print_nsap_net( pinfo->pool, tvb, offset, tmpl ) );
      offset += tmpl;
      len    -= ( tmpl + 1 );
    }
    dissect_osi_options( len, tvb, offset, network_tree, pinfo );
}


/*
 * Name: dissect_esis()
 *
 * Description:
 *   Main entry area for esis de-mangling.  This will build the
 *   main esis tree data and call the sub-protocols as needed.
 *
 * Input:
 *   tvbuff *      : tvbuff referring to packet data
 *   packet_info * : info for current packet
 *   proto_tree *  : tree of display data.  May be NULL.
 *
 * Output:
 *   void, but we will add to the proto_tree if it is not NULL.
 */
static int
dissect_esis(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_) {
  uint8_t version, length;
  proto_item *ti, *type_item;
  proto_tree *esis_tree    = NULL;
  uint8_t     variable_len, type;
  uint16_t    checksum;

  col_set_str(pinfo->cinfo, COL_PROTOCOL, "ESIS");
  col_clear(pinfo->cinfo, COL_INFO);

    ti = proto_tree_add_item(tree, proto_esis, tvb, 0, -1, ENC_NA);
    esis_tree = proto_item_add_subtree(ti, ett_esis);

    proto_tree_add_item( esis_tree, hf_esis_nlpi, tvb, 0, 1, ENC_BIG_ENDIAN);
    ti = proto_tree_add_item( esis_tree, hf_esis_length, tvb, 1, 1, ENC_BIG_ENDIAN );
    length = tvb_get_uint8(tvb, 1);
    if (length < ESIS_HDR_FIXED_LENGTH) {
      expert_add_info_format(pinfo, ti, &ei_esis_length,
                           "Bogus ESIS length (%u, must be >= %u)",
                           length, ESIS_HDR_FIXED_LENGTH );
    }

    version = tvb_get_uint8(tvb, 2);
    ti = proto_tree_add_item( esis_tree, hf_esis_version, tvb, 2, 1, ENC_BIG_ENDIAN);
    if (version != ESIS_REQUIRED_VERSION){
      expert_add_info_format(pinfo, ti, &ei_esis_version,
                           "Unknown ESIS version (%u vs %u)",
                           version, ESIS_REQUIRED_VERSION );
    }

    proto_tree_add_item( esis_tree, hf_esis_reserved, tvb, 3, 1, ENC_BIG_ENDIAN);

    type_item = proto_tree_add_item( esis_tree, hf_esis_type, tvb, 4, 1, ENC_BIG_ENDIAN);
    type = tvb_get_uint8(tvb, 4) & OSI_PDU_TYPE_MASK;

    proto_tree_add_item(esis_tree, hf_esis_holdtime, tvb, 5, 2, ENC_BIG_ENDIAN);

    checksum = tvb_get_ntohs(tvb, 7);
    if (checksum == 0) {
        /* No checksum present */
        proto_tree_add_checksum(esis_tree, tvb, 7, hf_esis_checksum, -1, NULL, pinfo, 0, ENC_BIG_ENDIAN, PROTO_CHECKSUM_NOT_PRESENT);
    } else {
        uint32_t c0 = 0, c1 = 0;

        if (osi_calc_checksum(tvb, 0, length, &c0, &c1)) {
            /* Successfully processed checksum, verify it */
            proto_tree_add_checksum(esis_tree, tvb, 7, hf_esis_checksum, hf_esis_checksum_status, &ei_esis_checksum, pinfo, c0 | c1, ENC_BIG_ENDIAN, PROTO_CHECKSUM_VERIFY|PROTO_CHECKSUM_ZERO);
        } else {
            proto_tree_add_checksum(esis_tree, tvb, 7, hf_esis_checksum, hf_esis_checksum_status, &ei_esis_checksum, pinfo, 0, ENC_BIG_ENDIAN, PROTO_CHECKSUM_NO_FLAGS);
        }
    }

  /*
   * Let us make sure we use the same names for all our decodes
   * here.  First, dump the name into info column, and THEN
   * dispatch the sub-type.
   */
  col_add_str(pinfo->cinfo, COL_INFO,
                val_to_str( type, esis_vals,
                            "Unknown (0x%x)" ) );

  variable_len = length - ESIS_HDR_FIXED_LENGTH;

  switch (type) {
  case ESIS_ESH_PDU:
    esis_dissect_esh_pdu( variable_len, tvb, esis_tree, pinfo);
    break;
  case ESIS_ISH_PDU:
    esis_dissect_ish_pdu( variable_len, tvb, esis_tree, pinfo);
    break;
  case ESIS_RD_PDU:
    esis_dissect_redirect_pdu( variable_len, tvb, esis_tree, pinfo);
    break;
  default:
    expert_add_info(pinfo, type_item, &ei_esis_type);
  }
  return tvb_captured_length(tvb);
} /* dissect_esis */


/*
 * Name: proto_register_esis()
 *
 * Description:
 *      main register for esis protocol set.  We register some display
 *      formats and the protocol module variables.
 *
 *      NOTE: this procedure to autolinked by the makefile process that
 *      builds register.c
 *
 * Input:
 *      void
 *
 * Output:
 *      void
 */
void
proto_register_esis(void) {
  static hf_register_info hf[] = {
    { &hf_esis_nlpi,
      { "Network Layer Protocol Identifier", "esis.nlpi",
        FT_UINT8, BASE_HEX, VALS(nlpid_vals), 0x0, NULL, HFILL }},

    { &hf_esis_length,
      { "PDU Length", "esis.length",  FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL }},

    { &hf_esis_version,
      { "Version", "esis.ver",  FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL }},

    { &hf_esis_reserved,
      { "Reserved(==0)", "esis.res",  FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL }},

    { &hf_esis_type,
      { "PDU Type", "esis.type",      FT_UINT8, BASE_DEC, VALS(esis_vals), OSI_PDU_TYPE_MASK, NULL, HFILL }},

    { &hf_esis_holdtime,
      { "Holding Time", "esis.htime", FT_UINT16, BASE_DEC|BASE_UNIT_STRING, UNS(&units_second_seconds), 0x0, NULL, HFILL }},

    { &hf_esis_checksum,
      { "Checksum", "esis.chksum",    FT_UINT16, BASE_HEX, NULL, 0x0, NULL, HFILL }},

    { &hf_esis_checksum_status,
      { "Checksum Status", "esis.chksum.status",    FT_UINT8, BASE_NONE, VALS(proto_checksum_vals), 0x0, NULL, HFILL }},

      /* Generated from convert_proto_tree_add_text.pl */
      { &hf_esis_number_of_source_addresses, { "Number of Source Addresses (SA, Format: NSAP)", "esis.number_of_source_addresses", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL }},
      { &hf_esis_sal, { "SAL", "esis.sal", FT_UINT8, BASE_DEC|BASE_UNIT_STRING, UNS(&units_octet_octets), 0x0, NULL, HFILL }},
      { &hf_esis_sa, { "SA", "esis.sa", FT_STRING, BASE_NONE, NULL, 0x0, NULL, HFILL }},
      { &hf_esis_netl, { "NETL", "esis.netl", FT_UINT8, BASE_DEC|BASE_UNIT_STRING, UNS(&units_octet_octets), 0x0, NULL, HFILL }},
      { &hf_esis_dal, { "DAL", "esis.dal", FT_UINT8, BASE_DEC|BASE_UNIT_STRING, UNS(&units_octet_octets), 0x0, NULL, HFILL }},
      { &hf_esis_bsnpal, { "BSNPAL", "esis.bsnpal", FT_UINT8, BASE_DEC|BASE_UNIT_STRING, UNS(&units_octet_octets), 0x0, NULL, HFILL }},
      { &hf_esis_net, { "NET", "esis.net", FT_STRING, BASE_NONE, NULL, 0x0, NULL, HFILL }},
      { &hf_esis_da, { "DA", "esis.da", FT_STRING, BASE_NONE, NULL, 0x0, NULL, HFILL }},
      { &hf_esis_bsnpa, { "BSNPA", "esis.bsnpa", FT_SYSTEM_ID, BASE_NONE, NULL, 0x0, NULL, HFILL }},
  };

  static int *ett[] = {
    &ett_esis,
    &ett_esis_area_addr,
    &ett_esis_network,
    &ett_esis_dest_addr,
    &ett_esis_subnetwork
  };

  static ei_register_info ei[] = {
    { &ei_esis_version, { "esis.ver.unknown", PI_PROTOCOL, PI_WARN, "Unknown ESIS version", EXPFILL }},
    { &ei_esis_length, { "esis.length.invalid", PI_MALFORMED, PI_ERROR, "Bogus ESIS length", EXPFILL }},
    { &ei_esis_type, { "esis.type.unknown", PI_PROTOCOL, PI_WARN, "Unknown ESIS packet type", EXPFILL }},
    { &ei_esis_checksum, { "esis.bad_checksum", PI_CHECKSUM, PI_ERROR, "Bad checksum", EXPFILL }},
  };

  expert_module_t* expert_esis;

  proto_esis = proto_register_protocol( PROTO_STRING_ESIS, "ESIS", "esis");
  proto_register_field_array(proto_esis, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));
  expert_esis = expert_register_protocol(proto_esis);
  expert_register_field_array(expert_esis, ei, array_length(ei));
  esis_handle = register_dissector("esis", dissect_esis, proto_esis);
}

void
proto_reg_handoff_esis(void)
{
  dissector_add_uint("osinl.incl", NLPID_ISO9542_ESIS, esis_handle);
}

/*
 * Editor modelines  -  https://www.wireshark.org/tools/modelines.html
 *
 * Local Variables:
 * c-basic-offset: 2
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * ex: set shiftwidth=2 tabstop=8 expandtab:
 * :indentSize=2:tabSize=8:noTabs=true:
 */
