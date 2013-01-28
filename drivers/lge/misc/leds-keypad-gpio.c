/*
 * Keyboard/Button LED Driver controlled by GPIO
 *
 * Copyright (C) 2013 Micha LaQua
 * Copyright (C) 2011 Ricardo Cerqueira
 * Copyright (C) 2011 LGE Inc.
 * Copyright (C) 2010 Texas Instruments
 *
 * Author: Dan Murphy <DMurphy@ti.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <linux/gpio.h>
#include <linux/leds.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/lge/leds_keypad.h>
#include <linux/delay.h>
#include <linux/android_alarm.h>
#include <linux/wakelock.h>

#define TOUCH_DELAY_MSEC    250

static int keypad_gpio;
static int use_hold_key = 0;
static int hold_key_gpio;

struct wake_lock wlock;
struct workqueue_struct *blink_workqueue;
struct delayed_work  blink_queue;
struct alarm alarm;

struct keypad_led_data {
	struct led_classdev keypad_led_class_dev;
};

long touchdelay;
int is_blinking = 0;

#if defined(CONFIG_MAX8971_CHARGER)&&  defined(CONFIG_MACH_LGE_P2_DCM)
int pw_led_on_off=1;
int cause_of_pw_pressed=0;
void set_pw_led_on_off(int value)
{
	printk(KERN_ERR ">>>>>>> set_pw_led_on_off PW_LED: %d, pw_led_on_off: %d>>>>>>>>>>>>\n", value, pw_led_on_off);
	if(value == PW_LED_ON && pw_led_on_off == 0)
	{
		//printk(KERN_ERR ">>>>>>> SYSFS_LED ON!>>>>>>>>>>>>\n");
		gpio_set_value(hold_key_gpio, 1);
		pw_led_on_off = 1;
		return ;
	}
	else if(value == PW_LED_OFF && pw_led_on_off == 1 && cause_of_pw_pressed!=1)
	{
		//printk(KERN_ERR " SYSFS_LED OFF!\n");
		gpio_set_value(hold_key_gpio, 0);
		pw_led_on_off = 0;
		return ;
	}
	printk(KERN_ERR ">>>>>>>: set_pw_led_on_off PW_LED: %d, pw_led_on_off: %d>>>>>>>>>>>>\n", value, pw_led_on_off);
	return ;
}
EXPORT_SYMBOL(set_pw_led_on_off);

#endif
static void keypad_led_store(struct led_classdev *led_cdev,
				enum led_brightness value)
{
	is_blinking = 0;

	if(led_cdev->br_maintain_trigger == 1){
		printk(KERN_ERR "[pwr_led]: br_maintain_on trigger is on!\n");
		return;
		}

	if (value == 127) {
		//printk(KERN_INFO "NOTIFICATION_LED: on\n");
		wake_lock(&wlock);
		is_blinking = 1;
		queue_delayed_work(blink_workqueue, &blink_queue, msecs_to_jiffies(100));

	} else if(value == 255){
		//printk(KERN_INFO "ALL_LED: SYSFS_LED On!\n");
		gpio_set_value(keypad_gpio, 1);
		if(use_hold_key)
			gpio_set_value(hold_key_gpio, 1);
#if defined(CONFIG_MAX8971_CHARGER)&&  defined(CONFIG_MACH_LGE_P2_DCM)
		pw_led_on_off = 1;
		cause_of_pw_pressed = 1;
#endif
	} else {
		//printk(KERN_INFO "ALL_LED: SYSFS_LED Off!");
		gpio_set_value(keypad_gpio, 0);
		if(use_hold_key)
			gpio_set_value(hold_key_gpio, 0);
#if defined(CONFIG_MAX8971_CHARGER)&&  defined(CONFIG_MACH_LGE_P2_DCM)
		pw_led_on_off = 0;
		cause_of_pw_pressed = 0;
#endif
	}
}

static void led_blinker_alarm(struct alarm *alarm)
{
	wake_lock(&wlock);
	queue_delayed_work(blink_workqueue, &blink_queue, msecs_to_jiffies(100));
}

static void led_blink_queue(struct work_struct *work)
{
	if (is_blinking) {
		//printk(KERN_INFO "NOTIFICATION_LED: blink\n");
		gpio_set_value(keypad_gpio, 1);
		msleep(touchdelay);
		gpio_set_value(keypad_gpio, 0);
		/* if device has a power led, light it up too (in succession) */
		if(use_hold_key) {
			gpio_set_value(hold_key_gpio, 1);
			msleep(touchdelay);
			gpio_set_value(hold_key_gpio, 0);
		}
		/* Insert a ~4 second pause between pulses */
		ktime_t delay = ktime_add(alarm_get_elapsed_realtime(), ktime_set(3, 0));
		alarm_start_range(&alarm, delay, delay);
	} else {
		//printk(KERN_INFO "NOTIFICATION_LED: off\n");
	}
	wake_unlock(&wlock);
}

