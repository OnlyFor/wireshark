/* packet-pmproxy.c
 *
 * Routines for dissecting Performance Co-Pilot Proxy
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "config.h"

#include <epan/packet.h>
#include <epan/conversation.h>
#include <epan/strutil.h>

#define PMPROXY_PORT 44322
/* length of "pmproxy-server 1\n" and also "pmproxy-client 1\n" */
#define PMPROXY_CLIENT_SERVER_VERSION_LENGTH 17
#define PMPROXY_START_OF_PACKET 0
#define PMPROXY_HOST_AND_PORT_DELIMETER_LENGTH 1

static dissector_handle_t pmproxy_handle;
static dissector_handle_t pcp_handle;

static int proto_pmproxy;

static int hf_pmproxy_host;
static int hf_pmproxy_port;
static int hf_pmproxy_client_version;
static int hf_pmproxy_server_version;

static int ett_pmproxy;

typedef struct pmproxy_conversation_info_t {
    uint32_t last_proxy_frame;
} pmproxy_conversation_info_t;

void proto_register_pmproxy(void);
void proto_reg_handoff_pmproxy(void);

static int is_ascii(const unsigned char *data, int size)
{
    const unsigned char *str = data;
    const unsigned char *end = str + size;
    for (; str != end; str++) {
        if (*str & 0x80)
            return false;
    }
    return true;
}

static int looks_like_proxy_exchange(tvbuff_t *tvb) {
    int packet_length;
    const unsigned char *packet_data;

    packet_length = tvb_ensure_captured_length_remaining(tvb, PMPROXY_START_OF_PACKET);
    packet_data = tvb_get_ptr(tvb, PMPROXY_START_OF_PACKET, packet_length);

    /* A proxy exchange packet only contains ascii characters (eg "localhost 44321") and terminated with \n */
    return is_ascii(packet_data, packet_length) && packet_data[packet_length-1] == '\n';
}

static int dissect_proxy_to_host(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree) {
    int offset = 0;
    int next_offset;
    int proxy_to_length;
    char *pmproxy_host_and_port_string;
    char **host_and_port;
    char *host;
    char *port;

    col_set_str(pinfo->cinfo, COL_INFO, "Proxy");

    proxy_to_length = tvb_find_line_end(tvb, offset, tvb_ensure_captured_length_remaining(tvb, offset), &next_offset, false);
    if(proxy_to_length != -1) {
        pmproxy_host_and_port_string = (char *) tvb_get_string_enc(pinfo->pool, tvb, offset, proxy_to_length, ENC_ASCII);
        host_and_port = wmem_strsplit(pinfo->pool, pmproxy_host_and_port_string, " ", -1);
        if(host_and_port != NULL) {
            host = host_and_port[0];
            if (host) {
                proto_tree_add_string(tree, hf_pmproxy_host, tvb, offset, (uint32_t)strlen(host), host);
                offset += (int)strlen(host) + PMPROXY_HOST_AND_PORT_DELIMETER_LENGTH;
                port = host_and_port[1];
                if (port) {
                    proto_tree_add_string(tree, hf_pmproxy_port, tvb, offset, (uint32_t)strlen(port), port);
                }
            } else {
                port = NULL;
            }
            col_append_fstr(pinfo->cinfo, COL_INFO, " Host=%s, Port=%s", host ? host : "", port ? port : "");
        }
    }
    return proxy_to_length;

}

static int is_banner_exchange_for(const char *type, tvbuff_t *tvb, packet_info *pinfo) {
    char *pmproxy_exchange_string;

    /* Exchange packets have to be this length */
    if(tvb_ensure_captured_length_remaining(tvb, PMPROXY_START_OF_PACKET) != PMPROXY_CLIENT_SERVER_VERSION_LENGTH) {
        return 0;
    }

    pmproxy_exchange_string = (char *) tvb_get_string_enc(pinfo->pool, tvb, PMPROXY_START_OF_PACKET,
                                                           PMPROXY_CLIENT_SERVER_VERSION_LENGTH, ENC_ASCII);
    return g_strcmp0(pmproxy_exchange_string, wmem_strdup_printf(pinfo->pool, "pmproxy-%s 1\n", type)) == 0;
}

static int is_server_exchange(tvbuff_t *tvb, packet_info *pinfo) {
    return is_banner_exchange_for("server", tvb, pinfo);
}

static int is_client_exchange(tvbuff_t *tvb, packet_info *pinfo) {
    return is_banner_exchange_for("client", tvb, pinfo);
}

static int dissect_server_exchange(tvbuff_t *tvb, packet_info *pinfo, proto_tree *pmproxy_tree) {
    int offset = PMPROXY_START_OF_PACKET;
    col_set_str(pinfo->cinfo, COL_INFO, "Server exchange");
    proto_tree_add_item(pmproxy_tree, hf_pmproxy_server_version, tvb, offset, PMPROXY_CLIENT_SERVER_VERSION_LENGTH, ENC_ASCII);
    offset += PMPROXY_CLIENT_SERVER_VERSION_LENGTH;
    return offset;
}

