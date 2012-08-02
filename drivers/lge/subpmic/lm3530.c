#include <linux/lge/lm3530.h>
#include <linux/power_supply.h>

#if defined(CONFIG_MAX8971_CHARGER)&&  defined(CONFIG_MACH_LGE_P2_DCM)

static int bl_on_off=1;

int lm3530_get_lcd_on_off()
{
	        return bl_on_off;
}
#endif


static int	lm3530_read_byte(struct lm3530_private_data* pdata, int reg)
{
	int		ret;

	mutex_lock(&pdata->update_lock);
	ret	=	i2c_smbus_read_byte_data(pdata->client, reg);
	mutex_unlock(&pdata->update_lock);

	return	ret;
}

static int	lm3530_write_byte(struct lm3530_private_data* pdata, int reg, int value)
{
	int		ret;

	mutex_lock(&pdata->update_lock);
	ret	=	i2c_smbus_write_byte_data(pdata->client, reg, value);
	mutex_unlock(&pdata->update_lock);

	return	ret;
}

static void	lm3530_store(struct lm3530_private_data* pdata)
{
	lm3530_write_byte(pdata, LM3530_REG_GP, pdata->reg_gp);
	lm3530_write_byte(pdata, LM3530_REG_BRR, pdata->reg_brr);
	lm3530_write_byte(pdata, LM3530_REG_BRT, pdata->reg_brt);

}

static void	lm3530_load(struct lm3530_private_data* pdata)
{
#if defined(CONFIG_MACH_LGE_P2)
	pdata->reg_gp   =	0x15;
#else
	pdata->reg_gp	=	lm3530_read_byte(pdata, LM3530_REG_GP);
#endif
	pdata->reg_brr   =       lm3530_read_byte(pdata, LM3530_REG_BRR);
	pdata->reg_brt	=	lm3530_read_byte(pdata, LM3530_REG_BRT);
}

int	lm3530_set_hwen(struct lm3530_private_data* pdata, int gpio, int status)
{
	if (status == 0) {
		lm3530_load(pdata);
		gpio_set_value(gpio, 0);
#if defined(CONFIG_MAX8971_CHARGER)&&  defined(CONFIG_MACH_LGE_P2_DCM)
		bl_on_off=0;
#endif
		return	0;
	}

	gpio_set_value(gpio, 1);
	lm3530_store(pdata);
#if defined(CONFIG_MAX8971_CHARGER)&&  defined(CONFIG_MACH_LGE_P2_DCM)
	bl_on_off=1;
#endif
	return	1;
}

int	lm3530_get_hwen(struct lm3530_private_data* pdata, int gpio)
{
	return	gpio_get_value(gpio);
}

#if defined(CONFIG_LG_FW_MAX17043_FUEL_GAUGE_I2C) || defined(CONFIG_LG_FW_MAX17048_FUEL_GAUGE_I2C)
extern int get_bat_present(void);
extern enum power_supply_type get_charging_ic_status(void);
#endif

int	lm3530_set_brightness_control(struct lm3530_private_data* pdata, int val)
{
	if ((val < 0) || (val > 255))
		return	-EINVAL;

#if defined(CONFIG_LG_FW_MAX17043_FUEL_GAUGE_I2C) || defined(CONFIG_LG_FW_MAX17048_FUEL_GAUGE_I2C)
	if (get_bat_present() == 0 &&
	    get_charging_ic_status() == POWER_SUPPLY_TYPE_FACTORY)
		return -1;
#endif
	return	lm3530_write_byte(pdata, LM3530_REG_BRT, val);
}

int	lm3530_get_brightness_control(struct lm3530_private_data* pdata)
{
	int		val;

	val	=	lm3530_read_byte(pdata, LM3530_REG_BRT);
	if (val < 0)
		return	val;

	return	(val & LM3530_BMASK);
}

int	lm3530_init(struct lm3530_private_data* pdata, struct i2c_client* client)
{
	mutex_init(&pdata->update_lock);
	pdata->client	=	client;

	lm3530_load(pdata);
	lm3530_store(pdata);

	return 0;
}

EXPORT_SYMBOL(lm3530_init);
EXPORT_SYMBOL(lm3530_set_hwen);
EXPORT_SYMBOL(lm3530_get_hwen);
EXPORT_SYMBOL(lm3530_set_brightness_control);
EXPORT_SYMBOL(lm3530_get_brightness_control);

MODULE_AUTHOR("LG Electronics");
MODULE_DESCRIPTION("Multi Display LED driver");
MODULE_LICENSE("GPL");
