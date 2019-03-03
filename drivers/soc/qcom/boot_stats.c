/* Copyright (c) 2013-2014,2016, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/cpu.h>
#include <linux/sched.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/export.h>
#include <linux/types.h>
#include <soc/qcom/boot_stats.h>
#include <soc/qcom/socinfo.h>

static void __iomem *mpm_counter_base;
static uint32_t mpm_counter_freq;
struct boot_stats __iomem *boot_stats;

static int mpm_parse_dt(void)
{
	struct device_node *np;
	u32 freq;

	np = of_find_compatible_node(NULL, NULL, "qcom,msm-imem-boot_stats");
	if (!np) {
		pr_err("can't find qcom,msm-imem node\n");
		return -ENODEV;
	}
	boot_stats = of_iomap(np, 0);
	if (!boot_stats) {
		pr_err("boot_stats: Can't map imem\n");
		return -ENODEV;
	}

	np = of_find_compatible_node(NULL, NULL, "qcom,mpm2-sleep-counter");
	if (!np) {
		pr_err("mpm_counter: can't find DT node\n");
		return -ENODEV;
	}

	if (!of_property_read_u32(np, "clock-frequency", &freq))
		mpm_counter_freq = freq;
	else
		return -ENODEV;

	if (of_get_address(np, 0, NULL, NULL)) {
		mpm_counter_base = of_iomap(np, 0);
		if (!mpm_counter_base) {
			pr_err("mpm_counter: cant map counter base\n");
			return -ENODEV;
		}
	}

	return 0;
}

static void print_boot_stats(void)
{
	pr_info("KPI: Bootloader start count = %u\n",
		readl_relaxed(&boot_stats->bootloader_start));
	pr_info("KPI: Bootloader end count = %u\n",
		readl_relaxed(&boot_stats->bootloader_end));
	pr_info("KPI: Bootloader display count = %u\n",
		readl_relaxed(&boot_stats->bootloader_display));
	pr_info("KPI: Bootloader load kernel count = %u\n",
		readl_relaxed(&boot_stats->bootloader_load_kernel));
	pr_info("KPI: Kernel MPM timestamp = %u\n",
		readl_relaxed(mpm_counter_base));
	pr_info("KPI: Kernel MPM Clock frequency = %u\n",
		mpm_counter_freq);
}

#ifdef CONFIG_ZTE_BOOT_MODE
/*lkej add code for pv version*/
#define SOCINFO_CMDLINE_PV_FLAG "androidboot.pv-version="
#define SOCINFO_CMDLINE_PV_VERSION "1"
#define SOCINFO_CMDLINE_NON_PV_VERSION "0"
static int __init zte_pv_flag_init(char *ver)
{
	int is_pv_ver = 0;

	if (!strncmp(ver, SOCINFO_CMDLINE_PV_VERSION,
				strlen(SOCINFO_CMDLINE_PV_VERSION)))
		is_pv_ver = 1;

	pr_info("pv flag: %d ", is_pv_ver);
	socinfo_set_pv_flag(is_pv_ver);
	return 0;
}
__setup(SOCINFO_CMDLINE_PV_FLAG, zte_pv_flag_init);
#endif

unsigned long long int msm_timer_get_sclk_ticks(void)
{
	unsigned long long int t1, t2;
	int loop_count = 10;
	int loop_zero_count = 3;
	int tmp = USEC_PER_SEC;
	void __iomem *sclk_tick;

	do_div(tmp, TIMER_KHZ);
	tmp /= (loop_zero_count-1);
	sclk_tick = mpm_counter_base;
	if (!sclk_tick)
		return -EINVAL;
	while (loop_zero_count--) {
		t1 = __raw_readl_no_log(sclk_tick);
		do {
			udelay(1);
			t2 = t1;
			t1 = __raw_readl_no_log(sclk_tick);
		} while ((t2 != t1) && --loop_count);
		if (!loop_count) {
			pr_err("boot_stats: SCLK  did not stabilize\n");
			return 0;
		}
		if (t1)
			break;

		udelay(tmp);
	}
	if (!loop_zero_count) {
		pr_err("boot_stats: SCLK reads zero\n");
		return 0;
	}
	return t1;
}

/*
 * Support for FTM & RECOVERY mode by ZTE_BOOT
 */
#ifdef CONFIG_ZTE_BOOT_MODE
typedef enum {
	HW_VER_01      = 0, /* [GPIO133,GPIO132]=[0,0]*/
	HW_VER_02      = 1, /* [GPIO133,GPIO132]=[0,1]*/
	HW_VER_03      = 2, /* [GPIO133,GPIO132]=[1,0]*/
	HW_VER_04      = 3, /* [GPIO133,GPIO132]=[1,1]*/
	HW_VER_INVALID = 4,
	HW_VER_MAX
} zte_hw_ver_type;

#if defined(CONFIG_BOARD_PEONY)
static const char *zte_hw_ver_str[HW_VER_MAX] = {
	[HW_VER_01]     = "u3tA",
	[HW_VER_02]     = "u3tB",
	[HW_VER_03]     = "u3tC",
	[HW_VER_04]     = "u3tD",
	[HW_VER_INVALID]= "INVALID"
};
#elif defined(CONFIG_BOARD_STOLLEN)
static const char *zte_hw_ver_str[HW_VER_MAX] = {
	[HW_VER_01]     = "u3mA",
	[HW_VER_02]     = "u3mB",
	[HW_VER_03]     = "u3mC",
	[HW_VER_04]     = "u3mD",
	[HW_VER_INVALID]= "INVALID"
};
#elif defined(CONFIG_BOARD_BOLTON)
static const char *zte_hw_ver_str[HW_VER_MAX] = {
	[HW_VER_01]     = "ctwA",
	[HW_VER_02]     = "ctwB",
	[HW_VER_03]     = "ctwC",
	[HW_VER_04]     = "ctwD",
	[HW_VER_INVALID]= "INVALID"
};
#else
static const char *zte_hw_ver_str[HW_VER_MAX] = {
	[HW_VER_01]     = "***A",
	[HW_VER_02]     = "***B",
	[HW_VER_03]     = "***C",
	[HW_VER_04]     = "***D",
	[HW_VER_INVALID]= "INVALID"
};
#endif

