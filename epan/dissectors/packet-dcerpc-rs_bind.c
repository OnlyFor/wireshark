/* packet-dcerpc-rs_bind.c
 *
 * Routines for DFS/RS_BIND
 * Copyright 2003, Jaime Fournier <Jaime.Fournier@hush.com>
 * This information is based off the released idl files from opengroup.
 * ftp://ftp.opengroup.org/pub/dce122/dce/src/security.tar.gz security/idl/rs_bind.idl
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

void proto_register_rs_bind (void);
void proto_reg_handoff_rs_bind (void);

static int proto_rs_bind;
static int hf_rs_bind_opnum;

static int ett_rs_bind;


static e_guid_t uuid_rs_bind =
  { 0xd46113d0, 0xa848, 0x11cb, {0xb8, 0x63, 0x08, 0x00, 0x1e, 0x04, 0x6a,
                                 0xa5}

};
static uint16_t ver_rs_bind = 2;


static const dcerpc_sub_dissector rs_bind_dissectors[] = {
  {0, "get_update_site", NULL, NULL},
  {0, NULL, NULL, NULL},

};

void
proto_register_rs_bind (void)
{
  static hf_register_info hf[] = {
        { &hf_rs_bind_opnum,
                { "Operation", "rs_bind.opnum", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }},
  };

  static int *ett[] = {
    &ett_rs_bind,
  };
  proto_rs_bind =
    proto_register_protocol ("DCE/RPC RS_BIND", "RS_BIND", "rs_bind");
  proto_register_field_array (proto_rs_bind, hf, array_length (hf));
  proto_register_subtree_array (ett, array_length (ett));
}

void
proto_reg_handoff_rs_bind (void)
{
  /* Register the protocol as dcerpc */
  dcerpc_init_uuid (proto_rs_bind, ett_rs_bind, &uuid_rs_bind, ver_rs_bind,
                    rs_bind_dissectors, hf_rs_bind_opnum);
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
