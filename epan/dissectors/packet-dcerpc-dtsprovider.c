/* packet-dcerpc-dtsprovider.c
 * Routines for dcerpc Time server dissection
 * Copyright 2002, Jaime Fournier <Jaime.Fournier@hush.com>
 * This information is based off the released idl files from opengroup.
 * ftp://ftp.opengroup.org/pub/dce122/dce/src/time.tar.gz time/service/dtsprovider.idl
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

void proto_register_dtsprovider (void);
void proto_reg_handoff_dtsprovider (void);

static int proto_dtsprovider;
static int hf_dtsprovider_opnum;
/* static int hf_dtsprovider_status; */


static int ett_dtsprovider;


static e_guid_t uuid_dtsprovider = { 0xbfca1238, 0x628a, 0x11c9, { 0xa0, 0x73, 0x08, 0x00, 0x2b, 0x0d, 0xea, 0x7a } };
static uint16_t ver_dtsprovider = 1;


static const dcerpc_sub_dissector dtsprovider_dissectors[] = {
	{ 0, "ContactProvider", NULL, NULL},
	{ 1, "ServerRequestProviderTime", NULL, NULL},
	{ 0, NULL, NULL, NULL }
};

void
proto_register_dtsprovider (void)
{
	static hf_register_info hf[] = {
	  { &hf_dtsprovider_opnum,
	    { "Operation", "dtsprovider.opnum", FT_UINT16, BASE_DEC,
	      NULL, 0x0, NULL, HFILL }},
#if 0
	  { &hf_dtsprovider_status,
	    { "Status", "dtsprovider.status", FT_UINT32, BASE_DEC|BASE_EXT_STRING,
	      &dce_error_vals_ext, 0x0, "Return code, status of executed command", HFILL }}
#endif
	};

	static int *ett[] = {
		&ett_dtsprovider,
	};
	proto_dtsprovider = proto_register_protocol ("DCE Distributed Time Service Provider", "DTSPROVIDER", "dtsprovider");
	proto_register_field_array (proto_dtsprovider, hf, array_length (hf));
	proto_register_subtree_array (ett, array_length (ett));
}

void
proto_reg_handoff_dtsprovider (void)
{
	/* Register the protocol as dcerpc */
	dcerpc_init_uuid (proto_dtsprovider, ett_dtsprovider, &uuid_dtsprovider, ver_dtsprovider, dtsprovider_dissectors, hf_dtsprovider_opnum);
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
