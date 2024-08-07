/* packet-tte-pcf.c
 * Routines for Time Triggered Ethernet Protocol Control Frame dissection
 *
 * Author: Valentin Ecker
 * Author: Benjamin Roch, benjamin.roch (AT) tttech.com
 *
 * TTTech Computertechnik AG, Austria.
 * https://www.tttech.com/technologies/time-triggered-ethernet/
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <epan/packet.h>
#include <epan/etypes.h>

#include "packet-tte.h"

void proto_register_tte_pcf(void);
void proto_reg_handoff_tte_pcf(void);

/* Initialize the protocol and registered fields */
static int proto_tte_pcf;

static int hf_tte_pcf_ic;
static int hf_tte_pcf_mn;
/* static int hf_tte_pcf_res0; */
static int hf_tte_pcf_sp;
static int hf_tte_pcf_sd;
static int hf_tte_pcf_type;
/* static int hf_tte_pcf_res1; */
static int hf_tte_pcf_tc;

/* Initialize the subtree pointers */
static int ett_tte_pcf;

static dissector_handle_t tte_pcf_handle;

static const value_string pcf_type_str_vals[] =
    { {2, "integration frame"}
    , {4, "coldstart frame"}
    , {8, "coldstart ack frame"}
    , {0, NULL}
    };


/* Code to actually dissect the packets */
static int
dissect_tte_pcf(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
    /* Set up structures needed to add the protocol subtree and manage it */
    proto_item *tte_pcf_root_item;
    proto_tree *tte_pcf_tree;

    /* variables used to store the fields displayed in the info_column */
    uint8_t sync_priority = 0;
    uint8_t sync_domain   = 0;

    /* Check that there's enough data */
    if (tvb_reported_length(tvb) < TTE_PCF_LENGTH )
    {
        return 0;
    }

    /* get sync_priority and sync_domain */
    sync_priority = tvb_get_uint8(tvb, TTE_PCF_IC_LENGTH+TTE_PCF_MN_LENGTH+
        TTE_PCF_RES0_LENGTH);
    sync_domain = tvb_get_uint8(tvb, TTE_PCF_IC_LENGTH+TTE_PCF_MN_LENGTH+
        TTE_PCF_RES0_LENGTH+TTE_PCF_SP_LENGTH);

    /* Make entries in Protocol column and Info column on summary display */
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "PCF");

    col_add_fstr(pinfo->cinfo, COL_INFO,
            "Sync Domain: 0x%02X  Sync Priority: 0x%02X",
            sync_domain, sync_priority);

    if (tree) {

        /* create display subtree for the protocol */
        tte_pcf_root_item = proto_tree_add_item(tree, proto_tte_pcf, tvb, 0,
            TTE_PCF_LENGTH, ENC_NA);

        tte_pcf_tree = proto_item_add_subtree(tte_pcf_root_item, ett_tte_pcf);

        proto_tree_add_item(tte_pcf_tree,
            hf_tte_pcf_ic, tvb, 0, TTE_PCF_IC_LENGTH, ENC_BIG_ENDIAN);

        proto_tree_add_item(tte_pcf_tree,
            hf_tte_pcf_mn, tvb, TTE_PCF_IC_LENGTH, TTE_PCF_MN_LENGTH, ENC_BIG_ENDIAN);

     /* RESERVED FIELD --- will not be displayed */
     /* proto_tree_add_item(tte_pcf_tree,
            hf_tte_pcf_res0, tvb, TTE_PCF_IC_LENGTH+TTE_PCF_MN_LENGTH,
            TTE_PCF_RES0_LENGTH, ENC_BIG_ENDIAN); */

        proto_tree_add_item(tte_pcf_tree,
            hf_tte_pcf_sp, tvb, TTE_PCF_IC_LENGTH+TTE_PCF_MN_LENGTH+
            TTE_PCF_RES0_LENGTH, TTE_PCF_SP_LENGTH, ENC_BIG_ENDIAN);

        proto_tree_add_item(tte_pcf_tree,
            hf_tte_pcf_sd, tvb, TTE_PCF_IC_LENGTH+TTE_PCF_MN_LENGTH+
            TTE_PCF_RES0_LENGTH+TTE_PCF_SP_LENGTH, TTE_PCF_SD_LENGTH, ENC_BIG_ENDIAN);

        proto_tree_add_item(tte_pcf_tree,
            hf_tte_pcf_type, tvb, TTE_PCF_IC_LENGTH+TTE_PCF_MN_LENGTH+
            TTE_PCF_RES0_LENGTH+TTE_PCF_SP_LENGTH+TTE_PCF_SD_LENGTH,
            TTE_PCF_TYPE_LENGTH, ENC_BIG_ENDIAN);

     /* RESERVED FIELD --- will not be displayed */
     /* proto_tree_add_item(tte_pcf_tree,
            hf_tte_pcf_res1, tvb, TTE_PCF_IC_LENGTH+TTE_PCF_MN_LENGTH+
            TTE_PCF_RES0_LENGTH+TTE_PCF_SP_LENGTH+TTE_PCF_SD_LENGTH+
            TTE_PCF_TYPE_LENGTH, TTE_PCF_RES1_LENGTH, ENC_NA); */

        proto_tree_add_item(tte_pcf_tree,
            hf_tte_pcf_tc, tvb, TTE_PCF_IC_LENGTH+TTE_PCF_MN_LENGTH+
            TTE_PCF_RES0_LENGTH+TTE_PCF_SP_LENGTH+TTE_PCF_SD_LENGTH+
            TTE_PCF_TYPE_LENGTH+TTE_PCF_RES1_LENGTH, TTE_PCF_TC_LENGTH, ENC_BIG_ENDIAN);
    }

    return tvb_captured_length(tvb);
}


