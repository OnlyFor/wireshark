/* packet-lsc.c
 * Routines for Pegasus LSC packet disassembly
 * Copyright 2006, Sean Sheedy <seansh@users.sourceforge.net>
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <epan/packet.h>

#include "packet-tcp.h"

void proto_register_lsc(void);
void proto_reg_handoff_lsc(void);

#define LSC_PAUSE        0x01
#define LSC_RESUME       0x02
#define LSC_STATUS       0x03
#define LSC_RESET        0x04
#define LSC_JUMP         0x05
#define LSC_PLAY         0x06
#define LSC_DONE         0x40
#define LSC_PAUSE_REPLY  0x81
#define LSC_RESUME_REPLY 0x82
#define LSC_STATUS_REPLY 0x83
#define LSC_RESET_REPLY  0x84
#define LSC_JUMP_REPLY   0x85
#define LSC_PLAY_REPLY   0x86

#define isReply(o) ((o) & 0x80)

static const value_string op_code_vals[] = {
  { LSC_PAUSE,          "LSC_PAUSE" },
  { LSC_RESUME,         "LSC_RESUME" },
  { LSC_STATUS,         "LSC_STATUS" },
  { LSC_RESET,          "LSC_RESET" },
  { LSC_JUMP,           "LSC_JUMP" },
  { LSC_PLAY,           "LSC_PLAY" },
  { LSC_DONE,           "LSC_DONE" },
  { LSC_PAUSE_REPLY,    "LSC_PAUSE_REPLY" },
  { LSC_RESUME_REPLY,   "LSC_RESUME_REPLY" },
  { LSC_STATUS_REPLY,   "LSC_STATUS_REPLY" },
  { LSC_RESET_REPLY,    "LSC_RESET_REPLY" },
  { LSC_JUMP_REPLY,     "LSC_JUMP_REPLY" },
  { LSC_PLAY_REPLY,     "LSC_PLAY_REPLY" },
  { 0,                  NULL }
};

#define LSC_OPCODE_LEN   3              /* Length to find op code */
#define LSC_MIN_LEN      8              /* Minimum packet length */
/* Length of each packet type */
#define LSC_PAUSE_LEN   12
#define LSC_RESUME_LEN  16
#define LSC_STATUS_LEN   8
#define LSC_RESET_LEN    8
#define LSC_JUMP_LEN    20
#define LSC_PLAY_LEN    20
#define LSC_REPLY_LEN   17

static const value_string status_code_vals[] = {
  { 0x00,       "LSC_OK" },
  { 0x01,       "TRICK_PLAY_NO_LONGER_CONSTRAINED" },
  { 0x04,       "TRICK_PLAY_CONSTRAINED" },
  { 0x05,       "SKIPPED_PLAYLIST_ITEM" },
  { 0x10,       "LSC_BAD_REQUEST" },
  { 0x11,       "LSC_BAD_STREAM" },
  { 0x12,       "LSC_WRONG_STATE" },
  { 0x13,       "LSC_UNKNOWN" },
  { 0x14,       "LSC_NO_PERMISSION" },
  { 0x15,       "LSC_BAD_PARAM" },
  { 0x16,       "LSC_NO_IMPLEMENT" },
  { 0x17,       "LSC_NO_MEMORY" },
  { 0x18,       "LSC_IMP_LIMIT" },
  { 0x19,       "LSC_TRANSIENT" },
  { 0x1a,       "LSC_NO_RESOURCES" },
  { 0x20,       "LSC_SERVER_ERROR" },
  { 0x21,       "LSC_SERVER_FAILURE" },
  { 0x30,       "LSC_BAD_SCALE" },
  { 0x31,       "LSC_BAD_START" },
  { 0x32,       "LSC_BAD_STOP" },
  { 0x40,       "LSC_MPEG_DELIVERY" },
  { 0,          NULL }
};

