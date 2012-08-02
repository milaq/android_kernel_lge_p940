
/*
 * MAXIM MAX8971 Charger Driver
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/max8971.h>

#if defined(CONFIG_MUIC)
#include <linux/muic/muic.h>
#include <linux/muic/muic_client.h>
#else
#include <linux/muic/muic.h>
#endif

// there is no this part in max8971 patch file
#include <mach/gpio.h>
#include <linux/delay.h>
#include <linux/hrtimer.h>
#include <linux/proc_fs.h>              // might need to get fuel gauge info

// define register map

// there is no this part in max8971 patch file

#define MAX8971_REG_CHGINT      0x0F

#define MAX8971_REG_CHGINT_MASK 0x01

#define MAX8971_REG_CHG_STAT    0x02

#define MAX8971_DCV_MASK        0x80
#define MAX8971_DCV_SHIFT       7
#define MAX8971_TOPOFF_MASK     0x40
#define MAX8971_TOPOFF_SHIFT    6
#define MAX8971_DCI_OK          0x40    // for status register
#define MAX8971_DCI_OK_SHIFT    6
#define MAX8971_DCOVP_MASK      0x20
#define MAX8971_DCOVP_SHIFT     5
#define MAX8971_DCUVP_MASK      0x10
#define MAX8971_DCUVP_SHIFT     4
#define MAX8971_CHG_MASK        0x08
#define MAX8971_CHG_SHIFT       3
#define MAX8971_BAT_MASK        0x04
#define MAX8971_BAT_SHIFT       2
#define MAX8971_THM_MASK        0x02
#define MAX8971_THM_SHIFT       1
#define MAX8971_PWRUP_OK_MASK   0x01
#define MAX8971_PWRUP_OK_SHIFT  0
#define MAX8971_I2CIN_MASK      0x01
#define MAX8971_I2CIN_SHIFT     0

#define MAX8971_REG_DETAILS1    0x03
#define MAX8971_DC_V_MASK       0x80
#define MAX8971_DC_V_SHIFT      7
#define MAX8971_DC_I_MASK       0x40
#define MAX8971_DC_I_SHIFT      6
#define MAX8971_DC_OVP_MASK     0x20
#define MAX8971_DC_OVP_SHIFT    5
#define MAX8971_DC_UVP_MASK     0x10
#define MAX8971_DC_UVP_SHIFT    4
#define MAX8971_THM_DTLS_MASK   0x07
#define MAX8971_THM_DTLS_SHIFT  0

#define MAX8971_THM_DTLS_COLD   1       // charging suspended(temperature<T1)
#define MAX8971_THM_DTLS_COOL   2       // (T1<temperature<T2)
#define MAX8971_THM_DTLS_NORMAL 3       // (T2<temperature<T3)
#define MAX8971_THM_DTLS_WARM   4       // (T3<temperature<T4)
#define MAX8971_THM_DTLS_HOT    5       // charging suspended(temperature>T4)

#define MAX8971_REG_DETAILS2    0x04
#define MAX8971_BAT_DTLS_MASK   0x30
#define MAX8971_BAT_DTLS_SHIFT  4
#define MAX8971_CHG_DTLS_MASK   0x0F
#define MAX8971_CHG_DTLS_SHIFT  0

#define MAX8971_BAT_DTLS_BATDEAD        0   // VBAT<2.1V
#define MAX8971_BAT_DTLS_TIMER_FAULT    1   // The battery is taking longer than expected to charge
#define MAX8971_BAT_DTLS_BATOK          2   // VBAT is okay.
#define MAX8971_BAT_DTLS_GTBATOV        3   // VBAT > BATOV

#define MAX8971_CHG_DTLS_DEAD_BAT           0   // VBAT<2.1V, TJSHDN<TJ<TJREG
#define MAX8971_CHG_DTLS_PREQUAL            1   // VBAT<3.0V, TJSHDN<TJ<TJREG
#define MAX8971_CHG_DTLS_FAST_CHARGE_CC     2   // VBAT>3.0V, TJSHDN<TJ<TJREG
#define MAX8971_CHG_DTLS_FAST_CHARGE_CV     3   // VBAT=VBATREG, TJSHDN<TJ<TJREG
#define MAX8971_CHG_DTLS_TOP_OFF            4   // VBAT>=VBATREG, TJSHDN<TJ<TJREG
#define MAX8971_CHG_DTLS_DONE               5   // VBAT>VBATREG, T>Ttopoff+16s, TJSHDN<TJ<TJREG
#define MAX8971_CHG_DTLS_TIMER_FAULT        6   // VBAT<VBATOV, TJ<TJSHDN
#define MAX8971_CHG_DTLS_TEMP_SUSPEND       7   // TEMP<T1 or TEMP>T4
#define MAX8971_CHG_DTLS_USB_SUSPEND        8   // charger is off, DC is invalid or chaarger is disabled(USBSUSPEND)
#define MAX8971_CHG_DTLS_THERMAL_LOOP_ACTIVE    9   // TJ > REGTEMP
#define MAX8971_CHG_DTLS_CHG_OFF            10  // charger is off and TJ >TSHDN

#define MAX8971_REG_CHGCNTL1    0x05
#define MAX8971_DCMON_DIS_MASK  0x02
#define MAX8971_DCMON_DIS_SHIFT 1
#define MAX8971_USB_SUS_MASK    0x01
#define MAX8971_USB_SUS_SHIFT   0

#define MAX8971_REG_FCHGCRNT    0x06
#define MAX8971_CHGCC_MASK      0x1F
#define MAX8971_CHGCC_SHIFT     0
#define MAX8971_FCHGTIME_MASK   0xE0
#define MAX8971_FCHGTIME_SHIFT  5


#define MAX8971_REG_DCCRNT      0x07
#define MAX8971_CHGRSTRT_MASK   0x40
#define MAX8971_CHGRSTRT_SHIFT  6
#define MAX8971_DCILMT_MASK     0x3F
#define MAX8971_DCILMT_SHIFT    0

#define MAX8971_REG_TOPOFF          0x08
#define MAX8971_TOPOFFTIME_MASK     0xE0
#define MAX8971_TOPOFFTIME_SHIFT    5
#define MAX8971_IFST2P8_MASK        0x10
#define MAX8971_IFST2P8_SHIFT       4
#define MAX8971_TOPOFFTSHLD_MASK    0x0C
#define MAX8971_TOPOFFTSHLD_SHIFT   2
#define MAX8971_CHGCV_MASK          0x03
#define MAX8971_CHGCV_SHIFT         0

#define MAX8971_REG_TEMPREG     0x09
#define MAX8971_REGTEMP_MASK    0xC0
#define MAX8971_REGTEMP_SHIFT   6
#define MAX8971_THM_CNFG_MASK   0x08
#define MAX8971_THM_CNFG_SHIFT  3
#define MAX8971_SAFETYREG_MASK  0x01
#define MAX8971_SAFETYREG_SHIFT 0

#define MAX8971_REG_PROTCMD     0x0A
#define MAX8971_CHGPROT_MASK    0x0C
#define MAX8971_CHGPROT_SHIFT   2

//dukwung.kim [start]

#define CHG_EN_SET_N_OMAP               83

#define CHR_IC_DEALY                            200     /* 200 us */
#define CHR_IC_SET_DEALY                        1500    /* 1500 us */
#define CHR_TIMER_SECS                          3600 /* 7200 secs*/
#define TEMP_NO_BATTERY  1 

