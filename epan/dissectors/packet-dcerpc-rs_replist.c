/* packet-dcerpc-rs_replist.c
 *
 * Routines for dcerpc RepServer Calls
 * Copyright 2002, Jaime Fournier <Jaime.Fournier@hush.com>
 * This information is based off the released idl files from opengroup.
 * ftp://ftp.opengroup.org/pub/dce122/dce/src/security.tar.gz security/idl/rs_repadm.idl
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

void proto_register_rs_replist (void);
void proto_reg_handoff_rs_replist (void);

static int proto_rs_replist;
static int hf_rs_replist_opnum;


static int ett_rs_replist;


static e_guid_t uuid_rs_replist = { 0x850446b0, 0xe95b, 0x11CA, { 0xad, 0x90, 0x08, 0x00, 0x1e, 0x01, 0x45, 0xb1 } };
static uint16_t ver_rs_replist = 2;


static const dcerpc_sub_dissector rs_replist_dissectors[] = {
	{ 0, "rs_replist_add_replica",     NULL, NULL},
	{ 1, "rs_replist_replace_replica", NULL, NULL},
	{ 2, "rs_replist_delete_replica",  NULL, NULL},
	{ 3, "rs_replist_read",            NULL, NULL},
	{ 4, "rs_replist_read_full",       NULL, NULL},
	{ 5, "rs_replist_add_replica",     NULL, NULL},
	{ 6, "rs_replist_replace_replica", NULL, NULL},
	{ 7, "rs_replist_delete_replica",  NULL, NULL},
	{ 8, "rs_replist_read",            NULL, NULL},
	{ 9, "rs_replist_read_full",       NULL, NULL},
	{ 0, NULL, NULL, NULL }
};

void
proto_register_rs_replist (void)
{
	static hf_register_info hf[] = {
		{ &hf_rs_replist_opnum,
		{ "Operation", "rs_replist.opnum", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }},
	};

	static int *ett[] = {
		&ett_rs_replist,
	};
	proto_rs_replist = proto_register_protocol ("DCE/RPC Repserver Calls", "RS_REPLIST", "rs_replist");

	proto_register_field_array (proto_rs_replist, hf, array_length (hf));
	proto_register_subtree_array (ett, array_length (ett));
}

void
proto_reg_handoff_rs_replist (void)
{
	/* Register the protocol as dcerpc */
	dcerpc_init_uuid (proto_rs_replist, ett_rs_replist, &uuid_rs_replist, ver_rs_replist, rs_replist_dissectors, hf_rs_replist_opnum);
}

/*
 * Editor modelines  -  https://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 8
 * tab-width: 8
 * indent-tabs-mode: t
 * End:
 *
 * vi: set shiftwidth=8 tabstop=8 noexpandtab:
 * :indentSize=8:tabSize=8:noTabs=false:
 */
