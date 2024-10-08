/* packet-sita.c
 * Routines for SITA protocol dissection (ALC, UTS, Frame Relay, X.25)
 * with a SITA specific link layer information header
 *
 * Copyright 2007, Fulko Hew, SITA INC Canada, Inc.
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* Use indentation = 4 */

#include "config.h"


#include <epan/packet.h>
#include <epan/tfs.h>
#include <wsutil/array.h>
#include <wiretap/wtap.h>
void proto_register_sita(void);
void proto_reg_handoff_sita(void);

static dissector_table_t    sita_dissector_table;
static int                  ett_sita;
static int                  ett_sita_flags;
static int                  ett_sita_signals;
static int                  ett_sita_errors1;
static int                  ett_sita_errors2;
static int                  proto_sita;       /* Initialize the protocol and registered fields */
static int                  hf_dir;
static int                  hf_framing;
static int                  hf_parity;
static int                  hf_collision;
static int                  hf_longframe;
static int                  hf_shortframe;
static int                  hf_droppedframe;
static int                  hf_nonaligned;
static int                  hf_abort;
static int                  hf_lostcd;
static int                  hf_lostcts;
static int                  hf_rxdpll;
static int                  hf_overrun;
static int                  hf_length;
static int                  hf_crc;
static int                  hf_break;
static int                  hf_underrun;
static int                  hf_uarterror;
static int                  hf_rtxlimit;
static int                  hf_proto;
static int                  hf_dsr;
static int                  hf_dtr;
static int                  hf_cts;
static int                  hf_rts;
static int                  hf_dcd;
static int                  hf_signals;

static dissector_handle_t  sita_handle;

#define MAX_FLAGS_LEN 64                                    /* max size of a 'flags' decoded string */
#define IOP                 "Local"
#define REMOTE              "Remote"

static const char *
format_flags_string(wmem_allocator_t *scope, unsigned char value, const char *array[])
{
    int         i;
    unsigned    bpos;
    wmem_strbuf_t   *buf;
    const char  *sep = "";

    buf = wmem_strbuf_new_sized(scope, MAX_FLAGS_LEN);
    for (i = 0; i < 8; i++) {
        bpos = 1 << i;
        if (value & bpos) {
            if (array[i][0]) {
                /* there is a string to emit... */
                wmem_strbuf_append_printf(buf, "%s%s", sep,
                    array[i]);
                sep = ", ";
            }
        }
    }
    return wmem_strbuf_get_str(buf);
}

