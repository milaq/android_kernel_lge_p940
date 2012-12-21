/*
 * SPI initializing code.
 *
 * Copyright (C) 2010 LG Electronic Inc.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/spi/ifx_n721_spi.h>
#include <plat/mcspi.h>
#include <lge/common.h>

#define ETH_KS8851_IRQ			34
#define ETH_KS8851_POWER_ON		48
#define ETH_KS8851_QUART		138

#if defined(CONFIG_SPI_IFX)
static void ifx_n721_dev_init(void)
{
	printk("[e]Board-4430: IFX_n721_DEV_INIT\n");

modem_pwron_gpio_request_err:
	gpio_free(MODEM_GPIO_PWRON);
modem_reset_gpio_request_err:
	gpio_free(IFX_MRDY_GPIO);
ifx_mrdy_gpio_request_err:
	gpio_free(IFX_SRDY_GPIO);
ifx_srdy_gpio_request_err:
	return;
}

static struct omap2_mcspi_device_config ifxn721_mcspi_config = {
	.turbo_mode		 = 0,
	.single_channel	 = 1,	/* 0: slave, 1: master */
};
#else
#define ifx_n721_dev_init	NULL
#endif /* CONFIG_SPI_IFX */

static struct spi_board_info spi_bd_info[] __initdata = {
	{
		.modalias		 = "ks8851",
		.bus_num		 = 1,
		.chip_select	 = 0,
		.max_speed_hz	 = 24000000,
		.irq			 = ETH_KS8851_IRQ,
	},
#if defined(CONFIG_SPI_IFX)
	{
		.modalias		 = "ifxn721",
		.bus_num		 = 4,
		.chip_select	 = 0,
		.max_speed_hz	 = 24000000,
		.controller_data = &ifxn721_mcspi_config,
		.irq			 = OMAP_GPIO_IRQ(119),
	},
#endif /* CONFIG_SPI_IFX */	
};

int __init iff_spi_init(void)
{
#if defined(CONFIG_SPI_IFX)
	int ret = 0;

	ret = lge_set_spi_board(spi_bd_info, ARRAY_SIZE(spi_bd_info));
	if (ret < 0)
		return ret;
	
	return lge_set_cp_init(ifx_n721_dev_init);
#endif
	return 0;
};

lge_machine_initcall(iff_spi_init);
