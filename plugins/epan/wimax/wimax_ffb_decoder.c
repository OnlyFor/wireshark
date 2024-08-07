/* wimax_ffb_decoder.c
 * WiMax Fast Feedback packet decoder
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
#include "wimax-int.h"

extern int proto_wimax;

static int proto_wimax_ffb_decoder;
static int ett_wimax_ffb_decoder;

/* static int hf_ffb_burst; */
static int hf_ffb_num_of_ffbs;
static int hf_ffb_type;
static int hf_ffb_subchannel;
static int hf_ffb_symboloffset;
static int hf_ffb_value;


static int dissect_wimax_ffb_decoder(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
	int offset = 0;
	unsigned length, num_of_ffbs, i;
	proto_item *ffb_item = NULL;
	proto_tree *ffb_tree = NULL;

	/* update the info column */
	col_append_sep_str(pinfo->cinfo, COL_INFO, NULL, "Fast Feedback Burst:");
	if (tree)
	{	/* we are being asked for details */
		/* get the tvb reported length */
		length = tvb_reported_length(tvb);
		/* display Fast Feedback Burst dissector info */
		ffb_item = proto_tree_add_protocol_format(tree, proto_wimax_ffb_decoder, tvb, offset, length, "Fast Feedback Burst (%u bytes)", length);
		/* add Fast Feedback Burst subtree */
		ffb_tree = proto_item_add_subtree(ffb_item, ett_wimax_ffb_decoder);
		/* get the number of FFBs */
		num_of_ffbs =  tvb_get_uint8(tvb, offset);
		/* display the number of FFBs */
		proto_tree_add_item(ffb_tree, hf_ffb_num_of_ffbs, tvb, offset++, 1, ENC_BIG_ENDIAN);
		/* display the FFB type */
		proto_tree_add_item(ffb_tree, hf_ffb_type, tvb, offset++, 1, ENC_BIG_ENDIAN);
		/* display the FFBs */
		for(i = 0; i < num_of_ffbs; i++)
		{
			proto_tree_add_item(ffb_tree, hf_ffb_subchannel, tvb, offset++, 1, ENC_BIG_ENDIAN);
			proto_tree_add_item(ffb_tree, hf_ffb_symboloffset, tvb, offset++, 1, ENC_BIG_ENDIAN);
			proto_tree_add_item(ffb_tree, hf_ffb_value, tvb, offset++, 1, ENC_BIG_ENDIAN);
		}
	}
	return tvb_captured_length(tvb);
}

/* Register Wimax FFB Protocol */
void wimax_proto_register_wimax_ffb(void)
{
	/* FFB display */
	static hf_register_info hf[] =
	{
#if 0
		{
			&hf_ffb_burst,
			{"Fast Feedback Burst", "wmx.ffb.burst", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL}
		},
#endif
		{
			&hf_ffb_num_of_ffbs,
			{"Number Of Fast Feedback", "wmx.ffb.num_of_ffbs", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL}
		},
		{
			&hf_ffb_type,
			{"Fast Feedback Type", "wmx.ffb.ffb_type", FT_UINT8, BASE_HEX, NULL, 0x0, NULL, HFILL}
		},
		{
			&hf_ffb_subchannel,
			{"Physical Subchannel", "wmx.ffb.subchannel", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL}
		},
		{
			&hf_ffb_symboloffset,
			{"Symbol Offset", "wmx.ffb.symbol_offset", FT_UINT8, BASE_DEC, NULL, 0x0, NULL, HFILL}
		},
		{
			&hf_ffb_value,
			{"Fast Feedback Value", "wmx.ffb.ffb_value", FT_UINT8, BASE_HEX, NULL, 0x0, NULL, HFILL}
		}
	};

	/* Setup protocol subtree array */
	static int *ett[] =
		{
			&ett_wimax_ffb_decoder,
		};

	proto_wimax_ffb_decoder = proto_wimax;

	/* register the field display messages */
	proto_register_field_array(proto_wimax_ffb_decoder, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));

	register_dissector("wimax_ffb_burst_handler", dissect_wimax_ffb_decoder, proto_wimax_ffb_decoder);
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
