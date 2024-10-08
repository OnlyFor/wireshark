/* packet-dcom-sysact.c
 * Routines for the ISystemActivator interface
 * Copyright 2004, Jelmer Vernooij <jelmer@samba.org>
 * Copyright 2012, Litao Gao <ltgao@juniper.net>
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <epan/packet.h>
#include "packet-dcerpc.h"
#include "packet-dcom.h"

void proto_register_ISystemActivator(void);
void proto_reg_handoff_ISystemActivator(void);

static int proto_ISystemActivator;

static int ett_isystemactivator;
static int hf_opnum;
static int hf_sysact_actproperties;
/* static int hf_sysact_unknown; */

static int ett_actproperties;
static int hf_sysact_totalsize;
static int hf_sysact_res;

static int ett_commonheader;
static int ett_propguids;
static int ett_properties;
static int hf_sysact_customhdrsize;
static int hf_sysact_dstctx;
static int hf_sysact_actpropnumber;
static int hf_sysact_actpropclsinfoid;
/* static int hf_sysact_actpropclsids; */
static int hf_sysact_actpropclsid;
/* static int hf_sysact_actpropsizes; */
static int hf_sysact_actpropsize;


static int ett_dcom_spclsysprop;
static int ett_dcom_reserved;
static int hf_sysact_spsysprop_sid;
static int hf_sysact_spsysprop_remotethissid;
static int hf_sysact_spsysprop_cltimpersonating;
static int hf_sysact_spsysprop_partitionid;
static int hf_sysact_spsysprop_defauthlvl;
static int hf_sysact_spsysprop_partition;
static int hf_sysact_spsysprop_procrqstflgs;
static int hf_sysact_spsysprop_origclsctx;
static int hf_sysact_spsysprop_flags;
/* static int hf_sysact_spsysprop_procid; */
/* static int hf_sysact_spsysprop_hwnd; */

static int ett_dcom_instantianinfo;
static int hf_sysact_instninfo_clsid;
static int hf_sysact_instninfo_clsctx;
static int hf_sysact_instninfo_actflags;
static int hf_sysact_instninfo_issurrogate;
static int hf_sysact_instninfo_iidcount;
static int hf_sysact_instninfo_instflags;
static int hf_sysact_instninfo_entiresize;
static int hf_sysact_instninfo_iid;

static int ett_dcom_actctxinfo;
static int hf_sysact_actctxinfo_cltok;
static int hf_sysact_context;

static int ett_dcom_instanceinfo;
static int hf_sysact_instinfo_mode;
static int hf_sysact_instinfo_stg;

static int ett_dcom_istorage;
static int hf_sysact_istg;

static int ett_dcom_context;
static int hf_sysact_ctx_id;
static int hf_sysact_ctx_flags;
static int hf_sysact_ctx_res;
static int hf_sysact_ctx_numextents;
static int hf_sysact_ctx_extentscnt;
static int hf_sysact_ctx_mashflags;
static int hf_sysact_ctx_count;
static int hf_sysact_ctx_frozen;

static int ett_dcom_securityinfo;
static int hf_sysact_si_authflalgs;
static int hf_sysact_si_ci_res;
static int hf_sysact_si_ci_string;
static int hf_sysact_si_serverinfo;

static int ett_dcom_locationinfo;
static int hf_sysact_li_string;
static int hf_sysact_li_procid;
static int hf_sysact_li_apartid;
static int hf_sysact_li_ctxid;

static int ett_dcom_scmrqstinfo;
static int ett_dcom_rmtrqst;

static int hf_sysact_sri_cltimplvl;
static int hf_sysact_sri_protseqnum;
static int hf_sysact_sri_protseq;

static int ett_dcom_propsoutput;
static int hf_sysact_pi_ifnum;
static int hf_sysact_pi_retval;
static int hf_sysact_pi_interf;
static int hf_sysact_pi_iid;

static int ett_dcom_scmrespinfo;
static int ett_dcom_rmtresp;
static int ett_dcom_oxidbinding;
static int hf_sysact_scmri_rmtunknid;
static int hf_sysact_scmri_authhint;
static int hf_sysact_scmri_binding;
static int hf_sysact_scmri_oxid;
static int hf_sysact_unused_buffer;

static int ett_typeszcommhdr;
static int ett_typeszprivhdr;
static int hf_typeszch;
static int hf_typeszph;
static int hf_typesz_ver;
static int hf_typesz_endianness;
static int hf_typesz_commhdrlen;
static int hf_typesz_filler;
static int hf_typesz_buflen;

static e_guid_t uuid_ISystemActivator = { 0x000001a0, 0x0000, 0x0000, { 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
static uint16_t ver_ISystemActivator;

/*static e_guid_t clsid_ActivationPropertiesIn = { 0x00000338, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46} };*/
/*static e_guid_t clsid_ActivationPropertiesOut = { 0x00000339, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46} };*/
static e_guid_t iid_ActivationPropertiesIn = { 0x000001a2, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46} };
static e_guid_t iid_ActivationPropertiesOut = { 0x000001a3, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46} };

static e_guid_t clsid_SpecialSystemProperties = { 0x000001b9, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46} };
static e_guid_t clsid_InstantiationInfo = { 0x000001ab, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46} };
static e_guid_t clsid_ActivationContextInfo = { 0x000001a5, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46} };
//static e_guid_t clsid_ContextMarshaler = { 0x0000033b, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46} };
static e_guid_t iid_IContext = { 0x000001c0, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46} };
static e_guid_t clsid_SecurityInfo = { 0x000001a6, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46} };
static e_guid_t clsid_ServerLocationInfo = { 0x000001a4, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46} };
static e_guid_t clsid_ScmRequestInfo = { 0x000001aa, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46} };
static e_guid_t clsid_PropsOutInfo = { 0x00000339, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46} };
static e_guid_t clsid_ScmReplyInfo = { 0x000001b6, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46} };
static e_guid_t clsid_InstanceInfo = { 0x000001ad, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46} };
static e_guid_t iid_IStorage = { 0x0000000B, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46} };


static const value_string instninfo_actflags[] = {
    { 0x00000002, "ACTVFLAGS_DISABLE_AAA" },
    { 0x00000004, "ACTVFLAGS_ACTIVATE_32_BIT_SERVER" },
    { 0x00000008, "ACTVFLAGS_ACTIVATE_64_BIT_SERVER" },
    { 0x00000020, "ACTVFLAGS_NO_FAILURE_LOG" },
    { 0,  NULL }
};

static const value_string boolean_flag_vals[] = {
    { 0x00000001, "TRUE" },
    { 0x00000000, "FALSE" },
    { 0,  NULL }
};

static const value_string dcom_context_flag_vals[] = {
    { 0x00000002, "MarshalByValue" },
    { 0,  NULL }
};

static const value_string ts_endian_vals[] = {
    { 0x10, "Little-endian" },
    { 0x00, "Big-endian" },
    { 0,  NULL }
};

/* MS-DCOM 2.2.28.1 */
#define MIN_ACTPROP_LIMIT 1
#define MAX_ACTPROP_LIMIT 10

typedef struct property_guids {
    e_guid_t guid[MAX_ACTPROP_LIMIT];
    uint32_t size[MAX_ACTPROP_LIMIT];
    uint32_t id_idx;
    uint32_t size_idx;
} property_guids_t;

