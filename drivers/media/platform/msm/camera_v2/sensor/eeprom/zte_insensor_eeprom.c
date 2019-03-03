#include "msm_sensor.h"
#include "msm_eeprom.h"


#define GROUP1_LSC_PAGE3 17
#define GROUP1_LSC_PAGE 64
#define GROUP1_LSC_PAGE9 23
#define GROUP2_LSC_PAGE9 40
#define GROUP2_LSC_PAGE 64
#define GAIN_DEFAULT       256

#define Golden_RG   631
#define Golden_BG   631

/* Logging macro */
#undef CDBG
#define CDBG(fmt, args...) pr_debug(fmt, ##args)

/*
* Add otp for s5k4h8 by lijianjun
*/
void  s5k4h8_sensor_read_wb(struct msm_sensor_ctrl_t *e_ctrl, uint16_t start_address)
{
	uint16_t r_gain, b_gain;

	e_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read(e_ctrl->sensor_i2c_client,
		start_address+1,
		&(e_ctrl->otp_ptr.module_id),
		MSM_CAMERA_I2C_BYTE_DATA);
	CDBG(" e_ctrl->otp_ptr.module_id = %d\n", e_ctrl->otp_ptr.module_id);
	e_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read(e_ctrl->sensor_i2c_client,
		start_address+2,
		&(e_ctrl->otp_ptr.lens_id),
		MSM_CAMERA_I2C_BYTE_DATA);
	CDBG(" e_ctrl->otp_ptr.lens_id = %d\n", e_ctrl->otp_ptr.lens_id);
	e_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read(e_ctrl->sensor_i2c_client,
		start_address+5,
		&(e_ctrl->otp_ptr.production_year),
		MSM_CAMERA_I2C_BYTE_DATA);
	CDBG(" e_ctrl->otp_ptr.production_year = %d\n", e_ctrl->otp_ptr.production_year);
	e_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read(e_ctrl->sensor_i2c_client,
		start_address+6,
		&(e_ctrl->otp_ptr.production_month),
		MSM_CAMERA_I2C_BYTE_DATA);
	CDBG(" e_ctrl->otp_ptr.production_month = %d\n", e_ctrl->otp_ptr.production_month);
	e_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read(e_ctrl->sensor_i2c_client,
		start_address+7,
		&(e_ctrl->otp_ptr.production_day),
		MSM_CAMERA_I2C_BYTE_DATA);
	CDBG(" e_ctrl->otp_ptr.production_day = %d\n", e_ctrl->otp_ptr.production_day);
	e_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read_seq(e_ctrl->sensor_i2c_client,
		start_address+8,
		(uint8_t *)&(e_ctrl->otp_ptr.awb),
		sizeof(e_ctrl->otp_ptr.awb));
	r_gain = (e_ctrl->otp_ptr.awb.r_over_gr_l)|(e_ctrl->otp_ptr.awb.r_over_gr_h<<8);
	b_gain = (e_ctrl->otp_ptr.awb.b_over_gb_l)|(e_ctrl->otp_ptr.awb.b_over_gb_h<<8);
	CDBG(" r_gain = 0x%x, b_gain = 0x%x\n", r_gain, b_gain);
}