static int
dissect_sita(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
    proto_item  *ti;
    unsigned char      flags, signals, errors1, errors2, proto;
    const char *errors1_string, *errors2_string, *flags_string;
    proto_tree  *sita_tree          = NULL;
    proto_tree  *sita_flags_tree    = NULL;
    proto_tree  *sita_errors1_tree  = NULL;
    proto_tree  *sita_errors2_tree  = NULL;
    static const char *rx_errors1_str[]   = {"Framing",       "Parity",   "Collision",    "Long-frame",   "Short-frame",  "",         "",     ""              };
    static const char *rx_errors2_str[]   = {"Non-Aligned",   "Abort",    "CD-lost",      "DPLL",         "Overrun",      "Length",   "CRC",  "Break"         };
#if 0
    static const char    *tx_errors1_str[]   = {"",              "",         "",             "",             "",             "",         "",     ""              };
#endif
    static const char *tx_errors2_str[]   = {"Underrun",      "CTS-lost", "UART",         "ReTx-limit",   "",             "",         "",     ""              };
    static const char *flags_str[]        = {"",              "",         "",             "",             "",             "",         "",     "No-buffers"    };


    static int * const signal_flags[] = {
        &hf_dcd,
        &hf_rts,
        &hf_cts,
        &hf_dtr,
        &hf_dsr,
        NULL
    };

    col_clear(pinfo->cinfo, COL_PROTOCOL);      /* erase the protocol */
    col_clear(pinfo->cinfo, COL_INFO);          /* and info columns so that the next decoder can fill them in */

    flags   = pinfo->pseudo_header->sita.sita_flags;
    signals = pinfo->pseudo_header->sita.sita_signals;
    errors1 = pinfo->pseudo_header->sita.sita_errors1;
    errors2 = pinfo->pseudo_header->sita.sita_errors2;
    proto   = pinfo->pseudo_header->sita.sita_proto;

    if ((flags & SITA_FRAME_DIR) == SITA_FRAME_DIR_TXED) {
        col_set_str(pinfo->cinfo, COL_DEF_SRC, IOP);  /* set the source (direction) column accordingly */
    } else {
        col_set_str(pinfo->cinfo, COL_DEF_SRC, REMOTE);
    }

    col_clear(pinfo->cinfo, COL_INFO);

    if (tree) {
        ti = proto_tree_add_protocol_format(tree, proto_sita, tvb, 0, 0, "Link Layer");
        sita_tree = proto_item_add_subtree(ti, ett_sita);

        proto_tree_add_uint(sita_tree, hf_proto, tvb, 0, 0, proto);

        flags_string = format_flags_string(pinfo->pool, flags, flags_str);
        sita_flags_tree = proto_tree_add_subtree_format(sita_tree, tvb, 0, 0,
                ett_sita_flags, NULL, "Flags: 0x%02x (From %s)%s%s",
                flags,
                ((flags & SITA_FRAME_DIR) == SITA_FRAME_DIR_TXED) ? IOP : REMOTE,
                strlen(flags_string) ? ", " : "",
                flags_string);
        proto_tree_add_boolean(sita_flags_tree, hf_droppedframe,    tvb, 0, 0, flags);
        proto_tree_add_boolean(sita_flags_tree, hf_dir,             tvb, 0, 0, flags);

        proto_tree_add_bitmask_value_with_flags(sita_tree, tvb, 0, hf_signals, ett_sita_signals,
                                                signal_flags, signals, BMT_NO_FALSE|BMT_NO_TFS);

        if ((flags & SITA_FRAME_DIR) == SITA_FRAME_DIR_RXED) {
            static int * const errors1_flags[] = {
                &hf_shortframe,
                &hf_longframe,
                &hf_collision,
                &hf_parity,
                &hf_framing,
                NULL
            };

            static int * const errors2_flags[] = {
                &hf_break,
                &hf_crc,
                &hf_length,
                &hf_overrun,
                &hf_rxdpll,
                &hf_lostcd,
                &hf_abort,
                &hf_nonaligned,
                NULL
            };

            errors1_string = format_flags_string(pinfo->pool, errors1, rx_errors1_str);
            sita_errors1_tree = proto_tree_add_subtree_format(sita_tree, tvb, 0, 0,
                ett_sita_errors1, NULL, "Receive Status: 0x%02x %s", errors1, errors1_string);
            proto_tree_add_bitmask_list_value(sita_errors1_tree, tvb, 0, 0, errors1_flags, errors1);

            errors2_string = format_flags_string(pinfo->pool, errors2, rx_errors2_str);
            sita_errors2_tree = proto_tree_add_subtree_format(sita_tree, tvb, 0, 0,
                ett_sita_errors2, NULL, "Receive Status: 0x%02x %s", errors2, errors2_string);
            proto_tree_add_bitmask_list_value(sita_errors2_tree, tvb, 0, 0, errors2_flags, errors2);
        } else {
            static int * const errors2_flags[] = {
                &hf_rtxlimit,
                &hf_uarterror,
                &hf_lostcts,
                &hf_underrun,
                NULL
            };

            errors2_string = format_flags_string(pinfo->pool, errors2, tx_errors2_str);
            sita_errors1_tree = proto_tree_add_subtree_format(sita_tree, tvb, 0, 0,
                ett_sita_errors1, NULL, "Transmit Status: 0x%02x %s", errors2, errors2_string);
            proto_tree_add_bitmask_list_value(sita_errors1_tree, tvb, 0, 0, errors2_flags, errors2);
        }
    }

    /* try to find and run an applicable dissector */
    if (!dissector_try_uint(sita_dissector_table, pinfo->pseudo_header->sita.sita_proto, tvb, pinfo, tree)) {
        /* if one can't be found... tell them we don't know how to decode this protocol
           and give them the details then */
        col_set_str(pinfo->cinfo, COL_PROTOCOL, "UNKNOWN");
        col_add_fstr(pinfo->cinfo, COL_INFO, "IOP protocol number: %u", pinfo->pseudo_header->sita.sita_proto);
        call_data_dissector(tvb, pinfo, tree);          /* call the generic (hex display) decoder instead */
    }
    return tvb_captured_length(tvb);
}

static const true_false_string tfs_sita_flags       = { "From Remote",  "From Local"    };
static const true_false_string tfs_sita_error       = { "Error",        ""              };
static const true_false_string tfs_sita_violation   = { "Violation",    ""              };
static const true_false_string tfs_sita_received    = { "Received",     ""              };
static const true_false_string tfs_sita_lost        = { "Lost",         ""              };
static const true_false_string tfs_sita_exceeded    = { "Exceeded",     ""              };

