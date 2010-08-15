/* drivers/video/msm_fb/mddi_client_eid.c
 *
 * Support for eid mddi client devices with Samsung S6D05A0
 *
 * Copyright (C) 2007 HTC Incorporated
 * Author: Jay Tu (jay_tu@htc.com)
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/leds.h>
#include <linux/kthread.h>
#include <linux/wakelock.h>
#include <linux/clk.h>
#include <asm/atomic.h>
#include <asm/mach-types.h>
#include <mach/msm_fb.h>
#include <mach/msm_iomap.h>
#include <mach/msm_panel.h>
#include "mddi_client_eid.h"

#if 0
#define B(s...) printk(s)
#else
#define B(s...) do {} while (0)
#endif

#define DEBUG_EID_CMD 0

enum {
	PANEL_SHARP,
	PANEL_SAMSUNG,
	PANEL_EID_40pin,
	PANEL_EID_24pin,
	PANEL_HEROC_EID_BOTTOM,
	PANEL_TPO,
	PANEL_HEROC_TPO,
	PANEL_ESPRESSO_TPO,
	PANEL_ESPRESSO_SHARP,
	PANEL_LIBERTY_TPO,
	PANEL_LIBERTY_EID_24pin,
	PANEL_EIDII,
	PANEL_UNKNOWN,
};

#define DEBUG_VSYNC_INT 0

#define VSYNC_EN (MSM_GPIO1_BASE + 0x800 + 0x8c)
#define VSYNC_CLEAR (MSM_GPIO1_BASE + 0x800 + 0x9c)
#define VSYNC_STATUS (MSM_GPIO1_BASE + 0x800 + 0xac)

static DECLARE_WAIT_QUEUE_HEAD(samsung_vsync_wait);

struct panel_info {
	struct msm_mddi_client_data *client_data;
	struct platform_device pdev;
	struct msm_panel_data panel_data;
	struct msmfb_callback *fb_callback;
	struct wake_lock idle_lock;
	int samsung_got_int;
	atomic_t depth;
};

static struct cabc_config cabc_config;
static struct platform_device mddi_samsung_cabc = {
	.name = "samsung_cabc",
	.id = 0,
	.dev = {
		.platform_data = &cabc_config,
	}
};

static struct clk *ebi1_clk;
static void samsung_dump_vsync(void)
{
	unsigned long rate = clk_get_rate(ebi1_clk);

	printk(KERN_INFO "STATUS %d %s EBI1 %lu\n",
		readl(VSYNC_STATUS) & 0x04,
		readl(VSYNC_EN) & 0x04 ? "ENABLED" : "DISABLED",
		rate);
}

static inline void samsung_clear_vsync(void)
{
	unsigned val;
	int retry = 1000;

	while (retry--) {
		writel((1U << (97 - 95)), VSYNC_CLEAR);
		wmb();
		val = readl(VSYNC_STATUS);
		if (!!(val & 0x04) == 0)
			break;
	}

	if (retry == 0)
		printk(KERN_ERR "%s: clear vsync failed!\n", __func__);
}

static void
samsung_request_vsync(struct msm_panel_data *panel_data,
		      struct msmfb_callback *callback)
{
	struct panel_info *panel = container_of(panel_data, struct panel_info,
						panel_data);
	struct msm_mddi_client_data *client_data = panel->client_data;

	panel->fb_callback = callback;
	if (panel->samsung_got_int) {
		panel->samsung_got_int = 0;
		client_data->activate_link(client_data); /* clears interrupt */
	}
	
	if (atomic_read(&panel->depth) <= 0) {
		atomic_inc(&panel->depth);
		samsung_clear_vsync();
		enable_irq(gpio_to_irq(97));
	}
}

static void samsung_wait_vsync(struct msm_panel_data *panel_data)
{
	struct panel_info *panel = container_of(panel_data, struct panel_info,
						panel_data);
	struct msm_mddi_client_data *client_data = panel->client_data;

	if (panel->samsung_got_int) {
		panel->samsung_got_int = 0;
		client_data->activate_link(client_data); /* clears interrupt */
	}

	if (wait_event_timeout(samsung_vsync_wait, panel->samsung_got_int,
			       HZ / 2) == 0)
		printk(KERN_ERR "timeout waiting for VSYNC\n");

	panel->samsung_got_int = 0;
	/* interrupt clears when screen dma starts */
}

/* --------------------------------------------------------------------------- */

static void
eid_pwrctl(struct msm_mddi_client_data *client_data, int panel_type,
	   uint8_t prm2, uint8_t prm5)
{
#if DEBUG_EID_CMD
	int i;
#endif
	uint8_t prm[12];

	memset(prm, 0, 12);
	prm[1] = prm2;
	prm[4] = prm5;
	prm[2] = 0x2a;
	if (panel_type == PANEL_EID_40pin) {
		prm[5] = 0x33;
		prm[6] = 0x38;
		prm[7] = 0x38;
	} else if ((panel_type == PANEL_TPO) ||
		   (panel_type == PANEL_HEROC_TPO) ||
                   (panel_type == PANEL_ESPRESSO_TPO)) {
		prm[5] = 0x33;
		prm[6] = 0x75;
		prm[7] = 0x75;
	} else if (panel_type == PANEL_LIBERTY_EID_24pin) {
		prm[5] = 0x33;
		prm[6] = 0x3C;
		prm[7] = 0x3C;
	} else if (panel_type == PANEL_LIBERTY_TPO) {
		prm[5] = 0x33;
		prm[6] = 0x6F;
		prm[7] = 0x6F;
	} else {
		prm[5] = 0x33;
		prm[6] = 0x29;
		prm[7] = 0x29;
	}

	client_data->remote_write_vals(client_data, prm, PWRCTL, 12);
#if DEBUG_EID_CMD
	printk(KERN_DEBUG "0x%x ", PWRCTL);
	for (i = 0; i < 12; i++)
		printk("0x%x ", prm[i]);
	printk("\n");
#endif
}

static void
eid_cmd(struct msm_mddi_client_data *client_data, unsigned cmd,
	uint8_t *prm, int size, uint8_t attrs, ...)
{
	int i;
	uint8_t tmp;
	va_list attr_list;

	if (size <= 0)
		return;

	prm[0] = attrs;

	va_start(attr_list, attrs);

	for (i = 1; i < size; i++) {
		tmp = (uint8_t) va_arg(attr_list, int);
		prm[i] = tmp;
	}

	va_end(attr_list);
#if DEBUG_EID_CMD
	printk(KERN_DEBUG "0x%x ", cmd);
	for (i = 0; i < size; i++)
		printk("0x%x ", prm[i]);
	printk("\n");
#endif
	client_data->remote_write_vals(client_data, prm, cmd, size);
}