void  s5k4h8_sensor_read_lsc(struct msm_sensor_ctrl_t *e_ctrl, uint16_t flag_address)
{
	if (flag_address == 0x0A3E) {
		msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x0A00, 0x00, MSM_CAMERA_I2C_BYTE_DATA);
		udelay(2000);
		msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x0A02, 0x03, MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x0A00, 0x01, MSM_CAMERA_I2C_BYTE_DATA);
		udelay(2000);
		e_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read_seq(e_ctrl->sensor_i2c_client,
		0x0A33,
		e_ctrl->otp_ptr.lsc,
		GROUP1_LSC_PAGE3);
		msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x0A00, 0x00, MSM_CAMERA_I2C_BYTE_DATA);
		udelay(2000);
		msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x0A02, 0x04, MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x0A00, 0x01, MSM_CAMERA_I2C_BYTE_DATA);
		udelay(2000);
		e_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read_seq(e_ctrl->sensor_i2c_client,
		0x0A04,
		e_ctrl->otp_ptr.lsc+GROUP1_LSC_PAGE3,
		GROUP1_LSC_PAGE);
		msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x0A00, 0x00, MSM_CAMERA_I2C_BYTE_DATA);
		udelay(2000);
		msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x0A02, 0x05, MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x0A00, 0x01, MSM_CAMERA_I2C_BYTE_DATA);
		udelay(2000);
		e_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read_seq(e_ctrl->sensor_i2c_client,
		0x0A04,
		e_ctrl->otp_ptr.lsc+GROUP1_LSC_PAGE3+GROUP1_LSC_PAGE,
		GROUP1_LSC_PAGE);
		msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x0A00, 0x00, MSM_CAMERA_I2C_BYTE_DATA);
		udelay(2000);
		msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x0A02, 0x06, MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x0A00, 0x01, MSM_CAMERA_I2C_BYTE_DATA);
		udelay(2000);
		e_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read_seq(e_ctrl->sensor_i2c_client,
		0x0A04,
		e_ctrl->otp_ptr.lsc+GROUP1_LSC_PAGE3+GROUP1_LSC_PAGE*2,
		GROUP1_LSC_PAGE);
		msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x0A00, 0x00, MSM_CAMERA_I2C_BYTE_DATA);
		udelay(2000);
		msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x0A02, 0x07, MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x0A00, 0x01, MSM_CAMERA_I2C_BYTE_DATA);
		udelay(2000);
		e_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read_seq(e_ctrl->sensor_i2c_client,
		0x0A04,
		e_ctrl->otp_ptr.lsc+GROUP1_LSC_PAGE3+GROUP1_LSC_PAGE*3,
		GROUP1_LSC_PAGE);
		msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x0A00, 0x00, MSM_CAMERA_I2C_BYTE_DATA);
		udelay(2000);
		msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x0A02, 0x08, MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x0A00, 0x01, MSM_CAMERA_I2C_BYTE_DATA);
		udelay(2000);
		e_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read_seq(e_ctrl->sensor_i2c_client,
		0x0A04,
		e_ctrl->otp_ptr.lsc+GROUP1_LSC_PAGE3+GROUP1_LSC_PAGE*4,
		GROUP1_LSC_PAGE);
		/*
		*disable NVM controller
		*/
		msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x0A00, 0x00, MSM_CAMERA_I2C_BYTE_DATA);
		udelay(2000);
		msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x0A02, 0x09, MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x0A00, 0x01, MSM_CAMERA_I2C_BYTE_DATA);
		udelay(2000);
		e_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read_seq(e_ctrl->sensor_i2c_client,
		0x0A04,
		e_ctrl->otp_ptr.lsc+GROUP1_LSC_PAGE3+GROUP1_LSC_PAGE*5,
		GROUP1_LSC_PAGE9);
	} else if (flag_address == 0x0A41) {
	}
}

int32_t s5k4h8_start_write(struct msm_sensor_ctrl_t *e_ctrl)
{
	msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x0100, 0x01, MSM_CAMERA_I2C_BYTE_DATA);
	msleep(20);
	/*
	*read  page 15, read enable
	*/
	msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x0A02, 0x0F, MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x0A00, 0x01, MSM_CAMERA_I2C_BYTE_DATA);
	msleep(20);
	return 0;
}