static zte_hw_ver_type hw_ver = HW_VER_INVALID;
static void zte_hw_ver_parse_dt(void)
{
	struct device_node *np;
	void *hw_ver_addr;
	uint32_t hw_ver_id=0;

	np = of_find_compatible_node(NULL, NULL, "zte,imem-hw-ver-id");
	if (!np) {
		pr_err("unable to find DT imem hw ver node\n");
	} else {
		hw_ver_addr = of_iomap(np, 0);
		if (!hw_ver_addr) {
			pr_err("unable to map imem hw ver offset\n");
		} else {
		        hw_ver_id = __raw_readl(hw_ver_addr);
		        printk(KERN_ERR "hw ver id: 0x%x\n", hw_ver_id);
		}
        }

	hw_ver = (zte_hw_ver_type)(hw_ver_id&0x03);
	socinfo_set_hw_ver(zte_hw_ver_str[hw_ver]);

}

const char* read_zte_hw_ver(void)
{
	return zte_hw_ver_str[hw_ver];
}
EXPORT_SYMBOL_GPL(read_zte_hw_ver);

static int __init bootmode_init(char *mode)
{
	int boot_mode = 0;

	if (!strncmp(mode, ANDROID_BOOT_MODE_NORMAL, strlen(ANDROID_BOOT_MODE_NORMAL)))	{
		boot_mode = ENUM_BOOT_MODE_NORMAL;
		pr_err("KERENEL:boot_mode:NORMAL\n");
	} else if (!strncmp(mode, ANDROID_BOOT_MODE_FTM, strlen(ANDROID_BOOT_MODE_FTM))) {
		boot_mode = ENUM_BOOT_MODE_FTM;
		pr_err("KERENEL:boot_mode:FTM\n");
	} else if (!strncmp(mode, ANDROID_BOOT_MODE_RECOVERY, strlen(ANDROID_BOOT_MODE_RECOVERY))) {
		boot_mode = ENUM_BOOT_MODE_RECOVERY;
		pr_err("KERENEL:boot_mode:RECOVERY\n");
	} else if (!strncmp(mode, ANDROID_BOOT_MODE_FFBM, strlen(ANDROID_BOOT_MODE_FFBM))) {
		boot_mode = ENUM_BOOT_MODE_FFBM;
		pr_err("KERENEL:boot_mode:FFBM\n");
	} else if (!strncmp(mode, ANDROID_BOOT_MODE_CHARGER, strlen(ANDROID_BOOT_MODE_CHARGER))) {
		boot_mode = ENUM_BOOT_MODE_CHARGER;
		pr_err("KERENEL:boot_mode:CHARGER\n");
	} else {
		boot_mode = ENUM_BOOT_MODE_NORMAL;
		pr_err("KERENEL:boot_mode:DEFAULT NORMAL\n");
	}

	socinfo_set_boot_mode(boot_mode);

	return 0;
}
__setup(ANDROID_BOOT_MODE, bootmode_init);
#endif

#if defined(CONFIG_ZTE_TABLET)
/*
 * Support for marking sw version by ZTE_BOOT
*/
#define SWVER_FLAG_RESERVED_COOKIE 0x20170122
#define ZTE_SW_VER_PROP "qcom,msm-imem-zte_sw_ver"
static const char *zte_sw_ver_str[2] = { "DEV", "PV"};

const char *read_zte_sw_ver(void)
{
	struct device_node *np;
	void *zte_sw_ver_imem_addr = NULL;

	np = of_find_compatible_node(NULL, NULL, ZTE_SW_VER_PROP);
	if (!np) {
		pr_err("unable to find DT imem zte sw ver node!\n");
	} else {
		zte_sw_ver_imem_addr = of_iomap(np, 0);
		if (zte_sw_ver_imem_addr) {
			if (SWVER_FLAG_RESERVED_COOKIE == *(u32 *)zte_sw_ver_imem_addr)
				return zte_sw_ver_str[1];
		} else {
			pr_err("unable to map imem zte sw ver addr!\n");
		}
	}

	return zte_sw_ver_str[0];

}
#endif

int boot_stats_init(void)
{
	int ret;

#if defined(CONFIG_ZTE_TABLET)
	const char *sw_ver = NULL;

	sw_ver = read_zte_sw_ver();
	pr_err("%s: zte sw_ver=%s\n", __func__, sw_ver);
	socinfo_sync_sysfs_zte_sw_ver(sw_ver);
#endif

	ret = mpm_parse_dt();
	if (ret < 0)
		return -ENODEV;

	print_boot_stats();

#ifdef CONFIG_ZTE_BOOT_MODE
	zte_hw_ver_parse_dt(); // add for zte hw ver info
#endif

	if (!(boot_marker_enabled()))
		boot_stats_exit();
	return 0;
}

int boot_stats_exit(void)
{
	iounmap(boot_stats);
	iounmap(mpm_counter_base);
	return 0;
}
