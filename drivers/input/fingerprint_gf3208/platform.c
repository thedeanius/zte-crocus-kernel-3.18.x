#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/timer.h>
#include <linux/err.h>

#include "gf_spi.h"

#if defined(USE_SPI_BUS)
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#elif defined(USE_PLATFORM_BUS)
#include <linux/platform_device.h>
#endif

#define gf_dbg(fmt, args...) pr_warn("gf:" fmt, ##args)

static int gf3208_request_named_gpio(struct gf_dev *gf_dev, const char *label, int *gpio)
{
	struct device *dev = &gf_dev->spi->dev;
	struct device_node *np = dev->of_node;
	int rc = of_get_named_gpio(np, label, 0);

	if (rc < 0)	{
		dev_err(dev, "failed to get '%s'\n", label);
		return rc;
	}
	*gpio = rc;
	rc = devm_gpio_request(dev, *gpio, label);
	if (rc)	{
		dev_err(dev, "failed to request gpio %d\n", *gpio);
		return rc;
	}
	dev_err(dev, "%s %d\n", label, *gpio);
	return 0;
}

#if 0
static int select_pin_ctl(struct gf_dev *gf_dev, const char *name)
{
	size_t i;
	int rc;
	struct device *dev = &gf_dev->spi->dev;

	for (i = 0; i < ARRAY_SIZE(gf_dev->pinctrl_state); i++) {
		const char *n = pctl_names[i];

		if (!strncmp(n, name, strlen(n))) {
			rc = pinctrl_select_state(gf_dev->fingerprint_pinctrl, gf_dev->pinctrl_state[i]);

			if (rc)
				dev_err(dev, "cannot select '%s'\n", name);
			else
				dev_err(dev, "Selected '%s'\n", name);
			goto exit;
		}
	}
	rc = -EINVAL;
	dev_err(dev, "%s:'%s' not found\n", __func__, name);
exit:
	return rc;
}
#endif

/*GPIO pins reference.*/
int gf_parse_dts(struct gf_dev *gf_dev)
{
	int rc = 0;

	pr_info("--------gf_parse_dts start.--------\n");

	/*get reset resource*/
	rc = gf3208_request_named_gpio(gf_dev, "goodix,gpio_reset", &gf_dev->reset_gpio);
	if (rc) {
		gf_dbg("Failed to request RESET GPIO. rc = %d\n", rc);
		return 0;
	}
	pr_info("gf_dev->reset_gpio:%d\n", gf_dev->reset_gpio);
	if (!gpio_is_valid(gf_dev->reset_gpio)) {
		pr_info("RESET GPIO is invalid.\n");
	}

	/*get irq resourece*/
	rc = gf3208_request_named_gpio(gf_dev, "goodix,gpio_irq", &gf_dev->irq_gpio);
	if (rc) {
		gf_dbg("Failed to request IRQ GPIO. rc = %d\n", rc);
		return 0;
	}
	pr_info("gf_dev->irq_gpio:%d\n", gf_dev->irq_gpio);
	if (!gpio_is_valid(gf_dev->irq_gpio)) {
		pr_info("IRQ GPIO is invalid.\n");
	}
	/*get pwr gpio resourece*/
	rc = gf3208_request_named_gpio(gf_dev, "goodix,gpio_pwr", &gf_dev->pwr_gpio);
	if (rc)	{
		gf_dbg("Failed to request PWR GPIO. rc = %d\n", rc);
		return 0;
	}
	pr_info("gf_dev->pwr_gpio:%d\n", gf_dev->pwr_gpio);
	if (!gpio_is_valid(gf_dev->pwr_gpio)) {
		pr_info("PWR GPIO is invalid.\n");
	}
#if 0
	gf_dev->fingerprint_pinctrl = devm_pinctrl_get(&gf_dev->spi->dev);
	for (i = 0; i < ARRAY_SIZE(gf_dev->pinctrl_state); i++)	{
		const char *n = pctl_names[i];
		struct pinctrl_state *state =
				pinctrl_lookup_state(gf_dev->fingerprint_pinctrl, n);
		if (IS_ERR(state)) {
			pr_err("cannot find '%s'\n", n);
			rc = -EINVAL;
		}
		pr_info("found pin control %s\n", n);
		gf_dev->pinctrl_state[i] = state;
	}

	rc = select_pin_ctl(gf_dev, "goodix_reset_active");
	if (rc)
		goto exit;
	rc = select_pin_ctl(gf_dev, "goodix_irq_active");
	if (rc)
		goto exit;
#else
	gpio_direction_output(gf_dev->pwr_gpio, 1);
	msleep(100);
	gpio_direction_output(gf_dev->reset_gpio, 1);
	msleep(50);
	gpio_direction_input(gf_dev->irq_gpio);
#endif

	pr_warn("--------gf_parse_dts end---OK.--------\n");

	return rc;
}