static int __devinit keypad_led_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct keypad_led_data *info;
	struct leds_keypad_platform_data *pdata = pdev->dev.platform_data;

	if (pdata == NULL) {
		dev_err(&pdev->dev, "%s(): platform_data NULL\n", __func__);
		return -EINVAL;
	}

	keypad_gpio   = pdata->keypad_gpio;
	hold_key_gpio = pdata->hold_key_gpio;
	use_hold_key  = pdata->use_hold_key;
	
	ret = gpio_request(keypad_gpio, "kp_leds_gpio"); 
	if(ret){
		dev_err(&pdev->dev, "request gpio %d failed!\n", keypad_gpio);
		return ret;
	}
	gpio_direction_output(keypad_gpio, 0); 
	gpio_set_value(keypad_gpio, 1);
	if (use_hold_key) {
		ret = gpio_request(hold_key_gpio, "pwr_leds_gpio");
		if(ret){
			dev_err(&pdev->dev, "request gpio %d failed!\n", hold_key_gpio);
			return ret;
		}
		gpio_direction_output(hold_key_gpio, 0);
		gpio_set_value(hold_key_gpio, 1);
	}

	info = kzalloc(sizeof(struct keypad_led_data), GFP_KERNEL);
	if (info == NULL) {
		ret = -ENOMEM;
		return ret;
	}

	platform_set_drvdata(pdev, info);

	if (pdata->name)
		info->keypad_led_class_dev.name = pdata->name;
	else
		info->keypad_led_class_dev.name = "keyboard-backlight";
	info->keypad_led_class_dev.brightness_set = keypad_led_store;
	info->keypad_led_class_dev.max_brightness = LED_FULL;

	ret = led_classdev_register(&pdev->dev, &info->keypad_led_class_dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "%s: Register led class failed\n", __func__);
		kfree(info);
		return ret;
	}

	touchdelay = TOUCH_DELAY_MSEC;
	wake_lock_init(&wlock, WAKE_LOCK_SUSPEND, "notificationlight");
	blink_workqueue = create_singlethread_workqueue("notificationlight");
	INIT_DELAYED_WORK(&blink_queue, led_blink_queue);
	alarm_init(&alarm, ANDROID_ALARM_ELAPSED_REALTIME_WAKEUP,
					led_blinker_alarm);

	return ret;
}

static int keypad_led_remove(struct platform_device *pdev)
{
	struct keypad_led_data *info = platform_get_drvdata(pdev);

	led_classdev_unregister(&info->keypad_led_class_dev);

	return 0;
}

static struct platform_driver keypad_led_driver = {
	.probe  = keypad_led_probe,
	.remove = keypad_led_remove,
	.driver = {
		.name = "keypad_led",
		.owner = THIS_MODULE,
	},
};

static int __init keypad_led_init(void)
{
	return platform_driver_register(&keypad_led_driver);
}

static void __exit keypad_led_exit(void)
{
	platform_driver_unregister(&keypad_led_driver);
}

module_init(keypad_led_init);
module_exit(keypad_led_exit);

MODULE_DESCRIPTION("Keyboard/Button LEDS driver");
MODULE_LICENSE("GPL");
