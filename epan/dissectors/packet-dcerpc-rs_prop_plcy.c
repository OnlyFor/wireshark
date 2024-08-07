/* packet-dcerpc-rs_prop_plcy.c
 *
 * Routines for rs_prop_plcy dissection
 * Copyright 2004, Jaime Fournier <jaime.fournier@hush.com>
 * This information is based off the released idl files from opengroup.
 * ftp://ftp.opengroup.org/pub/dce122/dce/src/security.tar.gz security/idl/rs_prop_plcy.idl
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"


#include <epan/packet.h>
#include "packet-dcerpc.h"

void proto_register_rs_prop_plcy (void);
void proto_reg_handoff_rs_prop_plcy (void);

static int proto_rs_prop_plcy;
static int hf_rs_prop_plcy_opnum;


static int ett_rs_prop_plcy;
static e_guid_t uuid_rs_prop_plcy =
  { 0xe6ac5cb8, 0xde3e, 0x11ca, {0x93, 0x76, 0x08, 0x00, 0x1e, 0x03, 0x94,
                                 0xc7} };

static uint16_t ver_rs_prop_plcy = 1;


static const dcerpc_sub_dissector rs_prop_plcy_dissectors[] = {
  {0, "rs_prop_properties_set_info",     NULL, NULL},
  {1, "rs_prop_plcy_set_info",           NULL, NULL},
  {2, "rs_prop_auth_plcy_set_info",      NULL, NULL},
  {3, "rs_prop_plcy_set_dom_cache_info", NULL, NULL},
  {0, NULL, NULL, NULL}
};

void
proto_register_rs_prop_plcy (void)
{
  static hf_register_info hf[] = {
    {&hf_rs_prop_plcy_opnum,
     {"Operation", "rs_prop_plcy.opnum", FT_UINT16, BASE_DEC, NULL, 0x0,
      NULL, HFILL}},
  };

  static int *ett[] = {
    &ett_rs_prop_plcy,
  };
  proto_rs_prop_plcy =
    proto_register_protocol
    ("DCE/RPC Registry server propagation interface - properties and policies",
     "rs_prop_plcy", "rs_prop_plcy");
  proto_register_field_array (proto_rs_prop_plcy, hf, array_length (hf));
  proto_register_subtree_array (ett, array_length (ett));
}

void
proto_reg_handoff_rs_prop_plcy (void)
{
  /* Register the protocol as dcerpc */
  dcerpc_init_uuid (proto_rs_prop_plcy, ett_rs_prop_plcy, &uuid_rs_prop_plcy,
                    ver_rs_prop_plcy, rs_prop_plcy_dissectors,
                    hf_rs_prop_plcy_opnum);
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