static const value_string tfs_sita_proto[] = {
    { SITA_PROTO_UNUSED,        "Unused"                },
    { SITA_PROTO_BOP_LAPB,      "LAPB"                  },
    { SITA_PROTO_ETHERNET,      "Ethernet"              },
    { SITA_PROTO_ASYNC_INTIO,   "Async (Interrupt I/O)" },
    { SITA_PROTO_ASYNC_BLKIO,   "Async (Block I/O)"     },
    { SITA_PROTO_ALC,           "IPARS"                 },
    { SITA_PROTO_UTS,           "UTS"                   },
    { SITA_PROTO_PPP_HDLC,      "PPP/HDLC"              },
    { SITA_PROTO_SDLC,          "SDLC"                  },
    { SITA_PROTO_TOKENRING,     "Token Ring"            },
    { SITA_PROTO_I2C,           "I2C"                   },
    { SITA_PROTO_DPM_LINK,      "DPM Link"              },
    { SITA_PROTO_BOP_FRL,       "Frame Relay"           },
    { 0,                        NULL                    }
};

void
proto_register_sita(void)
{
    static hf_register_info hf[] = {
        { &hf_proto,
          { "Protocol", "sita.errors.protocol",
            FT_UINT8, BASE_HEX, VALS(tfs_sita_proto), 0,
            "Protocol value", HFILL }
        },

        { &hf_dir,
          { "Direction", "sita.flags.flags",
            FT_BOOLEAN, 8, TFS(&tfs_sita_flags), SITA_FRAME_DIR,
            "true 'from Remote', false 'from Local'",   HFILL }
        },
        { &hf_droppedframe,
          { "No Buffers", "sita.flags.droppedframe",
            FT_BOOLEAN, 8, TFS(&tfs_sita_error), SITA_ERROR_NO_BUFFER,
            "true if Buffer Failure", HFILL }
        },

        { &hf_framing,
          { "Framing", "sita.errors.framing",
            FT_BOOLEAN, 8, TFS(&tfs_sita_error), SITA_ERROR_RX_FRAMING,
            "true if Framing Error", HFILL }
        },
        { &hf_parity,
          { "Parity", "sita.errors.parity",
            FT_BOOLEAN, 8, TFS(&tfs_sita_error), SITA_ERROR_RX_PARITY,
            "true if Parity Error", HFILL }
        },
        { &hf_collision,
          { "Collision", "sita.errors.collision",
            FT_BOOLEAN, 8, TFS(&tfs_sita_error), SITA_ERROR_RX_COLLISION,
            "true if Collision", HFILL }
        },
        { &hf_longframe,
          { "Long Frame", "sita.errors.longframe",
            FT_BOOLEAN, 8, TFS(&tfs_sita_error), SITA_ERROR_RX_FRAME_LONG,
            "true if Long Frame Received", HFILL }
        },
        { &hf_shortframe,
          { "Short Frame", "sita.errors.shortframe",
            FT_BOOLEAN, 8, TFS(&tfs_sita_error), SITA_ERROR_RX_FRAME_SHORT,
            "true if Short Frame", HFILL }
        },
        { &hf_nonaligned,
          { "NonAligned", "sita.errors.nonaligned",
            FT_BOOLEAN, 8, TFS(&tfs_sita_error), SITA_ERROR_RX_NONOCTET_ALIGNED,
            "true if NonAligned Frame", HFILL }
        },
        { &hf_abort,
          { "Abort", "sita.errors.abort",
            FT_BOOLEAN, 8, TFS(&tfs_sita_received), SITA_ERROR_RX_ABORT,
            "true if Abort Received", HFILL }
        },
        { &hf_lostcd,
          { "Carrier", "sita.errors.lostcd",
            FT_BOOLEAN, 8, TFS(&tfs_sita_lost), SITA_ERROR_RX_CD_LOST,
            "true if Carrier Lost", HFILL }
        },
        { &hf_rxdpll,
          { "DPLL", "sita.errors.rxdpll",
            FT_BOOLEAN, 8, TFS(&tfs_sita_error), SITA_ERROR_RX_DPLL,
            "true if DPLL Error", HFILL }
        },
        { &hf_overrun,
          { "Overrun", "sita.errors.overrun",
            FT_BOOLEAN, 8, TFS(&tfs_sita_error), SITA_ERROR_RX_OVERRUN,
            "true if Overrun Error", HFILL }
        },
        { &hf_length,
          { "Length", "sita.errors.length",
            FT_BOOLEAN, 8, TFS(&tfs_sita_violation), SITA_ERROR_RX_FRAME_LEN_VIOL,
            "true if Length Violation", HFILL }
        },
        { &hf_crc,
          { "CRC", "sita.errors.crc",
            FT_BOOLEAN, 8, TFS(&tfs_sita_error), SITA_ERROR_RX_CRC,
            "true if CRC Error", HFILL }
        },
        { &hf_break,
          { "Break", "sita.errors.break",
            FT_BOOLEAN, 8, TFS(&tfs_sita_received), SITA_ERROR_RX_BREAK,
            "true if Break Received", HFILL }
        },

        { &hf_underrun,
          { "Underrun", "sita.errors.underrun",
            FT_BOOLEAN, 8, TFS(&tfs_sita_error), SITA_ERROR_TX_UNDERRUN,
            "true if Tx Underrun", HFILL }
        },
        { &hf_lostcts,
          { "Clear To Send", "sita.errors.lostcts",
            FT_BOOLEAN, 8, TFS(&tfs_sita_lost), SITA_ERROR_TX_CTS_LOST,
            "true if Clear To Send Lost", HFILL }
        },
        { &hf_uarterror,
          { "UART", "sita.errors.uarterror",
            FT_BOOLEAN, 8, TFS(&tfs_sita_error), SITA_ERROR_TX_UART_ERROR,
            "true if UART Error", HFILL }
        },
        { &hf_rtxlimit,
          { "Retx Limit", "sita.errors.rtxlimit",
            FT_BOOLEAN, 8, TFS(&tfs_sita_exceeded), SITA_ERROR_TX_RETX_LIMIT,
            "true if Retransmit Limit reached", HFILL }
        },

        { &hf_dsr,
          { "DSR", "sita.signals.dsr",
            FT_BOOLEAN, 8, TFS(&tfs_on_off), SITA_SIG_DSR,
            "true if Data Set Ready", HFILL }
        },
        { &hf_dtr,
          { "DTR", "sita.signals.dtr",
            FT_BOOLEAN, 8, TFS(&tfs_on_off), SITA_SIG_DTR,
            "true if Data Terminal Ready", HFILL }
        },
        { &hf_cts,
          { "CTS", "sita.signals.cts",
            FT_BOOLEAN, 8, TFS(&tfs_on_off), SITA_SIG_CTS,
            "true if Clear To Send", HFILL }
        },
        { &hf_rts,
          { "RTS", "sita.signals.rts",
            FT_BOOLEAN, 8, TFS(&tfs_on_off), SITA_SIG_RTS,
            "true if Request To Send", HFILL }
        },
        { &hf_dcd,
          { "DCD", "sita.signals.dcd",
            FT_BOOLEAN, 8, TFS(&tfs_on_off), SITA_SIG_DCD,
            "true if Data Carrier Detect", HFILL }
        },
        { &hf_signals,
          { "Signals", "sita.signals",
            FT_UINT8, BASE_HEX, NULL, 0,
            NULL, HFILL }
        },
    };

    static int *ett[] = {
        &ett_sita,
        &ett_sita_flags,
        &ett_sita_signals,
        &ett_sita_errors1,
        &ett_sita_errors2,
    };

    proto_sita = proto_register_protocol("Societe Internationale de Telecommunications Aeronautiques", "SITA", "sita"); /* name, short name,abbreviation */
    sita_dissector_table = register_dissector_table("sita.proto", "SITA protocol number", proto_sita, FT_UINT8, BASE_HEX);
    proto_register_field_array(proto_sita, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));
    sita_handle = register_dissector("sita", dissect_sita, proto_sita);
}

void
proto_reg_handoff_sita(void)
{
    dissector_handle_t  lapb_handle;
    dissector_handle_t  frame_relay_handle;
    dissector_handle_t  uts_handle;
    dissector_handle_t  ipars_handle;

    lapb_handle     = find_dissector("lapb");
    frame_relay_handle  = find_dissector("fr");
    uts_handle      = find_dissector("uts");
    ipars_handle        = find_dissector("ipars");

    dissector_add_uint("sita.proto", SITA_PROTO_BOP_LAPB,   lapb_handle);
    dissector_add_uint("sita.proto", SITA_PROTO_BOP_FRL,        frame_relay_handle);
    dissector_add_uint("sita.proto", SITA_PROTO_UTS,        uts_handle);
    dissector_add_uint("sita.proto", SITA_PROTO_ALC,        ipars_handle);
    dissector_add_uint("wtap_encap", WTAP_ENCAP_SITA,       sita_handle);
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