/* Type Serialization Version 1 */
static int
dissect_TypeSzCommPrivHdr(tvbuff_t *tvb, int offset, packet_info *pinfo,
                       proto_tree *tree, dcerpc_info *di, uint8_t *drep)
{
    proto_item *sub_item;
    proto_tree *sub_tree;
    uint8_t drep_tmp;
    uint8_t endian = 0x10;
    int    old_offset;

    /* Common Header use little endian */
    sub_item = proto_tree_add_item(tree, hf_typeszch, tvb, offset, 0, ENC_NA);
    sub_tree = proto_item_add_subtree(sub_item, ett_typeszcommhdr);

    old_offset = offset;
    offset = dissect_dcom_BYTE(tvb, offset, pinfo, sub_tree, di, drep,
            hf_typesz_ver, NULL);

    offset = dissect_dcom_BYTE(tvb, offset, pinfo, sub_tree, di, drep,
            hf_typesz_endianness, &endian);
    if (endian == 0x10)
        *drep = DREP_LITTLE_ENDIAN;
    else
        *drep &= ~DREP_LITTLE_ENDIAN;

    drep_tmp = DREP_LITTLE_ENDIAN;
    offset = dissect_dcom_WORD(tvb, offset, pinfo, sub_tree, di, &drep_tmp,
            hf_typesz_commhdrlen, NULL);
    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, &drep_tmp,
            hf_typesz_filler, NULL);
    proto_item_set_len(sub_item, offset - old_offset);

    /* Private Header */
    old_offset = offset;
    sub_item = proto_tree_add_item(tree, hf_typeszph, tvb, offset, 0, ENC_NA);
    sub_tree = proto_item_add_subtree(sub_item, ett_typeszprivhdr);
    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_typesz_buflen, NULL);
    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_typesz_filler, NULL);
    proto_item_set_len(sub_item, offset - old_offset);

    return offset;
}



static int
dissect_dcom_Property_Guid(tvbuff_t *tvb, int offset, packet_info *pinfo,
                            proto_tree *tree, dcerpc_info *di, uint8_t *drep)
{
    property_guids_t *pg;

    pg = (property_guids_t*)di->private_data;

    if (pg->id_idx < MAX_ACTPROP_LIMIT) {
        offset = dissect_dcom_UUID(tvb, offset, pinfo, tree, di, drep,
                hf_sysact_actpropclsid, &pg->guid[pg->id_idx++]);
    }
    else {
        /* TODO: expert info */
        tvb_ensure_bytes_exist(tvb, offset, 16);
        offset += 16;
    }

    return offset;
}

static int
dissect_dcom_ActivationPropertiesCustomerHdr_PropertyGuids(tvbuff_t *tvb, int offset,
        packet_info *pinfo, proto_tree *tree, dcerpc_info *di, uint8_t *drep)
{
    offset = dissect_ndr_ucarray(tvb, offset, pinfo, tree, di, drep, dissect_dcom_Property_Guid);
    return offset;
}

static int
dissect_dcom_Property_Size(tvbuff_t *tvb, int offset, packet_info *pinfo,
                            proto_tree *tree, dcerpc_info *di, uint8_t *drep)
{
    property_guids_t *pg;

    pg = (property_guids_t*)di->private_data;

    if (pg->size_idx < MAX_ACTPROP_LIMIT) {
        offset = dissect_dcom_DWORD(tvb, offset, pinfo, tree, di, drep,
                hf_sysact_actpropsize, &pg->size[pg->size_idx++]);
    }
    else {
        /* TODO: expert info */
        tvb_ensure_bytes_exist(tvb, offset, 4);
        offset += 4;
    }

    return offset;
}

static int
dissect_dcom_ActivationPropertiesCustomerHdr_PropertySizes(tvbuff_t *tvb, int offset,
        packet_info *pinfo, proto_tree *tree, dcerpc_info *di, uint8_t *drep)
{
    offset = dissect_ndr_ucarray(tvb, offset, pinfo, tree, di, drep, dissect_dcom_Property_Size);
    return offset;
}

static int
dissect_dcom_ActivationPropertiesCustomerHdr(tvbuff_t *tvb, int offset, packet_info *pinfo,
                       proto_tree *tree, dcerpc_info *di, uint8_t *drep)
{
    uint32_t  u32TotalSize;
    uint32_t  u32CustomHdrSize;
    uint32_t  u32ActPropNumber;
    int       old_offset;

    proto_item *sub_item;
    proto_tree *sub_tree;

    sub_tree = proto_tree_add_subtree(tree, tvb, offset, 0, ett_commonheader, &sub_item, "CustomHeader");

    old_offset = offset;
    offset = dissect_TypeSzCommPrivHdr(tvb, offset, pinfo, sub_tree, di, drep);

    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_totalsize, &u32TotalSize);
    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_customhdrsize, &u32CustomHdrSize);
    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_res, NULL);
    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_dstctx, NULL);
    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_actpropnumber, &u32ActPropNumber);
    offset = dissect_dcom_UUID(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_actpropclsinfoid, NULL);

    /* ClsIdPtr, SizesPtr */
    offset = dissect_ndr_embedded_pointer(tvb, offset, pinfo, sub_tree, di, drep,
            dissect_dcom_ActivationPropertiesCustomerHdr_PropertyGuids, NDR_POINTER_UNIQUE,
            "ClsIdPtr",hf_sysact_actpropclsid);
    offset = dissect_ndr_embedded_pointer(tvb, offset, pinfo, sub_tree, di, drep,
            dissect_dcom_ActivationPropertiesCustomerHdr_PropertySizes, NDR_POINTER_UNIQUE,
            "ClsSizesPtr",hf_sysact_actpropclsid);
    offset = dissect_ndr_embedded_pointer(tvb, offset, pinfo, sub_tree, di, drep,
            NULL, NDR_POINTER_UNIQUE, "OpaqueDataPtr: Pointer To NULL", 0);

    dissect_deferred_pointers(pinfo, tvb, offset, di, drep);
    proto_item_set_len(sub_item, u32CustomHdrSize);

    return old_offset + u32CustomHdrSize;
}


static int
dissect_dcom_ActivationProperty(tvbuff_t *tvb, int offset, packet_info *pinfo,
                       proto_tree *tree, dcerpc_info *di, uint8_t *drep, e_guid_t *clsid, int size)
{
    dcom_dissect_fn_t routine = NULL;

    /* the following data depends on the clsid, get the routine by clsid */
    routine = dcom_get_routine_by_uuid(clsid);
    if (routine){
        routine(tvb, offset, pinfo, tree, di, drep, size);
    }

    return offset+size;
}



static int
dissect_dcom_ActivationPropertiesBody(tvbuff_t *tvb, int offset, packet_info *pinfo,
                       proto_tree *tree, dcerpc_info *di, uint8_t *drep)
{
    int       old_offset;

    proto_item *sub_item;
    proto_tree *sub_tree;
    property_guids_t *pg;
    uint32_t i;
    uint32_t min_idx;

    pg = (property_guids_t*)di->private_data;

    if (pg->id_idx == pg->size_idx) {
        min_idx = pg->id_idx;
    }
    else {
        /* TODO: expert info */
        min_idx = MIN(pg->id_idx, pg->size_idx);
    }

    sub_tree = proto_tree_add_subtree(tree, tvb, offset, 0, ett_properties, &sub_item, "Properties");

    old_offset = offset;
    for (i = 0; i < min_idx; i++) {
        offset = dissect_dcom_ActivationProperty(tvb, offset, pinfo, sub_tree, di, drep,
                                                    &pg->guid[i], pg->size[i]);
    }
    proto_item_set_len(sub_item, offset - old_offset);

    return offset;
}