static int
eid_client_init(struct msm_mddi_bridge_platform_data *bridge,
		struct msm_mddi_client_data *client_data)
{
	uint8_t prm[20];
	struct panel_data *panel = &bridge->panel_conf;

#if DEBUG_EID_CMD
	printk(KERN_DEBUG "%s: enter.\n", __func__);
#endif
	client_data->auto_hibernate(client_data, 0);

	if (panel->panel_id == PANEL_ESPRESSO_SHARP) {
		eid_cmd(client_data, DCON, prm, 4, 0x00, 0x00, 0x00, 0x00);
		eid_cmd(client_data, DISCTL, prm, 12, 0x14, 0x14, 0x0f, 0x13, 0x0f,
			0x13, 0x0f, 0x12, 0x02, 0x18, 0x18, 0x00);

		eid_cmd(client_data, 0xf6, prm, 4, 0x80, 0x10, 0x09, 0x00);

		eid_cmd(client_data, GATECTL, prm, 4, 0x33, 0x3b, 0x00, 0x00);

		eid_cmd(client_data, CASET, prm, 4, 0x00, 0x00, 0x01, 0x3f);

		eid_cmd(client_data, PASET, prm, 4, 0x00, 0x00, 0x01, 0xdf);

		eid_cmd(client_data, MADCTL, prm, 4, 0x00, 0x00, 0x00, 0x00);

		eid_cmd(client_data, COLMOD, prm, 4, 0x55, 0x00, 0x00, 0x00);

		eid_cmd(client_data, MDDICTL, prm, 4, 0x00, 0x00, 0x00, 0x00);

		eid_cmd(client_data, 0xe1, prm, 4, 0x00, 0x00, 0x00, 0x00);

		eid_cmd(client_data, TEON, prm, 4, 0x00, 0x00, 0x00, 0x00);

		eid_cmd(client_data, GAMCTL1, prm, 16, 0x08, 0x2e, 0x00, 0x2a,
			0x38, 0x3a, 0x35, 0x36, 0x07, 0x05, 0x06, 0x0c, 0x00, 0x14,
			0x14, 0x00);

		eid_cmd(client_data, GAMCTL2, prm, 16, 0x17, 0x13, 0x00, 0x0b,
			0x13, 0x1e, 0x23, 0x25, 0x18, 0x1f, 0x2a, 0x28, 0x00, 0x14,
			0x14, 0x00);

		eid_cmd(client_data, PWRCTL, prm, 12, 0x01, 0x00, 0x26, 0x26, 0x08,
			0x33, 0x6d, 0x6d, 0x00, 0x00, 0x00, 0x00);

		eid_cmd(client_data, VCMCTL, prm, 8, 0x4d, 0x4d, 0x5e, 0x5e, 0x77,
			0x00, 0x00, 0x00);

		eid_cmd(client_data, SRCCTL, prm, 8, 0x03, 0x11, 0x06, 0x00, 0x00,
			0x1f, 0x00, 0x00);

		eid_cmd(client_data, SLPOUT, prm, 4, 0x00, 0x00, 0x00, 0x00);
		hr_msleep(160);

		eid_cmd(client_data, 0x29, prm, 4, 0x00, 0x00, 0x00, 0x00);

		client_data->auto_hibernate(client_data, 1);
		return 0;
	}

	eid_pwrctl(client_data, panel->panel_id, 0x00, 0x00);
	if (panel->panel_id == PANEL_ESPRESSO_TPO)
		hr_msleep(15);	//more than 10ms
	eid_cmd(client_data, SLPOUT, prm, 4, 0x00, 0x00, 0x00, 0x00);
	hr_msleep(20);
	if (panel->panel_id == PANEL_ESPRESSO_TPO)
		eid_cmd(client_data, DISCTL, prm, 12, 0x16, 0x16, 0x0f, 0x11,
			0x11, 0x11, 0x11, 0x10, 0x02, 0x16, 0x16, 0x00);
	else if (panel->panel_id == PANEL_LIBERTY_EID_24pin)
		eid_cmd(client_data, DISCTL, prm, 12, 0x16, 0x16, 0x0f, 0x1b,
			0x07, 0x1b, 0x07, 0x10, 0x00, 0x16, 0x16, 0x00);
	else if (panel->panel_id == PANEL_LIBERTY_TPO)
		eid_cmd(client_data, DISCTL, prm, 12, 0x16, 0x16, 0x0f, 0x1b,
			0x07, 0x11, 0x11, 0x10, 0x00, 0x16, 0x16, 0x00);
	else
		eid_cmd(client_data, DISCTL, prm, 12, 0x16, 0x16, 0x0f, 0x11,
			0x11, 0x11, 0x11, 0x10, 0x00, 0x16, 0x16, 0x00);
	if (panel->panel_id == PANEL_ESPRESSO_TPO)
		hr_msleep(15);	//more than 10ms
	eid_pwrctl(client_data, panel->panel_id, 0x01, 0x00);

	if (panel->panel_id == PANEL_EID_40pin)
		eid_cmd(client_data, VCMCTL, prm, 8, 0x2a, 0x2a, 0x19, 0x19,
			0x00, 0x00, 0x00, 0x00);
	else if ((panel->panel_id == PANEL_TPO) ||
		(panel->panel_id == PANEL_HEROC_TPO))
		eid_cmd(client_data, VCMCTL, prm, 8, 0x60, 0x60, 0x7f, 0x7f,
			0x77, 0x00, 0x00, 0x00);
	else if (panel->panel_id == PANEL_ESPRESSO_TPO)
		eid_cmd(client_data, VCMCTL, prm, 8, 0x50, 0x50, 0x7f, 0x7f,
			0x77, 0x00, 0x00, 0x00);
	else if (panel->panel_id == PANEL_LIBERTY_EID_24pin)
		eid_cmd(client_data, VCMCTL, prm, 8, 0x1e, 0x1e, 0x26, 0x26,
			0x00, 0x00, 0x00, 0x00);
	else if (panel->panel_id == PANEL_LIBERTY_TPO)
		eid_cmd(client_data, VCMCTL, prm, 8, 0x5d, 0x5d, 0x7f, 0x7f,
			0x77, 0x00, 0x00, 0x00);
	else
		eid_cmd(client_data, VCMCTL, prm, 8, 0x1b, 0x1b, 0x18, 0x18,
			0x00, 0x00, 0x00, 0x00);

