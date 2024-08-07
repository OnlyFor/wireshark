/* packet-dtp.c
 * Routines for the disassembly for Cisco Dynamic Trunk Protocol
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * DTP support added by Charlie Lenahan <clenahan@fortresstech.com>
 *
 * Additional information comes from Yersinia (http://www.yersinia.net/)
 * by Alfredo Andres and David Barroso
 *
 * Improved dissection for Trunk Status and Trunk Type TLVs
 * based on DTP description in U.S. Patent #6,445,715 and
 * consultations with Aninda Chatterjee @ Cisco
 * by Peter Paluch <Peter.Paluch@fri.uniza.sk>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#include "config.h"

#include <epan/packet.h>
#include <epan/to_str.h>
#include <epan/expert.h>
#include <epan/cisco_pid.h>

/*
 * It's incomplete, and it appears to be inaccurate in a number of places,
 * but it's all I could find....
 */
void proto_register_dtp(void);
void proto_reg_handoff_dtp(void);

static dissector_handle_t dtp_handle;

static int proto_dtp;
static int hf_dtp_version;
static int hf_dtp_domain;
static int hf_dtp_tlvtype;
static int hf_dtp_tlvlength;
static int hf_dtp_senderid;
static int hf_dtp_tot;
static int hf_dtp_tat;
static int hf_dtp_tos;
static int hf_dtp_tas;
static int hf_dtp_data;

static int ett_dtp;
static int ett_dtp_tlv;
static int ett_dtp_status;
static int ett_dtp_type;

static expert_field ei_dtp_tlv_length_too_short;
static expert_field ei_dtp_tlv_length_invalid;
static expert_field ei_dtp_truncated;

static void
dissect_dtp_tlv(packet_info *pinfo, tvbuff_t *tvb, int offset, int length,
		proto_tree *tree, proto_item *ti, proto_item *tlv_length_item, uint8_t type);


#define	DTP_TLV_DOMAIN		0x01 /* VTP Domain Name */
#define	DTP_TLV_TRSTATUS	0x02 /* Trunk Status */
#define	DTP_TLV_TRTYPE		0x03 /* Trunk Type */
#define	DTP_TLV_SENDERID	0x04 /* Sender ID (MAC) */

#define	DTP_TOS_SHIFT		7
#define	DTP_TOT_SHIFT		5

#define	DTP_TOS_MASK		0x80
#define	DTP_TAS_MASK		0x07

#define	DTP_TOT_MASK		0xE0
#define	DTP_TAT_MASK		0x07

#define	DTP_TOSVALUE(status)	(((status) & DTP_TOS_MASK) >> DTP_TOS_SHIFT)
#define	DTP_TASVALUE(status)	((status) & DTP_TAS_MASK)

#define	DTP_TOTVALUE(type)	(((type) & DTP_TOT_MASK) >> DTP_TOT_SHIFT)
#define	DTP_TATVALUE(type)	((type) & DTP_TAT_MASK)

/* Trunk Operating Status */
#define	DTP_TOS_ACCESS		0x0
#define	DTP_TOS_TRUNK		0x1

/* Trunk Administrative Status */
#define	DTP_TAS_ON		0x1
#define	DTP_TAS_OFF		0x2
#define	DTP_TAS_DESIRABLE	0x3
#define	DTP_TAS_AUTO		0x4

/* Trunk Operating Type */
#define	DTP_TOT_NATIVE		0x1
#define	DTP_TOT_ISL		0x2
#define	DTP_TOT_DOT1Q		0x5

/* Trunk Administrative Type */
#define	DTP_TAT_NEGOTIATED	0x0
#define	DTP_TAT_NATIVE		0x1
#define	DTP_TAT_ISL		0x2
#define	DTP_TAT_DOT1Q		0x5


static const value_string dtp_tlv_type_vals[] = {
	{ DTP_TLV_DOMAIN,	"Domain" },
	{ DTP_TLV_TRSTATUS,	"Trunk Status" },
	{ DTP_TLV_TRTYPE,	"Trunk Type" },
	{ DTP_TLV_SENDERID, 	"Sender ID" },

	{ 0,			NULL }
};

static const value_string dtp_tos_vals[] = {
	{ DTP_TOS_ACCESS,	"Access" },
	{ DTP_TOS_TRUNK,	"Trunk"},

	{ 0,			NULL }
};

static const value_string dtp_tas_vals[] = {
	{ DTP_TAS_ON,		"On" },
	{ DTP_TAS_OFF,		"Off" },
	{ DTP_TAS_DESIRABLE,	"Desirable" },
	{ DTP_TAS_AUTO,		"Auto" },

	{ 0,			NULL }
};

static const value_string dtp_tot_vals[] = {
	{ DTP_TOT_NATIVE,	"Native" },
	{ DTP_TOT_ISL,		"ISL" },
	{ DTP_TOT_DOT1Q,	"802.1Q" },

	{ 0,			NULL }
};

