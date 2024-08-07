/* packet-dec-bpdu.c
 * Routines for DEC BPDU (DEC Spanning Tree Protocol) disassembly
 *
 * Copyright 2001 Paul Ionescu <paul@acorp.ro>
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <epan/packet.h>
#include <epan/etypes.h>
#include <epan/ppptypes.h>
#include <epan/tfs.h>

/* Offsets of fields within a BPDU */

#define BPDU_DEC_CODE            0
#define BPDU_TYPE                1
#define BPDU_VERSION             2
#define BPDU_FLAGS               3
#define BPDU_ROOT_PRI            4
#define BPDU_ROOT_MAC            6
#define BPDU_ROOT_PATH_COST     12
#define BPDU_BRIDGE_PRI         14
#define BPDU_BRIDGE_MAC         16
#define BPDU_PORT_IDENTIFIER    22
#define BPDU_MESSAGE_AGE        23
#define BPDU_HELLO_TIME         24
#define BPDU_MAX_AGE            25
#define BPDU_FORWARD_DELAY      26

#define DEC_BPDU_SIZE           27

/* Flag bits */

#define BPDU_FLAGS_SHORT_TIMERS         0x80
#define BPDU_FLAGS_TCACK                0x02
#define BPDU_FLAGS_TC                   0x01

void proto_register_dec_bpdu(void);
void proto_reg_handoff_dec_bpdu(void);

static dissector_handle_t dec_bpdu_handle;

static int proto_dec_bpdu;
static int hf_dec_bpdu_proto_id;
static int hf_dec_bpdu_type;
static int hf_dec_bpdu_version_id;
static int hf_dec_bpdu_flags;
static int hf_dec_bpdu_flags_short_timers;
static int hf_dec_bpdu_flags_tcack;
static int hf_dec_bpdu_flags_tc;
static int hf_dec_bpdu_root_pri;
static int hf_dec_bpdu_root_mac;
static int hf_dec_bpdu_root_cost;
static int hf_dec_bpdu_bridge_pri;
static int hf_dec_bpdu_bridge_mac;
static int hf_dec_bpdu_port_id;
static int hf_dec_bpdu_msg_age;
static int hf_dec_bpdu_hello_time;
static int hf_dec_bpdu_max_age;
static int hf_dec_bpdu_forward_delay;

static int ett_dec_bpdu;
static int ett_dec_bpdu_flags;

static const value_string protocol_id_vals[] = {
    { 0xe1, "DEC Spanning Tree Protocol" },
    { 0,    NULL }
};

#define BPDU_TYPE_TOPOLOGY_CHANGE       2
#define BPDU_TYPE_HELLO                 25

static const value_string bpdu_type_vals[] = {
    { BPDU_TYPE_TOPOLOGY_CHANGE, "Topology Change Notification" },
    { BPDU_TYPE_HELLO,           "Hello Packet" },
    { 0,                         NULL }
};