	if ((panel->panel_id == PANEL_TPO) ||
	    (panel->panel_id == PANEL_HEROC_TPO))
		eid_cmd(client_data, SRCCTL, prm, 8, 0x12, 0x00, 0x06, 0x01,
			0x01, 0x1d, 0x00, 0x00);
	else if ((panel->panel_id == PANEL_ESPRESSO_TPO) ||
	        (panel->panel_id == PANEL_LIBERTY_TPO))
		eid_cmd(client_data, SRCCTL, prm, 8, 0x12, 0x00, 0x03, 0x01,
                        0x01, 0x1d, 0x00, 0x00);
	else
		eid_cmd(client_data, SRCCTL, prm, 8, 0x12, 0x00, 0x0a, 0x01,
			0x01, 0x1d, 0x00, 0x00);
	if ((panel->panel_id == PANEL_TPO) ||
	    (panel->panel_id == PANEL_HEROC_TPO) ||
	    (panel->panel_id == PANEL_LIBERTY_TPO) ||
            (panel->panel_id == PANEL_ESPRESSO_TPO))
		eid_cmd(client_data, GATECTL, prm, 4, 0x11, 0x3b, 0x00, 0x00);
	else
		eid_cmd(client_data, GATECTL, prm, 4, 0x44, 0x3b, 0x00, 0x00);
	hr_msleep(20);

	eid_pwrctl(client_data, panel->panel_id, 0x03, 0x00);
	hr_msleep(20);

	eid_pwrctl(client_data, panel->panel_id, 0x07, 0x00);
	hr_msleep(20);

	eid_pwrctl(client_data, panel->panel_id, 0x0f, 0x02);
	hr_msleep(20);

	eid_pwrctl(client_data, panel->panel_id, 0x1f, 0x02);
	hr_msleep(20);

	if (panel->panel_id == PANEL_ESPRESSO_TPO)
		eid_pwrctl(client_data, panel->panel_id, 0x3f, 0x09);
	else
	eid_pwrctl(client_data, panel->panel_id, 0x3f, 0x08);
	hr_msleep(30);

	if (panel->panel_id == PANEL_ESPRESSO_TPO)
		eid_pwrctl(client_data, panel->panel_id, 0x7f, 0x09);
	else
	eid_pwrctl(client_data, panel->panel_id, 0x7f, 0x08);
	hr_msleep(40);

	eid_cmd(client_data, TEON, prm, 4, 0x00, 0x00, 0x00, 0x00);

	if ((panel->panel_id == PANEL_HEROC_EID_BOTTOM) ||
	    (panel->panel_id == PANEL_TPO) ||
	    (panel->panel_id == PANEL_HEROC_TPO))
		eid_cmd(client_data, MADCTL, prm, 4, 0x48, 0x00, 0x00, 0x00);
	else if (panel->panel_id == PANEL_ESPRESSO_TPO)
		eid_cmd(client_data, MADCTL, prm, 4, 0x08, 0x00, 0x00, 0x00);
	else if ((panel->panel_id == PANEL_LIBERTY_EID_24pin) ||
		(panel->panel_id == PANEL_LIBERTY_TPO))
		eid_cmd(client_data, MADCTL, prm, 4, 0x48, 0x00, 0x00, 0x00);
	else
		eid_cmd(client_data, MADCTL, prm, 4, 0x98, 0x00, 0x00, 0x00);
	if ((panel->panel_id == PANEL_HEROC_TPO) ||
            (panel->panel_id == PANEL_HEROC_EID_BOTTOM) ||
            (panel->panel_id == PANEL_ESPRESSO_TPO) ||
            (panel->panel_id == PANEL_LIBERTY_EID_24pin) ||
            (panel->panel_id == PANEL_LIBERTY_TPO))
		eid_cmd(client_data, COLMOD, prm, 4, 0x55, 0x00, 0x00, 0x00);
	else
		eid_cmd(client_data, COLMOD, prm, 4, 0x66, 0x00, 0x00, 0x00);

