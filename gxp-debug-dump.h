/* SPDX-License-Identifier: GPL-2.0 */
/*
 * GXP debug dump handler
 *
 * Copyright (C) 2020 Google LLC
 */
#ifndef __GXP_DEBUG_DUMP_H__
#define __GXP_DEBUG_DUMP_H__

#include <linux/bitops.h>
#include <linux/types.h>
#include <linux/workqueue.h>

#include "gxp-internal.h"

#define GXP_NUM_COMMON_SEGMENTS 2
#define GXP_NUM_CORE_SEGMENTS 7
#define GXP_SEG_HEADER_NAME_LENGTH 32

#define GXP_Q7_ICACHE_SIZE 131072 /* I-cache size in bytes */
#define GXP_Q7_ICACHE_LINESIZE 64 /* I-cache line size in bytes */
#define GXP_Q7_ICACHE_WAYS 4
#define GXP_Q7_ICACHE_SETS ((GXP_Q7_ICACHE_SIZE / GXP_Q7_ICACHE_WAYS) / \
			    GXP_Q7_ICACHE_LINESIZE)
#define GXP_Q7_ICACHE_WORDS_PER_LINE (GXP_Q7_ICACHE_LINESIZE / sizeof(u32))

#define GXP_Q7_DCACHE_SIZE 65536 /* D-cache size in bytes */
#define GXP_Q7_DCACHE_LINESIZE 64  /* D-cache line size in bytes */
#define GXP_Q7_DCACHE_WAYS 4
#define GXP_Q7_DCACHE_SETS ((GXP_Q7_DCACHE_SIZE / GXP_Q7_DCACHE_WAYS) / \
			    GXP_Q7_DCACHE_LINESIZE)
#define GXP_Q7_DCACHE_WORDS_PER_LINE (GXP_Q7_DCACHE_LINESIZE / sizeof(u32))
#define GXP_Q7_NUM_AREGS 64
#define GXP_Q7_DCACHE_TAG_RAMS 2

#define GXP_DEBUG_DUMP_INT 0x1
#define GXP_DEBUG_DUMP_INT_MASK BIT(GXP_DEBUG_DUMP_INT)
#define GXP_DEBUG_DUMP_RETRY_NUM 5

struct gxp_timer_registers {
	u32 comparator;
	u32 control;
	u32 value;
};

struct gxp_lpm_transition_registers {
	u32 next_state;
	u32 seq_addr;
	u32 timer_val;
	u32 timer_en;
	u32 trigger_num;
	u32 trigger_en;
};

struct gxp_lpm_state_table_registers {
	struct gxp_lpm_transition_registers trans[PSM_TRANS_COUNT];
	u32 enable_state;
};

struct gxp_lpm_psm_registers {
	struct gxp_lpm_state_table_registers state_table[PSM_STATE_TABLE_COUNT];
	u32 data[PSM_DATA_COUNT];
	u32 cfg;
	u32 status;
	u32 debug_cfg;
	u32 break_addr;
	u32 gpin_lo_rd;
	u32 gpin_hi_rd;
	u32 gpout_lo_rd;
	u32 gpout_hi_rd;
	u32 debug_status;
};

struct gxp_common_registers {
	u32 aurora_revision;
	u32 common_int_pol_0;
	u32 common_int_pol_1;
	u32 dedicated_int_pol;
	u32 raw_ext_int;
	u32 core_pd[CORE_PD_COUNT];
	u32 global_counter_low;
	u32 global_counter_high;
	u32 wdog_control;
	u32 wdog_value;
	struct gxp_timer_registers timer[TIMER_COUNT];
	u32 doorbell[DOORBELL_COUNT];
	u32 sync_barrier[SYNC_BARRIER_COUNT];
};

struct gxp_lpm_registers {
	u32 lpm_version;
	u32 trigger_csr_start;
	u32 imem_start;
	u32 lpm_config;
	u32 psm_descriptor[PSM_DESCRIPTOR_COUNT];
	u32 events_en[EVENTS_EN_COUNT];
	u32 events_inv[EVENTS_INV_COUNT];
	u32 function_select;
	u32 trigger_status;
	u32 event_status;
	u32 ops[OPS_COUNT];
	struct gxp_lpm_psm_registers psm_regs[PSM_COUNT];
};

struct gxp_core_header {
	u32 core_id; /* Aurora core ID */
	u32 dump_available; /* Dump data is available for core*/
	u32 dump_req_reason; /* Code indicating reason for debug dump request */
	u32 crash_reason; /* Error code identifying crash reason */
	u32 fw_version; /* Firmware version */
	u32 core_dump_size; /* Size of core dump */
};

struct gxp_seg_header {
	char name[GXP_SEG_HEADER_NAME_LENGTH]; /* Name of data type */
	u32 size; /* Size of segment data */
	u32 valid; /* Validity of segment data */
};

struct gxp_core_dump_header {
	struct gxp_core_header core_header;
	struct gxp_seg_header seg_header[GXP_NUM_CORE_SEGMENTS];
};

struct gxp_common_dump_data {
	struct gxp_common_registers common_regs; /* Seg 0 */
	struct gxp_lpm_registers lpm_regs; /* Seg 1 */
};

struct gxp_common_dump {
	struct gxp_seg_header seg_header[GXP_NUM_COMMON_SEGMENTS];
	struct gxp_common_dump_data common_dump_data;
};

struct gxp_core_dump {
	struct gxp_core_dump_header core_dump_header[GXP_NUM_CORES];
	/*
	 * A collection of 'GXP_NUM_CORES' core dumps;
	 * Each is core_dump_header[i].core_dump_size bytes long.
	 */
	uint32_t dump_data[];
};

struct gxp_debug_dump_manager {
	struct gxp_dev *gxp;
	struct gxp_core_dump *core_dump; /* start of the core dump */
	void *sscd_dev;
	void *sscd_pdata;
	dma_addr_t debug_dump_dma_handle; /* dma handle for debug dump */
	/* Lock protects kernel_init_dump_pending and kernel_init_dump_waitq */
	struct mutex lock;
	/* Keep track of which cores have kernel-initiated core dump ready */
	int kernel_init_dump_pending;
	wait_queue_head_t kernel_init_dump_waitq;
	struct work_struct wait_kernel_init_dump_work;
	/* SSCD lock to ensure SSCD is only processing one report at a time */
	struct mutex sscd_lock;
};

int gxp_debug_dump_init(struct gxp_dev *gxp, void *sscd_dev, void *sscd_pdata);
void gxp_debug_dump_exit(struct gxp_dev *gxp);
void gxp_debug_dump_process_dump(struct work_struct *work);

#endif /* __GXP_DEBUG_DUMP_H__ */