static int
dissect_dec_bpdu(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
    uint8_t bpdu_type;
    proto_tree *bpdu_tree;
    proto_item *ti;
    static int * const bpdu_flags[] = {
        &hf_dec_bpdu_flags_short_timers,
        &hf_dec_bpdu_flags_tcack,
        &hf_dec_bpdu_flags_tc,
        NULL
    };

    col_set_str(pinfo->cinfo, COL_PROTOCOL, "DEC_STP");
    col_clear(pinfo->cinfo, COL_INFO);

    bpdu_type = tvb_get_uint8(tvb, BPDU_TYPE);

    col_add_str(pinfo->cinfo, COL_INFO,
                val_to_str(bpdu_type, bpdu_type_vals,
                           "Unknown BPDU type (%u)"));

    set_actual_length(tvb, DEC_BPDU_SIZE);

    if (tree) {
        ti = proto_tree_add_item(tree, proto_dec_bpdu, tvb, 0, DEC_BPDU_SIZE,
                                 ENC_NA);
        bpdu_tree = proto_item_add_subtree(ti, ett_dec_bpdu);

        proto_tree_add_item(bpdu_tree, hf_dec_bpdu_proto_id, tvb,
                            BPDU_DEC_CODE, 1, ENC_BIG_ENDIAN);

        proto_tree_add_uint(bpdu_tree, hf_dec_bpdu_type, tvb,
                            BPDU_TYPE, 1, bpdu_type);

        proto_tree_add_item(bpdu_tree, hf_dec_bpdu_version_id, tvb,
                            BPDU_VERSION, 1, ENC_BIG_ENDIAN);

        proto_tree_add_bitmask_with_flags(bpdu_tree, tvb, BPDU_FLAGS, hf_dec_bpdu_flags, ett_dec_bpdu_flags, bpdu_flags, ENC_NA, BMT_NO_FALSE|BMT_NO_TFS);
        proto_tree_add_item(bpdu_tree, hf_dec_bpdu_root_pri, tvb,
                            BPDU_ROOT_PRI, 2, ENC_BIG_ENDIAN);
        proto_tree_add_item(bpdu_tree, hf_dec_bpdu_root_mac, tvb,
                            BPDU_ROOT_MAC, 6, ENC_NA);
        proto_tree_add_item(bpdu_tree, hf_dec_bpdu_root_cost, tvb,
                            BPDU_ROOT_PATH_COST, 2, ENC_BIG_ENDIAN);
        proto_tree_add_item(bpdu_tree, hf_dec_bpdu_bridge_pri, tvb,
                            BPDU_BRIDGE_PRI, 2, ENC_BIG_ENDIAN);
        proto_tree_add_item(bpdu_tree, hf_dec_bpdu_bridge_mac, tvb,
                            BPDU_BRIDGE_MAC, 6, ENC_NA);
        proto_tree_add_item(bpdu_tree, hf_dec_bpdu_port_id, tvb,
                            BPDU_PORT_IDENTIFIER, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item(bpdu_tree, hf_dec_bpdu_msg_age, tvb,
                            BPDU_MESSAGE_AGE, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item(bpdu_tree, hf_dec_bpdu_hello_time, tvb,
                            BPDU_HELLO_TIME, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item(bpdu_tree, hf_dec_bpdu_max_age, tvb,
                            BPDU_MAX_AGE, 1, ENC_BIG_ENDIAN);
        proto_tree_add_item(bpdu_tree, hf_dec_bpdu_forward_delay, tvb,
                            BPDU_FORWARD_DELAY, 1, ENC_BIG_ENDIAN);

    }
    return tvb_captured_length(tvb);
}

void
proto_register_dec_bpdu(void)
{

    static hf_register_info hf[] = {
        { &hf_dec_bpdu_proto_id,
          { "Protocol Identifier",          "dec_stp.protocol",
            FT_UINT8,       BASE_HEX,       VALS(protocol_id_vals), 0x0,
            NULL, HFILL }},
        { &hf_dec_bpdu_type,
          { "BPDU Type",                    "dec_stp.type",
            FT_UINT8,       BASE_DEC,       VALS(bpdu_type_vals),   0x0,
            NULL, HFILL }},
        { &hf_dec_bpdu_version_id,
          { "BPDU Version",                 "dec_stp.version",
            FT_UINT8,       BASE_DEC,       NULL,   0x0,
            NULL, HFILL }},
        { &hf_dec_bpdu_flags,
          { "BPDU flags",                   "dec_stp.flags",
            FT_UINT8,       BASE_HEX,       NULL,   0x0,
            NULL, HFILL }},
        { &hf_dec_bpdu_flags_short_timers,
          { "Use short timers",             "dec_stp.flags.short_timers",
            FT_BOOLEAN,     8,              TFS(&tfs_yes_no),       BPDU_FLAGS_SHORT_TIMERS,
            NULL, HFILL }},
        { &hf_dec_bpdu_flags_tcack,
          { "Topology Change Acknowledgment",  "dec_stp.flags.tcack",
            FT_BOOLEAN,     8,              TFS(&tfs_yes_no),       BPDU_FLAGS_TCACK,
            NULL, HFILL }},
        { &hf_dec_bpdu_flags_tc,
          { "Topology Change",              "dec_stp.flags.tc",
            FT_BOOLEAN,     8,              TFS(&tfs_yes_no),       BPDU_FLAGS_TC,
            NULL, HFILL }},
        { &hf_dec_bpdu_root_pri,
          { "Root Priority",                "dec_stp.root.pri",
            FT_UINT16,      BASE_DEC,       NULL,   0x0,
            NULL, HFILL }},
        { &hf_dec_bpdu_root_mac,
          { "Root MAC",                     "dec_stp.root.mac",
            FT_ETHER,       BASE_NONE,      NULL,   0x0,
            NULL, HFILL }},
        { &hf_dec_bpdu_root_cost,
          { "Root Path Cost",               "dec_stp.root.cost",
            FT_UINT16,      BASE_DEC,       NULL,   0x0,
            NULL, HFILL }},
        { &hf_dec_bpdu_bridge_pri,
          { "Bridge Priority",              "dec_stp.bridge.pri",
            FT_UINT16,      BASE_DEC,       NULL,   0x0,
            NULL, HFILL }},
        { &hf_dec_bpdu_bridge_mac,
          { "Bridge MAC",                   "dec_stp.bridge.mac",
            FT_ETHER,       BASE_NONE,      NULL,   0x0,
            NULL, HFILL }},
        { &hf_dec_bpdu_port_id,
          { "Port identifier",              "dec_stp.port",
            FT_UINT8,       BASE_DEC,       NULL,   0x0,
            NULL, HFILL }},
        { &hf_dec_bpdu_msg_age,
          { "Message Age",                  "dec_stp.msg_age",
            FT_UINT8,       BASE_DEC,       NULL,   0x0,
            NULL, HFILL }},
        { &hf_dec_bpdu_hello_time,
          { "Hello Time",                   "dec_stp.hello",
            FT_UINT8,       BASE_DEC,       NULL,   0x0,
            NULL, HFILL }},
        { &hf_dec_bpdu_max_age,
          { "Max Age",                      "dec_stp.max_age",
            FT_UINT8,       BASE_DEC,       NULL,   0x0,
            NULL, HFILL }},
        { &hf_dec_bpdu_forward_delay,
          { "Forward Delay",                "dec_stp.forward",
            FT_UINT8,       BASE_DEC,       NULL,   0x0,
            NULL, HFILL }},
    };
    static int *ett[] = {
        &ett_dec_bpdu,
        &ett_dec_bpdu_flags,
    };

    proto_dec_bpdu = proto_register_protocol("DEC Spanning Tree Protocol",
                                             "DEC_STP", "dec_stp");
    proto_register_field_array(proto_dec_bpdu, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));

    dec_bpdu_handle = register_dissector("dec_stp", dissect_dec_bpdu,
                                              proto_dec_bpdu);
}

void
proto_reg_handoff_dec_bpdu(void)
{
    dissector_add_uint("ethertype", ETHERTYPE_DEC_LB, dec_bpdu_handle);
    dissector_add_uint("chdlc.protocol", ETHERTYPE_DEC_LB, dec_bpdu_handle);
    dissector_add_uint("ppp.protocol", PPP_DEC_LB, dec_bpdu_handle);
}

/*
 * Editor modelines  -  https://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 4
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=4 tabstop=8 expandtab:
 * :indentSize=4:tabSize=8:noTabs=true:
 */
