/* packet-sebek.c
 * Routines for Sebek - Kernel based data capture - packet dissection
 * Modified to add sebek V3
 * Copyright 2006, Camilo Viecco <cviecco@indiana.edu>
 * Copyright 1999, Nathan Neulinger <nneul@umr.edu>
 *
 * See: http://project.honeynet.org/tools/sebek/ for more details
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <math.h>
#include <epan/packet.h>

/*
  Sebek v2:

        IP address:     32bit unsigned
        MAGIC Val:      32bit unsigned
        Sebek Ver:      16bit unsigned    #value must match 2
        Type            16bit unsigned
        Counter:        32bit unsigned
        Time_sec:       32bit unsigned
        Time_usec:      32bit unsigned
        Proc ID:        32bit unsigned
        User ID:        32bit unsigned
        File Desc:      32bit unsigned
        Command:        12char array
        Length:         Data Length

        Data:           Variable Length data


  Sebek v3 header
        IP address:     32bit unsigned
        MAGIC Val:      32bit unsigned
        Sebek Ver:      16bit unsigned    #value must match 3
        Type            16bit unsigned
        Counter:        32bit unsigned
        Time_sec:       32bit unsigned
        Time_usec:      32bit unsigned
        Parent_pid:     32bit unsigned
        Proc ID:        32bit unsigned
        User ID:        32bit unsigned
        File Desc:      32bit unsigned
        inode:          32bit unsigned
        Command:        12char array
        Length:         Data Length
        Data:           Variable data length

    Sebekv3 has a sock_socket_record subheader for IPV4:
        Dest_ip:         32bit unsigned
        Dest_port:       16bit unsigned
        Src_ip:          32bit unsigned
        src_port:        16bit unsigned
        call:            16bit unsigned
        proto             8bit unsigned

 */

/* By default, but can be completely different */
#define UDP_PORT_SEBEK	1101 /* Not IANA registered */

void proto_register_sebek(void);
void proto_reg_handoff_sebek(void);

static dissector_handle_t sebek_handle;

static int proto_sebek;

static int hf_sebek_magic;
static int hf_sebek_version;
static int hf_sebek_type;
static int hf_sebek_counter;
static int hf_sebek_time;
static int hf_sebek_pid;
static int hf_sebek_uid;
static int hf_sebek_fd;
static int hf_sebek_cmd;
static int hf_sebek_len;
static int hf_sebek_data;
static int hf_sebek_ppid;
static int hf_sebek_inode;
static int hf_sebek_socket_src_ip;
static int hf_sebek_socket_src_port;
static int hf_sebek_socket_dst_ip;
static int hf_sebek_socket_dst_port;
static int hf_sebek_socket_call;
static int hf_sebek_socket_proto;


static int ett_sebek;

/* dissect_sebek - dissects sebek packet data
 * tvb - tvbuff for packet data (IN)
 * pinfo - packet info
 * proto_tree - resolved protocol tree
 */