static int dissect_client_exchange(tvbuff_t *tvb, packet_info *pinfo, proto_tree *pmproxy_tree) {
    int offset = PMPROXY_START_OF_PACKET;
    col_set_str(pinfo->cinfo, COL_INFO, "Client exchange");
    proto_tree_add_item(pmproxy_tree, hf_pmproxy_client_version, tvb, offset, PMPROXY_CLIENT_SERVER_VERSION_LENGTH, ENC_ASCII);
    offset += PMPROXY_CLIENT_SERVER_VERSION_LENGTH;
    return offset;
}

static void mark_pmproxy_exchange_complete(packet_info *pinfo) {
    conversation_t *conversation;
    pmproxy_conversation_info_t *pmproxy_conversation;

    conversation = find_or_create_conversation(pinfo);
    pmproxy_conversation = (pmproxy_conversation_info_t *)conversation_get_proto_data(conversation, proto_pmproxy);

    if(pmproxy_conversation == NULL) {
        pmproxy_conversation = wmem_new(wmem_file_scope(), pmproxy_conversation_info_t);
    }
    pmproxy_conversation->last_proxy_frame = pinfo->num;
    conversation_add_proto_data(conversation, proto_pmproxy, pmproxy_conversation);
}

static int is_pmproxy_exchange_complete(packet_info *pinfo) {
    conversation_t *conversation;
    pmproxy_conversation_info_t *pmproxy_conversation;

    conversation = find_or_create_conversation(pinfo);
    pmproxy_conversation = (pmproxy_conversation_info_t *)conversation_get_proto_data(conversation, proto_pmproxy);

    if(pmproxy_conversation == NULL) {
        return false;
    }

    return pinfo->num >= pmproxy_conversation->last_proxy_frame;
}

static int dissect_through_pcp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree) {
    pinfo->can_desegment = pinfo->saved_can_desegment;
    return call_dissector(pcp_handle, tvb, pinfo, tree);
}

static int dissect_pmproxy_exchange(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree) {
    proto_item *root_pmproxy_item;
    proto_tree *pmproxy_tree;

    root_pmproxy_item = proto_tree_add_item(tree, proto_pmproxy, tvb, 0, -1, ENC_NA);
    pmproxy_tree = proto_item_add_subtree(root_pmproxy_item, ett_pmproxy);

    if (is_server_exchange(tvb, pinfo)) {
        return dissect_server_exchange(tvb, pinfo, pmproxy_tree);
    } else if (is_client_exchange(tvb, pinfo)) {
        return dissect_client_exchange(tvb, pinfo, pmproxy_tree);
    } else if (looks_like_proxy_exchange(tvb)) {
        return dissect_proxy_to_host(tvb, pinfo, pmproxy_tree);
    } else {
        /* If its none of these, it is going to be vanilla PCP traffic */
        mark_pmproxy_exchange_complete(pinfo);
    }
    return tvb_reported_length_remaining(tvb, 0);
}

static int dissect_pmproxy(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_) {

    col_set_str(pinfo->cinfo, COL_PROTOCOL, "PMPROXY");
    col_clear(pinfo->cinfo, COL_INFO);

    if(is_pmproxy_exchange_complete(pinfo)) {
        return dissect_through_pcp(tvb, pinfo, tree);
    }
    else {
        return dissect_pmproxy_exchange(tvb, pinfo, tree);
    }
}

void proto_register_pmproxy(void) {

    static hf_register_info hf[] = {
        { &hf_pmproxy_host,
          { "Host", "pmproxy.host",
            FT_STRING, BASE_NONE,
            NULL, 0x0,
            NULL, HFILL
          }
        },
        { &hf_pmproxy_port,
          { "Port", "pmproxy.port",
            FT_STRING, BASE_NONE,
            NULL, 0x0,
            NULL, HFILL
          }
        },
        { &hf_pmproxy_client_version,
          { "Client Version", "pmproxy.client_version",
            FT_STRING, BASE_NONE,
            NULL, 0x0,
            NULL, HFILL
          }
        },
        { &hf_pmproxy_server_version,
          { "Server Version", "pmproxy.server_version",
            FT_STRING, BASE_NONE,
            NULL, 0x0,
           NULL, HFILL
          }
        },
    };

    static int *ett[] = {
            &ett_pmproxy,
    };

    proto_pmproxy = proto_register_protocol("Performance Co-Pilot Proxy", "PMPROXY", "pmproxy");

    proto_register_field_array(proto_pmproxy, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));

    pmproxy_handle = register_dissector("pmproxy", dissect_pmproxy, proto_pmproxy);
}


void proto_reg_handoff_pmproxy(void) {

    pcp_handle = find_dissector_add_dependency("pcp", proto_pmproxy);

    dissector_add_uint_with_preference("tcp.port", PMPROXY_PORT, pmproxy_handle);

}

/*
 * Editor modelines  -  https://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 4
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=4 tabstop=8 expandtab:
 * :indentSize=4:tabSize=8:noTabs=true:
 */