#define MAX8971_TOPOFF_WORKAROUND

#ifdef MAX8971_TOPOFF_WORKAROUND
#define MAX8971_TOPOFF_DELAY    ((HZ) * 60 + 0 /* + topoff timer */ )  // 60 seconds + toppoff timer
#endif
struct max8971_chip *test_chip;
static DEFINE_MUTEX(charging_lock);
static int bat_soc;
struct delayed_work     charger_timer_work;
struct timer_list charging_timer;
enum power_supply_type charging_ic_status;


struct max8971_chip {
        struct i2c_client *client;

#ifdef MAX8971_TOPOFF_WORKAROUND
	struct delayed_work topoff_work;
        u8 start_topoff_work;
	u8 prev_chg_dtl;
#endif
	struct power_supply charger;
	struct max8971_platform_data *pdata;
    int irq;
    int chg_online;
};

static struct max8971_chip *max8971_chg;

static int max8971_write_reg(struct i2c_client *client, u8 reg, u8 value)
{
        int ret = i2c_smbus_write_byte_data(client, reg, value);

        if (ret < 0)
                dev_err(&client->dev, "%s: err %d\n", __func__, ret);

        return ret;


}


enum power_supply_type get_charging_ic_status()
{
        return charging_ic_status;
}
EXPORT_SYMBOL(get_charging_ic_status);

