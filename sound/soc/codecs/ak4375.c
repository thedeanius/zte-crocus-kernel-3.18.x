/*
 * ak4375.c  --  audio driver for AK4375
 *
 * Copyright (C) 2014 Asahi Kasei Microdevices Corporation
 *  Author				Date		Revision
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *					  14/06/25		1.1
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <sound/pcm_params.h>
#include <linux/regmap.h>

#include "ak4375.h"

#define AK4375_DEBUG			/* used at debug mode */
#define AK4375_CONTIF_DEBUG		/* used at debug mode */
#define CONFIG_DEBUG_FS_CODEC_AKM

#ifdef AK4375_DEBUG
#define akdbgprt pr_info
#else
#define akdbgprt pr_info
#endif

/* AK4375 Codec Private Data */
struct ak4375_priv {
	struct snd_soc_codec codec;
	struct i2c_client *i2c_client;
	struct regmap *regmap;
	u8 reg_cache[AK4375_MAX_REGISTERS];
	int fs1;
	int fs2;
	int rclk;			/*Master Clock */
	int nSeldain;		/* 0:Bypass, 1:SRC	(Dependent on a register bit) */
	int nBickFreq;		/* 0:32fs, 1:48fs, 2:64fs */
	int nSrcOutFsSel;	/* 0:48(44.1)kHz, 1:96(88.2)kHz, 2:192(176.4)kHz */
	int nPllMode;		/* 0:PLL OFF, 1: PLL ON */
	int nPllMCKI;		/* 0:PLL not use, 1: PLL use */
	int nSmt;			/* 0:1024/FSO, 1:2048/FSO, 2:4096/FSO, 3:8192/FSO */
	int dfsrc8fs;		/* DFTHR bit and SRCO8FS bit */
};
#ifdef CONFIG_DEBUG_FS_CODEC_AKM
static struct snd_soc_codec *ak4375_codec;
#endif
static struct ak4375_priv *ak4375_data;
static unsigned int AK4375_LDO_GPIO = 1;	/* default */
static unsigned int AK4375_RESET_GPIO = 0;

static const struct reg_default ak4375_reg_defaults[] = {
	{  0,  0x00}, {  1, 0x00 }, {   2, 0x00 }, {  3, 0x00 },
	{  4,  0x00}, {  5, 0x00 }, {   6, 0x00 }, {  7, 0x00 },
	{  8,  0x00}, {	 9, 0x00 }, {  10, 0x00 }, { 11, 0x19 },
	{  12, 0x19}, {	13, 0x75 }, {  14, 0x00 }, { 15, 0x00 },
	{  16, 0x00}, {	17, 0x00 }, {  18, 0x00 }, { 19, 0x00 },
	{  20, 0x00}, {	21, 0x00 }, {  22, 0x00 }, { 23, 0x00 },
	{  24, 0x00}, {	25, 0x00 }, {  26, 0x00 }, { 27, 0x00 },
	{  28, 0x00}, {	29, 0x00 }, {  30, 0x00 }, { 31, 0x00 },
	{  32, 0x00}, { 33, 0x00 }, {  34, 0x00 }, { 35, 0x00 },
	{  36, 0x00}, { 37, 0x00 }
};
/* ak4375 register cache & default register settings */
static const u8 ak4375_reg[AK4375_MAX_REGISTERS] = {
	0x00,	/*	0x00	AK4375_00_POWER_MANAGEMENT1			*/
	0x00,	/*	0x01	AK4375_01_POWER_MANAGEMENT2			*/
	0x00,	/*	0x02	AK4375_02_POWER_MANAGEMENT3			*/
	0x00,	/*	0x03	AK4375_03_POWER_MANAGEMENT4			*/
	0x00,	/*	0x04	AK4375_04_OUTPUT_MODE_SETTING		*/
	0x00,	/*	0x05	AK4375_05_CLOCK_MODE_SELECT			*/
	0x00,	/*	0x06	AK4375_06_DIGITAL_FILTER_SELECT		*/
	0x00,	/*	0x07	AK4375_07_DAC_MONO_MIXING			*/
	0x00,	/*	0x08	AK4375_08_JITTER_CLEANER_SETTING1	*/
	0x00,	/*	0x09	AK4375_09_JITTER_CLEANER_SETTING2	*/
	0x00,	/*	0x0A	AK4375_0A_JITTER_CLEANER_SETTING3	*/
	0x19,	/*	0x0B	AK4375_0B_LCH_OUTPUT_VOLUME			*/
	0x19,	/*	0x0C	AK4375_0C_RCH_OUTPUT_VOLUME			*/
	0x75,	/*	0x0D	AK4375_0D_HP_VOLUME_CONTROL			*/
	0x00,	/*	0x0E	AK4375_0E_PLL_CLK_SOURCE_SELECT		*/
	0x00,	/*	0x0F	AK4375_0F_PLL_REF_CLK_DIVIDER1		*/
	0x00,	/*	0x10	AK4375_10_PLL_REF_CLK_DIVIDER2		*/
	0x00,	/*	0x11	AK4375_11_PLL_FB_CLK_DIVIDER1		*/
	0x00,	/*	0x12	AK4375_12_PLL_FB_CLK_DIVIDER2		*/
	0x00,	/*	0x13	AK4375_13_SRC_CLK_SOURCE			*/
	0x00,	/*	0x14	AK4375_14_DAC_CLK_DIVIDER			*/
	0x00,	/*	0x15	AK4375_15_AUDIO_IF_FORMAT			*/
	0x00,	/*	0x16	AK4375_16_DUMMY						*/
	0x00,	/*	0x17	AK4375_17_DUMMY						*/
	0x00,	/*	0x18	AK4375_18_DUMMY						*/
	0x00,	/*	0x19	AK4375_19_DUMMY						*/
	0x00,	/*	0x1A	AK4375_1A_DUMMY						*/
	0x00,	/*	0x1B	AK4375_1B_DUMMY						*/
	0x00,	/*	0x1C	AK4375_1C_DUMMY						*/
	0x00,	/*	0x1D	AK4375_1D_DUMMY						*/
	0x00,	/*	0x1E	AK4375_1E_DUMMY						*/
	0x00,	/*	0x1F	AK4375_1F_DUMMY						*/
	0x00,	/*	0x20	AK4375_20_DUMMY						*/
	0x00,	/*	0x21	AK4375_21_DUMMY						*/
	0x00,	/*	0x22	AK4375_22_DUMMY						*/
	0x00,	/*	0x23	AK4375_23_DUMMY						*/
	0x00,	/*	0x24	AK4375_24_MODE_CONTROL				*/
};

static const struct {
	int readable;   /* Mask of readable bits */
	int writable;   /* Mask of writable bits */
} ak4375_access_masks[] = {
	{ 0xFF, 0xFF },	/* 0x00 */
	{ 0xFF, 0xFF },	/* 0x01 */
	{ 0xFF, 0xFF },	/* 0x02 */
	{ 0xFF, 0xFF },	/* 0x03 */
	{ 0xFF, 0xFF },	/* 0x04 */
	{ 0xFF, 0xFF },	/* 0x05 */
	{ 0xFF, 0xFF },	/* 0x06 */
	{ 0xFF, 0xFF },	/* 0x07 */
	{ 0xFF, 0xFF },	/* 0x08 */
	{ 0xFF, 0xFF },	/* 0x09 */
	{ 0xFF, 0xFF },	/* 0x0A */
	{ 0xFF, 0xFF },	/* 0x0B */
	{ 0xFF, 0xFF },	/* 0x0C */
	{ 0xFF, 0xFF },	/* 0x0D */
	{ 0xFF, 0xFF },	/* 0x0E */
	{ 0xFF, 0xFF },	/* 0x0F */
	{ 0xFF, 0xFF },	/* 0x10 */
	{ 0xFF, 0xFF },	/* 0x11 */
	{ 0xFF, 0xFF },	/* 0x12 */
	{ 0xFF, 0xFF },	/* 0x13 */
	{ 0xFF, 0xFF },	/* 0x14 */
	{ 0xFF, 0xFF },	/* 0x15 */
	{ 0x00, 0x00 },	/* 0x16 DUMMY */
	{ 0x00, 0x00 },	/* 0x17 DUMMY */
	{ 0x00, 0x00 },	/* 0x18 DUMMY */
	{ 0x00, 0x00 },	/* 0x19 DUMMY */
	{ 0x00, 0x00 },	/* 0x1A DUMMY */
	{ 0x00, 0x00 },	/* 0x1B DUMMY */
	{ 0x00, 0x00 },	/* 0x1C DUMMY */
	{ 0x00, 0x00 },	/* 0x1D DUMMY */
	{ 0x00, 0x00 },	/* 0x1E DUMMY */
	{ 0x00, 0x00 },	/* 0x1F DUMMY */
	{ 0x00, 0x00 },	/* 0x20 DUMMY */
	{ 0x00, 0x00 },	/* 0x21 DUMMY */
	{ 0x00, 0x00 },	/* 0x22 DUMMY */
	{ 0x00, 0x00 },	/* 0x23 DUMMY */
	{ 0xFF, 0xFF },	/* 0x24 */
};