static int
dissect_dcom_ActivationProperties(tvbuff_t *tvb, int offset, packet_info *pinfo,
        proto_tree *tree, dcerpc_info *di, uint8_t *drep, int size _U_)
{
    proto_item *sub_item;
    proto_tree *sub_tree;
    property_guids_t *old_pg = NULL;

    uint32_t   u32TotalSize;
    uint32_t   u32Res;

    sub_item = proto_tree_add_item(tree, hf_sysact_actproperties, tvb, offset, 0, ENC_NA);
    sub_tree = proto_item_add_subtree(sub_item, ett_actproperties);

    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_totalsize, &u32TotalSize);
    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_res, &u32Res);

    old_pg = (property_guids_t*)di->private_data;
    di->private_data = wmem_new0(pinfo->pool, property_guids_t);

    offset = dissect_dcom_ActivationPropertiesCustomerHdr(tvb, offset, pinfo, sub_tree, di, drep);
    offset = dissect_dcom_ActivationPropertiesBody(tvb, offset, pinfo, sub_tree, di, drep);

    di->private_data = old_pg;

    return offset;
}



static int
dissect_dcom_IStorage(tvbuff_t *tvb, int offset, packet_info *pinfo,
                       proto_tree *tree, dcerpc_info *di, uint8_t *drep, int size _U_)
{
    proto_item  *sub_item;
    proto_tree  *sub_tree;
    int         old_offset;

    old_offset = offset;
    sub_tree = proto_tree_add_subtree(tree, tvb, offset, 0, ett_dcom_istorage, &sub_item, "IStorage");

    offset = dissect_dcom_OBJREF(tvb, offset, pinfo, sub_tree, di, drep, hf_sysact_istg, NULL);

    proto_item_set_len(sub_item, offset - old_offset);

    return offset;
}

static int
dissect_dcom_IContext(tvbuff_t *tvb, int offset, packet_info *pinfo,
                       proto_tree *tree, dcerpc_info *di, uint8_t *drep, int size _U_)
{
    proto_item  *sub_item;
    proto_tree  *sub_tree;
    int         old_offset;

    uint32_t   u32Count;

    old_offset = offset;
    sub_tree = proto_tree_add_subtree(tree, tvb, offset, 0, ett_dcom_context, &sub_item, "Context");

    offset = dissect_dcom_COMVERSION(tvb, offset, pinfo, sub_tree, di, drep,
                NULL, NULL);
    offset = dissect_dcom_UUID(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_ctx_id, NULL);
    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_ctx_flags, NULL);
    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_ctx_res, NULL);
    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_ctx_numextents, NULL);
    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_ctx_extentscnt, NULL);
    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_ctx_mashflags, NULL);
    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_ctx_count, &u32Count);
    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_ctx_frozen, NULL);

    if (u32Count) {
        /*PropMarshalHeader array*/
        /*TBD*/
    }

    proto_item_set_len(sub_item, offset - old_offset);

    return offset;
}

static int
dissect_dcom_SpecialSystemProperties(tvbuff_t *tvb, int offset, packet_info *pinfo,
                       proto_tree *tree, dcerpc_info *di, uint8_t *drep, int size)
{
    proto_tree *sub_tree, *tr;
    int old_offset, len, i;

    old_offset = offset;

    if (size <= 0) {
        /* TODO: expert info */
        size = -1;
    }

    sub_tree = proto_tree_add_subtree(tree, tvb, offset, size, ett_dcom_spclsysprop, NULL, "SpecialSystemProperties");

    offset = dissect_TypeSzCommPrivHdr(tvb, offset, pinfo, sub_tree, di, drep);

    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_spsysprop_sid, NULL);
    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_spsysprop_remotethissid, NULL);
    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_spsysprop_cltimpersonating, NULL);
    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_spsysprop_partitionid, NULL);
    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_spsysprop_defauthlvl, NULL);
    offset = dissect_dcom_UUID(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_spsysprop_partition, NULL);
    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_spsysprop_procrqstflgs, NULL);
    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_spsysprop_origclsctx, NULL);
    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_spsysprop_flags, NULL);
/*
 *
 *    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
 *            hf_sysact_spsysprop_procid, NULL);
 *    offset = dissect_dcom_I8(tvb, offset, pinfo, sub_tree, drep,
 *            hf_sysact_spsysprop_hwnd, NULL);
 *
 */
    tr = proto_tree_add_subtree(sub_tree, tvb, offset, sizeof(uint32_t)*8,
                             ett_dcom_reserved, NULL, "Reserved: 8 DWORDs");
    for (i = 0; i < 8; i++) {
        offset = dissect_dcom_DWORD(tvb, offset, pinfo, tr, di, drep,
                hf_sysact_res, NULL);
    }

    len = offset - old_offset;
    if (size < len) {
        /* TODO expert info */
        size = len;
    }
    else if (size > len) {
        proto_tree_add_item(sub_tree, hf_sysact_unused_buffer, tvb, offset, size - len, ENC_NA);
    }

    offset = old_offset + size;
    return offset;
}

static int
dissect_dcom_InterfaceId(tvbuff_t *tvb, int offset, packet_info *pinfo,
                            proto_tree *tree, dcerpc_info *di, uint8_t *drep)
{
    offset = dissect_dcom_UUID(tvb, offset, pinfo, tree, di, drep,
                        hf_sysact_instninfo_iid, NULL);
    return offset;
}

static int
dissect_InstantiationInfoIids(tvbuff_t *tvb, int offset,
        packet_info *pinfo, proto_tree *tree, dcerpc_info *di, uint8_t *drep)
{
    offset = dissect_ndr_ucarray(tvb, offset, pinfo, tree, di, drep,
            dissect_dcom_InterfaceId);

    return offset;
}

static int
dissect_dcom_InstantiationInfo(tvbuff_t *tvb, int offset, packet_info *pinfo,
                       proto_tree *tree, dcerpc_info *di, uint8_t *drep, int size)
{
    proto_tree *sub_tree;
    int old_offset, len;

    old_offset = offset;

    if (size <= 0) {
        /* TODO: expert info */
        size = -1;
    }

    sub_tree = proto_tree_add_subtree(tree, tvb, offset, size, ett_dcom_instantianinfo, NULL, "InstantiationInfo");

    offset = dissect_TypeSzCommPrivHdr(tvb, offset, pinfo, sub_tree, di, drep);

    offset = dissect_dcom_UUID(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_instninfo_clsid, NULL);
    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_instninfo_clsctx, NULL);
    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_instninfo_actflags, NULL);
    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_instninfo_issurrogate, NULL);
    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_instninfo_iidcount, NULL);
    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_instninfo_instflags, NULL);

    offset = dissect_ndr_embedded_pointer(tvb, offset, pinfo, sub_tree, di, drep,
            dissect_InstantiationInfoIids, NDR_POINTER_UNIQUE,
            "InterfaceIdsPtr", -1);

    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_instninfo_entiresize, NULL);
    offset = dissect_dcom_COMVERSION(tvb, offset, pinfo, sub_tree, di, drep,
            NULL, NULL);

    offset = dissect_deferred_pointers(pinfo, tvb, offset, di, drep);

    len = offset - old_offset;
    if (size < len) {
        /* TODO expert info */
        size = len;
    }
    else if (size > len) {
        proto_tree_add_item(sub_tree, hf_sysact_unused_buffer, tvb, offset, size - len, ENC_NA);
    }

    offset = old_offset + size;
    return offset;
}

static int
dissect_ActCtxInfo_PropCtx(tvbuff_t *tvb _U_, int offset,
        packet_info *pinfo _U_, proto_tree *tree _U_, dcerpc_info *di _U_, uint8_t *drep _U_)
{
    /*TBD*/
    return offset;
}


static int
dissect_ActCtxInfo_CltCtx(tvbuff_t *tvb, int offset,
        packet_info *pinfo, proto_tree *tree, dcerpc_info *di, uint8_t *drep)
{
    if (di->conformant_run) {
        return offset;
    }

    offset = dissect_dcom_MInterfacePointer(tvb, offset, pinfo, tree, di, drep,
            hf_sysact_context, NULL);
    return offset;
}