void set_boot_charging_mode(int charging_mode)
{
        charging_ic_status = charging_mode;
}
EXPORT_SYMBOL(set_boot_charging_mode);

static int max8971_read_reg(struct i2c_client *client, u8 reg)
{
	int ret = i2c_smbus_read_byte_data(client, reg);

	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return ret;
}

static int max8971_set_bits(struct i2c_client *client, u8 reg, u8 mask, u8 data)
{
	u8 value = 0;
	int ret;

	ret = max8971_read_reg(client, reg);
	if (ret < 0)
		goto out;
	value &= ~mask;
	value |= data;
	ret = max8971_write_reg(client, reg, value);
out:
	return ret;
}


static int __set_charger(struct max8971_chip *chip, int enable)
{
    u8  reg_val= 0;

    reg_val = MAX8971_CHGPROT_UNLOCKED<<MAX8971_CHGPROT_SHIFT;
    max8971_write_reg(chip->client, MAX8971_REG_PROTCMD, reg_val);   

	if (enable) {
		/* enable charger */

        // Set fast charge current and timer
        reg_val = ((chip->pdata->chgcc<<MAX8971_CHGCC_SHIFT) |
                   (chip->pdata->fchgtime<<MAX8971_FCHGTIME_SHIFT));
        max8971_write_reg(chip->client, MAX8971_REG_FCHGCRNT, reg_val);

       // Set input current limit and charger restart threshold
        reg_val = ((chip->pdata->chgrstrt<<MAX8971_CHGRSTRT_SHIFT) |
                   (chip->pdata->dcilmt<<MAX8971_DCILMT_SHIFT));
        max8971_write_reg(chip->client, MAX8971_REG_DCCRNT, reg_val);

        // Set topoff condition
        reg_val = ((chip->pdata->topofftime<<MAX8971_TOPOFFTIME_SHIFT) |
                   (chip->pdata->topofftshld<<MAX8971_TOPOFFTSHLD_SHIFT) |
                   (chip->pdata->chgcv<<MAX8971_CHGCV_SHIFT));
        max8971_write_reg(chip->client, MAX8971_REG_TOPOFF, reg_val);

        // Set temperature condition
        reg_val = ((chip->pdata->regtemp<<MAX8971_REGTEMP_SHIFT) |
                   (chip->pdata->thm_config<<MAX8971_THM_CNFG_SHIFT) |
                   (chip->pdata->safetyreg<<MAX8971_SAFETYREG_SHIFT));
        max8971_write_reg(chip->client, MAX8971_REG_TEMPREG, reg_val);       

        // USB Suspend and DC Voltage Monitoring
        // Set DC Voltage Monitoring to Enable and USB Suspend to Disable
//        reg_val = (0<<MAX8971_DCMON_DIS_SHIFT) | (0<<MAX8971_USB_SUS_SHIFT);
//        max8971_write_reg(chip->client, MAX8971_REG_CHGCNTL1, reg_val);  

	} else {
		/* disable charge */
        max8971_set_bits(chip->client, MAX8971_REG_CHGCNTL1, MAX8971_USB_SUS_MASK, 1);
	}
	dev_info(&chip->client->dev, "%s\n", (enable) ? "Enable charger" : "Disable charger");
	return 0;
}