static const value_string mode_vals[] = {
  { 0x00,       "O   - Open Mode" },
  { 0x01,       "P   - Pause Mode" },
  { 0x02,       "ST  - Search Transport" },
  { 0x03,       "T   - Transport" },
  { 0x04,       "TP  - Transport Pause" },
  { 0x05,       "STP - Search Transport Pause" },
  { 0x06,       "PST - Pause Search Transport" },
  { 0x07,       "EOS - End of Stream" },
  { 0,          NULL }
};

/* Initialize the protocol and registered fields */
static int proto_lsc;
static int hf_lsc_version;
static int hf_lsc_trans_id;
static int hf_lsc_op_code;
static int hf_lsc_status_code;
static int hf_lsc_stream_handle;
static int hf_lsc_start_npt;
static int hf_lsc_stop_npt;
static int hf_lsc_current_npt;
static int hf_lsc_scale_num;
static int hf_lsc_scale_denom;
static int hf_lsc_mode;

/* Initialize the subtree pointers */
static int ett_lsc;

static dissector_handle_t lsc_udp_handle;
static dissector_handle_t lsc_tcp_handle;

/* Code to actually dissect the packets */
static int
dissect_lsc_common(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
  proto_item *ti;
  proto_tree *lsc_tree;
  uint8_t op_code;
  uint32_t stream;
  unsigned expected_len;

  /* Too little data? */
  if (tvb_captured_length(tvb) < LSC_MIN_LEN)
    return 0;

  /* Protocol is LSC, packet summary is not yet known */
  col_set_str(pinfo->cinfo, COL_PROTOCOL, "LSC");
  col_clear(pinfo->cinfo, COL_INFO);

  /* Get the op code */
  op_code = tvb_get_uint8(tvb, 2);
  /* And the stream handle */
  stream = tvb_get_ntohl(tvb, 4);

  /* Check the data length against what we actually received */
  switch (op_code)
    {
      case LSC_PAUSE:
        expected_len = LSC_PAUSE_LEN;
        break;
      case LSC_RESUME:
        expected_len = LSC_RESUME_LEN;
        break;
      case LSC_STATUS:
        expected_len = LSC_STATUS_LEN;
        break;
      case LSC_RESET:
        expected_len = LSC_RESET_LEN;
        break;
      case LSC_JUMP:
        expected_len = LSC_JUMP_LEN;
        break;
      case LSC_PLAY:
        expected_len = LSC_PLAY_LEN;
        break;
      case LSC_DONE:
      case LSC_PAUSE_REPLY:
      case LSC_RESUME_REPLY:
      case LSC_STATUS_REPLY:
      case LSC_RESET_REPLY:
      case LSC_JUMP_REPLY:
      case LSC_PLAY_REPLY:
        expected_len = LSC_REPLY_LEN;
        break;
      default:
        /* Unrecognized op code */
        expected_len = LSC_MIN_LEN;
        break;
    }

  /* Display the op code in the summary */
  col_add_fstr(pinfo->cinfo, COL_INFO, "%s, session %.8u",
          val_to_str(op_code, op_code_vals, "Unknown op code (0x%x)"), stream);

  if (tvb_reported_length(tvb) < expected_len)
    col_append_str(pinfo->cinfo, COL_INFO, " [Too short]");
  else if (tvb_reported_length(tvb) > expected_len)
    col_append_str(pinfo->cinfo, COL_INFO, " [Too long]");

  if (tree) {
    /* Create display subtree for the protocol */
    ti = proto_tree_add_item(tree, proto_lsc, tvb, 0, -1, ENC_NA);
    lsc_tree = proto_item_add_subtree(ti, ett_lsc);

    /* LSC Version */
    proto_tree_add_item(lsc_tree, hf_lsc_version, tvb, 0, 1, ENC_BIG_ENDIAN);

    /* Transaction ID */
    proto_tree_add_item(lsc_tree, hf_lsc_trans_id, tvb, 1, 1, ENC_BIG_ENDIAN);

    /* Op Code */
    proto_tree_add_uint(lsc_tree, hf_lsc_op_code, tvb, 2, 1, op_code);

    /* Only replies and LSC_DONE contain a status code */
    if (isReply(op_code) || op_code==LSC_DONE)
      proto_tree_add_item(lsc_tree, hf_lsc_status_code, tvb, 3, 1, ENC_BIG_ENDIAN);

    /* Stream Handle */
    proto_tree_add_uint_format_value(lsc_tree, hf_lsc_stream_handle, tvb, 4, 4, stream, "%.8u", stream);

    /* Add op code specific parts */
    switch (op_code)
      {
        case LSC_PAUSE:
          proto_tree_add_item(lsc_tree, hf_lsc_stop_npt, tvb, 8, 4, ENC_BIG_ENDIAN);
          break;
        case LSC_RESUME:
          proto_tree_add_item(lsc_tree, hf_lsc_start_npt, tvb, 8, 4, ENC_BIG_ENDIAN);
          proto_tree_add_item(lsc_tree, hf_lsc_scale_num, tvb, 12, 2, ENC_BIG_ENDIAN);
          proto_tree_add_item(lsc_tree, hf_lsc_scale_denom, tvb, 14, 2, ENC_BIG_ENDIAN);
          break;
        case LSC_JUMP:
        case LSC_PLAY:
          proto_tree_add_item(lsc_tree, hf_lsc_start_npt, tvb, 8, 4, ENC_BIG_ENDIAN);
          proto_tree_add_item(lsc_tree, hf_lsc_stop_npt, tvb, 12, 4, ENC_BIG_ENDIAN);
          proto_tree_add_item(lsc_tree, hf_lsc_scale_num, tvb, 16, 2, ENC_BIG_ENDIAN);
          proto_tree_add_item(lsc_tree, hf_lsc_scale_denom, tvb, 18, 2, ENC_BIG_ENDIAN);
          break;
        case LSC_DONE:
        case LSC_PAUSE_REPLY:
        case LSC_RESUME_REPLY:
        case LSC_STATUS_REPLY:
        case LSC_RESET_REPLY:
        case LSC_JUMP_REPLY:
        case LSC_PLAY_REPLY:
          proto_tree_add_item(lsc_tree, hf_lsc_current_npt, tvb, 8, 4, ENC_BIG_ENDIAN);
          proto_tree_add_item(lsc_tree, hf_lsc_scale_num, tvb, 12, 2, ENC_BIG_ENDIAN);
          proto_tree_add_item(lsc_tree, hf_lsc_scale_denom, tvb, 14, 2, ENC_BIG_ENDIAN);
          proto_tree_add_item(lsc_tree, hf_lsc_mode, tvb, 16, 1, ENC_BIG_ENDIAN);
          break;
        default:
          break;
      }
  }

  return tvb_captured_length(tvb);
}