int32_t  s5k4h8_update_awb(struct msm_sensor_ctrl_t *e_ctrl)
{
	int32_t r_ratio, b_ratio, r_gain, b_gain, g_gain, gr_gain, gb_gain;
	uint16_t Current_RG, Current_BG, R_GainH, R_GainL, B_GainH, B_GainL, G_GainH, G_GainL;

	Current_RG = (e_ctrl->otp_ptr.awb.r_over_gr_l)|(e_ctrl->otp_ptr.awb.r_over_gr_h << 8);
	Current_BG = (e_ctrl->otp_ptr.awb.b_over_gb_l)|(e_ctrl->otp_ptr.awb.b_over_gb_h << 8);
	r_ratio = 512 * Golden_RG / Current_RG;
	b_ratio = 512 * Golden_BG / Current_BG;
	if (!r_ratio || !b_ratio) {
		return 0;
	}
	if (r_ratio >= 512) {
		if (b_ratio >= 512) {
			r_gain = GAIN_DEFAULT * r_ratio / 512;
			g_gain = GAIN_DEFAULT;
			b_gain = GAIN_DEFAULT * b_ratio / 512;
		} else {
			r_gain = GAIN_DEFAULT * r_ratio / b_ratio;
			g_gain = GAIN_DEFAULT * 512 / b_ratio;
			b_gain = GAIN_DEFAULT;
		}
	} else {
		if (b_ratio >= 512) {
			r_gain = GAIN_DEFAULT;
			g_gain = GAIN_DEFAULT * 512 / r_ratio;
			b_gain = GAIN_DEFAULT * b_ratio / r_ratio;
		} else {
			gr_gain = GAIN_DEFAULT * 512 / r_ratio;
			gb_gain = GAIN_DEFAULT * 512 / b_ratio;
			if (gr_gain >= gb_gain) {
				r_gain = GAIN_DEFAULT;
				g_gain = GAIN_DEFAULT * 512 / r_ratio;
				b_gain = GAIN_DEFAULT * b_ratio / r_ratio;
			} else {
				r_gain = GAIN_DEFAULT * r_ratio  / b_ratio;
				g_gain = GAIN_DEFAULT * 512 / b_ratio;
				b_gain = GAIN_DEFAULT;
			}
		}
	}
	R_GainH = (r_gain & 0xff00) >> 8;
	R_GainL = (r_gain & 0x00ff);
	B_GainH = (b_gain & 0xff00) >> 8;
	B_GainL = (b_gain & 0x00ff);
	G_GainH = (g_gain & 0xff00) >> 8;
	G_GainL = (g_gain & 0x00ff);
	s5k4h8_start_write(e_ctrl);
	msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x6028, 0x4000, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x602A, 0x3058, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x6F12, 0x01, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x020e, G_GainH, MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x020f, G_GainL, MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x0210, R_GainH, MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x0211, R_GainL, MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x0212, B_GainH, MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x0213, B_GainL, MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x0214, G_GainH, MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x0215, G_GainL, MSM_CAMERA_I2C_BYTE_DATA);
	return 0;
}

int32_t s5k4h8_update_lsc(struct msm_sensor_ctrl_t *e_ctrl)
{
	s5k4h8_start_write(e_ctrl);
	msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x0B00, 0x0180, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x6028, 0x2000, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x602A, 0x0c40, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x6F12, 0x0140, MSM_CAMERA_I2C_WORD_DATA);
	msleep(20);
	msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x0100, 0x0103, MSM_CAMERA_I2C_WORD_DATA);
	return  0;
}

int32_t s5k4h8_sensor_read_otp(struct msm_sensor_ctrl_t *e_ctrl)
{
	uint16_t otp_flag = 0;

	s5k4h8_start_write(e_ctrl);
	e_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read(e_ctrl->sensor_i2c_client,
			0x0A04, &otp_flag, MSM_CAMERA_I2C_BYTE_DATA);
	if (otp_flag == 1) {
		s5k4h8_sensor_read_wb(e_ctrl, 0x0A04);
	} else {
		e_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read(e_ctrl->sensor_i2c_client,
				0x0A1A, &otp_flag, MSM_CAMERA_I2C_BYTE_DATA);
		if (otp_flag == 1) {
			s5k4h8_sensor_read_wb(e_ctrl, 0x0A1A);
		}
	 }
#if 0
	e_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read(e_ctrl->sensor_i2c_client,
		0x0A3E, &lsc_flag1, MSM_CAMERA_I2C_BYTE_DATA);
	e_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read(e_ctrl->sensor_i2c_client,
		0x0A41, &lsc_flag2, MSM_CAMERA_I2C_BYTE_DATA);
	e_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read(e_ctrl->sensor_i2c_client,
		0x0A40, &otp_flag, MSM_CAMERA_I2C_BYTE_DATA);
	if (lsc_flag1 == 1) {
		s5k4h8_sensor_read_lsc(e_ctrl, 0x0A3E);
	} else if (lsc_flag2 == 1) {
		s5k4h8_sensor_read_lsc(e_ctrl, 0x0A41);
	}
#endif
	msm_camera_cci_i2c_write(e_ctrl->sensor_i2c_client, 0x0A00, 0x00, MSM_CAMERA_I2C_BYTE_DATA);
	return 0;
}