	if (panel->panel_id == PANEL_EID_40pin) {
		eid_cmd(client_data, GAMCTL1, prm, 16, 0x00, 0x0d, 0x00, 0x05,
			0x1f, 0x2b, 0x2a, 0x2b, 0x12, 0x12, 0x10, 0x16, 0x06,
			0x22, 0x22, 0x00);
		eid_cmd(client_data, GAMCTL2, prm, 16, 0x00, 0x0d, 0x00, 0x05,
			0x1f, 0x2b, 0x2a, 0x2b, 0x12, 0x12, 0x10, 0x16, 0x06,
			0x22, 0x22, 0x00);
	} else if ((panel->panel_id == PANEL_TPO) ||
		(panel->panel_id == PANEL_HEROC_TPO)) {
		eid_cmd(client_data, GAMCTL1, prm, 16, 0x80, 0x0d, 0x00, 0x01,
			0x0b, 0x13, 0x17, 0x1e, 0x26, 0x16, 0x12, 0x27,
			0x0c, 0x22, 0x22, 0x00);
		eid_cmd(client_data, GAMCTL2, prm, 16, 0x8d, 0x00, 0x00, 0x00,
			0x09, 0x0b, 0x11, 0x17, 0x12, 0x0e, 0x12, 0x24,
			0x0b, 0x22, 0x22, 0x00);
		eid_cmd(client_data, 0xf9, prm, 16, 0x9a, 0x0d, 0x00, 0x0c,
			0x14, 0x28, 0x2d, 0x33, 0x0c, 0x08, 0x0f, 0x1d,
			0x0b, 0x22, 0x22, 0x00);
		eid_cmd(client_data, 0xfa, prm, 16, 0x8d, 0x1a, 0x00, 0x08,
			0x0f, 0x21, 0x29, 0x2d, 0x06, 0x05, 0x12, 0x1e,
			0x08, 0x22, 0x22, 0x00);
		eid_cmd(client_data, 0xfb, prm, 16, 0x9c, 0x0d, 0x00, 0x0e,
			0x14, 0x2b, 0x2e, 0x31, 0x0c, 0x09, 0x12, 0x20,
			0x0a, 0x22, 0x22, 0x00);
		eid_cmd(client_data, 0xfc, prm, 16, 0x8d, 0x1c, 0x00, 0x03,
			0x0a, 0x24, 0x28, 0x2b, 0x06, 0x04, 0x10, 0x1d,
			0x0b, 0x22, 0x22, 0x00);
		eid_cmd(client_data, BCMODE, prm, 4, 0x02, 0x00, 0x00, 0x00);
		eid_cmd(client_data, WRCABC, prm, 4, 0x00, 0x00, 0x00, 0x00);
         } else if (panel->panel_id == PANEL_ESPRESSO_TPO) {
                eid_cmd(client_data, GAMCTL1, prm, 16, 0x80, 0x0a, 0x00, 0x18,
                        0x32, 0x2f, 0x2a, 0x28, 0x13, 0x12, 0x14, 0x18,
                        0x06, 0x22, 0x22, 0x00);
                eid_cmd(client_data, GAMCTL2, prm, 16, 0x80, 0x00, 0x00, 0x18,
                        0x28, 0x24, 0x21, 0x20, 0x19, 0x17, 0x13, 0x17,
                        0x05, 0x22, 0x22, 0x00);
                eid_cmd(client_data, 0xf9, prm, 16, 0x94, 0x0a, 0x00, 0x1c,
                        0x3b, 0x3d, 0x3b, 0x39, 0x03, 0x03, 0x08, 0x11,
                        0x04, 0x22, 0x22, 0x00);
                eid_cmd(client_data, 0xfa, prm, 16, 0x80, 0x14, 0x00, 0x22,
                        0x3b, 0x38, 0x35, 0x36, 0x03, 0x03, 0x08, 0x10,
                        0x03, 0x22, 0x22, 0x00);
                eid_cmd(client_data, 0xfb, prm, 16, 0x80, 0x0a, 0x00, 0x29,
                        0x36, 0x2c, 0x26, 0x23, 0x1b, 0x22, 0x2c, 0x2c,
                        0x0f, 0x22, 0x22, 0x00);
                eid_cmd(client_data, 0xfc, prm, 16, 0x80, 0x00, 0x00, 0x23,
                        0x2b, 0x21, 0x1d, 0x1b, 0x22, 0x25, 0x2a, 0x29,
                        0x0d, 0x22, 0x22, 0x00);
		eid_cmd(client_data, BCMODE, prm, 4, 0x00, 0x00, 0x00, 0x00);
		eid_cmd(client_data, WRCABC, prm, 4, 0x00, 0x00, 0x00, 0x00);
	} else if (panel->panel_id == PANEL_LIBERTY_EID_24pin) {
		eid_cmd(client_data, GAMCTL1, prm, 16, 0x00, 0x00, 0x00, 0x06,
			0x27, 0x2C, 0x2A, 0x2A, 0x13, 0x13, 0x17, 0x19,
			0x00, 0x11, 0x84, 0x00);
		eid_cmd(client_data, GAMCTL2, prm, 16, 0x00, 0x00, 0x00, 0x06,
			0x27, 0x2C, 0x2A, 0x2A, 0x13, 0x13, 0x17, 0x19,
			0x00, 0x11, 0x84, 0x00);
		eid_cmd(client_data, 0xf9, prm, 16,  0x12, 0x00, 0x00, 0x19,
			0x2B, 0x36, 0x36, 0x37, 0x06, 0x06, 0x08, 0x0F,
			0x00, 0x11, 0x24, 0x00);
		eid_cmd(client_data, 0xfa, prm, 16,  0x00, 0x12, 0x00, 0x19,
			0x2B, 0x36, 0x36, 0x37, 0x06, 0x06, 0x08, 0x0F,
			0x00, 0x11, 0x24, 0x00);
		eid_cmd(client_data, 0xfb, prm, 16, 0x9C, 0x00, 0x00, 0x00, 0x10,
			0x2B, 0x30, 0x35, 0x04, 0x02, 0x02, 0x11,
			0x00, 0x11, 0x11, 0x00);
		eid_cmd(client_data, 0xfc, prm, 16,  0x00, 0x9C, 0x00, 0x00, 0x10,
			0x2B, 0x30, 0x35, 0x04, 0x02, 0x02, 0x11,
			0x00, 0x11, 0x11, 0x00);
		eid_cmd(client_data, BCMODE, prm, 4, 0x02, 0x00, 0x00, 0x00);
		eid_cmd(client_data, WRCABC, prm, 4, 0x00, 0x00, 0x00, 0x00);
	} else if (panel->panel_id == PANEL_LIBERTY_TPO) {
		eid_cmd(client_data, GAMCTL1, prm, 16, 0x91, 0x00, 0x00, 0x12,
			0x2c, 0x34, 0x32, 0x34, 0x11, 0x07, 0x05, 0x10,
			0x03, 0x22, 0x22, 0x00);
		eid_cmd(client_data, GAMCTL2, prm, 16, 0x8d, 0x11, 0x00, 0x11,
			0x1f, 0x21, 0x21, 0x27, 0x09, 0x08, 0x05, 0x0e,
			0x03, 0x22, 0x22, 0x00);
		eid_cmd(client_data, 0xf9, prm, 16,  0x9e, 0x00, 0x00, 0x10,
			0x2f, 0x3e, 0x3c, 0x3e, 0x01, 0x01, 0x02, 0x08,
			0x02, 0x22, 0x22, 0x00);
		eid_cmd(client_data, 0xfa, prm, 16,  0x8d, 0x1e, 0x00, 0x0f,
			0x25, 0x2e, 0x2e, 0x32, 0x04, 0x01, 0x05, 0x0d,
			0x03, 0x22, 0x22, 0x00);
		eid_cmd(client_data, 0xfb, prm, 16, 0x80, 0x00, 0x00, 0x01, 0x17,
			0x21, 0x1c, 0x18, 0x29, 0x31, 0x29, 0x1f,
			0x09, 0x22, 0x22, 0x00);
		eid_cmd(client_data, 0xfc, prm, 16,  0x8d, 0x00, 0x00, 0x01, 0x0b,
			0x0c, 0x09, 0x05, 0x31, 0x36, 0x2a, 0x1e,
			0x09, 0x22, 0x22, 0x00);
		eid_cmd(client_data, BCMODE, prm, 4, 0x02, 0x00, 0x00, 0x00);
		eid_cmd(client_data, WRCABC, prm, 4, 0x00, 0x00, 0x00, 0x00);
	} else {
		eid_cmd(client_data, GAMCTL1, prm, 16, 0x14, 0x00, 0x00, 0x1d,
			0x25, 0x32, 0x34, 0x36, 0x06, 0x03, 0x08, 0x16,
			0x00, 0x41, 0x81, 0x00);
		eid_cmd(client_data, GAMCTL2, prm, 16, 0x00, 0x14, 0x00, 0x1d,
			0x25, 0x32, 0x34, 0x36, 0x06, 0x03, 0x08, 0x16,
			0x00, 0x41, 0x81, 0x00);
		eid_cmd(client_data, 0xf9, prm, 16, 0x00, 0x00, 0x00, 0x1d,
			0x25, 0x2b, 0x2a, 0x2b, 0x13, 0x11, 0x11, 0x11,
			0x00, 0x12, 0x44, 0x00);
		eid_cmd(client_data, 0xfa, prm, 16, 0x00, 0x00, 0x00, 0x1d,
			0x25, 0x2b, 0x2a, 0x2b, 0x13, 0x11, 0x11, 0x11,
			0x00, 0x12, 0x44, 0x00);
		eid_cmd(client_data, 0xfb, prm, 16, 0x14, 0x00, 0x00, 0x28,
			0x34, 0x3a, 0x3a, 0x3c, 0x00, 0x00, 0x00, 0x04,
			0x00, 0x11, 0x11, 0x00);
		eid_cmd(client_data, 0xfc, prm, 16, 0x00, 0x14, 0x00, 0x28,
			0x34, 0x3a, 0x3a, 0x3c, 0x00, 0x00, 0x00, 0x04,
			0x00, 0x11, 0x11, 0x00);

		/* auto */
		eid_cmd(client_data, BCMODE, prm, 4, 0x02, 0x00, 0x00, 0x00);
		/* CABC off */
		eid_cmd(client_data, WRCABC, prm, 4, 0x00, 0x00, 0x00, 0x00);
	}
	eid_cmd(client_data, CASET, prm, 4, 0x00, 0x00, 0x01, 0x3f);
	eid_cmd(client_data, PASET, prm, 4, 0x00, 0x00, 0x01, 0xdf);
	if (panel->panel_id == PANEL_ESPRESSO_TPO)
		eid_cmd(client_data, DCON, prm, 4, 0x00, 0x00, 0x00, 0x00);
	else
		eid_cmd(client_data, DCON, prm, 4, 0x06, 0x00, 0x00, 0x00);
	eid_cmd(client_data, RAMWR, prm, 4, 0x00, 0x00, 0x00, 0x00);
	if ((panel->panel_id == PANEL_ESPRESSO_TPO) ||
	    (panel->panel_id == PANEL_LIBERTY_EID_24pin) ||
	    (panel->panel_id == PANEL_LIBERTY_TPO))
		hr_msleep(50);
	else
		hr_msleep(100);

