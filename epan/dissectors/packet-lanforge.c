/* packet-lanforge.c
 * Routines for "LANforge traffic generator IP protocol" dissection
 * Copyright 2008
 * Ben Greear <greearb@candelatech.com>
 *
 * Based on pktgen dissectory by:
 * Francesco Fondelli <francesco dot fondelli, gmail dot com>
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* LANforge generates network traffic for load & performance testing.
 * See http://www.candelatech.com for more info.
 */

#include "config.h"

#include <epan/packet.h>

void proto_register_lanforge(void);
void proto_reg_handoff_lanforge(void);

/* magic num used for heuristic */
#define LANFORGE_MAGIC 0x1a2b3c4d

/* Initialize the protocol and registered fields */
static int proto_lanforge;

/* lanforge header */
static int hf_lanforge_crc;
static int hf_lanforge_magic;
static int hf_lanforge_src_session;
static int hf_lanforge_dst_session;
static int hf_lanforge_pld_len_l;
static int hf_lanforge_pld_len_h;
static int hf_lanforge_pld_len;
static int hf_lanforge_pld_pattern;
static int hf_lanforge_seq;
static int hf_lanforge_tx_time_s;
static int hf_lanforge_tx_time_ns;
static int hf_lanforge_timestamp;

/* Initialize the subtree pointer */
static int ett_lanforge;

/* entry point */
static bool dissect_lanforge(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
    proto_item *ti;
    proto_tree *lanforge_tree;
    uint32_t offset = 0;
    uint32_t magic, pld_len, pld_len_h;

    /* check for min size */
    if(tvb_captured_length(tvb) < 28) {  /* Not a LANforge packet. */
        return false;
    }

    /* check for magic number */
    magic = tvb_get_ntohl(tvb, 4);
    if(magic != LANFORGE_MAGIC){
        /* Not a LANforge packet. */
        return false;
    }

    /* Make entries in Protocol column and Info column on summary display */

    col_set_str(pinfo->cinfo, COL_PROTOCOL, "LANforge");

    col_add_fstr(pinfo->cinfo, COL_INFO, "Seq: %u", tvb_get_ntohl(tvb, 16));

    /* create display subtree for the protocol */

    ti = proto_tree_add_item(tree, proto_lanforge, tvb, 0, -1, ENC_NA);

    lanforge_tree = proto_item_add_subtree(ti, ett_lanforge);

    /* add items to the subtree */

    proto_tree_add_item(lanforge_tree, hf_lanforge_crc, tvb, offset, 4, ENC_BIG_ENDIAN);
    offset+=4;

    proto_tree_add_item(lanforge_tree, hf_lanforge_magic, tvb, offset, 4, ENC_BIG_ENDIAN);
    offset+=4;

    proto_tree_add_item(lanforge_tree, hf_lanforge_src_session, tvb, offset, 2, ENC_BIG_ENDIAN);
    offset+=2;

    proto_tree_add_item(lanforge_tree, hf_lanforge_dst_session, tvb, offset, 2, ENC_BIG_ENDIAN);
    offset+=2;

    proto_tree_add_item_ret_uint(lanforge_tree, hf_lanforge_pld_len_l,
            tvb, offset, 2, ENC_BIG_ENDIAN, &pld_len);
    offset+=2;
    proto_tree_add_item_ret_uint(lanforge_tree, hf_lanforge_pld_len_h,
            tvb, offset, 1, ENC_BIG_ENDIAN, &pld_len_h);
    offset+=1;
    pld_len |= (pld_len_h << 16);
    proto_tree_add_uint(lanforge_tree, hf_lanforge_pld_len, tvb, offset-3, 3, pld_len);

    proto_tree_add_item(lanforge_tree, hf_lanforge_pld_pattern, tvb, offset, 1, ENC_BIG_ENDIAN);
    offset+=1;

    proto_tree_add_item(lanforge_tree, hf_lanforge_seq, tvb, offset, 4, ENC_BIG_ENDIAN);
    offset+=4;

    proto_tree_add_item(lanforge_tree, hf_lanforge_tx_time_s, tvb, offset, 4, ENC_BIG_ENDIAN);
    offset+=4;
    proto_tree_add_item(lanforge_tree, hf_lanforge_tx_time_ns, tvb, offset, 4, ENC_BIG_ENDIAN);
    offset+=4;
    proto_tree_add_item(lanforge_tree, hf_lanforge_timestamp,
            tvb, offset - 8, 8, ENC_TIME_SECS_NSECS|ENC_BIG_ENDIAN);

    if(tvb_reported_length_remaining(tvb, offset) > 0) /* random data */
        call_data_dissector(tvb_new_subset_remaining(tvb, offset), pinfo,
                lanforge_tree);

    return true;
}


