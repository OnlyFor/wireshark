/* packet-bjnp.c
 * Routines for Canon BJNP packet disassembly.
 *
 * Copyright 2009, Stig Bjorlykke <stig@bjorlykke.org>
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <epan/packet.h>
#define PNAME  "Canon BJNP"
#define PSNAME "BJNP"
#define PFNAME "bjnp"

#define BJNP_PORT_RANGE    "8611-8614"

/* dev_type */
#define PRINTER_COMMAND    0x01
#define SCANNER_COMMAND    0x02
#define PRINTER_RESPONSE   0x81
#define SCANNER_RESPONSE   0x82

/* cmd_code */
#define CMD_DISCOVER       0x01
#define CMD_PRINT_JOB_DET  0x10
#define CMD_CLOSE          0x11
#define CMD_GET_STATUS     0x20
#define CMD_PRINT          0x21
#define CMD_GET_ID         0x30
#define CMD_SCAN_JOB       0x32

void proto_register_bjnp(void);
void proto_reg_handoff_bjnp(void);

static int proto_bjnp;

static int hf_bjnp_id;
static int hf_dev_type;
static int hf_cmd_code;
static int hf_seq_no;
static int hf_session_id;
static int hf_payload_len;
static int hf_payload;

static int ett_bjnp;

static dissector_handle_t bjnp_handle;

static const value_string dev_type_vals[] = {
  { PRINTER_COMMAND,    "Printer Command"       },
  { SCANNER_COMMAND,    "Scanner Command"       },
  { PRINTER_RESPONSE,   "Printer Response"      },
  { SCANNER_RESPONSE,   "Scanner Response"      },
  { 0, NULL }
};

static const value_string cmd_code_vals[] = {
  { CMD_DISCOVER,       "Discover"              },
  { CMD_PRINT_JOB_DET,  "Print Job Details"     },
  { CMD_CLOSE,          "Request Closure"       },
  { CMD_GET_STATUS,     "Get Printer Status"    },
  { CMD_PRINT,          "Print"                 },
  { CMD_GET_ID,         "Get Printer Identity"  },
  { CMD_SCAN_JOB,       "Scan Job Details"      },
  { 0, NULL }
};

static int dissect_bjnp (tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
  proto_tree *bjnp_tree;
  proto_item *ti;
  int         offset = 0;
  uint32_t    payload_len;
  uint8_t     dev_type, cmd_code;
  char       *info;

  /* If it does not start with a printable character it's not BJNP */
  if(!g_ascii_isprint(tvb_get_uint8(tvb, 0)))
    return 0;

  col_set_str (pinfo->cinfo, COL_PROTOCOL, PSNAME);
  col_clear (pinfo->cinfo, COL_INFO);

  ti = proto_tree_add_item (tree, proto_bjnp, tvb, offset, -1, ENC_NA);
  bjnp_tree = proto_item_add_subtree (ti, ett_bjnp);

  proto_tree_add_item (bjnp_tree, hf_bjnp_id, tvb, offset, 4, ENC_ASCII);
  offset += 4;

  dev_type = tvb_get_uint8 (tvb, offset);
  proto_tree_add_item (bjnp_tree, hf_dev_type, tvb, offset, 1, ENC_BIG_ENDIAN);
  offset++;

  cmd_code = tvb_get_uint8 (tvb, offset);
  proto_tree_add_item (bjnp_tree, hf_cmd_code, tvb, offset, 1, ENC_BIG_ENDIAN);
  offset++;

  info = wmem_strdup_printf(pinfo->pool, "%s: %s",
                            val_to_str (dev_type, dev_type_vals, "Unknown type (%d)"),
                            val_to_str (cmd_code, cmd_code_vals, "Unknown code (%d)"));

  proto_item_append_text (ti, ", %s", info);
  col_add_str (pinfo->cinfo, COL_INFO, info);

  proto_tree_add_item (bjnp_tree, hf_seq_no, tvb, offset, 4, ENC_BIG_ENDIAN);
  offset += 4;

  proto_tree_add_item (bjnp_tree, hf_session_id, tvb, offset, 2, ENC_BIG_ENDIAN);
  offset += 2;

  payload_len = tvb_get_ntohl (tvb, offset);
  proto_tree_add_item (bjnp_tree, hf_payload_len, tvb, offset, 4, ENC_BIG_ENDIAN);
  offset += 4;

  if (payload_len > 0) {
    /* TBD: Dissect various commands */
    proto_tree_add_item (bjnp_tree, hf_payload, tvb, offset, payload_len, ENC_NA);
    offset += payload_len;
  }
  return offset;
}

void proto_register_bjnp (void)
{
  static hf_register_info hf[] = {
    { &hf_bjnp_id,
      { "Id", "bjnp.id", FT_STRING, BASE_NONE,
        NULL, 0x0, NULL, HFILL } },
    { &hf_dev_type,
      { "Type", "bjnp.type", FT_UINT8, BASE_DEC,
        VALS(dev_type_vals), 0x0, NULL, HFILL } },
    { &hf_cmd_code,
      { "Code", "bjnp.code", FT_UINT8, BASE_DEC,
        VALS(cmd_code_vals), 0x0, NULL, HFILL } },
    { &hf_seq_no,
      { "Sequence Number", "bjnp.seq_no", FT_UINT32, BASE_DEC,
        NULL, 0x0, NULL, HFILL } },
    { &hf_session_id,
      { "Session Id", "bjnp.session_id", FT_UINT16, BASE_DEC,
        NULL, 0x0, NULL, HFILL } },
    { &hf_payload_len,
      { "Payload Length", "bjnp.payload_len", FT_UINT32, BASE_DEC,
        NULL, 0x0, NULL, HFILL } },
    { &hf_payload,
      { "Payload", "bjnp.payload", FT_BYTES, BASE_NONE,
        NULL, 0x0, NULL, HFILL } },
  };

  static int *ett[] = {
    &ett_bjnp
  };

  proto_bjnp = proto_register_protocol (PNAME, PSNAME, PFNAME);

  bjnp_handle = register_dissector (PFNAME, dissect_bjnp, proto_bjnp);

  proto_register_field_array (proto_bjnp, hf, array_length (hf));
  proto_register_subtree_array (ett, array_length (ett));
}

void proto_reg_handoff_bjnp (void)
{
  dissector_add_uint_range_with_preference("udp.port", BJNP_PORT_RANGE, bjnp_handle);
}

/*
 * Editor modelines
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