	if (panel->panel_id == PANEL_ESPRESSO_TPO) {
		eid_cmd(client_data, CASET, prm, 4, 0x00, 0x00, 0x01, 0x3f);
		eid_cmd(client_data, PASET, prm, 4, 0x00, 0x00, 0x01, 0xdf);
	}
	eid_cmd(client_data, DCON, prm, 4, 0x07, 0x00, 0x00, 0x00);

	client_data->auto_hibernate(client_data, 1);
	return 0;
}

static int
eid_client_uninit(struct msm_mddi_bridge_platform_data *bridge,
		  struct msm_mddi_client_data *client_data)
{
	uint8_t prm[20];
	struct panel_data *panel = &bridge->panel_conf;

	B(KERN_DEBUG "%s: enter.\n", __func__);
	client_data->auto_hibernate(client_data, 0);
	if (bridge->panel_type == PANEL_ESPRESSO_SHARP)
	{
		eid_cmd(client_data, 0x28, prm, 4, 0x07, 0x00, 0x00, 0x00);
		eid_cmd(client_data, 0x10, prm, 4, 0x07, 0x00, 0x00, 0x00);
		hr_msleep(130);
		client_data->auto_hibernate(client_data, 1);
		return 0;
	}
	eid_cmd(client_data, DCON, prm, 4, 0x06, 0x00, 0x00, 0x00);
	hr_msleep(45);
	if ((panel->panel_id == PANEL_TPO) ||
	    (panel->panel_id == PANEL_HEROC_TPO) ||
	    (panel->panel_id == PANEL_LIBERTY_TPO))
		eid_cmd(client_data, DCON, prm, 4, 0x07, 0x00, 0x00, 0x00);
	else
		eid_cmd(client_data, DCON, prm, 4, 0x00, 0x00, 0x00, 0x00);
	hr_msleep(30);
	if (panel->panel_id == PANEL_ESPRESSO_TPO)
		eid_cmd(client_data, 0xf3, prm, 12, 0x00, 0x00, 0x2a, 0x00,
		0x00, 0x33, 0x75, 0x75, 0x00, 0x00, 0x00, 0x00);
	else
		eid_pwrctl(client_data, panel->panel_id, 0x00, 0x00);
	eid_cmd(client_data, SLPIN, prm, 4, 0x00, 0x00, 0x00, 0x00);
	if ((panel->panel_id == PANEL_EID_24pin) ||
	   (panel->panel_id == PANEL_ESPRESSO_TPO) ||
	   (panel->panel_id == PANEL_LIBERTY_EID_24pin))
		eid_cmd(client_data, MDDICTL, prm, 4, 0x02, 0x00, 0x00, 0x00);
	if (panel->panel_id != PANEL_LIBERTY_TPO)
		hr_msleep(210);

	client_data->auto_hibernate(client_data, 1);
	return 0;
}

struct mddi_cmd {
	u8 cmd;
	unsigned delay;
	u8 *vals;
	unsigned len;
};

#define LCM_CMD(_cmd, _delay, ...)                              \
{                                                               \
	.cmd = _cmd,                                            \
	.delay = _delay,                                        \
	.vals = (u8 []){__VA_ARGS__},                           \
	.len = sizeof((u8 []){__VA_ARGS__}) / sizeof(u8)        \
}

