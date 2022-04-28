// SPDX-License-Identifier: GPL-2.0
/*
 * GXP wakelock support
 *
 * Copyright (C) 2022 Google LLC
 */

#include "gxp-dma.h"
#include "gxp-pm.h"
#include "gxp-wakelock.h"

int gxp_wakelock_init(struct gxp_dev *gxp)
{
	struct gxp_wakelock_manager *mgr;

	mgr = devm_kzalloc(gxp->dev, sizeof(*mgr), GFP_KERNEL);
	if (!mgr)
		return -ENOMEM;

	mutex_init(&mgr->lock);

	gxp->wakelock_mgr = mgr;

	return 0;
}

int gxp_wakelock_acquire(struct gxp_dev *gxp)
{
	struct gxp_wakelock_manager *mgr = gxp->wakelock_mgr;
	int ret = 0;

	mutex_lock(&mgr->lock);

	if (mgr->suspended) {
		/*
		 * Don't allow a new client to obtain a wakelock, powering up
		 * BLK_AUR, when the system is going to sleep.
		 */
		dev_warn(gxp->dev,
			 "Attempt to obtain wakelock while suspending.\n");
		ret = -EAGAIN;
		goto out;
	}

	if (!mgr->count++) {
		ret = gxp_pm_blk_on(gxp);
		if (ret) {
			dev_err(gxp->dev,
				"Failed to power on BLK_AUR (ret=%d, client count=%u)\n",
				ret, mgr->count);
			goto err_blk_on;
		}
	}

out:
	mutex_unlock(&mgr->lock);

	return ret;

err_blk_on:
	mgr->count--;
	mutex_unlock(&mgr->lock);
	return ret;
}

void gxp_wakelock_release(struct gxp_dev *gxp)
{
	struct gxp_wakelock_manager *mgr = gxp->wakelock_mgr;
	int ret = 0;

	mutex_lock(&mgr->lock);

	if (!mgr->count) {
		dev_err(gxp->dev,
			"Attempt to release wakelock with none held.\n");
		goto out;
	}

	if (!--mgr->count) {
		ret = gxp_pm_blk_off(gxp);
		if (ret)
			dev_err(gxp->dev,
				"Failed to power down BLK_AUR (ret=%d, client count=%u)\n",
				ret, mgr->count);
	}

out:
	mutex_unlock(&mgr->lock);
}

int gxp_wakelock_suspend(struct gxp_dev *gxp)
{
	struct gxp_wakelock_manager *mgr = gxp->wakelock_mgr;
	int ret;

	mutex_lock(&mgr->lock);

	/* Can't suspend if there are any active clients */
	mgr->suspended = mgr->count == 0;
	ret = mgr->suspended ? 0 : -EAGAIN;

	mutex_unlock(&mgr->lock);

	return ret;
}

int gxp_wakelock_resume(struct gxp_dev *gxp)
{
	struct gxp_wakelock_manager *mgr = gxp->wakelock_mgr;

	mutex_lock(&mgr->lock);

	mgr->suspended = false;

	mutex_unlock(&mgr->lock);

	return 0;
}