static int max8971_charger_detail_irq(int irq, void *data, u8 *val)
{
	struct max8971_chip *chip = (struct max8971_chip *)data;

    switch (irq) 
    {
    case MAX8971_IRQ_PWRUP_OK:
 //       dev_info(&chip->client->dev, "Power Up OK Interrupt\n");
        if ((val[0] & MAX8971_DCUVP_MASK) == 0) {
            // check DCUVP_OK bit in CGH_STAT
            // Vbus is valid //
            // Mask interrupt regsiter //
            max8971_write_reg(chip->client, MAX8971_REG_CHGINT_MASK, chip->pdata->int_mask);            
            // DC_V valid and start charging
            chip->chg_online = 1;
         //   __set_charger(chip, 1);
        }
        break;

    case MAX8971_IRQ_THM:
  //      dev_info(&chip->client->dev, "Thermistor Interrupt: details-0x%x\n", (val[1] & MAX8971_THM_DTLS_MASK));
        break;

    case MAX8971_IRQ_BAT:
        dev_info(&chip->client->dev, "Battery Interrupt: details-0x%x\n", (val[2] & MAX8971_BAT_DTLS_MASK));
        switch ((val[2] & MAX8971_BAT_MASK)>>MAX8971_BAT_SHIFT) 
        {
        case MAX8971_BAT_DTLS_BATDEAD:
            break;
        case MAX8971_BAT_DTLS_TIMER_FAULT:
            break;
        case MAX8971_BAT_DTLS_BATOK:
            break;
        case MAX8971_BAT_DTLS_GTBATOV:
            break;
        default:
            break;
        }
        break;

    case MAX8971_IRQ_CHG:
  //      dev_info(&chip->client->dev, "Fast Charge Interrupt: details-0x%x\n", (val[2] & MAX8971_CHG_DTLS_MASK));
        switch (val[2] & MAX8971_CHG_DTLS_MASK) 
        {
        case MAX8971_CHG_DTLS_DEAD_BAT:
            // insert event if a customer need to do something //
       		dev_info(&chip->client->dev, "MAX8971_CHG_DTLS_DEAD_BAT\n");
            break;
        case MAX8971_CHG_DTLS_PREQUAL:
            // insert event if a customer need to do something //
            break;
        case MAX8971_CHG_DTLS_FAST_CHARGE_CC:
	    	dev_info(&chip->client->dev, "MAX8971_CHG_DTLS_FAST_CHARGE_CC\n");
            // insert event if a customer need to do something //
            break;
        case MAX8971_CHG_DTLS_FAST_CHARGE_CV:
	    dev_info(&chip->client->dev, "MAX8971_CHG_DTLS_FAST_CHARGE_CV\n");
            // insert event if a customer need to do something //
            break;
        case MAX8971_CHG_DTLS_TOP_OFF:
	    dev_info(&chip->client->dev, "MAX8971_CHG_DTLS_TOP_OFF\n");
            // insert event if a customer need to do something //
            break;
        case MAX8971_CHG_DTLS_DONE:
            // insert event if a customer need to do something //
	    dev_info(&chip->client->dev, "MAX8971_CHG_DTLS_DONE\n");
            // Charging done and charge off automatically
            break;
        case MAX8971_CHG_DTLS_TIMER_FAULT:
	    dev_info(&chip->client->dev, "MAX8971_CHG_DTLS_TIMER_FAULT\n");
            // insert event if a customer need to do something //
            break;
        case MAX8971_CHG_DTLS_TEMP_SUSPEND:
	    dev_info(&chip->client->dev, "MAX8971_CHG_DTLS_TEMP_SUSPEND\n");
            // insert event if a customer need to do something //
            break;
        case MAX8971_CHG_DTLS_USB_SUSPEND:
	    dev_info(&chip->client->dev, "MAX8971_CHG_DTLS_USB_SUSPEND\n");
            // insert event if a customer need to do something //
            break;
        case MAX8971_CHG_DTLS_THERMAL_LOOP_ACTIVE:
	    dev_info(&chip->client->dev, "MAX8971_CHG_DTLS_THERMAL_LOOP_ACTIVE \n");
            // insert event if a customer need to do something //
            break;
        case MAX8971_CHG_DTLS_CHG_OFF:
	    dev_info(&chip->client->dev, "MAX8971_CHG_DTLS_CHG_OFF \n");
            // insert event if a customer need to do something //
            break;
        default:
            break;
        }
        break;

    case MAX8971_IRQ_DCUVP:
        if ((val[1] & MAX8971_DC_UVP_MASK) == 0) {
            // VBUS is invalid. VDC < VDC_UVLO
            //__set_charger(chip, 0);
        }
        dev_info(&chip->client->dev, "DC Under voltage Interrupt: details-0x%x\n", (val[1] & MAX8971_DC_UVP_MASK));
        break;

    case MAX8971_IRQ_DCOVP:
        if (val[1] & MAX8971_DC_OVP_MASK) {
            // VBUS is invalid. VDC > VDC_OVLO
//            __set_charger(chip, 0);
        }
        dev_info(&chip->client->dev, "DC Over voltage Interrupt: details-0x%x\n", (val[1] & MAX8971_DC_OVP_MASK));
        break;

//    case MAX8971_IRQ_DCI:
//        dev_info(&chip->client->dev, "DC Input Current Limit Interrupt: details-0x%x\n", (val[1] & MAX8971_DC_I_MASK));
//        break;
    case  MAX8971_IRQ_TOPOFF:
        dev_info(&chip->client->dev, "DC MAX8971_IRQ_TOPOFF Interrupt: details-0x%x\n", (val[1] & MAX8971_TOPOFF_MASK));
        break;


    case MAX8971_IRQ_DCV:
        dev_info(&chip->client->dev, "DC Input Voltage Limit Interrupt: details-0x%x\n", (val[1] & MAX8971_DC_V_MASK));
        break;

    }
    return 0;
}

