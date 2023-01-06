/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Include all configuration files for Amalthea.
 *
 * Copyright (C) 2022 Google LLC
 */

#ifndef __AMALTHEA_CONFIG_H__
#define __AMALTHEA_CONFIG_H__

#define GXP_DRIVER_NAME "gxp_platform"
#define DSP_FIRMWARE_DEFAULT_PREFIX "gxp_fw_core"

#define AUR_DVFS_DOMAIN 17

#define GXP_NUM_CORES 4
#define GXP_NUM_MAILBOXES GXP_NUM_CORES
#define GXP_NUM_WAKEUP_DOORBELLS GXP_NUM_CORES

#define GXP_USE_LEGACY_MAILBOX 1

#define GXP_HAS_MCU 0

#include "config-pwr-state.h"
#include "context.h"
#include "csrs.h"
#include "iova.h"
#include "lpm.h"
#include "mailbox-regs.h"

#endif /* __AMALTHEA_CONFIG_H__ */