/* Output Digital volume control:
 * from -12.5 to 3 dB in 0.5 dB steps (mute instead of -12.5 dB) */
static DECLARE_TLV_DB_SCALE(ovl_tlv, -1250, 50, 0);
static DECLARE_TLV_DB_SCALE(ovr_tlv, -1250, 50, 0);

/* HP-Amp Analog volume control:
 * from -42 to 6 dB in 2 dB steps (mute instead of -42 dB) */
static DECLARE_TLV_DB_SCALE(hpg_tlv, -4200, 20, 0);

static const char * const ak4375_ovolcn_select_texts[] = {"Dependent", "Independent"};
static const char * const ak4375_mdacl_select_texts[] = {"x1", "x1/2"};
static const char * const ak4375_mdacr_select_texts[] = {"x1", "x1/2"};
static const char * const ak4375_invl_select_texts[] = {"Normal", "Inverting"};
static const char * const ak4375_invr_select_texts[] = {"Normal", "Inverting"};
static const char * const ak4375_cpmod_select_texts[] = {"Automatic Switching",
					"+-VDD Operation", "+-1/2VDD Operation"};
static const char * const ak4375_hphl_select_texts[] = {"9ohm", "200kohm"};
static const char * const ak4375_hphr_select_texts[] = {"9ohm", "200kohm"};
static const char * const ak4375_dacfil_select_texts[] = {"Sharp Roll-Off", "Slow Roll-Off",
					"Short Delay Sharp Roll-Off", "Short Delay Slow Roll-Off"};
static const char * const ak4375_srcfil_select_texts[] = {"Sharp Roll-Off", "Slow Roll-Off",
					"Short Delay Sharp Roll-Off", "Short Delay Slow Roll-Off"};

static const struct soc_enum ak4375_dac_enum[] = {
	SOC_ENUM_SINGLE(AK4375_0B_LCH_OUTPUT_VOLUME, 7,
			ARRAY_SIZE(ak4375_ovolcn_select_texts), ak4375_ovolcn_select_texts),
	SOC_ENUM_SINGLE(AK4375_07_DAC_MONO_MIXING, 2,
			ARRAY_SIZE(ak4375_mdacl_select_texts), ak4375_mdacl_select_texts),
	SOC_ENUM_SINGLE(AK4375_07_DAC_MONO_MIXING, 6,
			ARRAY_SIZE(ak4375_mdacr_select_texts), ak4375_mdacr_select_texts),
	SOC_ENUM_SINGLE(AK4375_07_DAC_MONO_MIXING, 3,
			ARRAY_SIZE(ak4375_invl_select_texts), ak4375_invl_select_texts),
	SOC_ENUM_SINGLE(AK4375_07_DAC_MONO_MIXING, 7,
			ARRAY_SIZE(ak4375_invr_select_texts), ak4375_invr_select_texts),
	SOC_ENUM_SINGLE(AK4375_03_POWER_MANAGEMENT4, 2,
			ARRAY_SIZE(ak4375_cpmod_select_texts), ak4375_cpmod_select_texts),
	SOC_ENUM_SINGLE(AK4375_04_OUTPUT_MODE_SETTING, 0,
			ARRAY_SIZE(ak4375_hphl_select_texts), ak4375_hphl_select_texts),
	SOC_ENUM_SINGLE(AK4375_04_OUTPUT_MODE_SETTING, 1,
			ARRAY_SIZE(ak4375_hphr_select_texts), ak4375_hphr_select_texts),
	SOC_ENUM_SINGLE(AK4375_06_DIGITAL_FILTER_SELECT, 6,
			ARRAY_SIZE(ak4375_dacfil_select_texts), ak4375_dacfil_select_texts),
	SOC_ENUM_SINGLE(AK4375_09_JITTER_CLEANER_SETTING2, 4,
			ARRAY_SIZE(ak4375_srcfil_select_texts), ak4375_srcfil_select_texts),
};

static const char * const bickfreq_on_select[] = {"32fs", "48fs", "64fs"};

static const char * const srcoutfs_on_select[] =
#ifdef SRC_OUT_FS_48K
	{"48kHz", "96kHz", "192kHz"};
#else
	{"44.1kHz", "88.2kHz", "176.4kHz"};
#endif

static const char * const pllmode_on_select[] = {"OFF", "ON"};
static const char * const smtcycle_on_select[] = {"1024", "2048", "4096", "8192"};
static const char * const dfsrc8fs_on_select[] = {"Digital Filter", "Bypass", "8fs mode"};

static const struct soc_enum ak4375_bitset_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(bickfreq_on_select), bickfreq_on_select),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(srcoutfs_on_select), srcoutfs_on_select),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(pllmode_on_select), pllmode_on_select),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(smtcycle_on_select), smtcycle_on_select),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(dfsrc8fs_on_select), dfsrc8fs_on_select),
};

static int ak4375_writeMask(struct snd_soc_codec *, u16, u16, u16);
static inline u32 ak4375_read_reg_cache(struct snd_soc_codec *, u16);
#ifdef AK4375_CONTIF_DEBUG
static unsigned int ak4375_i2c_read(struct snd_soc_codec *codec, unsigned int reg);
static int ak4375_i2c_write(struct snd_soc_codec *codec, unsigned int reg, unsigned int value);
#endif
static int get_bickfs(
struct snd_kcontrol	   *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct ak4375_priv *ak4375 = snd_soc_codec_get_drvdata(codec);

	ucontrol->value.enumerated.item[0] = ak4375->nBickFreq;

	return 0;
}

static int ak4375_set_bickfs(struct snd_soc_codec *codec)
{
	struct ak4375_priv *ak4375 = snd_soc_codec_get_drvdata(codec);

	if (ak4375->nBickFreq == 0) {     /* 32fs */
		snd_soc_update_bits(codec, AK4375_15_AUDIO_IF_FORMAT, 0x03, 0x01);	/* DL1-0=01(16bit, >=32fs) */
	} else if (ak4375->nBickFreq == 1) {    /* 48fs */
		snd_soc_update_bits(codec, AK4375_15_AUDIO_IF_FORMAT, 0x02, 0x02);	/* DL1-0=00(24bit, >=64fs) */
	} else {                               /* 64fs */
		snd_soc_update_bits(codec, AK4375_15_AUDIO_IF_FORMAT, 0x02, 0x02);	/* DL1-0=1x(32bit, >=64fs) */
	}

	return 0;
}

static int set_bickfs(
struct snd_kcontrol	   *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct ak4375_priv *ak4375 = snd_soc_codec_get_drvdata(codec);

	ak4375->nBickFreq = ucontrol->value.enumerated.item[0];
	pr_info("[LHS] %s ak4375->nBickFreq=%d\n", __func__, ak4375->nBickFreq);
	ak4375_set_bickfs(codec);

	return 0;
}