static irqreturn_t max8971_charger_handler(int irq, void *data)
{	
    struct max8971_chip *chip = (struct max8971_chip *)data;
    int irq_val, irq_mask, irq_name;
    u8 val[3];
//    D("  max8971_charger_handler\n");
    irq_val = max8971_read_reg(chip->client, MAX8971_REG_CHGINT);
    irq_mask = max8971_read_reg(chip->client, MAX8971_REG_CHGINT_MASK);

    val[0] = max8971_read_reg(chip->client, MAX8971_REG_CHG_STAT);
    val[1] = max8971_read_reg(chip->client, MAX8971_REG_DETAILS1);
    val[2] = max8971_read_reg(chip->client, MAX8971_REG_DETAILS2);

        dev_info(&chip->client->dev, "MAX8971_REG_CHG_STAT- 0x%x\n", val[0]);
        dev_info(&chip->client->dev, "MAX8971_REG_DETAILS1- 0x%x\n", val[1]);
        dev_info(&chip->client->dev, "MAX8971_REG_DETAILS2- 0x%x\n", val[2]);

    for (irq_name = MAX8971_IRQ_PWRUP_OK; irq_name<MAX8971_NR_IRQS; irq_name++) {
        if ((irq_val & (0x01<<irq_name)) && !(irq_mask & (0x01<<irq_name))) {
            max8971_charger_detail_irq(irq_name, data, val);
        }
    }
	return IRQ_HANDLED;
}