static int
dissect_dcom_ActivationContextInfo(tvbuff_t *tvb, int offset, packet_info *pinfo,
                       proto_tree *tree, dcerpc_info *di, uint8_t *drep, int size)
{
    proto_tree *sub_tree;
    int old_offset, len;

    old_offset = offset;

    if (size <= 0) {
        /* TODO: expert info */
        size = -1;
    }

    sub_tree = proto_tree_add_subtree(tree, tvb, offset, size, ett_dcom_actctxinfo, NULL, "ActivationContextInfo");

    offset = dissect_TypeSzCommPrivHdr(tvb, offset, pinfo, sub_tree, di, drep);

    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_actctxinfo_cltok, NULL);
    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_res, NULL);
    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_res, NULL);
    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_res, NULL);

    offset = dissect_ndr_embedded_pointer(tvb, offset, pinfo, sub_tree, di, drep,
            dissect_ActCtxInfo_CltCtx, NDR_POINTER_UNIQUE,
            "ClientPtr", -1);
    offset = dissect_ndr_embedded_pointer(tvb, offset, pinfo, sub_tree, di, drep,
            dissect_ActCtxInfo_PropCtx, NDR_POINTER_UNIQUE,
            "PrototypePtr", -1);
    offset = dissect_deferred_pointers(pinfo, tvb, offset, di, drep);

    len = offset - old_offset;
    if (size < len) {
        /* TODO expert info */
        size = len;
    }
    else if (size > len) {
        proto_tree_add_item(sub_tree, hf_sysact_unused_buffer, tvb, offset, size - len, ENC_NA);
    }

    offset = old_offset + size;
    return offset;
}


static int
dissect_InstInfo_IfdROT(tvbuff_t *tvb _U_, int offset,
        packet_info *pinfo _U_, proto_tree *tree _U_, dcerpc_info *di _U_, uint8_t *drep _U_)
{
    /*TBD*/
    return offset;
}


static int
dissect_InstInfo_IfdStg(tvbuff_t *tvb, int offset,
        packet_info *pinfo, proto_tree *tree, dcerpc_info *di, uint8_t *drep)
{
    if (di->conformant_run) {
        return offset;
    }

    offset = dissect_dcom_MInterfacePointer(tvb, offset, pinfo, tree, di, drep,
            hf_sysact_instinfo_stg, NULL);
    return offset;
}


static int
dissect_dcom_InstanceInfo(tvbuff_t *tvb, int offset, packet_info *pinfo,
              proto_tree *tree, dcerpc_info *di, uint8_t *drep, int size)
{
    proto_tree *sub_tree;
    int old_offset, len;

    old_offset = offset;

    if (size <= 0) {
        /* TODO: expert info */
        size = -1;
    }

    sub_tree = proto_tree_add_subtree(tree, tvb, offset, size, ett_dcom_instanceinfo, NULL, "InstanceInfo");

    offset = dissect_TypeSzCommPrivHdr(tvb, offset, pinfo, sub_tree, di, drep);

    /*
    typedef struct tagInstanceInfoData {
        [string] wchar_t* fileName;
        DWORD mode;
        MInterfacePointer* ifdROT;
        MInterfacePointer* ifdStg;
    } InstanceInfoData;
    */

    offset = dissect_ndr_embedded_pointer(tvb, offset, pinfo, sub_tree, di, drep,
            dissect_ndr_wchar_cvstring, NDR_POINTER_UNIQUE, "FileNamePtr",
            hf_sysact_li_string);

    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_instinfo_mode, NULL);

    offset = dissect_ndr_embedded_pointer(tvb, offset, pinfo, sub_tree, di, drep,
            dissect_InstInfo_IfdROT, NDR_POINTER_UNIQUE,
            "ifdROTPtr", -1);
    offset = dissect_ndr_embedded_pointer(tvb, offset, pinfo, sub_tree, di, drep,
            dissect_InstInfo_IfdStg, NDR_POINTER_UNIQUE,
            "ifdStgPtr", -1);
    offset = dissect_deferred_pointers(pinfo, tvb, offset, di, drep);

    len = offset - old_offset;
    if (size < len) {
        /* TODO expert info */
        size = len;
    }
    else if (size > len) {
        proto_tree_add_item(sub_tree, hf_sysact_unused_buffer, tvb, offset, size - len, ENC_NA);
    }

    offset = old_offset + size;
    return offset;
}


static int
dissect_dcom_COSERVERINFO(tvbuff_t *tvb, int offset,
        packet_info *pinfo, proto_tree *tree, dcerpc_info *di, uint8_t *drep, int hfindex)
{
    proto_item *sub_item;
    proto_tree *sub_tree;
    int old_offset;

    if (di->conformant_run) {
        return offset;
    }

    sub_item = proto_tree_add_item(tree, hfindex, tvb, offset, 0, ENC_NA);
    sub_tree = proto_item_add_subtree(sub_item, ett_dcom_securityinfo);

    old_offset = offset;
    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_si_ci_res, NULL);
    offset = dissect_ndr_embedded_pointer(tvb, offset, pinfo, sub_tree, di, drep,
            dissect_ndr_wchar_cvstring, NDR_POINTER_UNIQUE, "Name(wstring)",
            hf_sysact_si_ci_string);
    offset = dissect_ndr_embedded_pointer(tvb, offset, pinfo, sub_tree, di, drep,
            NULL, NDR_POINTER_UNIQUE, "AuthInfoPtr", -1);
    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_si_ci_res, NULL);

    offset = dissect_deferred_pointers(pinfo, tvb, offset, di, drep);

    proto_item_set_len(sub_item, offset - old_offset);

    return offset;
}

static int
dissect_dcom_SI_ServerInfo(tvbuff_t *tvb, int offset,
        packet_info *pinfo, proto_tree *tree, dcerpc_info *di, uint8_t *drep)
{
    offset = dissect_dcom_COSERVERINFO(tvb, offset, pinfo, tree, di, drep,
            hf_sysact_si_serverinfo);
    return offset;
}

static int
dissect_dcom_SecurtiyInfo(tvbuff_t *tvb, int offset, packet_info *pinfo,
                       proto_tree *tree, dcerpc_info *di, uint8_t *drep, int size)
{
    proto_tree *sub_tree;
    int old_offset, len;

    old_offset = offset;

    if (size <= 0) {
        /* TODO: expert info */
        size = -1;
    }

    sub_tree = proto_tree_add_subtree(tree, tvb, offset, size, ett_dcom_securityinfo, NULL, "SecurityInfo");

    offset = dissect_TypeSzCommPrivHdr(tvb, offset, pinfo, sub_tree, di ,drep);

    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_si_authflalgs, NULL);
    offset = dissect_ndr_embedded_pointer(tvb, offset, pinfo, sub_tree, di, drep,
            dissect_dcom_SI_ServerInfo, NDR_POINTER_UNIQUE, "ServerInfoPtr", -1);
    /*This SHOULD be NULL and MUST be ignored on receipt*/
    offset = dissect_ndr_embedded_pointer(tvb, offset, pinfo, sub_tree, di, drep,
            NULL, NDR_POINTER_UNIQUE, "ReservedPtr", -1);
    offset = dissect_deferred_pointers(pinfo, tvb, offset, di, drep);

    len = offset - old_offset;
    if (size < len) {
        /* TODO expert info */
        size = len;
    }
    else if (size > len) {
        proto_tree_add_item(sub_tree, hf_sysact_unused_buffer, tvb, offset, size - len, ENC_NA);
    }

    offset = old_offset + size;
    return offset;
}