static const value_string dtp_tat_vals[] = {
	{ DTP_TAT_NEGOTIATED,	"Negotiated" },
	{ DTP_TAT_NATIVE,	"Native" },
	{ DTP_TAT_ISL,		"ISL" },
	{ DTP_TAT_DOT1Q,	"802.1Q" },

	{ 0,			NULL }
};

static int
dissect_dtp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
	proto_item *ti;
	proto_tree *dtp_tree;
	proto_tree *tlv_tree;
	int offset = 0;

	col_set_str(pinfo->cinfo, COL_PROTOCOL, "DTP");
	col_set_str(pinfo->cinfo, COL_INFO, "Dynamic Trunk Protocol");

	ti = proto_tree_add_item(tree, proto_dtp, tvb, offset, -1, ENC_NA);
	dtp_tree = proto_item_add_subtree(ti, ett_dtp);

	/* We assume version */
	proto_tree_add_item(dtp_tree, hf_dtp_version, tvb, offset, 1, ENC_BIG_ENDIAN);
	offset += 1;

	while (tvb_reported_length_remaining(tvb, offset) > 0) {
		int type, length, valuelength;
		proto_item * tlv_length_item;

		/* XXX - why not just let tvbuff exceptions handle this? */
		if (tvb_reported_length_remaining(tvb, offset) < 4) {
			expert_add_info(pinfo, dtp_tree, &ei_dtp_truncated);
			break;
		}

		type = tvb_get_ntohs(tvb, offset);
		length = tvb_get_ntohs(tvb, offset + 2);

		tlv_tree = proto_tree_add_subtree(dtp_tree, tvb, offset, length, ett_dtp_tlv, NULL,
					 val_to_str(type, dtp_tlv_type_vals, "Unknown TLV type: 0x%02x"));

		proto_tree_add_uint(tlv_tree, hf_dtp_tlvtype, tvb, offset, 2, type);
		offset+=2;

		tlv_length_item = proto_tree_add_uint(tlv_tree, hf_dtp_tlvlength, tvb, offset, 2, length);
		offset+=2;

		if (length <= 4) {
			/* Length includes type and length fields, so it
			   must be >= 4, and no known TLVs have a value
			   length of 0, so it must be > 4. */
			expert_add_info(pinfo, tlv_length_item, &ei_dtp_tlv_length_too_short);
			break;
		}
		valuelength = (length-4);
		dissect_dtp_tlv(pinfo, tvb, offset, valuelength, tlv_tree, ti, tlv_length_item, (uint8_t) type);
		offset += valuelength;
	}
	return tvb_captured_length(tvb);
}

static void
dissect_dtp_tlv(packet_info *pinfo, tvbuff_t *tvb, int offset, int length,
		proto_tree *tree, proto_item *ti, proto_item *tlv_length_item, uint8_t type)
{
	switch (type) {

	case DTP_TLV_DOMAIN:
		if (length <= 33) { /* VTP domain name is at most 32 bytes long and is null-terminated */
			proto_item_append_text(ti, ": %s", tvb_format_text(pinfo->pool, tvb, offset, length - 1));
			proto_tree_add_item(tree, hf_dtp_domain, tvb, offset, length, ENC_ASCII);
		}
		else
			expert_add_info(pinfo, tlv_length_item, &ei_dtp_tlv_length_invalid);

		break;

	case DTP_TLV_TRSTATUS:
		if (length == 1) { /* Value field length must be 1 byte */
			proto_tree * field_tree = NULL;
			uint8_t trunk_status = tvb_get_uint8(tvb, offset);

			proto_item_append_text(ti,
				" (Operating/Administrative): %s/%s (0x%02x)",
				val_to_str_const(DTP_TOSVALUE(trunk_status), dtp_tos_vals, "Unknown operating status"),
				val_to_str_const(DTP_TASVALUE(trunk_status), dtp_tas_vals, "Unknown administrative status"),
				trunk_status);
			field_tree = proto_tree_add_subtree_format(tree, tvb, offset, length, ett_dtp_status, NULL, "Value: %s/%s (0x%02x)",
				val_to_str_const(DTP_TOSVALUE(trunk_status), dtp_tos_vals, "Unknown operating status"),
				val_to_str_const(DTP_TASVALUE(trunk_status), dtp_tas_vals, "Unknown administrative status"),
				trunk_status);
			proto_tree_add_item(field_tree, hf_dtp_tos, tvb, offset, length, ENC_BIG_ENDIAN);
			proto_tree_add_item(field_tree, hf_dtp_tas, tvb, offset, length, ENC_BIG_ENDIAN);
			}
			else
				expert_add_info(pinfo, tlv_length_item, &ei_dtp_tlv_length_invalid);

		break;

	case DTP_TLV_TRTYPE:
		if (length == 1) { /* Value field length must be 1 byte */
			proto_tree * field_tree;
			uint8_t trunk_type = tvb_get_uint8(tvb, offset);
			proto_item_append_text(ti,
				" (Operating/Administrative): %s/%s (0x%02x)",
				val_to_str_const(DTP_TOTVALUE(trunk_type), dtp_tot_vals, "Unknown operating type"),
				val_to_str_const(DTP_TATVALUE(trunk_type), dtp_tat_vals, "Unknown administrative type"),
				trunk_type);
			field_tree = proto_tree_add_subtree_format(tree, tvb, offset, length, ett_dtp_type, NULL, "Value: %s/%s (0x%02x)",
				val_to_str_const(DTP_TOTVALUE(trunk_type), dtp_tot_vals, "Unknown operating type"),
				val_to_str_const(DTP_TATVALUE(trunk_type), dtp_tat_vals, "Unknown administrative type"),
				trunk_type);
			proto_tree_add_item(field_tree, hf_dtp_tot, tvb, offset, length, ENC_BIG_ENDIAN);
			proto_tree_add_item(field_tree, hf_dtp_tat, tvb, offset, length, ENC_BIG_ENDIAN);
			}
			else
				expert_add_info(pinfo, tlv_length_item, &ei_dtp_tlv_length_invalid);

		break;

	case DTP_TLV_SENDERID:
		if (length == 6) { /* Value length must be 6 bytes for a MAC address */
			proto_item_append_text(ti, ": %s",
				tvb_ether_to_str(pinfo->pool, tvb, offset));	/* XXX - resolve? */
			proto_tree_add_item(tree, hf_dtp_senderid, tvb, offset, length, ENC_NA);
		}
		else
			expert_add_info(pinfo, tlv_length_item, &ei_dtp_tlv_length_invalid);

		break;

	default:
		proto_tree_add_item(tree, hf_dtp_data, tvb, offset, length, ENC_NA);
		break;
	}
}