static int get_srcfs(
struct snd_kcontrol	   *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct ak4375_priv *ak4375 = snd_soc_codec_get_drvdata(codec);

	ucontrol->value.enumerated.item[0] = ak4375->nSrcOutFsSel;

	return 0;
}

static int set_srcfs(
struct snd_kcontrol	   *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct ak4375_priv *ak4375 = snd_soc_codec_get_drvdata(codec);

	ak4375->nSrcOutFsSel = ucontrol->value.enumerated.item[0];

	return 0;
}

/* static int get_smtcycle(
struct snd_kcontrol	   *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct ak4375_priv *ak4375 = snd_soc_codec_get_drvdata(codec);

	ucontrol->value.enumerated.item[0] = ak4375->nSmt;

	return 0;
}

static int set_smtcycle(
struct snd_kcontrol	   *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct ak4375_priv *ak4375 = snd_soc_codec_get_drvdata(codec);

	ak4375->nSmt = ucontrol->value.enumerated.item[0];

	 *0:1024, 1:2048, 2:4096, 3:8192
	ak4375_writeMask(codec, AK4375_09_JITTER_CLEANER_SETTING2, 0x0C, ((ak4375->nSmt) << 2));

	return 0;
} */

static int get_dfsrc8fs(
struct snd_kcontrol	   *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct ak4375_priv *ak4375 = snd_soc_codec_get_drvdata(codec);

	ucontrol->value.enumerated.item[0] = ak4375->dfsrc8fs;

	return 0;
}

static int ak4375_ldo_en(struct device *dev)
{
	int ret;

	AK4375_LDO_GPIO = of_get_named_gpio(dev->of_node, "akm,cdc-ldo-gpio", 0);
	pr_info("%s: AK4375_LDO_GPIO : %d\n", __func__, AK4375_LDO_GPIO);
	if (AK4375_LDO_GPIO < 0) {
		pr_err("Looking up AK4375_LDO_GPIO failed\n");
	}

	if (AK4375_LDO_GPIO) {
		ret = gpio_request(AK4375_LDO_GPIO, "ldo-en");
		if (ret < 0)
			pr_err("%s:Failed to request gpio AK4375_LDO_GPIO %d\n", __func__,
				AK4375_LDO_GPIO);
	}

	AK4375_RESET_GPIO = of_get_named_gpio(dev->of_node, "akm,cdc-reset-gpio", 0);
	pr_info("%s: AK4375_RESET_GPIO : %d\n", __func__, AK4375_RESET_GPIO);

	if (AK4375_RESET_GPIO < 0) {
		pr_err("Looking up %s property in node failed %d\n",
			"akm,cdc-reset-gpio", AK4375_RESET_GPIO);
	}

	if (AK4375_RESET_GPIO) {
		ret = gpio_request(AK4375_RESET_GPIO, "akm_reset_gpio");
		if (ret < 0) {
			pr_err("%s:Failed to request gpio AK4375_RESET_GPIO %d\n", __func__,
				AK4375_RESET_GPIO);
		}
	}

	if (AK4375_LDO_GPIO) {
		ret = gpio_direction_output(AK4375_LDO_GPIO, 1);
		if (ret < 0)
			pr_err("Failed to output gpio AK4375_LDO_GPIO to 1");
		msleep(30);
	}

	if (AK4375_RESET_GPIO) {
		ret = gpio_direction_output(AK4375_RESET_GPIO, 0);
		if (ret < 0)
			pr_err("Failed to output gpio AK4375_RESET_GPIO  0");
		msleep(20);
		ret = gpio_direction_output(AK4375_RESET_GPIO, 1);
		if (ret < 0)
			pr_err("Failed to output gpio AK4375_RESET_GPIO  1");
		msleep(20);
	}

	pr_debug("%s : exit\n", __func__);
	return 0;
}

static int ak4375_set_dfsrc8fs(struct snd_soc_codec *codec)
{
	struct ak4375_priv *ak4375 = snd_soc_codec_get_drvdata(codec);

	switch (ak4375->dfsrc8fs) {
	case 0:    /* DAC Filter */
		snd_soc_update_bits(codec, AK4375_06_DIGITAL_FILTER_SELECT, 0x08, 0x00);    /* DFTHR=0 */
		snd_soc_update_bits(codec, AK4375_0A_JITTER_CLEANER_SETTING3, 0x20, 0x00);  /* SRCO8FS=0 */
		break;
	case 1:    /* Bypass */
		snd_soc_update_bits(codec, AK4375_06_DIGITAL_FILTER_SELECT, 0x08, 0x08);    /* DFTHR=1 */
		snd_soc_update_bits(codec, AK4375_0A_JITTER_CLEANER_SETTING3, 0x20, 0x00);  /* SRCO8FS=0 */
		break;
	case 2:    /* 8fs mode */
		snd_soc_update_bits(codec, AK4375_06_DIGITAL_FILTER_SELECT, 0x08, 0x08);    /* DFTHR=1 */
		snd_soc_update_bits(codec, AK4375_0A_JITTER_CLEANER_SETTING3, 0x20, 0x20);  /* SRCO8FS=1 */
		break;
	}
	return 0;
}

static int set_dfsrc8fs(
struct snd_kcontrol	   *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	struct ak4375_priv *ak4375 = snd_soc_codec_get_drvdata(codec);

	ak4375->dfsrc8fs = ucontrol->value.enumerated.item[0];

	ak4375_set_dfsrc8fs(codec);

	return 0;
}

#ifdef AK4375_DEBUG
static const char * const test_reg_select[] = {
	"read AK4375 Reg 00:24",
};

static const struct soc_enum ak4375_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(test_reg_select), test_reg_select),
};

static int nTestRegNo = 0;

static int get_test_reg(
struct snd_kcontrol	   *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
	/* Get the current output routing */
	ucontrol->value.enumerated.item[0] = nTestRegNo;

	return 0;
}

static int set_test_reg(
struct snd_kcontrol	   *kcontrol,
struct snd_ctl_elem_value  *ucontrol)
{
	struct snd_soc_codec *codec = snd_soc_kcontrol_codec(kcontrol);
	u32	currMode = ucontrol->value.enumerated.item[0];
	int	i, value;
	int	   regs, rege;

	nTestRegNo = currMode;

	regs = 0x00;
	rege = 0x15;

	for (i = regs ; i <= rege ; i++) {
		value = snd_soc_read(codec, i);
		pr_info("[LHS]***AK4375 Addr,Reg=(%x, %x)\n", i, value);
	}
	value = snd_soc_read(codec, 0x24);
	pr_info("***AK4375 Addr,Reg=(%x, %x)\n", 0x24, value);

	return 0;
}
#endif

static const struct snd_kcontrol_new ak4375_snd_controls[] = {
	SOC_SINGLE_TLV("AK4375 Digital Output VolumeL",
			AK4375_0B_LCH_OUTPUT_VOLUME, 0, 0x1F, 0, ovl_tlv),
	SOC_SINGLE_TLV("AK4375 Digital Output VolumeR",
			AK4375_0C_RCH_OUTPUT_VOLUME, 0, 0x1F, 0, ovr_tlv),
	SOC_SINGLE_TLV("AK4375 HP-Amp Analog Volume",
			AK4375_0D_HP_VOLUME_CONTROL, 0, 0x1F, 0, hpg_tlv),

	SOC_ENUM("AK4375 Digital Volume Control", ak4375_dac_enum[0]),
	SOC_ENUM("AK4375 DACL Signal Level", ak4375_dac_enum[1]),
	SOC_ENUM("AK4375 DACR Signal Level", ak4375_dac_enum[2]),
	SOC_ENUM("AK4375 DACL Signal Invert", ak4375_dac_enum[3]),
	SOC_ENUM("AK4375 DACR Signal Invert", ak4375_dac_enum[4]),
	SOC_ENUM("AK4375 Charge Pump Mode", ak4375_dac_enum[5]),
	SOC_ENUM("AK4375 HPL Power-down Resistor", ak4375_dac_enum[6]),
	SOC_ENUM("AK4375 HPR Power-down Resistor", ak4375_dac_enum[7]),
	SOC_ENUM("AK4375 DAC Digital Filter Mode", ak4375_dac_enum[8]),
	SOC_ENUM("AK4375 SRC Digital Filter Mode", ak4375_dac_enum[9]),

	SOC_ENUM_EXT("AK4375 Data Output mode", ak4375_bitset_enum[4], get_dfsrc8fs, set_dfsrc8fs),
	SOC_ENUM_EXT("AK4375 BICK Frequency Select", ak4375_bitset_enum[0], get_bickfs, set_bickfs),
	SOC_ENUM_EXT("AK4375 SRC Output FS", ak4375_bitset_enum[1], get_srcfs, set_srcfs),
	/* SOC_ENUM_EXT("AK4375 Soft Mute Cycle Select", ak4375_bitset_enum[3], get_smtcycle, set_smtcycle), */

	SOC_SINGLE("AK4375 SRC Semi-Auto Mode", AK4375_09_JITTER_CLEANER_SETTING2, 1, 1, 0),
	SOC_SINGLE("AK4375 SRC Dither", AK4375_0A_JITTER_CLEANER_SETTING3, 4, 1, 0),
	SOC_SINGLE("AK4375 Soft Mute Control", AK4375_09_JITTER_CLEANER_SETTING2, 0, 1, 0),

#ifdef AK4375_DEBUG
	SOC_ENUM_EXT("Reg Read", ak4375_enum[0], get_test_reg, set_test_reg),
#endif
};