static int
dissect_dcom_LocationInfo(tvbuff_t *tvb, int offset, packet_info *pinfo,
                       proto_tree *tree, dcerpc_info *di, uint8_t *drep, int size)
{
    proto_tree *sub_tree;
    int old_offset, len;

    old_offset = offset;

    if (size <= 0) {
        /* TODO: expert info */
        size = -1;
    }

    sub_tree = proto_tree_add_subtree(tree, tvb, offset, size, ett_dcom_locationinfo, NULL, "LocationInfo");

    offset = dissect_TypeSzCommPrivHdr(tvb, offset, pinfo, sub_tree, di, drep);

    offset = dissect_ndr_embedded_pointer(tvb, offset, pinfo, sub_tree, di, drep,
            dissect_ndr_wchar_cvstring, NDR_POINTER_UNIQUE, "MachineNamePtr",
            hf_sysact_li_string);

    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_li_procid, NULL);
    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_li_apartid, NULL);
    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_li_ctxid, NULL);

    offset = dissect_deferred_pointers(pinfo, tvb, offset, di, drep);

    len = offset - old_offset;
    if (size < len) {
        /* TODO expert info */
        size = len;
    }
    else if (size > len) {
        proto_tree_add_item(sub_tree, hf_sysact_unused_buffer, tvb, offset, size - len, ENC_NA);
    }

    offset = old_offset + size;

    return offset;
}

static int
dissect_dcom_ProtoSeq(tvbuff_t *tvb, int offset, packet_info *pinfo,
                        proto_tree *tree, dcerpc_info *di, uint8_t *drep)
{
    offset = dissect_dcom_WORD(tvb, offset, pinfo, tree, di, drep,
                        hf_sysact_sri_protseq, NULL);

    return offset;
}

static int
dissect_dcom_ProtoSeqArray(tvbuff_t *tvb, int offset,
                    packet_info *pinfo, proto_tree *tree, dcerpc_info *di, uint8_t *drep)
{
    offset = dissect_ndr_ucarray(tvb, offset, pinfo, tree, di, drep,
            dissect_dcom_ProtoSeq);
    return offset;
}

static int
dissect_dcom_customREMOTE_REQUEST_SCM_INFO(tvbuff_t *tvb, int offset,
        packet_info *pinfo, proto_tree *tree, dcerpc_info *di, uint8_t *drep)
{
    proto_item *sub_item;
    proto_tree *sub_tree;
    int old_offset;

    if (di->conformant_run) {
        return offset;
    }

    sub_tree = proto_tree_add_subtree(tree, tvb, offset, 0, ett_dcom_rmtrqst, &sub_item, "RemoteRequest");

    old_offset = offset;
    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_sri_cltimplvl, NULL);
    offset = dissect_dcom_WORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_sri_protseqnum, NULL);
    offset = dissect_ndr_embedded_pointer(tvb, offset, pinfo, sub_tree, di, drep,
            dissect_dcom_ProtoSeqArray, NDR_POINTER_UNIQUE, "ProtocolSeqsArrayPtr", -1);
    offset = dissect_deferred_pointers(pinfo, tvb, offset, di, drep);

    proto_item_set_len(sub_item, offset - old_offset);

    return offset;
}

static int
dissect_dcom_ScmRqstInfo(tvbuff_t *tvb, int offset, packet_info *pinfo,
                       proto_tree *tree, dcerpc_info *di, uint8_t *drep, int size)
{
    proto_tree *sub_tree;
    int old_offset, len;

    old_offset = offset;

    if (size <= 0) {
        /* TODO: expert info */
        size = -1;
    }

    sub_tree = proto_tree_add_subtree(tree, tvb, offset, size, ett_dcom_scmrqstinfo, NULL, "ScmRequestInfo");

    offset = dissect_TypeSzCommPrivHdr(tvb, offset, pinfo, sub_tree, di, drep);

    /*This MUST be set to NULL and MUST be ignored on receipt*/
    offset = dissect_ndr_embedded_pointer(tvb, offset, pinfo, sub_tree, di, drep,
            NULL, NDR_POINTER_UNIQUE, "Ptr", -1);
    offset = dissect_ndr_embedded_pointer(tvb, offset, pinfo, sub_tree, di, drep,
            dissect_dcom_customREMOTE_REQUEST_SCM_INFO, NDR_POINTER_UNIQUE,
            "RemoteRequestPtr", -1);
    offset = dissect_deferred_pointers(pinfo, tvb, offset, di, drep);

    len = offset - old_offset;
    if (size < len) {
        /* TODO expert info */
        size = len;
    }
    else if (size > len) {
        proto_tree_add_item(sub_tree, hf_sysact_unused_buffer, tvb, offset, size - len, ENC_NA);
    }

    offset = old_offset + size;

    return offset;
}

static int
dissect_dcom_IfId(tvbuff_t *tvb, int offset, packet_info *pinfo,
                        proto_tree *tree, dcerpc_info *di, uint8_t *drep)
{
    offset = dissect_dcom_UUID(tvb, offset, pinfo, tree, di, drep,
            hf_sysact_pi_iid, NULL);
    return offset;
}

static int
dissect_dcom_IfIds(tvbuff_t *tvb, int offset,
                    packet_info *pinfo, proto_tree *tree, dcerpc_info *di, uint8_t *drep)
{
    offset = dissect_ndr_ucarray(tvb, offset, pinfo, tree, di, drep,
            dissect_dcom_IfId);
    return offset;
}

static int
dissect_dcom_ReturnVal(tvbuff_t *tvb, int offset, packet_info *pinfo,
                            proto_tree *tree, dcerpc_info *di, uint8_t *drep)
{
    offset = dissect_dcom_DWORD(tvb, offset, pinfo, tree, di, drep,
                        hf_sysact_pi_retval, NULL);
    return offset;
}

static int
dissect_dcom_ReturnVals(tvbuff_t *tvb, int offset,
        packet_info *pinfo, proto_tree *tree, dcerpc_info *di, uint8_t *drep)
{
    offset = dissect_ndr_ucarray(tvb, offset, pinfo, tree, di, drep,
            dissect_dcom_ReturnVal);
    return offset;
}

static int
dissect_OneInterfData(tvbuff_t *tvb, int offset,
        packet_info *pinfo, proto_tree *tree, dcerpc_info *di, uint8_t *drep)
{
    offset = dissect_dcom_MInterfacePointer(tvb, offset, pinfo, tree, di, drep,
            hf_sysact_pi_interf, NULL);
    return offset;
}

static int
dissect_dcom_OneInterfDataPtr(tvbuff_t *tvb, int offset, packet_info *pinfo,
                            proto_tree *tree, dcerpc_info *di, uint8_t *drep)
{
    offset = dissect_ndr_embedded_pointer(tvb, offset, pinfo, tree, di, drep,
            dissect_OneInterfData, NDR_POINTER_UNIQUE, "InterfacePtr", -1);
    return offset;
}

/*
 * This MUST be an array of MInterfacePointer pointers containing the OBJREFs for
 * the interfaces returned by the server.
 */
static int
dissect_dcom_InterfData(tvbuff_t *tvb, int offset,
        packet_info *pinfo, proto_tree *tree, dcerpc_info *di, uint8_t *drep)
{
    offset = dissect_ndr_ucarray(tvb, offset, pinfo, tree, di, drep,
            dissect_dcom_OneInterfDataPtr);
    offset = dissect_deferred_pointers(pinfo, tvb, offset, di, drep);
    return offset;
}

