/* packet-pw-common.c
 * Common functions and objects for PWE3 dissectors.
 * Copyright 2009, Artem Tamazov <artem.tamazov@tellabs.com>
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <epan/packet.h>
#include <wsutil/str_util.h>
#include "packet-pw-common.h"

void proto_register_pw_padding(void);

static const char string_ok[] = "Ok";

const value_string
pwc_vals_cw_l_bit[] = {
	{ 0x0,	string_ok },
	{ 0x1,	"Attachment Circuit Fault" },
	{ 0,	NULL }
};


const value_string
pwc_vals_cw_r_bit[] = {
	{ 0x0,	string_ok },
	{ 0x1,	"Packet Loss State" },
	{ 0,	NULL }
};

const value_string
pwc_vals_cw_frag[] = {
	{ 0x0,	"Unfragmented" },
	{ 0x1,	"First fragment" },
	{ 0x2,	"Last fragment" },
	{ 0x3,	"Intermediate fragment" },
	{ 0,	NULL }
};


void pwc_item_append_cw(proto_item* item, const uint32_t cw, const bool append_text)
{
	if (item != NULL)
	{
		if (append_text)
		{
			proto_item_append_text(item, ", CW");
		}
		proto_item_append_text(item, ": 0x%.8" PRIx32, cw);
	}
	return;
}


void pwc_item_append_text_n_items(proto_item* item, const int n, const char * const item_text)
{
	if (item != NULL)
	{
		if (n >=0)
		{
			proto_item_append_text(item, ", %d %s%s", n, item_text, plurality(n,"","s"));
		}
	}
	return;
}


static int proto_pw_padding;
static int ett_pw_common;
static int hf_padding_len;

static
int dissect_pw_padding(tvbuff_t * tvb, packet_info * pinfo, proto_tree * tree, void* data _U_)
{
	int size;
	proto_item* item;
	proto_tree* tree_p;
	size = tvb_reported_length_remaining(tvb, 0);
	item = proto_tree_add_item(tree, proto_pw_padding, tvb, 0, -1, ENC_NA);
	pwc_item_append_text_n_items(item,size,"byte");
	tree_p = proto_item_add_subtree(item, ett_pw_common);

	call_data_dissector(tvb, pinfo, tree_p);
	item = proto_tree_add_int(tree_p, hf_padding_len, tvb, 0, 0, size);
	proto_item_set_hidden(item); /*allow filtering*/

	return tvb_captured_length(tvb);
}

void proto_register_pw_padding(void)
{
	static hf_register_info hfpadding[] = {
		{&hf_padding_len	,{"Length"			,"pw.padding.len"
					,FT_INT32	,BASE_DEC	,NULL		,0
					,NULL						,HFILL }}
	};
	static int *ett_array[] = {
		&ett_pw_common
	};
	proto_pw_padding = proto_register_protocol("Pseudowire Padding","PW Padding","pw.padding");
	proto_register_field_array(proto_pw_padding, hfpadding, array_length(hfpadding));
	proto_register_subtree_array(ett_array, array_length(ett_array));
	register_dissector("pw_padding", dissect_pw_padding, proto_pw_padding);
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