/* DAC control */
static int ak4375_dac_event2(struct snd_soc_codec *codec, int event)
{
	pr_info("[LHS]%s event=%d\n", __func__, event);
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:	/* before widget power up */
		snd_soc_update_bits(codec, AK4375_00_POWER_MANAGEMENT1, 0x01, 0x01);/* PMPLL=1 */
		snd_soc_update_bits(codec, AK4375_01_POWER_MANAGEMENT2, 0x01, 0x01);/* PMCP1=1 */
		msleep(20);
		usleep_range(400, 500);
		snd_soc_update_bits(codec, AK4375_01_POWER_MANAGEMENT2, 0x30, 0x30);/* PMLDO1P/N=1 */
		usleep_range(900, 1000);
		break;
	case SND_SOC_DAPM_POST_PMU:	/* after widget power up */
		snd_soc_update_bits(codec, AK4375_01_POWER_MANAGEMENT2, 0x02, 0x02);/* PMCP2=1 */
		msleep(20);
		break;
	case SND_SOC_DAPM_PRE_PMD:	/* before widget power down */
		snd_soc_update_bits(codec, AK4375_01_POWER_MANAGEMENT2, 0x02, 0x00);/* PMCP2=0 */
		break;
	case SND_SOC_DAPM_POST_PMD:	/* after widget power down */
		snd_soc_update_bits(codec, AK4375_01_POWER_MANAGEMENT2, 0x30, 0x00);/* PMLDO1P/N=0 */
		snd_soc_update_bits(codec, AK4375_01_POWER_MANAGEMENT2, 0x01, 0x00);/* PMCP1=0 */
		ak4375_writeMask(codec, AK4375_00_POWER_MANAGEMENT1, 0x01, 0x00);/* PMPLL=1 */
		/* if (ak4375_data->nPllMode==1) ak4375_set_PLL_MCKI(codec, 0); */
		break;
	}
	return 0;
}

static int ak4375_dac_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event) /* CONFIG_LINF */
{
	struct snd_soc_codec *codec = w->codec;

	akdbgprt("\t[LHS][AK4375] %s(%d)\n", __func__, __LINE__);
	ak4375_dac_event2(codec, event);

	return 0;
}

/* DAC MUX */
static const char * const ak4375_seldain_select_texts[] = {"SDTI", "SRC"};

static const struct soc_enum ak4375_seldain_mux_enum =
	SOC_ENUM_SINGLE(AK4375_0A_JITTER_CLEANER_SETTING3, 1,
			ARRAY_SIZE(ak4375_seldain_select_texts), ak4375_seldain_select_texts);

static const struct snd_kcontrol_new ak4375_seldain_mux_control =
	SOC_DAPM_ENUM("SRC Select", ak4375_seldain_mux_enum);

/* HPL Mixer */
static const struct snd_kcontrol_new ak4375_hpl_mixer_controls[] = {
	SOC_DAPM_SINGLE("LDACL", AK4375_07_DAC_MONO_MIXING, 0, 1, 0),
	SOC_DAPM_SINGLE("RDACL", AK4375_07_DAC_MONO_MIXING, 1, 1, 0),
};

/* HPR Mixer */
static const struct snd_kcontrol_new ak4375_hpr_mixer_controls[] = {
	SOC_DAPM_SINGLE("LDACR", AK4375_07_DAC_MONO_MIXING, 4, 1, 0),
	SOC_DAPM_SINGLE("RDACR", AK4375_07_DAC_MONO_MIXING, 5, 1, 0),
};