static const struct mddi_cmd eidII_init_cmd_table[] = {
	LCM_CMD(0xF3, 0, 0x0, 0x0, 0x2A, 0x0, 0x0, 0x33, 0x3C, 0x3C,
						0x0, 0x0, 0x0, 0x0),
	LCM_CMD(0x11, 20, 0x0, 0x0, 0x0, 0x0),
	LCM_CMD(0xF2, 0, 0x16, 0x16, 0xF, 0x11, 0x11, 0x11, 0x11, 0x10,
						0x0, 0x16, 0x16, 0x0),
	LCM_CMD(0xF3, 0, 0x0, 0x1, 0x2A, 0x0, 0x0, 0x33, 0x3C, 0x3C, 0x0,
						0x0, 0x0, 0x0),
	LCM_CMD(0xF4, 0, 0x1E, 0x1E, 0x26, 0x26, 0x0, 0x0, 0x0, 0x0),
	LCM_CMD(0xF5, 0, 0x12, 0x0, 0xA, 0x1, 0x1, 0x1D, 0x0, 0x0),
	LCM_CMD(0xFD, 20, 0x44, 0x3B, 0x0, 0x0),
	LCM_CMD(0xF3, 20, 0x0, 0x3, 0x2A, 0x0, 0x0, 0x33, 0x3C, 0x3C, 0x0,
						0x0, 0x0, 0x0),
	LCM_CMD(0xF3, 20, 0x0, 0x7, 0x2A, 0x0, 0x0, 0x33, 0x3C, 0x3C, 0x0,
						0x0, 0x0, 0x0),
	LCM_CMD(0xF3, 20, 0x0, 0xF, 0x2A, 0x0, 0x2, 0x33, 0x3C, 0x3C, 0x0,
						0x0, 0x0, 0x0),
	LCM_CMD(0xF3, 20, 0x0, 0x1F, 0x2A, 0x0, 0x2, 0x33, 0x3C, 0x3C, 0x0,
						0x0, 0x0, 0x0),
	LCM_CMD(0xF3, 30, 0x0, 0x3F, 0x2A, 0x0, 0x8, 0x33, 0x3C, 0x3C, 0x0,
						0x0, 0x0, 0x0),
	LCM_CMD(0xF3, 40, 0x0, 0x7F, 0x2A, 0x0, 0x8, 0x33, 0x3C, 0x3C, 0x0,
						0x0, 0x0, 0x0),
	LCM_CMD(0x35, 0, 0x0, 0x0, 0x0, 0x0),
	LCM_CMD(0x36, 0, 0x48, 0x0, 0x0, 0x0),
#ifdef CONFIG_MACH_HERO
	LCM_CMD(0x3A, 0, 0x66, 0x0, 0x0, 0x0),
#else
	LCM_CMD(0x3A, 0, 0x55, 0x0, 0x0, 0x0),
#endif
	LCM_CMD(0xF7, 0, 0x0, 0x0, 0x0, 0x06, 0x27, 0x2C, 0x2A, 0x2A, 0x13,
				0x13, 0x17, 0x19, 0x0, 0x11, 0x84, 0x0),
	LCM_CMD(0xF8, 0, 0x0, 0x0, 0x0, 0x06, 0x27, 0x2C, 0x2A, 0x2A, 0x13,
				0x13, 0x17, 0x19, 0x0, 0x11, 0x84, 0x0),
	LCM_CMD(0xF9, 0, 0x12, 0x0, 0x0, 0x19, 0x2B, 0x36, 0x36, 0x37, 0x06,
				0x06, 0x08, 0x0F, 0x0, 0x11, 0x24, 0x0),
	LCM_CMD(0xFA, 0, 0x0, 0x12, 0x0, 0x19, 0x2B, 0x36, 0x36, 0x37, 0x06,
				0x06, 0x08, 0x0F, 0x0, 0x11, 0x24, 0x0),
	LCM_CMD(0xFB, 0, 0x9C, 0x0, 0x0, 0x0, 0x10, 0x2B, 0x30, 0x35, 0x04,
				0x02, 0x02, 0x11, 0x0, 0x11, 0x11, 0x0),
	LCM_CMD(0xFC, 0, 0x0, 0x9C, 0x0, 0x0, 0x10, 0x2B, 0x30, 0x35, 0x04,
				0x02, 0x02, 0x11, 0x0, 0x11, 0x11, 0x0),
	LCM_CMD(0xCB, 0, 0x2, 0x0, 0x0, 0x0),
	LCM_CMD(0x55, 0, 0x0, 0x0, 0x0, 0x0),
	LCM_CMD(0x2A, 0, 0x0, 0x0, 0x1, 0x3F),
	LCM_CMD(0x2B, 0, 0x0, 0x0, 0x1, 0xDF),
	LCM_CMD(0xEF, 0, 0x6, 0x0, 0x0, 0x0),
	LCM_CMD(0x2C, 45, 0x0, 0x0, 0x0, 0x0),
	LCM_CMD(0xEF, 0, 0x7, 0x0, 0x0, 0x0),
};

static int
eid2_client_init(struct msm_mddi_bridge_platform_data *bridge,
		struct msm_mddi_client_data *client_data)
{
	int i = 0;
	B(KERN_DEBUG "%s\n", __func__);

	for (i = 0; i < ARRAY_SIZE(eidII_init_cmd_table); i++) {
		client_data->remote_write_vals(client_data,
				eidII_init_cmd_table[i].vals,
				eidII_init_cmd_table[i].cmd,
				eidII_init_cmd_table[i].len);
		if (eidII_init_cmd_table[i].delay)
			hr_msleep(eidII_init_cmd_table[i].delay);
	}
	return 0;
}

static int
eid2_client_uninit(struct msm_mddi_bridge_platform_data *bridge,
		struct msm_mddi_client_data *client_data)
{
	B(KERN_DEBUG "%s\n", __func__);
	uint8_t prm[20];
	struct panel_data *panel = &bridge->panel_conf;

	client_data->auto_hibernate(client_data, 0);
	eid_cmd(client_data, DCON, prm, 4, 0x06, 0x00, 0x00, 0x00);
	hr_msleep(45);
	eid_cmd(client_data, DCON, prm, 4, 0x00, 0x00, 0x00, 0x00);
	hr_msleep(30);

	eid_pwrctl(client_data, panel->panel_id, 0x00, 0x00);
	eid_cmd(client_data, SLPIN, prm, 4, 0x00, 0x00, 0x00, 0x00);
	hr_msleep(210);

	client_data->auto_hibernate(client_data, 1);
	return 0;
}

static int
samsung_client_init(struct msm_mddi_bridge_platform_data *bridge,
		    struct msm_mddi_client_data *client_data)
{
	uint8_t prm[20];

	B(KERN_DEBUG "%s: enter.\n", __func__);
	client_data->auto_hibernate(client_data, 0);

	eid_cmd(client_data, PWRCTL, prm, 12, 0x00, 0x00, 0x26, 0x26, 0x0a,
		0x22, 0x6f, 0x6f, 0x00, 0x00, 0x00, 0x00);
	eid_cmd(client_data, SLPOUT, prm, 4, 0x00, 0x00, 0x00, 0x00);
	hr_msleep(20);

	eid_cmd(client_data, DISCTL, prm, 12, 0x15, 0x15, 0x0f, 0x11, 0x11,
		0x11, 0x11, 0x10, 0x04, 0x15, 0x15, 0x00);
	eid_cmd(client_data, PWRCTL, prm, 12, 0x00, 0x01, 0x26, 0x26, 0x0a,
		0x22, 0x6f, 0x6f, 0x00, 0x00, 0x00, 0x00);
	eid_cmd(client_data, VCMCTL, prm, 8, 0x6d, 0x6d, 0x7a, 0x7a, 0x44, 0x00,
		0x00, 0x00);
	eid_cmd(client_data, SRCCTL, prm, 8, 0x12, 0x00, 0x06, 0xf1, 0x41, 0x1f,
		0x00, 0x00);
	eid_cmd(client_data, GATECTL, prm, 4, 0x33, 0x3b, 0x00, 0x00);
	hr_msleep(20);

	eid_cmd(client_data, PWRCTL, prm, 12, 0x00, 0x03, 0x26, 0x26, 0x0a,
		0x22, 0x6f, 0x6f, 0x00, 0x00, 0x00, 0x00);
	hr_msleep(20);

	eid_cmd(client_data, PWRCTL, prm, 12, 0x00, 0x07, 0x26, 0x26, 0x0a,
		0x22, 0x6f, 0x6f, 0x00, 0x00, 0x00, 0x00);
	hr_msleep(20);

