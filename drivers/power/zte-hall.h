#ifndef __ZTE_HALL_H__
#define __ZTE_HALL_H__

#ifdef CONFIG_ZTE_HALL_AVAILABLE
struct hall_pwrkey {
	struct input_dev *hall_pwr;
};

enum {
	HALL_STATE_NULL,
	HALL_STATE_OPEN,
	HALL_STATE_CLOSE,
};
#endif

#endif
