/*
 * Miscellaneous initializing code.
 *
 * Copyright (C) 2010 LG Electronics, Inc.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/bitops.h>
#include <lge/common.h>
#include <lge/board.h>

unsigned long lge_get_no_pad_wakeup_gpios(int bank_id)
{
	unsigned long v = 0;

	switch(bank_id) {
		case 0:
			break;
		case 1:
			/* GPIO 63 as it is used for HDMI HPD
			 * GPIO 52 as it is used for TOUCH
			 */
			v = BIT(31) | BIT(20);
			break;
		case 2:
			break;
		case 3:
			break;
		case 4:
			break;
		case 5:
			/* GPIO 161 as it is used for MHL */
			v = BIT(1);
			break;
		default:
			break;
	}
	return v;
}
EXPORT_SYMBOL(lge_get_no_pad_wakeup_gpios);

int __init p940_misc_init(void)
{
	return 0;
}

lge_machine_initcall(p940_misc_init);
