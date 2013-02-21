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

static int keypad_gpio;
static int use_hold_key = 0;
static int hold_key_gpio;
static int pulseInterval = 4;
static int pulseLength = 200;

struct wake_lock wlock;
struct workqueue_struct *pulse_workqueue;
struct delayed_work  pulse_queue;
struct alarm alarm;

struct keypad_led_data {
	struct led_classdev keypad_led_class_dev;
};

long touchdelay;
int is_pulsing = 0;

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
	is_pulsing = 0;

	if(led_cdev->br_maintain_trigger == 1){
		printk(KERN_ERR "[pwr_led]: br_maintain_on trigger is on!\n");
		return;
		}

	if (value == 127) {
		//printk(KERN_INFO "NOTIFICATION_LED: on\n");
		wake_lock(&wlock);
		is_pulsing = 1;
		queue_delayed_work(pulse_workqueue, &pulse_queue, msecs_to_jiffies(100));

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

static ssize_t led_pulse_interval_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
    sscanf(buf, "%d\n", &pulseInterval);
    if (pulseInterval < 1) pulseInterval = 1;
    else if (pulseInterval > 60) pulseInterval = 60;

    return count;
}

static ssize_t led_pulse_interval_show(struct device *dev,struct device_attribute *attr,char *buf)
{
   return sprintf(buf, "%d\n", pulseInterval);
}

static ssize_t led_pulse_length_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
    sscanf(buf, "%d\n", &pulseLength);
    if (pulseLength < 100) pulseLength = 100;
    else if (pulseLength > 5000) pulseLength = 5000;

    return count;
}

static ssize_t led_pulse_length_show(struct device *dev,struct device_attribute *attr,char *buf)
{
   return sprintf(buf, "%d\n", pulseLength);
}

static DEVICE_ATTR(led_pulse_interval, 0666, led_pulse_interval_show, led_pulse_interval_store);
static DEVICE_ATTR(led_pulse_length, 0666, led_pulse_length_show, led_pulse_length_store);


static void led_pulse_alarm(struct alarm *alarm)
{
	wake_lock(&wlock);
	queue_delayed_work(pulse_workqueue, &pulse_queue, msecs_to_jiffies(100));
}

static void led_pulse_queue(struct work_struct *work)
{
	if (is_pulsing) {
		//printk(KERN_INFO "NOTIFICATION_LED: pulse\n");
		gpio_set_value(keypad_gpio, 1);
		/* if device has a power led, light it up too */
		if(use_hold_key)
			gpio_set_value(hold_key_gpio, 1);
		msleep(pulseLength);
		gpio_set_value(keypad_gpio, 0);
		if(use_hold_key)
			gpio_set_value(hold_key_gpio, 0);
		
		/* Insert a pause (set via sysfs - default is 4 seconds) between pulses */
		ktime_t delay = ktime_add(alarm_get_elapsed_realtime(), ktime_set(pulseInterval, 0));
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

	ret = device_create_file(&pdev->dev, &dev_attr_led_pulse_interval);
	if (ret)
		printk("led_pulse_interval sysfs register failed: Fail\n");

        ret = device_create_file(&pdev->dev, &dev_attr_led_pulse_length);
        if (ret)
                printk("led_pulse_length sysfs register failed: Fail\n");

	wake_lock_init(&wlock, WAKE_LOCK_SUSPEND, "notificationlight");
	pulse_workqueue = create_singlethread_workqueue("notificationlight");
	INIT_DELAYED_WORK(&pulse_queue, led_pulse_queue);
	alarm_init(&alarm, ANDROID_ALARM_ELAPSED_REALTIME_WAKEUP,
					led_pulse_alarm);

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