int32_t s5k5e2_sensor_read_otp(struct msm_eeprom_ctrl_t *e_ctrl)
{
	int rc = 0;

	e_ctrl->i2c_client.addr_type = MSM_CAMERA_I2C_WORD_ADDR;

	rc = e_ctrl->i2c_client.i2c_func_tbl->i2c_write(
			&(e_ctrl->i2c_client), 0x0a00,
			0x4, MSM_CAMERA_I2C_BYTE_DATA);
	if (rc) {
		pr_err("s5k5e2 set otp fail\n");
	}
	rc = e_ctrl->i2c_client.i2c_func_tbl->i2c_write(
			&(e_ctrl->i2c_client), 0x0a02,
			0x3, MSM_CAMERA_I2C_BYTE_DATA);
	if (rc) {
		pr_err("s5k5e2 set otp fail\n");
	}
	rc = e_ctrl->i2c_client.i2c_func_tbl->i2c_write(
			&(e_ctrl->i2c_client), 0x0a00,
			0x1, MSM_CAMERA_I2C_BYTE_DATA);
	if (rc) {
		pr_err("s5k5e2 set otp fail\n");
	}

	msleep(20);

	return rc;
}

int32_t s5k4h8_117_read_otp_init(struct msm_eeprom_ctrl_t *e_ctrl)
{
	int rc = 0;

	e_ctrl->i2c_client.addr_type = MSM_CAMERA_I2C_WORD_ADDR;
	rc = e_ctrl->i2c_client.i2c_func_tbl->i2c_write(&(e_ctrl->i2c_client), 0x0100, 0x01,
		MSM_CAMERA_I2C_BYTE_DATA);
	msleep(20);
	rc = e_ctrl->i2c_client.i2c_func_tbl->i2c_write(&(e_ctrl->i2c_client), 0x0A02, 0x0F,
		MSM_CAMERA_I2C_BYTE_DATA);
	rc = e_ctrl->i2c_client.i2c_func_tbl->i2c_write(&(e_ctrl->i2c_client), 0x0A00, 0x01,
		MSM_CAMERA_I2C_BYTE_DATA);
	return rc;
}

int zte_eeprom_read_ready(struct msm_eeprom_ctrl_t *e_ctrl)
{
	struct msm_eeprom_board_info *eboard_info = e_ctrl->eboard_info;

	pr_info("eeprom  name %s\n", eboard_info->eeprom_name);
	if (!strcmp(eboard_info->eeprom_name, "zte_s5k5e2")) {
		s5k5e2_sensor_read_otp(e_ctrl);
	} else if (!strcmp(eboard_info->eeprom_name, "s5k4h8_117")) {
		s5k4h8_117_read_otp_init(e_ctrl);
	}
	return 0;
}

void zte_read_insensor_otp(struct msm_sensor_ctrl_t *e_ctrl)
{
	if (e_ctrl && (strcmp(e_ctrl->sensordata->sensor_name, "s5k4h8") == 0)) {
		s5k4h8_sensor_read_otp(e_ctrl);
	}
}

void zte_apply_insensor_otp(struct msm_sensor_ctrl_t *e_ctrl)
{
	if (e_ctrl && (strcmp(e_ctrl->sensordata->sensor_name, "s5k4h8") == 0)) {
		s5k4h8_update_awb(e_ctrl);
	}
}
