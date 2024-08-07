/* packet-rx.h
 * Definitions for packet disassembly structures and routines
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef PACKET_RX_H
#define PACKET_RX_H

/*
 * Private data passed from the RX dissector to the AFS dissector.
 */
struct rxinfo {
	uint8_t	type;
	uint8_t	flags;
	uint16_t	serviceid;
	uint32_t	epoch;
	uint32_t	cid;
	uint32_t	callnumber;
	uint32_t	seq;
};

/*
 * RX protocol definitions.
 */

/*
 * Packet types.
 */
#define RX_PACKET_TYPE_DATA		1
#define RX_PACKET_TYPE_ACK		2
#define RX_PACKET_TYPE_BUSY		3
#define RX_PACKET_TYPE_ABORT		4
#define RX_PACKET_TYPE_ACKALL		5
#define RX_PACKET_TYPE_CHALLENGE	6
#define RX_PACKET_TYPE_RESPONSE		7
#define RX_PACKET_TYPE_DEBUG		8
#define RX_PACKET_TYPE_PARAMS		9
#define RX_PACKET_TYPE_VERSION		13

/*
 * Flag bits in the RX header.
 */
#define RX_CLIENT_INITIATED 1
#define RX_REQUEST_ACK 2
#define RX_LAST_PACKET 4
#define RX_MORE_PACKETS 8
#define RX_FREE_PACKET 16
#define RX_SLOW_START_OR_JUMBO 32

#define RX_ACK_TYPE_NACK 0
#define RX_ACK_TYPE_ACK 1

/* ACK reasons */
#define RX_ACK_REQUESTED 1
#define RX_ACK_DUPLICATE 2
#define RX_ACK_OUT_OF_SEQUENCE 3
#define RX_ACK_EXEEDS_WINDOW 4
#define RX_ACK_NOSPACE 5
#define RX_ACK_PING 6
#define RX_ACK_PING_RESPONSE 7
#define RX_ACK_DELAY 8
#define RX_ACK_IDLE 9

#define RX_MAXCALLS	4

#endif