int max8971_stop_charging(void)
{
    u8 reg_val=0;
//    reg_val = max8971_chg->pdata->int_mask=0xFF ;
//    max8971_write_reg(max8971_chg->client,MAX8971_REG_CHGINT_MASK , reg_val);
    // Charger removed
    if(max8971_chg != NULL )
    {
	printk(" max8971_chg pointer is not NULL\n");
     	max8971_chg->chg_online = 0;
    	__set_charger(max8971_chg, 0);
    }
    else
	printk(" max8971_chg pointer is NULL\n");

        charging_ic_status = POWER_SUPPLY_TYPE_BATTERY;
    // Disable GSM TEST MODE
  //  max8971_set_bits(max8971_chg->client, MAX8971_REG_TOPOFF, MAX8971_IFST2P8_MASK, 0<<MAX8971_IFST2P8_SHIFT);

    return 0;
}
EXPORT_SYMBOL(max8971_stop_charging);

int max8971_stop_factory_charging(void)
{
    u8 reg_val=0;
    // Charger removed
    if(max8971_chg != NULL)
    {
	printk(" max8971_chg pointer is not NULL\n");
    	max8971_chg->chg_online = 0;
    	__set_charger(max8971_chg, 0);

    // Disable GSM TEST MODE
    max8971_set_bits(max8971_chg->client, MAX8971_REG_TOPOFF, MAX8971_IFST2P8_MASK, 0<<MAX8971_IFST2P8_SHIFT);
    }
    else
        printk(" max8971_chg pointer is NULL\n");
    return 0;
}
EXPORT_SYMBOL(max8971_stop_factory_charging);


int max8971_start_charging(unsigned mA)
{
    u8 reg_val=0;
    if(mA==900)
    {
	charging_ic_status = POWER_SUPPLY_TYPE_MAINS;
	reg_val = (0<<MAX8971_DCMON_DIS_SHIFT) | (0<<MAX8971_USB_SUS_SHIFT);
        max8971_write_reg(max8971_chg->client, MAX8971_REG_CHGCNTL1, reg_val);
        dev_info(&max8971_chg->client->dev, "[LGE_MAX8971] MAX8971_REG_CHGCNTL1 = 0x%x", reg_val);
    }
    else if(mA==850) 
    {
	charging_ic_status = POWER_SUPPLY_TYPE_MAINS;
        reg_val = (1<<MAX8971_DCMON_DIS_SHIFT) | (0<<MAX8971_USB_SUS_SHIFT);
        max8971_write_reg(max8971_chg->client, MAX8971_REG_CHGCNTL1, reg_val);
        dev_info(&max8971_chg->client->dev, "[LGE_MAX8971] MAX8971_REG_CHGCNTL1 = 0x%x", reg_val);
    }
    else if(mA==500) 
    {
	charging_ic_status = POWER_SUPPLY_TYPE_USB;
        reg_val = (0<<MAX8971_DCMON_DIS_SHIFT) | (0<<MAX8971_USB_SUS_SHIFT);
        max8971_write_reg(max8971_chg->client, MAX8971_REG_CHGCNTL1, reg_val);
        dev_info(&max8971_chg->client->dev, "[LGE_MAX8971] MAX8971_REG_CHGCNTL1 = 0x%x", reg_val);
    }
    else if(mA==450) 
    {
	charging_ic_status = POWER_SUPPLY_TYPE_USB;
        reg_val = (0<<MAX8971_DCMON_DIS_SHIFT) | (0<<MAX8971_USB_SUS_SHIFT);
        max8971_write_reg(max8971_chg->client, MAX8971_REG_CHGCNTL1, reg_val);
        dev_info(&max8971_chg->client->dev, "[LGE_MAX8971] MAX8971_REG_CHGCNTL1 = 0x%x", reg_val);
    }

    else if(mA==1550)
    {
	charging_ic_status = POWER_SUPPLY_TYPE_FACTORY ;
	reg_val = (0<<MAX8971_DCMON_DIS_SHIFT) | (0<<MAX8971_USB_SUS_SHIFT);
        max8971_write_reg(max8971_chg->client, MAX8971_REG_CHGCNTL1, reg_val);
        dev_info(&max8971_chg->client->dev, "[LGE_MAX8971] MAX8971_REG_CHGCNTL1 = 0x%x", reg_val);
    }
    else if(mA==400)  
    {
	charging_ic_status = POWER_SUPPLY_TYPE_USB ;
	reg_val = (1<<MAX8971_DCMON_DIS_SHIFT) | (0<<MAX8971_USB_SUS_SHIFT);
	max8971_write_reg(max8971_chg->client, MAX8971_REG_CHGCNTL1, reg_val);
        dev_info(&max8971_chg->client->dev, "[LGE_MAX8971] MAX8971_REG_CHGCNTL1 = 0x%x", reg_val);
    }
    else
	printk("[max8971_start_chargin] can not find current\n");

    reg_val = max8971_chg->pdata->int_mask=0xF3 ;
    max8971_write_reg(max8971_chg->client,MAX8971_REG_CHGINT_MASK , reg_val);
    // charger inserted
    max8971_chg->chg_online = 1;
    max8971_chg->pdata->chgcc = FCHG_CURRENT(mA);
    printk("[max8971] max8971_chg->pdata->chgcc: 0x%x \n", max8971_chg->pdata->chgcc);
    
    __set_charger(max8971_chg, 1);



    return 0;
}
EXPORT_SYMBOL(max8971_start_charging);

