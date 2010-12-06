/*
 * OLED panel driver for CMEL OLED43
 *
 * Author: Christopher Ladden <christopher.ladden@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/leds.h>
#include <linux/fb.h>

#include <plat/display.h>

#define CS_PIN          139
#define MOSI_PIN        144
#define CLK_PIN         138
#define RESET_PIN       137
#define PANEL_PWR_PIN   143

#define digitalWrite(PIN,VALUE) (gpio_direction_output(PIN,VALUE))

#define CS_LOW digitalWrite(CS_PIN, 0)
#define CS_HIGH digitalWrite(CS_PIN, 1)
#define MOSI_LOW digitalWrite(MOSI_PIN, 0)
#define MOSI_HIGH digitalWrite(MOSI_PIN, 1)
#define CLK_LOW digitalWrite(CLK_PIN, 0)
#define CLK_HIGH digitalWrite(CLK_PIN, 1)
#define RESET_LOW digitalWrite(RESET_PIN, 0)
#define RESET_HIGH digitalWrite(RESET_PIN, 1)
#define PANEL_PWR_LOW digitalWrite(PANEL_PWR_PIN, 0)
#define PANEL_PWR_HIGH digitalWrite(PANEL_PWR_PIN, 1)

/*
 *   Video Timings
 *       
 *   Frame Rate = 1/((vbp + vfp + 272)*(hbp + hfp + 480)/pixel_clock)
 *   Where hfp must be > 30,
 *         hbp must be = 102,
 *         vfp must be > 10,
 *         vbp must be = 20,
 */
static struct omap_video_timings oled43_timings = {
	.x_res = 480,
	.y_res = 272,

	.pixel_clock	= 12342,

	.hsw		= 30,
	.hfp		= 31,
	.hbp		= 102,

	.vsw		= 3,
	.vfp		= 11,
	.vbp		= 20,
};

/* Initialize the software SPI interface */
static void oled43_spi_init(void) {
    CS_HIGH;
    MOSI_HIGH;
    CLK_HIGH;
}

/* Write an 8-bit byte onto the SPI bus */
static void oled43_spi_write8(uint8_t data) {
uint8_t ii;
uint8_t bit;

    for (ii=0; ii < 8; ii++) {
        bit = (data >> (7-ii)) & 1;
        if (bit) {
            MOSI_HIGH;
        } else {
            MOSI_LOW;
        }

        CLK_LOW;		//clock the data in
        CLK_HIGH;
    }
}

/* Write a value to the OLED panel */
static void oled43_writeReg(uint8_t index, uint8_t val) {
    CS_LOW;
    oled43_spi_write8((index << 1));
    oled43_spi_write8(val);
    CS_HIGH;
}

static int oled43_hardware_init(void) {
uint8_t brightness;
double percent; 

	/* Panel init sequence from the panel datasheet */
	PANEL_PWR_LOW;						// just to be sure, hold the oled power supply off
	RESET_LOW;							// panel in reset
	oled43_spi_init();					// init spi interface
	RESET_HIGH;							// panel out of reset

    oled43_writeReg(0x04, 0x23); //DISPLAY_MODE2
    oled43_writeReg(0x05, 0x82); //DISPLAY_MODE3

    oled43_writeReg(0x07, 0x0F); //DRIVER_CAPABILITY
    oled43_writeReg(0x34, 0x18);
    oled43_writeReg(0x35, 0x28);
    oled43_writeReg(0x36, 0x16);
    oled43_writeReg(0x37, 0x01);

    oled43_writeReg(0x03, 35);   //VGAM1_LEVEL

    /* Set the brightness
     *  0x20 - 200 nits
     *  0x1E - 175 nits
     *  0x1C - 150 nits
     *  0x17 - 100 nits
     *  0x14 -  70 nits
     *  0x11 -  50 nits */
    brightness = 0x14;
    percent = ((double)brightness)/
               (0x20) * 100.0;

    printk(KERN_INFO "cmel_oled43_panel: Setting brightness to %d percent\n", (int)percent);
    
    oled43_writeReg(0x3A, brightness);    

    /* Display ON */
    oled43_writeReg(0x06, 0x03); //POWER_CTRL1

	PANEL_PWR_HIGH;
    
	return 0;
}

static int oled43_panel_power_on(struct omap_dss_device *dssdev)
{
	oled43_hardware_init();

	return 0;
}