static int
dissect_dcom_PropsOutInfo(tvbuff_t *tvb, int offset, packet_info *pinfo,
                       proto_tree *tree, dcerpc_info *di, uint8_t *drep, int size)
{
    proto_tree *sub_tree;
    int old_offset, len;

    old_offset = offset;

    if (size <= 0) {
        /* TODO: expert info */
        size = -1;
    }

    sub_tree = proto_tree_add_subtree(tree, tvb, offset, size, ett_dcom_propsoutput, NULL, "PropertiesOutput");

    offset = dissect_TypeSzCommPrivHdr(tvb, offset, pinfo, sub_tree, di, drep);

    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_pi_ifnum, NULL);

    offset = dissect_ndr_embedded_pointer(tvb, offset, pinfo, sub_tree, di, drep,
            dissect_dcom_IfIds, NDR_POINTER_UNIQUE, "InterfaceIdsPtr", -1);
    offset = dissect_ndr_embedded_pointer(tvb, offset, pinfo, sub_tree, di, drep,
            dissect_dcom_ReturnVals, NDR_POINTER_UNIQUE, "ReturnValuesPtr", -1);
    offset = dissect_ndr_embedded_pointer(tvb, offset, pinfo, sub_tree, di, drep,
            dissect_dcom_InterfData, NDR_POINTER_UNIQUE, "InterfacePtrsPtr", -1);
    offset = dissect_deferred_pointers(pinfo, tvb, offset, di, drep);

    len = offset - old_offset;
    if (size < len) {
        /* TODO expert info */
        size = len;
    }
    else if (size > len) {
        proto_tree_add_item(sub_tree, hf_sysact_unused_buffer, tvb, offset, size - len, ENC_NA);
    }

    offset = old_offset + size;

    return offset;
}


/*
 *typedef struct tagDUALSTRINGARRAY {
 *  unsigned short wNumEntries;
 *  unsigned short wSecurityOffset;
 *  [size_is(wNumEntries)] unsigned short aStringArray[];
 *} DUALSTRINGARRAY;
 */
static int
dissect_dcom_OxidBindings(tvbuff_t *tvb, int offset,
                    packet_info *pinfo, proto_tree *tree, dcerpc_info *di, uint8_t *drep)
{
    proto_item *sub_item;
    proto_tree *sub_tree;
    int old_offset;

    if (di->conformant_run) {
        return offset;
    }

    old_offset = offset;
    sub_tree = proto_tree_add_subtree(tree, tvb, offset, 0, ett_dcom_oxidbinding, &sub_item, "OxidBindings");

    offset = dissect_dcom_dcerpc_array_size(tvb, offset, pinfo, sub_tree, di, drep, NULL);
    offset = dissect_dcom_DUALSTRINGARRAY(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_scmri_binding, NULL);

    proto_item_set_len(sub_item, offset - old_offset);
    return offset;
}


static int
dissect_dcom_customREMOTE_REPLY_SCM_INFO(tvbuff_t *tvb, int offset,
        packet_info *pinfo, proto_tree *tree, dcerpc_info *di, uint8_t *drep)
{
    proto_item *sub_item;
    proto_tree *sub_tree;
    int old_offset;

    if (di->conformant_run) {
        return offset;
    }

    sub_tree = proto_tree_add_subtree(tree, tvb, offset, 0, ett_dcom_rmtresp, &sub_item, "RemoteReply");

    old_offset = offset;
    offset = dissect_dcom_ID(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_scmri_oxid, NULL);
    offset = dissect_ndr_embedded_pointer(tvb, offset, pinfo, sub_tree, di, drep,
            dissect_dcom_OxidBindings, NDR_POINTER_UNIQUE, "OxidBindingsPtr", -1);
    offset = dissect_dcom_UUID(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_scmri_rmtunknid, NULL);
    offset = dissect_dcom_DWORD(tvb, offset, pinfo, sub_tree, di, drep,
            hf_sysact_scmri_authhint, NULL);
    offset = dissect_dcom_COMVERSION(tvb, offset, pinfo, sub_tree, di, drep,
                NULL, NULL);
    offset = dissect_deferred_pointers(pinfo, tvb, offset, di, drep);

    proto_item_set_len(sub_item, offset - old_offset);

    return offset;
}


static int
dissect_dcom_ScmReplyInfo(tvbuff_t *tvb, int offset, packet_info *pinfo,
                       proto_tree *tree, dcerpc_info *di, uint8_t *drep, int size)
{
    proto_tree *sub_tree;
    int old_offset, len;

    old_offset = offset;

    if (size <= 0) {
        /* TODO: expert info */
        size = -1;
    }

    sub_tree = proto_tree_add_subtree(tree, tvb, offset, size, ett_dcom_scmrespinfo, NULL, "ScmReplyInfo");

    offset = dissect_TypeSzCommPrivHdr(tvb, offset, pinfo, sub_tree, di, drep);

    /*This MUST be set to NULL and MUST be ignored on receipt*/
    offset = dissect_ndr_embedded_pointer(tvb, offset, pinfo, sub_tree, di, drep,
            NULL, NDR_POINTER_UNIQUE, "Ptr", -1);
    offset = dissect_ndr_embedded_pointer(tvb, offset, pinfo, sub_tree, di, drep,
            dissect_dcom_customREMOTE_REPLY_SCM_INFO, NDR_POINTER_UNIQUE,
            "RemoteRequestPtr", -1);
    offset = dissect_deferred_pointers(pinfo, tvb, offset, di, drep);

    len = offset - old_offset;
    if (size < len) {
        /* TODO expert info */
        size = len;
    }
    else if (size > len) {
        proto_tree_add_item(sub_tree, hf_sysact_unused_buffer, tvb, offset, size - len, ENC_NA);
    }

    offset = old_offset + size;

    return offset;
}

static void
sysact_register_routines(void)
{
    dcom_register_routine(dissect_dcom_ActivationProperties, &iid_ActivationPropertiesIn);
    dcom_register_routine(dissect_dcom_ActivationProperties, &iid_ActivationPropertiesOut);
    dcom_register_routine(dissect_dcom_SpecialSystemProperties, &clsid_SpecialSystemProperties);
    dcom_register_routine(dissect_dcom_InstantiationInfo, &clsid_InstantiationInfo);
    dcom_register_routine(dissect_dcom_ActivationContextInfo, &clsid_ActivationContextInfo);
    dcom_register_routine(dissect_dcom_IContext, &iid_IContext);
    dcom_register_routine(dissect_dcom_SecurtiyInfo, &clsid_SecurityInfo);
    dcom_register_routine(dissect_dcom_LocationInfo, &clsid_ServerLocationInfo);
    dcom_register_routine(dissect_dcom_ScmRqstInfo, &clsid_ScmRequestInfo);
    dcom_register_routine(dissect_dcom_PropsOutInfo, &clsid_PropsOutInfo);
    dcom_register_routine(dissect_dcom_ScmReplyInfo, &clsid_ScmReplyInfo);
    dcom_register_routine(dissect_dcom_InstanceInfo, &clsid_InstanceInfo);
    dcom_register_routine(dissect_dcom_IStorage, &iid_IStorage);

    return;
}

static int
dissect_remsysact_remotecreateinstance_rqst(tvbuff_t *tvb, int offset,
    packet_info *pinfo, proto_tree *tree, dcerpc_info *di, uint8_t *drep)
{

    sysact_register_routines();

    offset = dissect_dcom_this(tvb, offset, pinfo, tree, di, drep);

    /* XXX - what is this? */
    offset = dissect_dcom_nospec_data(tvb, offset, pinfo, tree, drep, 4);
    offset = dissect_dcom_PMInterfacePointer(tvb, offset, pinfo, tree, di, drep,
                        hf_sysact_actproperties, NULL /* XXX */);
    return offset;
}

