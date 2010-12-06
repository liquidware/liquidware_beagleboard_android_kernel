#ifndef __LINUX_I2C_TSC2007_H
#define __LINUX_I2C_TSC2007_H

/* linux/i2c/tsc2007.h */

/**
 * struct tsc2007_platform_data - Platform data of the TSC2007 driver
 *
 * @model:		Touch controller model, currently 2003 or 2007.
 * @x_plate_ohms:	X-Plate resistance.
 * @z1_low_threshold:	All measured Z1 values below or equal to the threshold
 *			are regarded as 0.
 * @get_pendown_state:	Optional callback used to check if the film is touched.
 *			Can be useful since the pressure value can fluctuate for
 *			quite a while after lifting the pen.
 * @clear_penirq:	Optional callback clear 2nd level interrupt source.
 * @init_platform_hw:	Optional callback to initialise the hardware.
 * @exit_platform_hw:	Optional callback to de-initialise the hardware.
 */
struct tsc2007_platform_data {
	u16	model;
	u16	x_plate_ohms;
    u16	z1_low_threshold;

	int	(*get_pendown_state)(void);
	void	(*clear_penirq)(void);
	int	(*init_platform_hw)(void);
	void	(*exit_platform_hw)(void);
};

#endif
