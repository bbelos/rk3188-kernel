////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Newport Media Inc.  All rights reserved.
//
// Module Name:  nmi_wlan_cfg.h
//
//
//////////////////////////////////////////////////////////////////////////////

#ifndef NMI_WLAN_CFG_H
#define NMI_WLAN_CFG_H

typedef struct {
	uint16_t id;
	uint16_t val;
} nmi_cfg_byte_t;

typedef struct {
	uint16_t id;
	uint16_t val;
} nmi_cfg_hword_t;

typedef struct {
	uint32_t id;
	uint32_t val;
} nmi_cfg_word_t;

typedef struct {
	uint32_t id;
	uint8_t *str;
} nmi_cfg_str_t;

#endif