static int
dissect_remsysact_remotecreateinstance_resp(tvbuff_t *tvb, int offset,
    packet_info *pinfo, proto_tree *tree, dcerpc_info *di, uint8_t *drep)
{
    sysact_register_routines();

    offset = dissect_dcom_that(tvb, offset, pinfo, tree, di, drep);

    offset = dissect_dcom_PMInterfacePointer(tvb, offset, pinfo, tree, di, drep,
                        hf_sysact_actproperties, NULL /* XXX */);

    offset = dissect_dcom_HRESULT(tvb, offset, pinfo, tree, di, drep,
                     NULL /* pu32HResult */);

    return offset;
}

static int
dissect_remsysact_remotegetclassobject_rqst(tvbuff_t *tvb, int offset,
    packet_info *pinfo, proto_tree *tree, dcerpc_info *di, uint8_t *drep)
{
    sysact_register_routines();

    offset = dissect_dcom_this(tvb, offset, pinfo, tree, di, drep);
    offset = dissect_dcom_PMInterfacePointer(tvb, offset, pinfo, tree, di, drep,
                        hf_sysact_actproperties, NULL);

    return offset;
}

static int
dissect_remsysact_remotegetclassobject_resp(tvbuff_t *tvb, int offset,
    packet_info *pinfo, proto_tree *tree, dcerpc_info *di, uint8_t *drep)
{
    sysact_register_routines();

    offset = dissect_dcom_that(tvb, offset, pinfo, tree, di, drep);

    offset = dissect_dcom_PMInterfacePointer(tvb, offset, pinfo, tree, di, drep,
                        hf_sysact_actproperties, NULL);

    offset = dissect_dcom_HRESULT(tvb, offset, pinfo, tree, di, drep,
                     NULL /* pu32HResult */);

    return offset;
}

static const dcerpc_sub_dissector ISystemActivator_dissectors[] = {
    { 0, "QueryInterfaceIRemoteSCMActivator", NULL, NULL },
    { 1, "AddRefIRemoteISCMActivator", NULL, NULL },
    { 2, "ReleaseIRemoteISCMActivator", NULL, NULL },
    { 3, "RemoteGetClassObject", dissect_remsysact_remotegetclassobject_rqst, dissect_remsysact_remotegetclassobject_resp },
    { 4, "RemoteCreateInstance", dissect_remsysact_remotecreateinstance_rqst, dissect_remsysact_remotecreateinstance_resp },
    { 0, NULL, NULL, NULL },
};

