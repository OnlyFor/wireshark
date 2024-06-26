/* msg_dsx_rvd.c
 * WiMax MAC Management DSX-RVD Message decoder
 *
 * Copyright (c) 2007 by Intel Corporation.
 *
 * Author: Lu Pan <lu.pan@intel.com>
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1999 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* Include files */

#include "config.h"

#include <epan/packet.h>
#include "wimax_mac.h"

void proto_register_mac_mgmt_msg_dsx_rvd(void);
void proto_reg_handoff_mac_mgmt_msg_dsx_rvd(void);

static dissector_handle_t dsx_rvd_handle;

static int proto_mac_mgmt_msg_dsx_rvd_decoder;
static int ett_mac_mgmt_msg_dsx_rvd_decoder;

/* fix fields */
static int hf_dsx_rvd_transaction_id;
static int hf_dsx_rvd_confirmation_code;


/* Decode DSX-RVD messages. */
static int dissect_mac_mgmt_msg_dsx_rvd_decoder(tvbuff_t *tvb, packet_info *pinfo _U_, proto_tree *tree, void* data _U_)
{
	unsigned offset = 0;
	proto_item *dsx_rvd_item;
	proto_tree *dsx_rvd_tree;

	{	/* we are being asked for details */
		/* display MAC message type */
		dsx_rvd_item = proto_tree_add_protocol_format(tree, proto_mac_mgmt_msg_dsx_rvd_decoder, tvb, offset, -1, "DSx Received (DSX-RVD)");
		/* add MAC DSx subtree */
		dsx_rvd_tree = proto_item_add_subtree(dsx_rvd_item, ett_mac_mgmt_msg_dsx_rvd_decoder);
		/* display the Transaction ID */
		proto_tree_add_item(dsx_rvd_tree, hf_dsx_rvd_transaction_id, tvb, offset, 2, ENC_BIG_ENDIAN);
		/* move to next field */
		offset += 2;
		/* display the Confirmation Code */
		proto_tree_add_item(dsx_rvd_tree, hf_dsx_rvd_confirmation_code, tvb, offset, 1, ENC_BIG_ENDIAN);
	}
	return tvb_captured_length(tvb);
}

/* Register Wimax Mac Payload Protocol and Dissector */
void proto_register_mac_mgmt_msg_dsx_rvd(void)
{
	/* DSX_RVD display */
	static hf_register_info hf_dsx_rvd[] =
	{
		{
			&hf_dsx_rvd_confirmation_code,
			{ "Confirmation code", "wmx.dsx_rvd.confirmation_code", FT_UINT8, BASE_HEX, NULL, 0x0, NULL, HFILL}
		},
		{
			&hf_dsx_rvd_transaction_id,
			{ "Transaction ID", "wmx.dsx_rvd.transaction_id", FT_UINT16, BASE_HEX, NULL, 0x0, NULL, HFILL}
		}
	};

	/* Setup protocol subtree array */
	static int *ett[] =
		{
			&ett_mac_mgmt_msg_dsx_rvd_decoder,
		};

	proto_mac_mgmt_msg_dsx_rvd_decoder = proto_register_protocol (
		"WiMax DSX-RVD Message", /* name       */
		"WiMax DSX-RVD (dsx_rvd)",   /* short name */
		"wmx.dsx_rvd"                /* abbrev     */
		);

	proto_register_field_array(proto_mac_mgmt_msg_dsx_rvd_decoder, hf_dsx_rvd, array_length(hf_dsx_rvd));
	proto_register_subtree_array(ett, array_length(ett));
	dsx_rvd_handle = register_dissector("mac_mgmt_msg_dsx_rvd_handler", dissect_mac_mgmt_msg_dsx_rvd_decoder, proto_mac_mgmt_msg_dsx_rvd_decoder);
}

void
proto_reg_handoff_mac_mgmt_msg_dsx_rvd(void)
{
	dissector_add_uint("wmx.mgmtmsg", MAC_MGMT_MSG_DSX_RVD, dsx_rvd_handle);
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