static int
dissect_sebek(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void *data _U_)
{
	proto_tree *sebek_tree;
	proto_item *ti;
	int         offset     = 0;
	int         sebek_ver  = 0;
	int         sebek_type = 0;
	int         cmd_len    = 0;

	col_set_str(pinfo->cinfo, COL_PROTOCOL, "SEBEK");

	col_set_str(pinfo->cinfo, COL_INFO, "SEBEK - ");

	if (tvb_captured_length(tvb)<6)
		sebek_ver = 0;
	else
		sebek_ver = tvb_get_ntohs(tvb, 4);

	switch (sebek_ver) {
		case 2:	col_append_fstr(pinfo->cinfo, COL_INFO, " pid(%d)", tvb_get_ntohl(tvb, 20));
				col_append_fstr(pinfo->cinfo, COL_INFO, " uid(%d)", tvb_get_ntohl(tvb, 24));
				col_append_fstr(pinfo->cinfo, COL_INFO, " fd(%d)", tvb_get_ntohl(tvb, 28));
				col_append_fstr(pinfo->cinfo, COL_INFO, " cmd: %s", tvb_format_text(pinfo->pool, tvb, 32, 12));
				break;
		case 3:	col_append_fstr(pinfo->cinfo, COL_INFO, " pid(%d)", tvb_get_ntohl(tvb, 24));
				col_append_fstr(pinfo->cinfo, COL_INFO, " uid(%d)", tvb_get_ntohl(tvb, 28));
				col_append_fstr(pinfo->cinfo, COL_INFO, " fd(%d)", tvb_get_ntohl(tvb, 32));
				cmd_len = tvb_strnlen(tvb, 40, 12);
				if (cmd_len<0)
					cmd_len = 0;
				col_append_fstr(pinfo->cinfo, COL_INFO, " cmd: %s", tvb_format_text(pinfo->pool, tvb, 40, cmd_len));
				break;
		default:
			break;
	}

	if (tree) {
		/* Adding Sebek item and subtree */
		ti = proto_tree_add_item(tree, proto_sebek, tvb, 0, -1, ENC_NA);
		sebek_tree = proto_item_add_subtree(ti, ett_sebek);

		/* check for minimum length before deciding where to go*/
		if (tvb_captured_length(tvb)<6)
			sebek_ver = 0;
		else
			sebek_ver = tvb_get_ntohs(tvb, 4);

		switch (sebek_ver) {
			case 2: proto_tree_add_item(sebek_tree, hf_sebek_magic, tvb, offset, 4, ENC_BIG_ENDIAN);
				offset += 4;

				proto_tree_add_item(sebek_tree, hf_sebek_version, tvb, offset, 2, ENC_BIG_ENDIAN);
				offset += 2;

				proto_tree_add_item(sebek_tree, hf_sebek_type, tvb, offset, 2, ENC_BIG_ENDIAN);
				offset += 2;

				proto_tree_add_item(sebek_tree, hf_sebek_counter, tvb, offset, 4, ENC_BIG_ENDIAN);
				offset += 4;

				proto_tree_add_item(sebek_tree, hf_sebek_time, tvb, offset, 8, ENC_TIME_SECS_NSECS|ENC_BIG_ENDIAN);
				offset += 8;

				proto_tree_add_item(sebek_tree, hf_sebek_pid, tvb, offset, 4, ENC_BIG_ENDIAN);
				offset += 4;

				proto_tree_add_item(sebek_tree, hf_sebek_uid, tvb, offset, 4, ENC_BIG_ENDIAN);
				offset += 4;

				proto_tree_add_item(sebek_tree, hf_sebek_fd, tvb, offset, 4, ENC_BIG_ENDIAN);
				offset += 4;

				proto_tree_add_item(sebek_tree, hf_sebek_cmd, tvb, offset, 12, ENC_ASCII);
				offset += 12;

				proto_tree_add_item(sebek_tree, hf_sebek_len, tvb, offset, 4, ENC_BIG_ENDIAN);
				offset += 4;

				proto_tree_add_item(sebek_tree, hf_sebek_data, tvb, offset, -1, ENC_ASCII);

				break;

			case 3:	proto_tree_add_item(sebek_tree, hf_sebek_magic, tvb, offset, 4, ENC_BIG_ENDIAN);
				offset += 4;

				proto_tree_add_item(sebek_tree, hf_sebek_version, tvb, offset, 2, ENC_BIG_ENDIAN);
				offset += 2;

				sebek_type=tvb_get_ntohs(tvb, offset);
				proto_tree_add_item(sebek_tree, hf_sebek_type, tvb, offset, 2, ENC_BIG_ENDIAN);
				offset += 2;

				proto_tree_add_item(sebek_tree, hf_sebek_counter, tvb, offset, 4, ENC_BIG_ENDIAN);
				offset += 4;

				proto_tree_add_item(sebek_tree, hf_sebek_time, tvb, offset, 8, ENC_TIME_SECS_NSECS|ENC_BIG_ENDIAN);
				offset += 8;

				proto_tree_add_item(sebek_tree, hf_sebek_ppid, tvb, offset, 4, ENC_BIG_ENDIAN);
				offset += 4;

				proto_tree_add_item(sebek_tree, hf_sebek_pid, tvb, offset, 4, ENC_BIG_ENDIAN);
				offset += 4;

				proto_tree_add_item(sebek_tree, hf_sebek_uid, tvb, offset, 4, ENC_BIG_ENDIAN);
				offset += 4;

				proto_tree_add_item(sebek_tree, hf_sebek_fd, tvb, offset, 4, ENC_BIG_ENDIAN);
				offset += 4;

				proto_tree_add_item(sebek_tree, hf_sebek_inode, tvb, offset, 4, ENC_BIG_ENDIAN);
				offset += 4;

				proto_tree_add_item(sebek_tree, hf_sebek_cmd, tvb, offset, 12, ENC_ASCII);
				offset += 12;

				proto_tree_add_item(sebek_tree, hf_sebek_len, tvb, offset, 4, ENC_BIG_ENDIAN);
				offset += 4;

				if (sebek_type == 2) {
					/*data is socket data, process accordingly*/
					proto_tree_add_item(sebek_tree, hf_sebek_socket_dst_ip, tvb, offset, 4, ENC_BIG_ENDIAN);
					offset += 4;
					proto_tree_add_item(sebek_tree, hf_sebek_socket_dst_port, tvb, offset, 2, ENC_BIG_ENDIAN);
					offset += 2;
					proto_tree_add_item(sebek_tree, hf_sebek_socket_src_ip, tvb, offset, 4, ENC_BIG_ENDIAN);
					offset += 4;
					proto_tree_add_item(sebek_tree, hf_sebek_socket_src_port, tvb, offset, 2, ENC_BIG_ENDIAN);
					offset += 2;
					proto_tree_add_item(sebek_tree, hf_sebek_socket_call, tvb, offset, 2, ENC_BIG_ENDIAN);
					offset += 2;
					proto_tree_add_item(sebek_tree, hf_sebek_socket_proto, tvb, offset, 1, ENC_BIG_ENDIAN);
					offset += 1;
				} else {
					proto_tree_add_item(sebek_tree, hf_sebek_data, tvb, offset, -1, ENC_ASCII);
				}

				break;

			default:
				break;

		}
	}
	return offset;
}