void
proto_register_tte_pcf(void)
{
    static hf_register_info hf[] = {

        { &hf_tte_pcf_ic,
            { "Integration Cycle", "tte_pcf.ic",
            FT_UINT32, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
            { &hf_tte_pcf_mn,
            { "Membership New", "tte_pcf.mn",
            FT_UINT32, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
#if 0
            { &hf_tte_pcf_res0,
            { "Reserved 0", "tte_pcf.res0",
            FT_UINT32, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
#endif
        { &hf_tte_pcf_sp,
            { "Sync Priority", "tte_pcf.sp",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_tte_pcf_sd,
            { "Sync Domain", "tte_pcf.sd",
            FT_UINT8, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        },
        { &hf_tte_pcf_type,
            { "Type", "tte_pcf.type",
            FT_UINT8, BASE_HEX, VALS(pcf_type_str_vals), 0x0F,
            NULL, HFILL }
        },
#if 0
        { &hf_tte_pcf_res1,
            { "Reserved 1", "tte_pcf.res1",
            FT_BYTES, BASE_NONE, NULL, 0x0,
            NULL, HFILL }
        },
#endif
        { &hf_tte_pcf_tc,
            { "Transparent Clock", "tte_pcf.tc",
            FT_UINT64, BASE_HEX, NULL, 0x0,
            NULL, HFILL }
        }
    };

    /* Setup protocol subtree array */
    static int *ett[] = {
        &ett_tte_pcf
    };

    /* Register the protocol name and description */
    proto_tte_pcf = proto_register_protocol("TTEthernet Protocol Control Frame",
        "TTE PCF", "tte_pcf");

    /* Required function calls to register header fields and subtrees used */
    proto_register_field_array(proto_tte_pcf, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));

    tte_pcf_handle = register_dissector("tte_pcf", dissect_tte_pcf, proto_tte_pcf);
}


void
proto_reg_handoff_tte_pcf(void)
{
    dissector_add_uint("ethertype", ETHERTYPE_TTE_PCF, tte_pcf_handle);

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