/* Decode LSC over UDP */
static int
dissect_lsc_udp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data)
{
  return dissect_lsc_common(tvb, pinfo, tree, data);
}

/* Determine length of LSC message */
static unsigned
get_lsc_pdu_len(packet_info *pinfo _U_, tvbuff_t *tvb, int offset, void *data _U_)
{
  uint8_t op_code;
  unsigned pdu_len;

  /* Get the op code */
  op_code = tvb_get_uint8(tvb, offset + 2);

  switch (op_code)
    {
      case LSC_PAUSE:
        pdu_len = LSC_PAUSE_LEN;
        break;
      case LSC_RESUME:
        pdu_len = LSC_RESUME_LEN;
        break;
      case LSC_STATUS:
        pdu_len = LSC_STATUS_LEN;
        break;
      case LSC_RESET:
        pdu_len = LSC_RESET_LEN;
        break;
      case LSC_JUMP:
        pdu_len = LSC_JUMP_LEN;
        break;
      case LSC_PLAY:
        pdu_len = LSC_PLAY_LEN;
        break;
      case LSC_DONE:
      case LSC_PAUSE_REPLY:
      case LSC_RESUME_REPLY:
      case LSC_STATUS_REPLY:
      case LSC_RESET_REPLY:
      case LSC_JUMP_REPLY:
      case LSC_PLAY_REPLY:
        pdu_len = LSC_REPLY_LEN;
        break;
      default:
        /* Unrecognized op code */
        pdu_len = LSC_OPCODE_LEN;
        break;
    }

  return pdu_len;
}