int max8971_start_Factory_charging(void)
{
    printk("[max8971] max8971_start_Factory_charging\n");
    u8 reg_val=0;
    charging_ic_status = POWER_SUPPLY_TYPE_FACTORY;
    reg_val = MAX8971_CHGPROT_UNLOCKED<<MAX8971_CHGPROT_SHIFT;
    max8971_write_reg(max8971_chg->client, MAX8971_REG_PROTCMD, reg_val);   
    reg_val = max8971_chg->pdata->int_mask=0xF3 ;
    max8971_write_reg(max8971_chg->client,MAX8971_REG_CHGINT_MASK , reg_val);
    max8971_chg->chg_online = 1;
    reg_val = (1<<MAX8971_DCMON_DIS_SHIFT) | (0<<MAX8971_USB_SUS_SHIFT);
    max8971_write_reg(max8971_chg->client, MAX8971_REG_CHGCNTL1, reg_val);
    max8971_set_bits(max8971_chg->client, MAX8971_REG_TOPOFF, MAX8971_IFST2P8_MASK, 1<<MAX8971_IFST2P8_SHIFT);
    return 0;
}
EXPORT_SYMBOL(max8971_start_Factory_charging);

static int max8971_charger_get_property(struct power_supply *psy,
                                        enum power_supply_property psp,
                                        union power_supply_propval *val)
{
	struct max8971_chip *chip = container_of(psy, struct max8971_chip, charger);
	int ret = 0;
    	int chg_dtls_val;
	printk(" max8971_charger_get_property: psp= %d\n",psp);

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = chip->chg_online;
		break;
    case POWER_SUPPLY_PROP_STATUS:
		ret = max8971_read_reg(chip->client, MAX8971_REG_DETAILS2);
        chg_dtls_val = (ret & MAX8971_CHG_DTLS_MASK);
        if (chip->chg_online) {
            if (chg_dtls_val == MAX8971_CHG_DTLS_DONE) {
                val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
            }
            else if ((chg_dtls_val == MAX8971_CHG_DTLS_TIMER_FAULT) ||
                     (chg_dtls_val == MAX8971_CHG_DTLS_TEMP_SUSPEND) ||
                     (chg_dtls_val == MAX8971_CHG_DTLS_USB_SUSPEND) ||
                     (chg_dtls_val == MAX8971_CHG_DTLS_CHG_OFF)) {
                val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
            }
            else {
                val->intval = POWER_SUPPLY_STATUS_CHARGING;
            }
        }
        else {
            val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
        }
        ret = 0;
		break;	
    default:
		ret = -ENODEV;
		break;
	}
	return ret;
}