void
proto_register_sebek(void)
{
	static hf_register_info hf[] = {
		{ &hf_sebek_magic, {
			"Magic", "sebek.magic", FT_UINT32, BASE_HEX,
			NULL, 0, "Magic Number", HFILL }},
		{ &hf_sebek_version, {
			"Version", "sebek.version", FT_UINT16, BASE_DEC,
			NULL, 0, "Version Number", HFILL }},
		{ &hf_sebek_type, {
			"Type", "sebek.type", FT_UINT16, BASE_DEC,
			NULL, 0, NULL, HFILL }},
		{ &hf_sebek_counter, {
			"Counter", "sebek.counter", FT_UINT32, BASE_DEC,
			NULL, 0, NULL, HFILL }},
		{ &hf_sebek_time, {
			"Time", "sebek.time.sec", FT_ABSOLUTE_TIME, ABSOLUTE_TIME_LOCAL,
			NULL, 0, NULL, HFILL }},
		{ &hf_sebek_pid, {
			"Process ID", "sebek.pid", FT_UINT32, BASE_DEC,
			NULL, 0, NULL, HFILL }},
		{ &hf_sebek_uid, {
			"User ID", "sebek.uid", FT_UINT32, BASE_DEC,
			NULL, 0, NULL, HFILL }},
		{ &hf_sebek_fd, {
			"File Descriptor", "sebek.fd", FT_UINT32, BASE_DEC,
			NULL, 0, "File Descriptor Number", HFILL }},
		{ &hf_sebek_cmd, {
			"Command Name", "sebek.cmd", FT_STRING, BASE_NONE,
			NULL, 0, NULL, HFILL }},
		{ &hf_sebek_len, {
			"Data Length", "sebek.len", FT_UINT32, BASE_DEC,
			NULL, 0, NULL, HFILL }},
		{ &hf_sebek_ppid, {
			"Parent Process ID", "sebek.ppid", FT_UINT32, BASE_DEC,
			NULL, 0, NULL, HFILL }},
		{ &hf_sebek_inode, {
			"Inode ID", "sebek.inode", FT_UINT32, BASE_DEC,
			NULL, 0, NULL, HFILL }},
		{ &hf_sebek_data, {
			"Data", "sebek.data", FT_STRING, BASE_NONE,
			NULL, 0, NULL, HFILL }},
		{ &hf_sebek_socket_src_ip, {
			"Socket.local_ip", "sebek.socket.src_ip", FT_IPv4, BASE_NONE,
			NULL, 0, "Socket.src_ip", HFILL }},
		{ &hf_sebek_socket_src_port, {
			"Socket.local_port", "sebek.socket.src_port", FT_UINT16, BASE_DEC,
			NULL, 0, "Socket.src_port", HFILL }},
		{ &hf_sebek_socket_dst_ip, {
			"Socket.remote_ip", "sebek.socket.dst_ip", FT_IPv4, BASE_NONE,
			NULL, 0, "Socket.dst_ip", HFILL }},
		{ &hf_sebek_socket_dst_port, {
			"Socket.remote_port", "sebek.socket.dst_port", FT_UINT16, BASE_DEC,
			NULL, 0, "Socket.dst_port", HFILL }},
		{ &hf_sebek_socket_call, {
			"Socket.Call_id", "sebek.socket.call", FT_UINT16, BASE_DEC,
			NULL, 0, "Socket.call", HFILL }},
		{ &hf_sebek_socket_proto, {
			"Socket.ip_proto", "sebek.socket.ip_proto", FT_UINT8, BASE_DEC,
			NULL, 0, NULL, HFILL }}
	};
	static int *ett[] = {
		&ett_sebek
	};

	proto_sebek = proto_register_protocol("SEBEK - Kernel Data Capture", "SEBEK", "sebek");
	proto_register_field_array(proto_sebek, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));

	sebek_handle = register_dissector("sebek", dissect_sebek, proto_sebek);
}

void
proto_reg_handoff_sebek(void)
{
	dissector_add_uint_with_preference("udp.port", UDP_PORT_SEBEK, sebek_handle);
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
