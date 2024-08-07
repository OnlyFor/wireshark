/* packet-stat.c
 * Routines for async NSM stat callback dissection
 * 2001 Ronnie Sahlberg <See AUTHORS for email>
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include "packet-rpc.h"
#include "packet-stat-notify.h"

void proto_register_statnotify(void);
void proto_reg_handoff_statnotify(void);

static int proto_statnotify;
static int hf_statnotify_procedure_v1;
static int hf_statnotify_name;
static int hf_statnotify_state;
static int hf_statnotify_priv;

static int ett_statnotify;


static int
dissect_statnotify_mon(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, void* data _U_)
{
	int offset = 0;

	offset = dissect_rpc_string(tvb,tree,hf_statnotify_name,offset,NULL);

	offset = dissect_rpc_uint32(tvb,tree,hf_statnotify_state,offset);

	proto_tree_add_item(tree,hf_statnotify_priv,tvb,offset,16,ENC_NA);
	offset += 16;

	return offset;
}

/* proc number, "proc name", dissect_request, dissect_reply */

static const vsff statnotify1_proc[] = {
	{ 0, "NULL",
	  dissect_rpc_void, dissect_rpc_void },
	{ STATNOTIFYPROC_MON,   "MON-CALLBACK",
	  dissect_statnotify_mon, dissect_rpc_void },
	{ 0, NULL, NULL, NULL }
};
static const value_string statnotify1_proc_vals[] = {
	{ 0, "NULL" },
	{ STATNOTIFYPROC_MON,   "MON-CALLBACK" },
	{ 0, NULL }
};
/* end of stat-notify version 1 */

static const rpc_prog_vers_info statnotify_vers_info[] = {
	{ 1, statnotify1_proc, &hf_statnotify_procedure_v1 },
};


void
proto_register_statnotify(void)
{
	static hf_register_info hf[] = {
		{ &hf_statnotify_procedure_v1, {
			"V1 Procedure", "statnotify.procedure_v1", FT_UINT32, BASE_DEC,
			VALS(statnotify1_proc_vals), 0, NULL, HFILL }},
		{ &hf_statnotify_name, {
			"Name", "statnotify.name", FT_STRING, BASE_NONE,
			NULL, 0, "Name of client that changed", HFILL }},
		{ &hf_statnotify_state, {
			"State", "statnotify.state", FT_UINT32, BASE_DEC,
			NULL, 0, "New state of client that changed", HFILL }},
		{ &hf_statnotify_priv, {
			"Priv", "statnotify.priv", FT_BYTES, BASE_NONE,
			NULL, 0, "Client supplied opaque data", HFILL }},
	};

	static int *ett[] = {
		&ett_statnotify,
	};

	proto_statnotify = proto_register_protocol("Network Status Monitor CallBack Protocol", "STAT-CB", "statnotify");
	proto_register_field_array(proto_statnotify, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
}

void
proto_reg_handoff_statnotify(void)
{
	/* Register the protocol as RPC */
	rpc_init_prog(proto_statnotify, STATNOTIFY_PROGRAM, ett_statnotify,
	    G_N_ELEMENTS(statnotify_vers_info), statnotify_vers_info);
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