static void oled43_panel_power_off(struct omap_dss_device *dssdev)
{
    /* Turn off the power supply */
	PANEL_PWR_LOW; 
}

static void oled43_panel_led_set(struct led_classdev *cdev, enum led_brightness value)
{
	int level;
    
    level = value / 8; //scaled for the controller
    if (value <= 20) {
        /* brightness threshold reached, turning off */
        printk(KERN_INFO "cmel_oled43_panel: brightness set at or below threshold, turning OFF\n");
        level = 0;
    }
    oled43_writeReg(0x3A, level);
}

static struct led_classdev cmel_oled43_panel_led_ops = {
    .name = "lcd-backlight",
	.brightness_set = oled43_panel_led_set,
    .brightness = 159,
    .max_brightness = 255,
};

static int oled43_panel_probe(struct omap_dss_device *dssdev)
{
    struct led_classdev	*cdev;

	dssdev->panel.config = OMAP_DSS_LCD_TFT;
	dssdev->panel.timings = oled43_timings;
    
	cdev = led_classdev_register((struct device *)&dssdev->dev,
			&cmel_oled43_panel_led_ops);

	return 0;
}

static void oled43_panel_remove(struct omap_dss_device *dssdev)
{
    oled43_panel_power_off(dssdev);
}

static int oled43_panel_enable(struct omap_dss_device *dssdev)
{
	int r = 0;
	pr_info("cmel_oled43_panel: panel_enable begin\n");
	r = oled43_panel_power_on(dssdev);
	if (r)
		return r;

	dssdev->state = OMAP_DSS_DISPLAY_ACTIVE;
	pr_info("cmel_oled43_panel: panel_enable end\n");

	return 0;
}

static void oled43_panel_disable(struct omap_dss_device *dssdev)
{
	oled43_panel_power_off(dssdev);

	dssdev->state = OMAP_DSS_DISPLAY_DISABLED;
}

static int oled43_panel_suspend(struct omap_dss_device *dssdev)
{
	oled43_panel_power_off(dssdev);
	dssdev->state = OMAP_DSS_DISPLAY_SUSPENDED;
	pr_info("cmel_oled43_panel: panel_suspend\n");
	return 0;
}

static int oled43_panel_resume(struct omap_dss_device *dssdev)
{
	int r = 0;
	pr_info("cmel_oled43_panel: panel_resume\n");
	r = oled43_panel_power_on(dssdev);
	if (r)
		return r;

	dssdev->state = OMAP_DSS_DISPLAY_ACTIVE;

	return 0;
}

static struct omap_dss_driver oled43_driver = {
	.probe		= oled43_panel_probe,
	.remove		= oled43_panel_remove,

	.enable		= oled43_panel_enable,
	.disable	= oled43_panel_disable,
	.suspend	= oled43_panel_suspend,
	.resume		= oled43_panel_resume,

	.driver         = {
		.name   = "cmel_oled43_panel",
		.owner  = THIS_MODULE,
	},
};

static int __init oled43_panel_drv_init(void)
{
	int ret = 0;

	printk(KERN_INFO "cmel_oled43_panel: init panel\n");

	/* Request GPIO pins used for the panel */
    printk(KERN_INFO "cmel_oled43_panel: requesting GPIOs\n");
	gpio_request(CS_PIN, "OLED43_CS_PIN");
	gpio_request(MOSI_PIN, "OLED43_MOSI_PIN");
	gpio_request(CLK_PIN, "OLED43_CLK_PIN");
	gpio_request(RESET_PIN, "OLED43_RESET_PIN");
	gpio_request(PANEL_PWR_PIN, "OLED43_PANEL_PWR_PIN");
    
    /* Hold the power supply off until enabled */
	PANEL_PWR_LOW;

	ret = omap_dss_register_driver(&oled43_driver);
	if (ret != 0)
		pr_err("cmel_oled43: Unable to register panel driver: %d\n", ret);

	printk(KERN_INFO "cmel_oled43_panel: done\n");
	return ret;
}

static void __exit oled43_panel_drv_exit(void)
{
	omap_dss_unregister_driver(&oled43_driver);
}

module_init(oled43_panel_drv_init);
module_exit(oled43_panel_drv_exit);
MODULE_LICENSE("GPL");