/* Register the protocol with Wireshark */
void proto_register_lanforge(void)
{
    /* Setup list of header fields */

    static hf_register_info hf[] = {

        { &hf_lanforge_crc,
          {
              "CRC", "lanforge.CRC",
              FT_UINT32, BASE_HEX, NULL, 0x0,
              "The LANforge CRC number", HFILL
          }
        },

        { &hf_lanforge_magic,
          {
              "Magic number", "lanforge.magic",
              FT_UINT32, BASE_HEX, NULL, 0x0,
              "The LANforge magic number", HFILL
          }
        },

        { &hf_lanforge_src_session,
          {
              "Source session ID", "lanforge.source-session-id",
              FT_UINT16, BASE_DEC, NULL, 0x0,
              "The LANforge source session ID", HFILL
          }
        },

        { &hf_lanforge_dst_session,
          {
              "Dest session ID", "lanforge.dest-session-id",
              FT_UINT16, BASE_DEC, NULL, 0x0,
              "The LANforge dest session ID", HFILL
          }
        },

        { &hf_lanforge_pld_len_l,
          {
              "Payload Length(L)", "lanforge.pld-len-L",
              FT_UINT16, BASE_DEC, NULL, 0x0,
              "The LANforge payload length (low bytes)", HFILL
          }
        },

        { &hf_lanforge_pld_len_h,
          {
              "Payload Length(H)", "lanforge.pld-len-H",
              FT_UINT8, BASE_DEC, NULL, 0x0,
              "The LANforge payload length (high byte)", HFILL
          }
        },

        { &hf_lanforge_pld_len,
          {
              "Payload Length", "lanforge.pld-length",
              FT_UINT32, BASE_DEC, NULL, 0x0,
              "The LANforge payload length", HFILL
          }
        },

        { &hf_lanforge_pld_pattern,
          {
              "Payload Pattern", "lanforge.pld-pattern",
              FT_UINT16, BASE_DEC, NULL, 0x0,
              "The LANforge payload pattern", HFILL
          }
        },

        { &hf_lanforge_seq,
          {
              "Sequence Number", "lanforge.seqno",
              FT_UINT32, BASE_DEC, NULL, 0x0,
              "The LANforge Sequence Number", HFILL
          }
        },

        { &hf_lanforge_tx_time_s,
          {
              "Timestamp Secs", "lanforge.ts-secs",
              FT_UINT32, BASE_DEC, NULL, 0x0,
              NULL, HFILL
          }
        },

        { &hf_lanforge_tx_time_ns,
          {
              "Timestamp nsecs", "lanforge.ts-nsecs",
              FT_UINT32, BASE_DEC, NULL, 0x0,
              NULL, HFILL
          }
        },

        { &hf_lanforge_timestamp,
          {
              "Timestamp", "lanforge.timestamp",
              FT_ABSOLUTE_TIME, ABSOLUTE_TIME_LOCAL, NULL, 0x0,
              NULL, HFILL
          }
        }
    };

    /* Setup protocol subtree array */

    static int *ett[] = {
        &ett_lanforge
    };

    /* Register the protocol name and description */

    proto_lanforge = proto_register_protocol("LANforge Traffic Generator", "LANforge", "lanforge");

    /* Required function calls to register the header fields and subtrees used */

    proto_register_field_array(proto_lanforge, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));
}


void proto_reg_handoff_lanforge(void)
{
    /* Register as a heuristic UDP dissector */
    heur_dissector_add("udp", dissect_lanforge, "LANforge over UDP", "lanforge_udp", proto_lanforge, HEURISTIC_ENABLE);
    heur_dissector_add("tcp", dissect_lanforge, "LANforge over TCP", "lanforge_tcp", proto_lanforge, HEURISTIC_ENABLE);
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
