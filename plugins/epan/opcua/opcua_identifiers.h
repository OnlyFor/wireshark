/******************************************************************************
** Copyright (C) 2006-2007 ascolab GmbH. All Rights Reserved.
** Web: http://www.ascolab.com
**
** SPDX-License-Identifier: GPL-2.0-or-later
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** Project: OpcUa Wireshark Plugin
**
** Description: Parser type definitions.
**
** This file was autogenerated on 6/10/2007 2:35:22 AM.
** DON'T MODIFY THIS FILE!
** XXX - well, except that you may have to.  See the README.
**
******************************************************************************/

#include <glib.h>
#include <epan/packet.h>

/* declare service parser function prototype */
typedef void (*fctServiceParser)(proto_tree *tree, tvbuff_t *tvb, packet_info *pinfo, int *pOffset);
/* declare enum parser function prototype */
typedef void (*fctEnumParser)(proto_tree *tree, tvbuff_t *tvb, packet_info *pinfo, int *pOffset);
/* declare type parser function prototype */
typedef void (*fctComplexTypeParser)(proto_tree *tree, tvbuff_t *tvb, packet_info *pinfo, int *pOffset, const char *szFieldName);
/* declare type parser function prototype */
typedef proto_item* (*fctSimpleTypeParser)(proto_tree *tree, tvbuff_t *tvb, packet_info *pinfo, int *pOffset, int hfIndex);

typedef struct _ParserEntry
{
	int iRequestId;
	fctServiceParser pParser;
} ParserEntry;

typedef struct _ExtensionObjectParserEntry
{
	int iRequestId;
	fctComplexTypeParser pParser;
	const char *typeName;
} ExtensionObjectParserEntry;