/* ak4375 dapm widgets */
static const struct snd_soc_dapm_widget ak4375_dapm_widgets[] = {
/* DAC */
	SND_SOC_DAPM_DAC_E("AK4375 DAC", "NULL", AK4375_02_POWER_MANAGEMENT3, 0, 0,
			ak4375_dac_event, (SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD
							| SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)),

#ifdef PLL_BICK_MODE
	SND_SOC_DAPM_SUPPLY("AK4375 PLL", AK4375_00_POWER_MANAGEMENT1, 0, 0, NULL, 0),
#endif
	SND_SOC_DAPM_SUPPLY("AK4375 OSC", AK4375_00_POWER_MANAGEMENT1, 4, 0, NULL, 0),

	SND_SOC_DAPM_MUX("AK4375 DAC MUX", SND_SOC_NOPM, 0, 0, &ak4375_seldain_mux_control),

	SND_SOC_DAPM_AIF_IN("AK4375 SRC", "Playback", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_IN("AK4375 SDTI", "Playback", 0, SND_SOC_NOPM, 0, 0),

/* Analog Output */
	SND_SOC_DAPM_OUTPUT("AK4375 HPL"),
	SND_SOC_DAPM_OUTPUT("AK4375 HPR"),

	SND_SOC_DAPM_MIXER("AK4375 HPR Mixer", AK4375_03_POWER_MANAGEMENT4, 1, 0,
			&ak4375_hpr_mixer_controls[0], ARRAY_SIZE(ak4375_hpr_mixer_controls)),

	SND_SOC_DAPM_MIXER("AK4375 HPL Mixer", AK4375_03_POWER_MANAGEMENT4, 0, 0,
			&ak4375_hpl_mixer_controls[0], ARRAY_SIZE(ak4375_hpl_mixer_controls)),
};

static const struct snd_soc_dapm_route ak4375_intercon[] = {
#ifdef PLL_BICK_MODE
	{"AK4375 DAC", "NULL", "AK4375 PLL"},
#endif
	{"AK4375 SRC", "NULL", "AK4375 OSC"},
	{"AK4375 DAC MUX", "SRC", "AK4375 SRC"},
	{"AK4375 DAC MUX", "SDTI", "AK4375 SDTI"},
	{"AK4375 DAC", "NULL", "AK4375 DAC MUX"},

	{"AK4375 HPL Mixer", "LDACL", "AK4375 DAC"},
	{"AK4375 HPL Mixer", "RDACL", "AK4375 DAC"},
	{"AK4375 HPR Mixer", "LDACR", "AK4375 DAC"},
	{"AK4375 HPR Mixer", "RDACR", "AK4375 DAC"},

	{"AK4375 HPL", "NULL", "AK4375 HPL Mixer"},
	{"AK4375 HPR", "NULL", "AK4375 HPR Mixer"},
};

static int ak4375_set_mcki(struct snd_soc_codec *codec, int fs, int rclk)
{
	u8 mode;
	u8 mode2;
	int mcki_rate;

	akdbgprt("\t[AK4375] %s fs=%d rclk=%d\n", __func__, fs, rclk);

	if ((fs != 0) && (rclk != 0)) {
		if (rclk > 28800000)
			return -EINVAL;
		mcki_rate = rclk/fs;

		mode = snd_soc_read(codec, AK4375_05_CLOCK_MODE_SELECT);
		mode &= ~AK4375_CM;

		if (ak4375_data->nSeldain == 0) {  /* SRC Bypass Mode */
			switch (mcki_rate) {
			case 128:
				mode |= AK4375_CM_3;
				break;
			case 256:
				mode |= AK4375_CM_0;
				mode2 = snd_soc_read(codec, AK4375_24_MODE_CONTROL);
				if (fs <= 12000) {
					mode2 &= 0x40;  /* DSMLP=1 */
					snd_soc_write(codec, AK4375_24_MODE_CONTROL, mode2);
				} else {
					mode2 &= ~0x40; /* DSMLP=0 */
					snd_soc_write(codec, AK4375_24_MODE_CONTROL, mode2);
				}
				break;
			case 512:
				mode |= AK4375_CM_1;
				break;
			case 1024:
				mode |= AK4375_CM_2;
				break;
			default:
				return -EINVAL;
			}
		} else {    /* SRC Mode */
			switch (mcki_rate) {
			case 256:
				mode |= AK4375_CM_0;
				break;
			case 512:
				mode |= AK4375_CM_1;
				break;
			case 1024:
				mode |= AK4375_CM_2;
				break;
			default:
				return -EINVAL;
			}
		}
		snd_soc_write(codec, AK4375_05_CLOCK_MODE_SELECT, mode);
	}

	return 0;
}

static int ak4375_set_src_mcki(struct snd_soc_codec *codec, int fs)
{
	u8 nrate;
	int oclk_rate;
	int src_out_fs;

	nrate = snd_soc_read(codec, AK4375_08_JITTER_CLEANER_SETTING1);
	nrate &= ~0x7F; /* CM21-0 bits, FS24-0 bits */

#ifdef SRC_OUT_FS_48K
	src_out_fs = 48000 * (1 << (ak4375_data->nSrcOutFsSel));
#else
	src_out_fs = 44100 * (1 << (ak4375_data->nSrcOutFsSel));
#endif
	switch (src_out_fs) {
	case 44100:
		nrate |= AK4375_FS_44_1KHZ;
		break;
	case 48000:
		nrate |= AK4375_FS_48KHZ;
		break;
	case 88200:
		nrate |= AK4375_FS_88_2KHZ;
		break;
	case 96000:
		nrate |= AK4375_FS_96KHZ;
		break;
	case 176400:
		nrate |= AK4375_FS_176_4KHZ;
		break;
	case 192000:
		nrate |= AK4375_FS_192KHZ;
		break;
	default:
		return -EINVAL;
	}

	oclk_rate = XTAL_OSC_FS/src_out_fs;
	switch (oclk_rate) {
	case 128:
		nrate |= AK4375_CM_3;
		break;
	case 256:
		nrate |= AK4375_CM_0;
		break;
	case 512:
		nrate |= AK4375_CM_1;
		break;
	case 1024:
		nrate |= AK4375_CM_2;
		break;
	default:
		return -EINVAL;
	}

	ak4375_data->fs2 = src_out_fs;
	snd_soc_write(codec, AK4375_08_JITTER_CLEANER_SETTING1, nrate);

	return 0;
}

static int ak4375_set_pllblock(struct snd_soc_codec *codec, int fs)
{
	u8 mode;
	int nMClk, nPLLClk, nRefClk;
	int PLDbit, PLMbit, MDIVbit, DIVbit;
	int nTemp;

	mode = snd_soc_read(codec, AK4375_05_CLOCK_MODE_SELECT);
	mode &= ~AK4375_CM;

	if (ak4375_data->nSeldain == 0) {    /* SRC Bypass */
		if (fs <= 24000) {
			mode |= AK4375_CM_1;
			nMClk = 512 * fs;
		} else if (fs <= 96000) {
			mode |= AK4375_CM_0;
			nMClk = 256 * fs;
		} else {
			mode |= AK4375_CM_3;
			nMClk = 128 * fs;
		}
	} else {                              /* SRC */
		if (fs <= 24000) {
			mode |= AK4375_CM_1;
			nMClk = 512 * fs;
		} else {
			mode |= AK4375_CM_0;
			nMClk = 256 * fs;
		}
	}
	snd_soc_write(codec, AK4375_05_CLOCK_MODE_SELECT, mode);
	pr_info("LHS ak4375_data->nBickFreq=%d", ak4375_data->nBickFreq);
	if ((fs % 8000) == 0) {
		nPLLClk = 122880000;
	} else if ((fs == 11025) && (ak4375_data->nBickFreq == 1)) {
		nPLLClk = 101606400;
	} else {
		nPLLClk = 112896000;
	}

	if (ak4375_data->nBickFreq == 0) {      /* 32fs */
		if (fs <= 96000)
			PLDbit = 1;
		else
			PLDbit = 2;
		nRefClk = 32 * fs / PLDbit;
	} else {                               /* 64fs */
		if (fs <= 48000)
			PLDbit = 1;
		else if (fs <= 96000)
			PLDbit = 2;
		else
			PLDbit = 4;
		nRefClk = 64 * fs / PLDbit;
	}
	pr_info("LHS nPLLClk=%d nRefClk=%d\n", nPLLClk, nRefClk);
	PLMbit = nPLLClk / nRefClk;
	pr_info("LHS nPLLClk=%d PLMbit=%d\n", nPLLClk, PLMbit);
	if ((ak4375_data->nSeldain == 0) || (fs <= 96000)) {
		MDIVbit = nPLLClk / nMClk;
		DIVbit = 0;
	} else {
		MDIVbit = 5;
		DIVbit = 1;
	}

	PLDbit--;
	PLMbit--;
	MDIVbit--;
	pr_info("LHS PLDbit=%d PLMbit=%d  MDIVbit=%d\n", PLDbit, PLMbit, MDIVbit);
	/* PLD15-0 */
	snd_soc_write(codec, AK4375_0F_PLL_REF_CLK_DIVIDER1, ((PLDbit & 0xFF00) >> 8));
	snd_soc_write(codec, AK4375_10_PLL_REF_CLK_DIVIDER2, ((PLDbit & 0x00FF) >> 0));
	/* PLM15-0 */
	snd_soc_write(codec, AK4375_11_PLL_FB_CLK_DIVIDER1, ((PLMbit & 0xFF00) >> 8));
	snd_soc_write(codec, AK4375_12_PLL_FB_CLK_DIVIDER2, ((PLMbit & 0x00FF) >> 0));
	/* DIVbit */
	nTemp = snd_soc_read(codec, AK4375_13_SRC_CLK_SOURCE);
	nTemp &= ~0x10;
	nTemp |= (DIVbit << 4);
	snd_soc_write(codec, AK4375_13_SRC_CLK_SOURCE, (nTemp|0x01));/* DIV=0or1,SRCCKS=1(PLL) */
	snd_soc_update_bits(codec, AK4375_0E_PLL_CLK_SOURCE_SELECT, 0x01, 0x01);   /* PLS=1(BICK) */

	/* MDIV7-0 */
	snd_soc_write(codec, AK4375_14_DAC_CLK_DIVIDER, MDIVbit);

	return 0;
}

static int ak4375_set_timer(struct snd_soc_codec *codec)
{
	int ret, curdata;
	int count, tm, nfs;
	int lvdtm, vddtm, hptm;

	lvdtm = 0;
	vddtm = 0;
	hptm = 0;

	if (ak4375_data->nSeldain == 1)
	   nfs = ak4375_data->fs2;
	else
	   nfs = ak4375_data->fs1;

	/* LVDTM2-0 bits set */
	ret = snd_soc_read(codec, AK4375_03_POWER_MANAGEMENT4);
	curdata = (ret & 0x70) >> 4;   /* Current data Save */
	ret &= ~0x70;
	do {
	   count = 1000 * (64 << lvdtm);
	   tm = count / nfs;
	   if (tm > LVDTM_HOLD_TIME)
		   break;
	   lvdtm++;
	} while (lvdtm < 7);    /* LVDTM2-0 = 0~7 */
	if (curdata != lvdtm) {
	   snd_soc_write(codec, AK4375_03_POWER_MANAGEMENT4, (ret | (lvdtm << 4)));
	}

	/* VDDTM3-0 bits set */
	ret = snd_soc_read(codec, AK4375_04_OUTPUT_MODE_SETTING);
	curdata = (ret & 0x3C) >> 2;   /* Current data Save */
	ret &= ~0x3C;
	do {
	   count = 1000 * (1024 << vddtm);
	   tm = count / nfs;
	   if (tm > VDDTM_HOLD_TIME)
		   break;
	   vddtm++;
	} while (vddtm < 8);    /* VDDTM3-0 = 0~8 */
	if (curdata != vddtm) {
	   snd_soc_write(codec, AK4375_04_OUTPUT_MODE_SETTING, (ret | (vddtm<<2)));
	}

	/* HPTM2-0 bits set */
	ret = snd_soc_read(codec, AK4375_0D_HP_VOLUME_CONTROL);
	curdata = (ret & 0xE0) >> 5;   /* Current data Save */
	ret &= ~0xE0;
	do {
	   count = 1000 * (128 << hptm);
	   tm = count / nfs;
	   if (tm > HPTM_HOLD_TIME)
		   break;
	   hptm++;
	} while (hptm < 4);			/* HPTM2-0 = 0~4 */
	if (curdata != hptm) {
	   snd_soc_write(codec, AK4375_0D_HP_VOLUME_CONTROL, (ret | (hptm<<5)));
	}

	return 0;
}

static int ak4375_hw_params_set(struct snd_soc_codec *codec, int nfs1)
{
	u8 fs;
	u8 src;

	akdbgprt("\t[LHS AK4375] %s(%d)\n", __func__, __LINE__);

	src = snd_soc_read(codec, AK4375_0A_JITTER_CLEANER_SETTING3);
	src = (src & 0x02) >> 1;

	fs = snd_soc_read(codec, AK4375_05_CLOCK_MODE_SELECT);
	fs &= ~AK4375_FS;

	/* ak4375_data->fs1 = params_rate(params); */
	akdbgprt("\t[LHS ak4375] %s: (nfs1)=(%d)\n", __func__, nfs1);
	switch (nfs1) {
	case 8000:
		fs |= AK4375_FS_8KHZ;
		break;
	case 11025:
		fs |= AK4375_FS_11_025KHZ;
		break;
	case 16000:
		fs |= AK4375_FS_16KHZ;
		break;
	case 22050:
		fs |= AK4375_FS_22_05KHZ;
		break;
	case 32000:
		fs |= AK4375_FS_32KHZ;
		break;
	case 44100:
		fs |= AK4375_FS_44_1KHZ;
		break;
	case 48000:
		fs |= AK4375_FS_48KHZ;
		break;
	case 88200:
		fs |= AK4375_FS_88_2KHZ;
		break;
	case 96000:
		fs |= AK4375_FS_96KHZ;
		break;
	case 176400:
		fs |= AK4375_FS_176_4KHZ;
		break;
	case 192000:
		fs |= AK4375_FS_192KHZ;
		break;
	default:
		return -EINVAL;
	}
	snd_soc_write(codec, AK4375_05_CLOCK_MODE_SELECT, fs);

	if (ak4375_data->nPllMode == 0) { /* Not PLL mode */
		ak4375_set_mcki(codec, nfs1, ak4375_data->rclk);
	} else {           /* PLL mode */
		ak4375_set_pllblock(codec, nfs1);
	}

	if (src == 1) {    /* SRC mode */
		ak4375_data->nSeldain = 1;
		/* XCKSEL=XCKCPSEL=SELDAIN=1 */
		snd_soc_update_bits(codec, AK4375_0A_JITTER_CLEANER_SETTING3, 0xC2, 0xC2);
		ak4375_set_src_mcki(codec, nfs1);
	} else {   /* SRC Bypass mode */
		ak4375_data->nSeldain = 0;
		/* XCKSEL=XCKCPSEL=SELDAIN=0 */
		snd_soc_update_bits(codec, AK4375_0A_JITTER_CLEANER_SETTING3, 0xC2, 0x00);
	}

	ak4375_set_timer(codec);

	return 0;
}

static int ak4375_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params,
		struct snd_soc_dai *dai)
{
	int format;
	struct snd_soc_codec *codec = dai->codec;