	eid_cmd(client_data, PWRCTL, prm, 12, 0x00, 0x0f, 0x26, 0x26, 0x0a,
		0x22, 0x6f, 0x6f, 0x00, 0x00, 0x00, 0x00);
	hr_msleep(20);

	eid_cmd(client_data, PWRCTL, prm, 12, 0x00, 0x3f, 0x26, 0x26, 0x0a,
		0x22, 0x6f, 0x6f, 0x00, 0x00, 0x00, 0x00);
	hr_msleep(25);

	eid_cmd(client_data, PWRCTL, prm, 12, 0x00, 0x7f, 0x26, 0x26, 0x0a,
		0x22, 0x6f, 0x6f, 0x00, 0x00, 0x00, 0x00);
	hr_msleep(35);

	eid_cmd(client_data, TEON, prm, 4, 0x00, 0x00, 0x00, 0x00);
	eid_cmd(client_data, MADCTL, prm, 4, 0x98, 0x00, 0x00, 0x00);
	eid_cmd(client_data, COLMOD, prm, 4, 0x66, 0x00, 0x00, 0x00);
	eid_cmd(client_data, GAMCTL1, prm, 16, 0x00, 0x1c, 0x00, 0x04,
		0x3b, 0x38, 0x33, 0x33, 0x0d, 0x0e, 0x0d, 0x16,
		0x07, 0x02, 0x22, 0x00);
	eid_cmd(client_data, GAMCTL2, prm, 16, 0x00, 0x20, 0x00, 0x02,
		0x3b, 0x38, 0x33, 0x32, 0x0d, 0x0e, 0x0d, 0x16,
		0x07, 0x02, 0x22, 0x00);
	eid_cmd(client_data, 0xf9, prm, 16, 0x00, 0x1c, 0x00, 0x02,
		0x39, 0x36, 0x31, 0x31, 0x12, 0x13, 0x12, 0x1b,
		0x0c, 0x02, 0x22, 0x00);
	eid_cmd(client_data, 0xfa, prm, 16, 0x00, 0x20, 0x00, 0x00,
		0x39, 0x36, 0x31, 0x30, 0x12, 0x13, 0x12, 0x1b,
		0x0c, 0x02, 0x22, 0x00);
	eid_cmd(client_data, 0xfb, prm, 16, 0x00, 0x1c, 0x00, 0x05,
		0x37, 0x34, 0x30, 0x2b, 0x19, 0x1f, 0x1e, 0x27,
		0x18, 0x02, 0x22, 0x00);
	eid_cmd(client_data, 0xfc, prm, 16, 0x00, 0x20, 0x00, 0x03,
		0x37, 0x34, 0x2f, 0x2b, 0x19, 0x1f, 0x1d, 0x27,
		0x18, 0x02, 0x22, 0x00);
	eid_cmd(client_data, BCMODE, prm, 4, 0x02, 0x00, 0x00, 0x00);
	eid_cmd(client_data, WRCABC, prm, 4, 0x00, 0x00, 0x00, 0x00);
	eid_cmd(client_data, CASET, prm, 4, 0x00, 0x00, 0x01, 0x3f);
	eid_cmd(client_data, PASET, prm, 4, 0x00, 0x00, 0x01, 0xdf);
	eid_cmd(client_data, DCON, prm, 4, 0x06, 0x00, 0x01, 0xdf);
	eid_cmd(client_data, RAMWR, prm, 4, 0x00, 0x00, 0x00, 0x00);
	hr_msleep(40);
	eid_cmd(client_data, DCON, prm, 4, 0x07, 0x00, 0x01, 0xdf);

	client_data->auto_hibernate(client_data, 1);
	return 0;
}

static int
samsung_client_uninit(struct msm_mddi_bridge_platform_data *bridge,
		      struct msm_mddi_client_data *client_data)
{
	uint8_t prm[20];
	B(KERN_DEBUG "%s: enter.\n", __func__);

	client_data->auto_hibernate(client_data, 0);

	eid_cmd(client_data, 0xf0, prm, 4, 0x5a, 0x30, 0x00, 0x00);
	eid_cmd(client_data, 0xb0, prm, 4, 0x01, 0x00, 0x00, 0x00);

	client_data->auto_hibernate(client_data, 1);
	return 0;
}

/* got called by msmfb_suspend */
static int samsung_suspend(struct msm_panel_data *panel_data)
{
	struct panel_info *panel = container_of(panel_data, struct panel_info,
						panel_data);
	struct msm_mddi_client_data *client_data = panel->client_data;
	struct msm_mddi_bridge_platform_data *bridge_data =
		client_data->private_client_data;
	int ret;

	wake_lock(&panel->idle_lock);
	ret = bridge_data->uninit(bridge_data, client_data);
	wake_unlock(&panel->idle_lock);
	if (ret) {
		printk(KERN_ERR "mddi samsung client: non zero return from "
				"uninit\n");
		return ret;
	}

	client_data->suspend(client_data);
	return 0;
}

/* got called by msmfb_resume */
static int samsung_resume(struct msm_panel_data *panel_data)
{
	struct panel_info *panel = container_of(panel_data, struct panel_info,
						panel_data);
	struct msm_mddi_client_data *client_data = panel->client_data;
	struct msm_mddi_bridge_platform_data *bridge_data =
		client_data->private_client_data;
	int ret;

	B(KERN_DEBUG "%s\n", __func__);
	client_data->resume(client_data);

	wake_lock(&panel->idle_lock);
	ret = bridge_data->init(bridge_data, client_data);
	wake_unlock(&panel->idle_lock);
	return ret;
}

static int samsung_blank(struct msm_panel_data *panel_data)
{
	struct panel_info *panel = container_of(panel_data, struct panel_info,
						panel_data);
	struct msm_mddi_client_data *client_data = panel->client_data;
	struct msm_mddi_bridge_platform_data *bridge_data =
	    client_data->private_client_data;

	if (cabc_config.bl_handle)
		cabc_config.bl_handle(&mddi_samsung_cabc, LED_OFF);
	return bridge_data->blank(bridge_data, client_data);
}

static int samsung_unblank(struct msm_panel_data *panel_data)
{
	struct panel_info *panel = container_of(panel_data, struct panel_info,
						panel_data);
	struct msm_mddi_client_data *client_data = panel->client_data;
	struct msm_mddi_bridge_platform_data *bridge_data =
	    client_data->private_client_data;
	if (cabc_config.bl_handle) {
		hr_msleep(40);
		cabc_config.bl_handle(&mddi_samsung_cabc, LED_FULL);
	}
	return bridge_data->unblank(bridge_data, client_data);
}


