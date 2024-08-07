/* packet-tpkt.h
 *
 * Routines for TPKT dissection
 *
 * Copyright 2000, Philips Electronics N.V.
 * Andreas Sikkema <andreas.sikkema@philips.com>
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "ws_symbol_export.h"

/*
 * Check whether this could be a TPKT-encapsulated PDU.
 * Returns -1 if it's not, and the PDU length from the TPKT header
 * if it is.
 *
 * "min_len" is the minimum length of the PDU; the length field in the
 * TPKT header must be at least "4+min_len" in order for this to be a
 * valid TPKT PDU for the protocol in question.
 */
WS_DLL_PUBLIC int is_tpkt(tvbuff_t *tvb, int min_len);

/*
 * Dissect TPKT-encapsulated data in a TCP stream.
 */
WS_DLL_PUBLIC void dissect_tpkt_encap(tvbuff_t *tvb, packet_info *pinfo,
    proto_tree *tree, bool desegment,
    dissector_handle_t subdissector_handle);

/*
 * Check whether this could be a ASCII TPKT-encapsulated PDU.
 * Returns -1 if it's not, and the PDU length from the TPKT header
 * if it is.
 *
 * "min_len" is the minimum length of the PDU; the length field in the
 * TPKT header must be at least "8+min_len" in order for this to be a
 * valid TPKT PDU for the protocol in question.
 */
extern uint16_t is_asciitpkt(tvbuff_t *tvb);

/*
 * Dissect ASCII TPKT-encapsulated data in a TCP stream.
 */
extern void dissect_asciitpkt(tvbuff_t *tvb, packet_info *pinfo,
    proto_tree *tree, dissector_handle_t subdissector_handle);