	ak4375_data->fs1 = params_rate(params);
	format = params_format(params);

	akdbgprt("\t[AK4375]LHS %s:format=(%d)\n", __func__, format);
	if (format == SNDRV_PCM_FORMAT_S16_LE) {
		snd_soc_update_bits(codec, AK4375_15_AUDIO_IF_FORMAT, 0x03, 0x01);
		ak4375_data->nBickFreq = 0;
		akdbgprt("\t[AK4375]LHS %s:format= 16bit\n", __func__);
	} else if (format == SNDRV_PCM_FORMAT_S24_LE) {
		snd_soc_update_bits(codec, AK4375_15_AUDIO_IF_FORMAT, 0x02, 0x02);
		ak4375_data->nBickFreq = 1;
		akdbgprt("\t[AK4375]LHS %s:format= 24bit\n", __func__);
	} else {
		snd_soc_update_bits(codec, AK4375_15_AUDIO_IF_FORMAT, 0x02, 0x02);
		ak4375_data->nBickFreq = 2;
		akdbgprt("\t[AK4375]LHS %s:format= 32bit\n", __func__);
	}

	ak4375_hw_params_set(codec, ak4375_data->fs1);

	return 0;
}

static int ak4375_set_dai_sysclk(struct snd_soc_dai *dai, int clk_id,
		unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = dai->codec;

	akdbgprt("\t[AK4375] %s(%d)\n", __func__, __LINE__);

	ak4375_data->rclk = freq;

	if (ak4375_data->nPllMode == 0) { /* Not PLL mode */
		ak4375_set_mcki(codec, ak4375_data->fs1, freq);
	}

	return 0;
}

static int ak4375_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{

	struct snd_soc_codec *codec = dai->codec;
	u8 format;

	akdbgprt("\t[AK4375] %s(%d)\n", __func__, __LINE__);

	/* set master/slave audio interface */
	format = snd_soc_read(codec, AK4375_15_AUDIO_IF_FORMAT);
	format &= ~AK4375_DIF;

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		format |= AK4375_DIF_I2S_MODE;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		format |= AK4375_DIF_MSB_MODE;
		break;
	default:
		return -EINVAL;
	}

	/* set format */
	snd_soc_write(codec, AK4375_15_AUDIO_IF_FORMAT, format);

	return 0;
}

static int ak4375_volatile(struct snd_soc_codec *codec, unsigned int reg)
{
	int	ret;

	switch (reg) {
	default:
		ret = 0;
		break;
	}
	return ret;
}

static int ak4375_readable(struct snd_soc_codec *codec, unsigned int reg)
{

	if (reg >= ARRAY_SIZE(ak4375_access_masks))
		return 0;
	return ak4375_access_masks[reg].readable != 0;
}

/*
* Read ak4375 register cache
 */
static inline u32 ak4375_read_reg_cache(struct snd_soc_codec *codec, u16 reg)
{
	u8 *cache = codec->reg_cache;

	WARN_ON(reg > ARRAY_SIZE(ak4375_reg));
	return (u32)cache[reg];
}

#ifdef AK4375_CONTIF_DEBUG
/*
 * Write ak4375 register cache
 */
static inline void ak4375_write_reg_cache(
struct snd_soc_codec *codec, u16 reg, u16 value)
{
	u8 *cache = codec->reg_cache;

	WARN_ON(reg > ARRAY_SIZE(ak4375_reg));

	cache[reg] = (u8)value;
}