void
proto_register_ISystemActivator (void)
{
    /* fields */
    static hf_register_info hf[] = {
        { &hf_opnum,
          { "Operation", "isystemactivator.opnum", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_actproperties,
        { "IActProperties", "isystemactivator.actproperties", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }},
#if 0
        { &hf_sysact_unknown,
        { "IUnknown", "isystemactivator.unknown", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }},
#endif
    };

    static hf_register_info hf_actproperties[] = {
        { &hf_sysact_totalsize,
        { "Totalsize", "isystemactivator.actproperties.size", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_res,
        { "Reserved", "isystemactivator.actproperties.resv", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},

        { &hf_sysact_customhdrsize,
        { "CustomHeaderSize", "isystemactivator.customhdr.size", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_dstctx,
        { "DestinationContext", "isystemactivator.customhdr.dc", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_actpropnumber,
        { "NumActivationPropertyStructs", "isystemactivator.customhdr.actpropnumber", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_actpropclsinfoid,
        { "ClassInfoClsid", "isystemactivator.customhdr.clsinfoid", FT_GUID, BASE_NONE, NULL, 0x0, NULL, HFILL }},
#if 0
        { &hf_sysact_actpropclsids,
        { "PropertyGuids", "isystemactivator.customhdr.clsids", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }},
#endif
        { &hf_sysact_actpropclsid,
        { "PropertyStructGuid", "isystemactivator.customhdr.clsid", FT_GUID, BASE_NONE, NULL, 0x0, NULL, HFILL }},
#if 0
        { &hf_sysact_actpropsizes,
        { "PropertyDataSizes", "isystemactivator.customhdr.datasizes", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }},
#endif
        { &hf_sysact_actpropsize,
        { "PropertyDataSize", "isystemactivator.customhdr.datasize", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},

        /*SpecialSystemProperties*/
        { &hf_sysact_spsysprop_sid,
        { "SessionID", "isystemactivator.properties.spcl.sid", FT_UINT32, BASE_DEC_HEX, NULL, 0x0, "A value that uniquely identifies a logon session on the server", HFILL }},
        { &hf_sysact_spsysprop_remotethissid,
        { "RemoteThisSessionID", "isystemactivator.properties.spcl.remotesid", FT_UINT32, BASE_DEC_HEX, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_spsysprop_cltimpersonating,
        { "ClientImpersonating", "isystemactivator.properties.spcl.cltimp", FT_UINT32, BASE_DEC_HEX, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_spsysprop_partitionid,
        { "PartitionIDPresent", "isystemactivator.properties.spcl.partitionid", FT_UINT32, BASE_DEC_HEX, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_spsysprop_defauthlvl,
        { "DefaultAuthnLevel", "isystemactivator.properties.spcl.defauthlvl", FT_UINT32, BASE_DEC_HEX, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_spsysprop_partition,
        { "PartitionGuid", "isystemactivator.properties.spcl.partition", FT_GUID, BASE_NONE, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_spsysprop_procrqstflgs,
        { "ProcessRequestFlags", "isystemactivator.properties.spcl.procreqstflgs", FT_UINT32, BASE_DEC_HEX, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_spsysprop_origclsctx,
        { "OriginalClassContext", "isystemactivator.properties.spcl.origclsctx", FT_UINT32, BASE_DEC_HEX, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_spsysprop_flags,
        { "Flags", "isystemactivator.properties.spcl.flags", FT_UINT32, BASE_DEC_HEX, NULL, 0x0, NULL, HFILL }},
#if 0
        { &hf_sysact_spsysprop_procid,
        { "ProcessID", "isystemactivator.properties.spcl.procid", FT_UINT32, BASE_DEC_HEX, NULL, 0x0, NULL, HFILL }},
#endif
#if 0
        { &hf_sysact_spsysprop_hwnd,
        { "hWnd", "isystemactivator.properties.spcl.hwnd", FT_UINT64, BASE_DEC_HEX, NULL, 0x0, NULL, HFILL }},
#endif

        /*InstantiationInfo*/
        { &hf_sysact_instninfo_clsid,
        { "InstantiatedObjectClsId", "isystemactivator.properties.instninfo.clsid", FT_GUID, BASE_NONE, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_instninfo_clsctx,
        { "ClassContext", "isystemactivator.properties.instninfo.clsctx", FT_UINT32, BASE_DEC_HEX, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_instninfo_actflags,
        { "ActivationFlags", "isystemactivator.properties.instninfo.actflags", FT_UINT32, BASE_DEC_HEX, VALS(instninfo_actflags), 0x0, NULL, HFILL }},
        { &hf_sysact_instninfo_issurrogate,
        { "FlagsSurrogate", "isystemactivator.properties.instninfo.issurogate", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_instninfo_iidcount,
        { "InterfaceIdCount", "isystemactivator.properties.instninfo.iidcount", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_instninfo_instflags,
        { "InstantiationFlag", "isystemactivator.properties.instninfo.instflags", FT_UINT32, BASE_DEC_HEX, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_instninfo_entiresize,
        { "EntirePropertySize", "isystemactivator.properties.instninfo.entiresize", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_instninfo_iid,
        { "InterfaceIds", "isystemactivator.properties.instninfo.iid", FT_GUID, BASE_NONE, NULL, 0x0, NULL, HFILL }},

        /*ActivationContextInfo*/
        { &hf_sysact_actctxinfo_cltok,
        { "ClientOk", "isystemactivator.properties.actctxinfo.cltok", FT_INT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_context,
        { "ClientContext", "isystemactivator.properties.context", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }},

        /*InstanceInfo*/
        { &hf_sysact_instinfo_mode,
        { "Mode", "isystemactivator.properties.instinfo.mode", FT_INT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_instinfo_stg,
        { "IfdStg", "isystemactivator.properties.instinfo.ifdstg", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }},

        /*IStorage*/
        { &hf_sysact_istg,
        { "IStorage", "isystemactivator.properties.istg", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }},

        /*dcom Context*/
        { &hf_sysact_ctx_id,
        { "ContextID", "isystemactivator.properties.context.id", FT_GUID, BASE_NONE, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_ctx_flags,
        { "Flags", "isystemactivator.properties.context.flags", FT_UINT32, BASE_HEX, VALS(dcom_context_flag_vals), 0x0, NULL, HFILL }},
        { &hf_sysact_ctx_res,
        { "Reserved", "isystemactivator.properties.context.res", FT_UINT32, BASE_HEX, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_ctx_numextents,
        { "NumExtents", "isystemactivator.properties.context.numext", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_ctx_extentscnt,
        { "ExtentCount", "isystemactivator.properties.context.extcnt", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_ctx_mashflags,
        { "MarshalFlags", "isystemactivator.properties.context.mashflags", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_ctx_count,
        { "ContextPropertyCount", "isystemactivator.properties.context.cnt", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_ctx_frozen,
        { "Frozen", "isystemactivator.properties.context.frz", FT_UINT32, BASE_HEX, VALS(boolean_flag_vals), 0x0, NULL, HFILL }},

        /*Security Info*/
        { &hf_sysact_si_authflalgs,
        { "AuthenticationFlags", "isystemactivator.properties.si.authflags", FT_UINT32, BASE_HEX, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_si_serverinfo,
        { "ServerInfo", "isystemactivator.properties.si.ci", FT_NONE, BASE_NONE, NULL, 0, NULL, HFILL }},
        { &hf_sysact_si_ci_res,
        { "Reserved", "isystemactivator.properties.si.ci.res", FT_UINT32, BASE_HEX, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_si_ci_string,
        { "String", "isystemactivator.properties.si.ci.name", FT_STRING, BASE_NONE, NULL, 0, NULL, HFILL }},

        /*Location info*/
        { &hf_sysact_li_string,
        { "String", "isystemactivator.properties.li.name", FT_STRING, BASE_NONE, NULL, 0, NULL, HFILL }},
        { &hf_sysact_li_procid,
        { "ProcessId", "isystemactivator.properties.li.procid", FT_UINT32, BASE_DEC_HEX, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_li_apartid,
        { "ApartmentId", "isystemactivator.properties.li.apartid", FT_UINT32, BASE_DEC_HEX, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_li_ctxid,
        { "ContextId", "isystemactivator.properties.li.ctxid", FT_UINT32, BASE_DEC_HEX, NULL, 0x0, NULL, HFILL }},

        /*ScmRequst info*/
        { &hf_sysact_sri_cltimplvl,
        { "ClientImpersonationLevel", "isystemactivator.properties.sri.cltimplvl", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_sri_protseqnum,
        { "NumProtocolSequences", "isystemactivator.properties.sri.protseqnum", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_sri_protseq,
        { "ProtocolSeq", "isystemactivator.properties.sri.protseq", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }},

        /*PropsOutInfo*/
        { &hf_sysact_pi_ifnum,
        { "NumInterfaces", "isystemactivator.properties.pi.ifnum", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_pi_retval,
        { "ReturnValue", "isystemactivator.properties.retval", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_pi_interf,
        { "Interface", "isystemactivator.properties.interf", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_pi_iid,
        { "IID", "isystemactivator.properties.iid", FT_GUID, BASE_NONE, NULL, 0x0, NULL, HFILL }},

        /*ScmReply info*/
        { &hf_sysact_scmri_rmtunknid,
        { "IRemUnknownInterfacePointerId", "isystemactivator.properties.scmresp.rmtunknid", FT_GUID, BASE_NONE, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_scmri_authhint,
        { "AuthenticationHint", "isystemactivator.properties.scmresp.authhint", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_scmri_binding,
        { "Bindings", "isystemactivator.properties.scmresp.binding", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_scmri_oxid,
        { "OXID", "isystemactivator.properties.scmresp.oxid", FT_UINT64, BASE_HEX, NULL, 0x0, NULL, HFILL }},
        { &hf_sysact_unused_buffer,
        { "Unused buffer", "isystemactivator.unused_buffer", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL }},
    };

    static hf_register_info hf_tshdr[] = {
        { &hf_typeszch,
        { "CommonHeader", "isystemactivator.actproperties.ts.hdr", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }},
        { &hf_typeszph,
        { "PrivateHeader", "isystemactivator.actproperties.ts.hdr", FT_NONE, BASE_NONE, NULL, 0x0, NULL, HFILL }},
        { &hf_typesz_ver,
        { "Version", "isystemactivator.actproperties.ts.ver", FT_UINT8, BASE_HEX, NULL, 0x0, NULL, HFILL }},
        { &hf_typesz_endianness,
        { "Endianness", "isystemactivator.actproperties.ts.end", FT_UINT8, BASE_HEX, VALS(ts_endian_vals), 0x0, NULL, HFILL }},
        { &hf_typesz_commhdrlen,
        { "CommonHeaderLength", "isystemactivator.actproperties.ts.chl", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL }},
        { &hf_typesz_filler,
        { "Filler", "isystemactivator.actproperties.ts.fil", FT_UINT32, BASE_HEX, NULL, 0x0, NULL, HFILL }},
        { &hf_typesz_buflen,
        { "ObjectBufferLength", "isystemactivator.actproperties.ts.buflen", FT_UINT32, BASE_DEC, NULL, 0x0, NULL, HFILL }},
    };


    /* Tree */
    static int *ett[] = {
        &ett_isystemactivator,
        &ett_actproperties,
        &ett_properties,
        &ett_commonheader,
        &ett_propguids,
        &ett_typeszcommhdr,
        &ett_typeszprivhdr,
        &ett_dcom_spclsysprop,
        &ett_dcom_reserved,
        &ett_dcom_instantianinfo,
        &ett_dcom_actctxinfo,
        &ett_dcom_instanceinfo,
        &ett_dcom_istorage,
        &ett_dcom_context,
        &ett_dcom_securityinfo,
        &ett_dcom_locationinfo,
        &ett_dcom_scmrqstinfo,
        &ett_dcom_rmtrqst,

        &ett_dcom_propsoutput,
        &ett_dcom_scmrespinfo,
        &ett_dcom_rmtresp,
        &ett_dcom_oxidbinding,

    };

    proto_ISystemActivator = proto_register_protocol ("ISystemActivator ISystemActivator Resolver", "ISystemActivator", "isystemactivator");
    proto_register_field_array (proto_ISystemActivator, hf, array_length (hf));
    proto_register_field_array (proto_ISystemActivator, hf_actproperties, array_length (hf_actproperties));
    proto_register_field_array(proto_ISystemActivator, hf_tshdr, array_length(hf_tshdr));
    proto_register_subtree_array (ett, array_length (ett));
}

void
proto_reg_handoff_ISystemActivator (void)
{
    /* Register the protocol as dcerpc */
    dcerpc_init_uuid (proto_ISystemActivator, ett_isystemactivator, &uuid_ISystemActivator,
            ver_ISystemActivator, ISystemActivator_dissectors, hf_opnum);
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
