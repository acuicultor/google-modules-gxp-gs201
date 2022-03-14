// SPDX-License-Identifier: GPL-2.0
/*
 * GXP client structure.
 *
 * Copyright (C) 2021 Google LLC
 */

#include <linux/slab.h>
#include <linux/types.h>

#include "gxp-client.h"
#include "gxp-dma.h"
#include "gxp-internal.h"
#include "gxp-pm.h"
#include "gxp-vd.h"
#include "gxp-wakelock.h"

struct gxp_client *gxp_client_create(struct gxp_dev *gxp)
{
	struct gxp_client *client;

	client = kmalloc(sizeof(*client), GFP_KERNEL);
	if (!client)
		return ERR_PTR(-ENOMEM);

	client->gxp = gxp;
	init_rwsem(&client->semaphore);
	client->has_block_wakelock = false;
	client->requested_power_state = AUR_OFF;
	client->requested_memory_power_state = 0;
	client->vd = NULL;
	client->tpu_mbx_allocated = false;
	return client;
}

void gxp_client_destroy(struct gxp_client *client)
{
	struct gxp_dev *gxp = client->gxp;

	down_write(&gxp->vd_semaphore);

#if IS_ENABLED(CONFIG_ANDROID) && !IS_ENABLED(CONFIG_GXP_GEM5)
	/*
	 * Unmap TPU buffers, if the mapping is already removed, this
	 * is a no-op.
	 */
	gxp_dma_unmap_tpu_buffer(gxp, client->mbx_desc);
#endif  // CONFIG_ANDROID && !CONFIG_GXP_GEM5

	if (client->has_vd_wakelock)
		gxp_vd_stop(client->vd);

	up_write(&gxp->vd_semaphore);

	if (client->has_block_wakelock) {
		gxp_wakelock_release(client->gxp);
		gxp_pm_update_requested_power_state(
			gxp, client->requested_power_state, AUR_OFF);
		gxp_pm_update_requested_memory_power_state(
			gxp, client->requested_memory_power_state,
			AUR_MEM_UNDEFINED);
	}

	gxp_vd_release(client->vd);

	kfree(client);
}