void
proto_register_dtp(void)
{
	static hf_register_info hf[] = {
	{ &hf_dtp_version,
		{ "Version",	"dtp.version", FT_UINT8, BASE_DEC,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_dtp_domain,
		{ "Domain",	"dtp.domain", FT_STRING, BASE_NONE,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_dtp_tlvtype,
		{ "Type",	"dtp.tlv_type", FT_UINT16, BASE_HEX,
		VALS(dtp_tlv_type_vals), 0x0, NULL, HFILL }},

	{ &hf_dtp_tlvlength,
		{ "Length",	"dtp.tlv_len", FT_UINT16, BASE_DEC,
		NULL, 0x0, NULL, HFILL }},

	{ &hf_dtp_tos,
		{ "Trunk Operating Status", "dtp.tos", FT_UINT8, BASE_HEX,
		VALS(dtp_tos_vals), DTP_TOS_MASK, NULL, HFILL }},

	{ &hf_dtp_tas,
		{ "Trunk Administrative Status", "dtp.tas", FT_UINT8, BASE_HEX,
		VALS(dtp_tas_vals), DTP_TAS_MASK, NULL, HFILL }},

	{ &hf_dtp_tot,
		{ "Trunk Operating Type", "dtp.tot", FT_UINT8, BASE_HEX,
		VALS(dtp_tot_vals), DTP_TOT_MASK, NULL, HFILL }},

	{ &hf_dtp_tat,
		{ "Trunk Administrative Type", "dtp.tat", FT_UINT8, BASE_HEX,
		VALS(dtp_tat_vals), DTP_TAT_MASK, NULL, HFILL }},

	{ &hf_dtp_senderid,
		{ "Sender ID", "dtp.senderid", FT_ETHER, BASE_NONE,
		NULL, 0x0, "MAC Address of neighbor", HFILL }},

	{ &hf_dtp_data,
		{ "Data", "dtp.data", FT_ETHER, BASE_NONE,
		NULL, 0x0, NULL, HFILL }},
	};

	static int *ett[] = {
		&ett_dtp,
		&ett_dtp_tlv,
		&ett_dtp_status,
		&ett_dtp_type,
	};

	static ei_register_info ei[] = {
	{ &ei_dtp_tlv_length_too_short,
		{ "dtp.tlv_len.too_short", PI_MALFORMED, PI_ERROR,
		  "Indicated length is less than the minimum length", EXPFILL }},

	{ &ei_dtp_tlv_length_invalid,
		{ "dtp.tlv_len.invalid", PI_MALFORMED, PI_ERROR,
		  "Indicated length does not correspond to this record type", EXPFILL }},

	{ &ei_dtp_truncated,
		{ "dtp.truncated", PI_MALFORMED, PI_ERROR,
		  "DTP message is truncated prematurely", EXPFILL }}

	};

	expert_module_t *expert_dtp;

	proto_dtp = proto_register_protocol("Dynamic Trunk Protocol", "DTP", "dtp");
	proto_register_field_array(proto_dtp, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));

	expert_dtp = expert_register_protocol(proto_dtp);
	expert_register_field_array(expert_dtp, ei, array_length(ei));

	dtp_handle = register_dissector("dtp", dissect_dtp, proto_dtp);
}

void
proto_reg_handoff_dtp(void)
{
	dissector_add_uint("llc.cisco_pid", CISCO_PID_DTP, dtp_handle);
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