static unsigned int ak4375_i2c_read(struct snd_soc_codec *codec, unsigned int reg)
{

	unsigned int ret;

	ret = i2c_smbus_read_byte_data(ak4375_data->i2c_client, (u8)(reg & 0xFF));

	if (ret < 0) {
		akdbgprt("\t[AK4375] %s(%d)\n", __func__, __LINE__);
	}
	return ret;
}

static int ak4375_i2c_write(struct snd_soc_codec *codec, unsigned int reg,
	unsigned int value)
{
	ak4375_write_reg_cache(codec, reg, value);

	akdbgprt("\t[ak4375] %s: (addr,data)=(%x, %x)\n", __func__, reg, value);

	if (i2c_smbus_write_byte_data(ak4375_data->i2c_client,
		(u8)(reg & 0xFF), (u8)(value & 0xFF)) < 0) {
		akdbgprt("\t[AK4375] %s(%d)\n", __func__, __LINE__);
		return -EIO;
	}

	return 0;
}
#endif

/*
 * Write with Mask to  AK4375 register space
 */
#ifdef CONFIG_DEBUG_FS_CODEC_AKM
static ssize_t reg_data_show(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	int ret, i, fpt;
	u8 rx[32] = {0};
	int reg_value = 0;

	fpt = 0;
	ret = 0;

	for (i = 0 ; i <= AK4375_15_AUDIO_IF_FORMAT ; i++) {
		reg_value = ak4375_i2c_read(ak4375_codec, i);
		if (reg_value < 0) {
			pr_info("Read AK4375 register error\n");
		}
		pr_info("AK4375 read reg %x, data=%x\n", i, reg_value);
		rx[i] = (u8)reg_value;
	}
	if (i == AK4375_15_AUDIO_IF_FORMAT + 1) {
		for (i = 0; i <= AK4375_15_AUDIO_IF_FORMAT; i++, fpt += 6) {
			ret += snprintf(buf+fpt, 15, "%02x,%02x\n", i, rx[i]);
		}
		return ret;
	} else {
		return snprintf(buf, 12, "read error");
	}
	return 0;
}
static ssize_t reg_data_store(struct device *dev,
					struct device_attribute *attr, const char *buf, size_t count)
{
	char *ptr_data = (char *)buf;
	char  *p;
	int   i, pt_count = 0;
	unsigned short val[20];

	while ((p = strsep(&ptr_data, ","))) {
		if (!*p)
			break;

		if (pt_count >= 20)
			break;

		val[pt_count] = kstrtoul(p, 16, NULL); /* simple_strtoul */
		pt_count++;
	}
	for (i = 0; i < pt_count; i += 2) {
		ak4375_i2c_write(ak4375_codec, val[i], val[i+1]);
		pr_info("AK4375 write add=%d, val=%d.\n", val[i], val[i+1]);
	}
	return count;
}

static DEVICE_ATTR(reg_data, 0644, reg_data_show, reg_data_store);
#endif
static int ak4375_writeMask(struct snd_soc_codec *codec,
					u16 reg, u16 mask, u16 value)
{
	u16 olddata;
	u16 newdata;

	if ((mask == 0) || (mask == 0xFF)) {
		newdata = value;
	} else {
		olddata = ak4375_read_reg_cache(codec, reg);
		newdata = (olddata & ~(mask)) | value;
	}

	snd_soc_write(codec, (unsigned int)reg, (unsigned int)newdata);

	akdbgprt("\t[ak4375_writeMask] %s(%d): (addr,data)=(%x, %x)\n", __func__, __LINE__, reg, newdata);

	return 0;
}

/* for AK4375 */
static int ak4375_trigger(struct snd_pcm_substream *substream, int cmd, struct snd_soc_dai *codec_dai)
{
	int ret = 0;
	/*struct snd_soc_codec *codec = codec_dai->codec; */

	akdbgprt("\t[AK4375] %s(%d)\n", __func__, __LINE__);

	return ret;
}

static int ak4375_set_bias_level(struct snd_soc_codec *codec,
		enum snd_soc_bias_level level)
{
	akdbgprt("\t[AK4375] %s(%d)\n", __func__, __LINE__);

	switch (level) {
	case SND_SOC_BIAS_ON:
	case SND_SOC_BIAS_PREPARE:
	case SND_SOC_BIAS_STANDBY:
		break;
	case SND_SOC_BIAS_OFF:
		break;
	}
	codec->dapm.bias_level = level;

	return 0;
}

static int ak4375_set_dai_mute2(struct snd_soc_codec *codec, int mute)
{
	int ret = 0;
	int nfs, ndt, ndt2;

	if (ak4375_data->nSeldain == 1)
		nfs = ak4375_data->fs2;
	else
		nfs = ak4375_data->fs1;

	akdbgprt("\t[AK4375] %s mute[%s]\n", __func__, mute ? "ON" : "OFF");

	if (mute) { /* SMUTE: 1 , MUTE */
		if (ak4375_data->nSeldain) {
			ret = snd_soc_update_bits(codec, AK4375_09_JITTER_CLEANER_SETTING2, 0x01, 0x01);
			ndt = (1024000 << ak4375_data->nSmt) / nfs;
			msleep(ndt);
			ret = snd_soc_update_bits(codec, AK4375_02_POWER_MANAGEMENT3, 0x80, 0x00);
		}
	} else {    /*  SMUTE: 0 ,NORMAL operation */
		ak4375_data->nSmt = (ak4375_data->nSrcOutFsSel + SMUTE_TIME_MODE);
		ret = snd_soc_update_bits(codec, AK4375_09_JITTER_CLEANER_SETTING2, 0x0C, (ak4375_data->nSmt << 2));
		ndt = (26 * nfs) / 44100;		/* for After HP-Amp Power up */
		if (ak4375_data->nSeldain) {
			ret = snd_soc_update_bits(codec, AK4375_02_POWER_MANAGEMENT3, 0x80, 0x80);
			ndt2 = (1024000 << ak4375_data->nSmt) / nfs;
			ndt -= ndt2;
			if (ndt < 4)
				ndt = 4;
			msleep(ndt);
			ret = snd_soc_update_bits(codec, AK4375_09_JITTER_CLEANER_SETTING2, 0x01, 0x00);
			msleep(ndt2);
		} else {
			msleep(ndt);
		}
	}
	return ret;
}

static int ak4375_set_dai_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_codec *codec = dai->codec;

	ak4375_set_dai_mute2(codec, mute);

	return 0;
}

#define AK4375_RATES (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 |\
				SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 |\
				SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 |\
				SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_88200 |\
				SNDRV_PCM_RATE_96000 | SNDRV_PCM_RATE_176400 |\
				SNDRV_PCM_RATE_192000)

#define AK4375_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)

static struct snd_soc_dai_ops ak4375_dai_ops = {
	.hw_params	= ak4375_hw_params,
	.set_sysclk	= ak4375_set_dai_sysclk,
	.set_fmt	= ak4375_set_dai_fmt,
	.trigger = ak4375_trigger,
	.digital_mute = ak4375_set_dai_mute,
};

struct snd_soc_dai_driver ak4375_dai[] = {
	{
		.name = "ak4375-AIF1",
		.playback = {
			   .stream_name = "Playback",
			   .channels_min = 1,
			   .channels_max = 2,
			   .rates = AK4375_RATES,
			   .formats = AK4375_FORMATS,
		},
		.ops = &ak4375_dai_ops,
	},
};

/*
static int ak4375_write_cache_reg(
struct snd_soc_codec *codec,
u16  regs,
u16  rege)
{
	u32	reg, cache_data;

	reg = regs;
	do {
		cache_data = ak4375_read_reg_cache(codec, reg);
		snd_soc_write(codec, (unsigned int)reg, (unsigned int)cache_data);
		reg ++;
	} while (reg <= rege);

	return 0;
}
*/
static int ak4375_init_reg(struct snd_soc_codec *codec)
{
	usleep_range(750, 850);
	ak4375_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	akdbgprt("\t[AK4375 bias] %s(%d)\n", __func__, __LINE__);

#ifdef PLL_BICK_MODE
		/* SRCCKS=1(SRC Clock Select=PLL) */
		snd_soc_update_bits(codec, AK4375_13_SRC_CLK_SOURCE, 0x01, 0x01);
		/* PLS=1(BICK) */
		snd_soc_update_bits(codec, AK4375_0E_PLL_CLK_SOURCE_SELECT, 0x01, 0x01);
#endif

	return 0;
}

