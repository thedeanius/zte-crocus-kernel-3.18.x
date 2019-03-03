/* Copyright (c) 2011-2014, The Linux Foundation. All rights reserved.
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

#include <linux/device.h>
#include "msm_sensor.h"
#include "msm_sd.h"
#include "msm_cci.h"
#include "msm_camera_dt_util.h"

#define OV5695_EEPROM_SLAVE_ADDR 0x50
#define OV5695_EEPROM_MODULE_ADDR 0x0
#define OV5695_EEPROM_KERR_MODULE_INFO 0x3
#define OV5695_EEPROM_SHIN_MODULE_INFO 0xA1
#define OV5695A_EEPROM_QTEC_MODULE_INFO 0x06


int zte_camera_sensor_check_eeprom_module(struct msm_camera_sensor_slave_info *slave_info,
		struct msm_camera_i2c_client  *sensor_i2c_client)
{
	uint16_t data_temp = 0;
	int rc = 0;
	uint16_t sid_o = 0;
	enum msm_camera_i2c_reg_addr_type addr_type_o = MSM_CAMERA_I2C_WORD_ADDR;

	if (!strcmp(slave_info->sensor_name, "ov5695_b")) {
		sid_o = sensor_i2c_client->cci_client->sid;
		addr_type_o = sensor_i2c_client->addr_type;
		sensor_i2c_client->cci_client->sid = OV5695_EEPROM_SLAVE_ADDR;
		sensor_i2c_client->addr_type = MSM_CAMERA_I2C_BYTE_ADDR;

		rc = sensor_i2c_client->i2c_func_tbl->i2c_read(sensor_i2c_client,
				OV5695_EEPROM_MODULE_ADDR,
				&data_temp, MSM_CAMERA_I2C_BYTE_DATA);

		pr_info("read diff reg 0x0 0x%x\n", data_temp);
		sensor_i2c_client->cci_client->sid = sid_o;
		sensor_i2c_client->addr_type = addr_type_o;

		if (rc < 0) {
			pr_err("%s camera eeprom read error\n", slave_info->sensor_name);
			return rc;
		}
		if (data_temp == OV5695_EEPROM_KERR_MODULE_INFO ||
			data_temp == OV5695A_EEPROM_QTEC_MODULE_INFO) {
			pr_info("camera suit\n");
		} else {
			return -EINVAL;
		}
	} else if (!strcmp(slave_info->sensor_name, "ov5695_b_shinetech")) {
		sid_o = sensor_i2c_client->cci_client->sid;
		addr_type_o = sensor_i2c_client->addr_type;
		sensor_i2c_client->cci_client->sid = OV5695_EEPROM_SLAVE_ADDR;
		sensor_i2c_client->addr_type = MSM_CAMERA_I2C_BYTE_ADDR;
		pr_info("step5\n");

		rc = sensor_i2c_client->i2c_func_tbl->i2c_read(sensor_i2c_client,
				OV5695_EEPROM_MODULE_ADDR,
				&data_temp, MSM_CAMERA_I2C_BYTE_DATA);

		pr_info("read diff reg 0x0 0x%x\n", data_temp);
		sensor_i2c_client->cci_client->sid = sid_o;
		sensor_i2c_client->addr_type = addr_type_o;

		if (rc < 0) {
			pr_err("%s camera eeprom read error\n", slave_info->sensor_name);
			return rc;
		}

		if (data_temp != OV5695_EEPROM_SHIN_MODULE_INFO) {
			return -EINVAL;
		}
	}

	return rc;
}