static enum power_supply_property max8971_charger_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
    POWER_SUPPLY_PROP_STATUS,
};

static struct power_supply max8971_charger_ps = {
   .name = "max8971",
  // .type = POWER_SUPPLY_TYPE_MAINS,
   .type = POWER_SUPPLY_TYPE_UNKNOWN ,
   .properties = max8971_charger_props,
   .num_properties = ARRAY_SIZE(max8971_charger_props),
   .get_property = max8971_charger_get_property,
};

static __devinit int max8971_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{

	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct max8971_chip *chip;
    int ret;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE))
		return -EIO;

	chip = kzalloc(sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->client = client;
	chip->pdata = client->dev.platform_data;

	max8971_chg = chip;
//    test_chip = chip;

	i2c_set_clientdata(client, chip);

//	irq_val = max8971_read_reg(chip->client, MAX8971_REG_CHGINT);
       //TEMP_DUK_S
	chip->charger = max8971_charger_ps;
	//TEMP_DUK_E

	//TEMP_DUK_S
	//ret = power_supply_register(&client->dev, &max8971_charger_ps);
	ret = power_supply_register(&client->dev, &chip->charger);
	//TEMP_DUK_E

	if (ret) {
		dev_err(&client->dev, "failed: power supply register\n");
		i2c_set_clientdata(client, NULL);
		goto out;
	}
//    charger_fsm_max8971(CHARG_FSM_CAUSE_ANY);

    ret = request_threaded_irq(client->irq, NULL, max8971_charger_handler,
            IRQF_ONESHOT | IRQF_TRIGGER_LOW, client->name, chip);
    if (unlikely(ret < 0))
    {
        pr_debug("max8971: failed to request IRQ	%X\n", ret);
        goto out;
    }

        ret=max8971_read_reg(chip->client, MAX8971_REG_CHGINT);
        printk(" MAX8971_REG_CHGINT : %d\n", ret);
        printk(" MAX8971_REG_CHGINT : %d\n", ret);
	chip->chg_online = 0;
	ret = max8971_read_reg(chip->client, MAX8971_REG_CHG_STAT);

	if (ret >= 0) 
    {
		chip->chg_online = (ret & MAX8971_DCUVP_MASK) ? 0 : 1;
        if (chip->chg_online) 
        {
            max8971_write_reg(chip->client, MAX8971_REG_CHGINT_MASK, chip->pdata->int_mask);
        }
	}

	return 0;
out:
	kfree(chip);
	return ret;
}

static __devexit int max8971_remove(struct i2c_client *client)
{
    struct max8971_chip *chip = i2c_get_clientdata(client);

	free_irq(client->irq, chip);
	power_supply_unregister(&max8971_charger_ps);
	kfree(chip);

	return 0;
}

static const struct i2c_device_id max8971_id[] = {
	{ "max8971", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, max8971_id);

static struct i2c_driver max8971_i2c_driver = {
	.driver = {
		.name = "max8971",
	},
	.probe		= max8971_probe,
	.remove		= __devexit_p(max8971_remove),
	.id_table	= max8971_id,
};

static int __init max8971_init(void)
{
	return i2c_add_driver(&max8971_i2c_driver);
}
module_init(max8971_init);

static void __exit max8971_exit(void)
{
	i2c_del_driver(&max8971_i2c_driver);
}
module_exit(max8971_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Clark Kim <clark.kim@maxim-ic.com>");
MODULE_DESCRIPTION("Power supply driver for MAX8971");
MODULE_VERSION("3.3");
MODULE_ALIAS("platform:max8971-charger");