static int samsung_shutdown(struct msm_panel_data *panel_data)
{
        struct panel_info *panel = container_of(panel_data, struct panel_info,
                                                panel_data);
        struct msm_mddi_client_data *client_data = panel->client_data;
        struct msm_mddi_bridge_platform_data *bridge_data =
                client_data->private_client_data;
	if (bridge_data->shutdown) {
		B(KERN_INFO "%s: panel shutdown\n", __func__);
		eid_client_uninit(bridge_data, client_data);
		return bridge_data->shutdown(bridge_data, client_data);
	}
	return 0;
}

static irqreturn_t eid_vsync_interrupt(int irq, void *data)
{
	struct panel_info *panel = data;

#if DEBUG_VSYNC_INT
	uint32_t val;
	val = readl(VSYNC_STATUS);
	if (!!(val & 0x04) == 0)
		printk(KERN_ERR "BUG!! val: %x\n", val);
#endif
	if (atomic_read(&panel->depth) > 0) {
		atomic_dec(&panel->depth);
		disable_irq_nosync(gpio_to_irq(97));
	}

	if (panel->fb_callback) {
		panel->fb_callback->func(panel->fb_callback);
		panel->fb_callback = NULL;
	}

	panel->samsung_got_int = 1;
	wake_up(&samsung_vsync_wait);
	return IRQ_HANDLED;
}

static int setup_vsync(struct panel_info *panel, int init)
{
	int ret;
	int gpio = 97;
	unsigned int irq;

	if (!init) {
		ret = 0;
		goto uninit;
	}

	ret = irq = gpio_to_irq(gpio);
	if (ret < 0)
		goto err_get_irq_num_failed;

	samsung_clear_vsync();
	ret = request_irq(irq, eid_vsync_interrupt, IRQF_TRIGGER_HIGH,
			"vsync", panel);
	if (ret)
		goto err_request_irq_failed;
	disable_irq_nosync(irq);

	printk(KERN_INFO "vsync on gpio %d now %d\n", gpio,
			gpio_get_value(gpio));
	return 0;

uninit:
	free_irq(gpio_to_irq(gpio), panel->client_data);
err_request_irq_failed:
err_get_irq_num_failed:
	printk(KERN_ERR "%s fail (%d)\n", __func__, ret);
	return ret;
}

static int mddi_samsung_probe(struct platform_device *pdev)
{
	int ret, err = -EINVAL;
	struct panel_info *panel;
	struct msm_mddi_client_data *client_data = pdev->dev.platform_data;
	struct msm_mddi_bridge_platform_data *bridge_data =
	    client_data->private_client_data;
	struct panel_data *panel_data = &bridge_data->panel_conf;

	B(KERN_DEBUG "%s: enter\n", __func__);
	cabc_config.pwm_data = panel_data->pwm;
	cabc_config.min_level = panel_data->min_level;
	cabc_config.shrink = panel_data->shrink;
	cabc_config.shrink_br = panel_data->shrink_br;
	cabc_config.default_br = panel_data->default_br;

	panel = kzalloc(sizeof(struct panel_info), GFP_KERNEL);
	if (!panel) {
		err = -ENOMEM;
		goto err_out;
	}
	platform_set_drvdata(pdev, panel);

	if ((panel_data->panel_id == PANEL_EID_40pin) ||
	    (panel_data->panel_id == PANEL_EID_24pin) ||
	    (panel_data->panel_id == PANEL_TPO) ||
	    (panel_data->panel_id == PANEL_HEROC_EID_BOTTOM) ||
	    (panel_data->panel_id == PANEL_HEROC_TPO) ||
	    (panel_data->panel_id == PANEL_LIBERTY_TPO) ||
	    (panel_data->panel_id == PANEL_LIBERTY_EID_24pin) ||
	    (panel_data->panel_id == PANEL_ESPRESSO_SHARP) ||
	    (panel_data->panel_id == PANEL_ESPRESSO_TPO)) {
		bridge_data->init = eid_client_init;
		bridge_data->uninit = eid_client_uninit;
	} else if (panel_data->panel_id == PANEL_EIDII) {
		bridge_data->init = eid2_client_init;
		bridge_data->uninit = eid2_client_uninit;
	} else if (panel_data->panel_id == PANEL_SAMSUNG) {
		bridge_data->init = samsung_client_init;
		bridge_data->uninit = samsung_client_uninit;
	} else {
		printk(KERN_ERR "%s: unknown panel type %d\n",
			__func__, panel_data->panel_id);
		err = -ENXIO;
		goto err_panel;
	}

	if (panel_data->caps & MSMFB_CAP_CABC) {
		printk(KERN_INFO "CABC enabled\n");
		cabc_config.client = client_data;
		platform_device_register(&mddi_samsung_cabc);
	}

	ret = setup_vsync(panel, 1);
	if (ret) {
		dev_err(&pdev->dev, "mddi_samsung_setup_vsync failed\n");
		err = -EIO;
		goto err_panel;
	}

	panel->client_data = client_data;
	panel->panel_data.suspend = samsung_suspend;
	panel->panel_data.resume = samsung_resume;
	panel->panel_data.wait_vsync = samsung_wait_vsync;
	panel->panel_data.request_vsync = samsung_request_vsync;
	panel->panel_data.blank = samsung_blank;
	panel->panel_data.unblank = samsung_unblank;
	/* panel->panel_data.clear_vsync = samsung_clear_vsync; */
	panel->panel_data.dump_vsync = samsung_dump_vsync;
	panel->panel_data.shutdown = samsung_shutdown;
	panel->panel_data.fb_data = &bridge_data->fb_data;
	panel->panel_data.caps = ~MSMFB_CAP_PARTIAL_UPDATES;
	atomic_set(&panel->depth, 0);
	panel->pdev.name = "msm_panel";
	panel->pdev.id = pdev->id;
	panel->pdev.resource = client_data->fb_resource;
	panel->pdev.num_resources = 1;
	panel->pdev.dev.platform_data = &panel->panel_data;
	platform_device_register(&panel->pdev);
	wake_lock_init(&panel->idle_lock, WAKE_LOCK_IDLE, "eid_idle_lock");
	/* for debuging vsync issue */
	ebi1_clk = clk_get(NULL, "ebi1_clk");
	return 0;

err_panel:
	kfree(panel);
err_out:
	return err;
}

static int mddi_samsung_remove(struct platform_device *pdev)
{
	struct panel_info *panel = platform_get_drvdata(pdev);

	setup_vsync(panel, 0);
	kfree(panel);
	return 0;
}

static struct platform_driver mddi_client_0101_0000 = {
	.probe = mddi_samsung_probe,
	.remove = mddi_samsung_remove,
	.driver = {.name = "mddi_c_0101_0000"},
};

static int __init mddi_client_samsung_init(void)
{
	return platform_driver_register(&mddi_client_0101_0000);
}

module_init(mddi_client_samsung_init);