static int ak4375_probe(struct snd_soc_codec *codec)
{
	struct ak4375_priv *ak4375 = snd_soc_codec_get_drvdata(codec);
	struct snd_soc_dapm_context *dapm = &codec->dapm; /* ZTE_ZJB_201019 modify for suspend */
	int ret = 0;

	akdbgprt("\t[AK4375] %s(%d)\n", __func__, __LINE__);

	/*ret = snd_soc_codec_set_cache_io(codec, 8, 8, SND_SOC_I2C);
	* if (ret != 0) {
	*	 dev_err(codec->dev, "Failed to set cache I/O: %d\n", ret);
	*	 return ret;
	* }
	*/

#ifdef AK4375_CONTIF_DEBUG
	/* codec->write = ak4375_i2c_write; */
	/* codec->read = ak4375_i2c_read; */
#endif
#ifdef CONFIG_DEBUG_FS_CODEC_AKM
	ak4375_codec = codec;
#endif

	ak4375_init_reg(codec);

	akdbgprt("\t[AK4375 Effect] %s(%d)\n", __func__, __LINE__);

	snd_soc_add_codec_controls(codec, ak4375_snd_controls,
						ARRAY_SIZE(ak4375_snd_controls));

	/* ZTE_ZJB_201019 modify for suspend */
	snd_soc_dapm_ignore_suspend(dapm, "AK4375 HPL");
	snd_soc_dapm_ignore_suspend(dapm, "AK4375 HPR");
	snd_soc_dapm_sync(dapm);

	ak4375->fs1 = 48000;
	ak4375->fs2 = 48000;
	ak4375->rclk = 0;
	ak4375->nSeldain = 0;		/* 0:Bypass, 1:SRC	(Dependent on a register bit) */
	ak4375->nBickFreq = 0;		/* 0:32fs, 1:48fs, 2:64fs */
	ak4375->nSrcOutFsSel = 0;	/* 0:48(44.1)kHz, 1:96(88.2)kHz, 2:192(176.4)kHz */
#ifdef PLL_BICK_MODE
	ak4375->nPllMode = 1;		/* 1: PLL ON */
#else
	ak4375->nPllMode = 0;		/* 0:PLL OFF */
#endif
	ak4375->nPllMode = 1;		/* 1: PLL ON */
	ak4375->nSmt = 0;			/* 0:1024/FSO, 1:2048/FSO, 2:4096/FSO, 3:8192/FSO */
	ak4375->dfsrc8fs = 0;		/* 0:DAC Filter, 1:Bypass, 2:8fs mode */
#ifdef CONFIG_DEBUG_FS_CODEC_AKM
	ret = device_create_file(codec->dev, &dev_attr_reg_data);
	if (ret) {
		pr_info("AK4375 ERROR to create reg_data\n");
	}
#endif

	pr_info("[LHS AK4375] (success)\n");
	return ret;
}

static int ak4375_remove(struct snd_soc_codec *codec)
{

	akdbgprt("\t[AK4375] %s(%d)\n", __func__, __LINE__);

	ak4375_set_bias_level(codec, SND_SOC_BIAS_OFF);
#ifdef CONFIG_DEBUG_FS_CODEC_AKM
	device_remove_file(codec->dev, &dev_attr_reg_data);
#endif

	return 0;
}

/* static int ak4375_suspend(struct snd_soc_codec *codec, pm_message_t state) */
static int ak4375_suspend(struct snd_soc_codec *codec)

{

	ak4375_set_bias_level(codec, SND_SOC_BIAS_OFF);

	if (AK4375_RESET_GPIO) {
		gpio_direction_output(AK4375_RESET_GPIO, 0);
		akdbgprt("\t[AK4375]%s  AK4375_RESET_GPIO=0 LINE(%d)\n", __func__, __LINE__);
		msleep(20);
	}
	return 0;
}

static int ak4375_resume(struct snd_soc_codec *codec)
{
	if (AK4375_RESET_GPIO) {
		gpio_direction_output(AK4375_RESET_GPIO, 1);
		akdbgprt("\t[AK4375]%s  AK4375_RESET_GPIO=1 LINE(%d)\n", __func__, __LINE__);
		msleep(20);
	}

	ak4375_init_reg(codec);

	return 0;
}


struct snd_soc_codec_driver soc_codec_dev_ak4375 = {
	.probe = ak4375_probe,
	.remove = ak4375_remove,
	.suspend =	ak4375_suspend,
	.resume =	ak4375_resume,
	.read = ak4375_i2c_read,
	.write = ak4375_i2c_write,
	.set_bias_level = ak4375_set_bias_level,
	.reg_cache_size = ARRAY_SIZE(ak4375_reg),
	.reg_word_size = sizeof(u8),
	.reg_cache_default = ak4375_reg,
	.readable_register = ak4375_readable,
	.volatile_register = ak4375_volatile,
	.dapm_widgets = ak4375_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(ak4375_dapm_widgets),
	.dapm_routes = ak4375_intercon,
	.num_dapm_routes = ARRAY_SIZE(ak4375_intercon),
};
EXPORT_SYMBOL_GPL(soc_codec_dev_ak4375);

static const struct regmap_config ak4375_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,

	.max_register = AK4375_MAX_REGISTERS,
	.reg_defaults = ak4375_reg_defaults,
	.num_reg_defaults = ARRAY_SIZE(ak4375_reg_defaults),
	.cache_type = REGCACHE_RBTREE,
};
static int ak4375_i2c_probe(struct i2c_client *i2c,
							const struct i2c_device_id *id)
{
	struct ak4375_priv *ak4375;
	int ret = 0;

	akdbgprt("\t[AK4375] %s(%d)\n", __func__, __LINE__);

	ak4375 = kzalloc(sizeof(struct ak4375_priv), GFP_KERNEL);
	if (ak4375 == NULL)
		return -ENOMEM;

	ak4375->regmap = devm_regmap_init_i2c(i2c, &ak4375_regmap_config);
	if (IS_ERR(ak4375->regmap)) {
		pr_err("[LHS] %s regmap failed!\n", __func__);
		return PTR_ERR(ak4375->regmap);
	}

	i2c_set_clientdata(i2c, ak4375);
	ak4375->i2c_client = i2c;
	ak4375_data = ak4375;

	dev_set_name(&i2c->dev, "ak4375");
	ak4375_ldo_en(&i2c->dev);

	ret = snd_soc_register_codec(&i2c->dev,
			&soc_codec_dev_ak4375, &ak4375_dai[0], ARRAY_SIZE(ak4375_dai));
	if (ret < 0) {
		kfree(ak4375);
		akdbgprt("\t[AK4375 Error!] %s(%d)\n", __func__, __LINE__);
	}
	return ret;
}

static int ak4375_i2c_remove(struct i2c_client *client)
{
	snd_soc_unregister_codec(&client->dev);
	kfree(i2c_get_clientdata(client));
	return 0;
}

static const struct i2c_device_id ak4375_i2c_id[] = {
	{ "ak4375", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ak4375_i2c_id);

static struct i2c_driver ak4375_i2c_driver = {
	.driver = {
		.name = "ak4375",
		.owner = THIS_MODULE,
	},
	.probe = ak4375_i2c_probe,
	.remove = ak4375_i2c_remove,
	.id_table = ak4375_i2c_id,
};

static int __init ak4375_modinit(void)
{

	akdbgprt("\t[AK4375] %s(%d)\n", __func__, __LINE__);

	return i2c_add_driver(&ak4375_i2c_driver);
}

module_init(ak4375_modinit);

static void __exit ak4375_exit(void)
{
	i2c_del_driver(&ak4375_i2c_driver);
}
module_exit(ak4375_exit);

MODULE_DESCRIPTION("ASoC ak4375 codec driver");
MODULE_LICENSE("GPL");