void gf_cleanup(struct gf_dev *gf_dev)
{
	gf_dbg("[info]	enter%s\n", __func__);

	if (gpio_is_valid(gf_dev->irq_gpio)) {
		devm_gpio_free(&gf_dev->spi->dev, gf_dev->irq_gpio);
		gf_dbg("remove irq_gpio success\n");
	}

	if (gpio_is_valid(gf_dev->reset_gpio)) {
		devm_gpio_free(&gf_dev->spi->dev, gf_dev->reset_gpio);
		gf_dbg("remove reset_gpio success\n");
	}

	if (gf_dev->fingerprint_pinctrl != NULL) {
		devm_pinctrl_put(gf_dev->fingerprint_pinctrl);
		gf_dev->fingerprint_pinctrl = NULL;
		gf_dbg("gx	fingerprint_pinctrl	release success\n");
	}
}

/*power management*/
int gf_power_on(struct gf_dev *gf_dev)
{
	int rc = 0;

	if (gpio_is_valid(gf_dev->pwr_gpio)) {
		gpio_direction_output(gf_dev->pwr_gpio, 1);
	}
	msleep(20);
	pr_info("---- power on ok ----\n");

	return rc;
}

int gf_power_off(struct gf_dev *gf_dev)
{
	int rc = 0;

	if (gpio_is_valid(gf_dev->pwr_gpio)) {
		gpio_direction_output(gf_dev->pwr_gpio, 1);
	}
	pr_info("---- power off ----\n");
	return rc;
}

static int hw_reset(struct	gf_dev *gf_dev)
{
	int irq_gpio;
	struct device *dev = &gf_dev->spi->dev;

#if 0
	int rc = select_pin_ctl(gf_dev, "goodixfp_reset_reset");

	if (rc)
		goto exit;
	msleep(20);

	rc = select_pin_ctl(gf_dev, "goodixfp_reset_active");
	if (rc)
		goto exit;

	irq_gpio = gpio_get_value(gf_dev->irq_gpio);
#else
	gpio_direction_output(gf_dev->reset_gpio, 1);

	gpio_set_value(gf_dev->reset_gpio, 0);
	msleep(20);
	gpio_set_value(gf_dev->reset_gpio, 1);
	irq_gpio = gpio_get_value(gf_dev->irq_gpio);
#endif
	dev_info(dev, "IRQ after reset %d\n", irq_gpio);

	return 0;
}


/********************************************************************
 *CPU output low level in RST pin to reset GF. This is the MUST action for GF.
 *Take care of this function. IO Pin driver strength / glitch and so on.
 ********************************************************************/
int gf_hw_reset(struct gf_dev *gf_dev, unsigned int delay_ms)
{
	if (gf_dev == NULL)	{
		pr_info("Input buff is NULL.\n");
		return 0;
	}
	hw_reset(gf_dev);
	msleep(delay_ms);
	return 0;
}

int gf_irq_num(struct gf_dev *gf_dev)
{
	if (gf_dev == NULL) {
		pr_info("Input buff is NULL.\n");
		return 0;
	} else {
		return gpio_to_irq(gf_dev->irq_gpio);
	}
}