/* Decode LSC over TCP */
static int
dissect_lsc_tcp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data)
{
  tcp_dissect_pdus(tvb, pinfo, tree, true, LSC_OPCODE_LEN, get_lsc_pdu_len,
                   dissect_lsc_common, data);
  return tvb_captured_length(tvb);
}

/* Register the protocol with Wireshark */
void
proto_register_lsc(void)
{
  /* Setup list of header fields */
  static hf_register_info hf[] = {
    { &hf_lsc_version,
      { "Version", "lsc.version",
        FT_UINT8, BASE_DEC, NULL, 0,
        "Version of the Pegasus LSC protocol", HFILL }
    },
    { &hf_lsc_trans_id,
      { "Transaction ID", "lsc.trans_id",
        FT_UINT8, BASE_DEC, NULL, 0,
        NULL, HFILL }
    },
    { &hf_lsc_op_code,
      { "Op Code", "lsc.op_code",
        FT_UINT8, BASE_HEX, VALS(op_code_vals), 0,
        "Operation Code", HFILL }
    },
    { &hf_lsc_status_code,
      { "Status Code", "lsc.status_code",
        FT_UINT8, BASE_HEX, VALS(status_code_vals), 0,
        NULL, HFILL }
    },
    { &hf_lsc_stream_handle,
      { "Stream Handle", "lsc.stream_handle",
        FT_UINT32, BASE_DEC, NULL, 0,
        "Stream identification handle", HFILL }
    },
    { &hf_lsc_start_npt,
      { "Start NPT", "lsc.start_npt",
        FT_INT32, BASE_DEC, NULL, 0,
        "Start Time (milliseconds)", HFILL }
    },
    { &hf_lsc_stop_npt,
      { "Stop NPT", "lsc.stop_npt",
        FT_INT32, BASE_DEC, NULL, 0,
        "Stop Time (milliseconds)", HFILL }
    },
    { &hf_lsc_current_npt,
      { "Current NPT", "lsc.current_npt",
        FT_INT32, BASE_DEC, NULL, 0,
        "Current Time (milliseconds)", HFILL }
    },
    { &hf_lsc_scale_num,
      { "Scale Numerator", "lsc.scale_num",
        FT_INT16, BASE_DEC, NULL, 0,
        NULL, HFILL }
    },
    { &hf_lsc_scale_denom,
      { "Scale Denominator", "lsc.scale_denum",
        FT_UINT16, BASE_DEC, NULL, 0,
        NULL, HFILL }
    },
    { &hf_lsc_mode,
      { "Server Mode", "lsc.mode",
        FT_UINT8, BASE_HEX, VALS(mode_vals), 0,
        "Current Server Mode", HFILL }
    }
  };

  /* Setup protocol subtree array */
  static int *ett[] = {
    &ett_lsc,
  };

  /* Register the protocol name and description */
  proto_lsc = proto_register_protocol("Pegasus Lightweight Stream Control", "LSC", "lsc");
  lsc_udp_handle = register_dissector("lsc_udp", dissect_lsc_udp, proto_lsc);
  lsc_tcp_handle = register_dissector("lsc_tcp", dissect_lsc_tcp, proto_lsc);

  /* Required function calls to register the header fields and subtrees used */
  proto_register_field_array(proto_lsc, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));
}

void
proto_reg_handoff_lsc(void)
{
  dissector_add_for_decode_as_with_preference("udp.port", lsc_udp_handle);
  dissector_add_for_decode_as_with_preference("tcp.port", lsc_tcp_handle);
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
