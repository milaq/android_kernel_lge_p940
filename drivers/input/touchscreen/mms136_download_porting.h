
/*--------------------------------------------------------
//
//
//	Melfas MMS100 Series Download base v1.7 2011.09.23
//
//
//--------------------------------------------------------*/

#ifndef __MELFAS_DOWNLOAD_PORTING_H_INCLUDED__
#define __MELFAS_DOWNLOAD_PORTING_H_INCLUDED__

/*============================================================
//
//      Porting order
//
//============================================================*/
/*

1. melfas_download_porting.h
   - Check typedef      [melfas_download_porting.h]

   - Check download options     [melfas_download_porting.h]

   - Add Port control code  ( CE, RESETB, I2C,... )     [melfas_download_porting.h]

   - Apply your delay function ( inside mcsdl_delaly() )        [melfas_download.c]
      Modify delay parameter constant ( ex. MCSDL_DELAY_5MS ) to make it fit to your delay function.

   - Rename 'uart_printf()' to your console print function for debugging. [melfas_download_porting.h]
	or, define uart_printf() as different function properly.

   - Check Watchdog timer, Interrupt factor

   - Including Melfas binary .c file

   - Basenad dealy function
      fill up mcsdl_delay()

   - Implement processing external Melfas binary .bin file.

*/


/*============================================================
//
//      Type define
//
//============================================================*/

typedef char                            INT8;
typedef unsigned char           UINT8;

typedef short                           INT16;
typedef unsigned short          UINT16;

typedef int                                     INT32;
typedef unsigned int            UINT32;
typedef unsigned char           BOOLEAN;


#ifndef TRUE
#define TRUE                            (1 == 1)
#endif

#ifndef FALSE
#define FALSE                           (1 == 0)
#endif

#ifndef NULL
#define NULL                            0
#endif


/*============================================================
//
//      Porting Download Options
//
//============================================================*/


/* For printing debug information. ( Please check 'printing function' )*/
#define MELFAS_ENABLE_DBG_PRINT                                                                                 1
#define MELFAS_ENABLE_DBG_PROGRESS_PRINT                                                                1

/* For delay function test. ( Disable after Porting is finished )*/
#define MELFAS_ENABLE_DELAY_TEST                                                                                0


/*============================================================
//
//      IO Control poting.
//
//      Fill 'Using signal' up only.
//      See MCSDL_USE_VDD_CONTROL,
//              MCSDL_USE_CE_CONTROL,
//
//============================================================*/


#define GPIO_TOUCH_EN 0
#define GPIO_TOUCH_INT 52

#define GPIO_TSP_SCL 128
#define GPIO_TSP_SDA 129


/*----------------
// VDD
//----------------*/
#if MCSDL_USE_VDD_CONTROL
#if 0
#define MCSDL_VDD_SET_HIGH()                                    gpio_set_value(GPIO_TOUCH_EN, 1)
#define MCSDL_VDD_SET_LOW()                                     gpio_set_value(GPIO_TOUCH_EN, 0)
#else
extern int (*g_power_enable)(int, bool);
#define MCSDL_VDD_SET_HIGH()               g_power_enable(1, true)
#define MCSDL_VDD_SET_LOW()                g_power_enable(0, true)
#endif
#else
#define MCSDL_VDD_SET_HIGH()
#define MCSDL_VDD_SET_LOW()
#endif

/*----------------
// CE
//----------------*/
#if MCSDL_USE_CE_CONTROL
#define MCSDL_CE_SET_HIGH()
#define MCSDL_CE_SET_LOW()
#define MCSDL_CE_SET_OUTPUT(n)
#else
#define MCSDL_CE_SET_HIGH()
#define MCSDL_CE_SET_LOW()
#define MCSDL_CE_SET_OUTPUT(n)
#endif


/*----------------
// RESETB
//----------------*/
#if MCSDL_USE_RESETB_CONTROL
#define MCSDL_RESETB_SET_HIGH()                         gpio_set_value(GPIO_TOUCH_INT, 1)
#define MCSDL_RESETB_SET_LOW()                          gpio_set_value(GPIO_TOUCH_INT, 0)
#define MCSDL_RESETB_SET_OUTPUT(n)                      gpio_direction_output(GPIO_TOUCH_INT, n)
#define MCSDL_RESETB_SET_INPUT()                        gpio_direction_input(GPIO_TOUCH_INT)
#else
#define MCSDL_RESETB_SET_HIGH()
#define MCSDL_RESETB_SET_LOW()
#define MCSDL_RESETB_SET_OUTPUT()
#define MCSDL_RESETB_SET_INPUT(n)
#endif

/*------------------
// I2C SCL & SDA
//------------------*/

#define MCSDL_GPIO_SCL_SET_HIGH()                                       gpio_set_value(GPIO_TSP_SCL, 1)
#define MCSDL_GPIO_SCL_SET_LOW()                                        gpio_set_value(GPIO_TSP_SCL, 0)

#define MCSDL_GPIO_SDA_SET_HIGH()                                       gpio_set_value(GPIO_TSP_SDA, 1)
#define MCSDL_GPIO_SDA_SET_LOW()                                        gpio_set_value(GPIO_TSP_SDA, 0)

#define MCSDL_GPIO_SCL_SET_OUTPUT(n)                            gpio_direction_output(GPIO_TSP_SCL, n)
#define MCSDL_GPIO_SCL_SET_INPUT()                                      gpio_direction_input(GPIO_TSP_SCL)

#define MCSDL_GPIO_SDA_SET_OUTPUT(n)                            gpio_direction_output(GPIO_TSP_SDA, n)
#define MCSDL_GPIO_SDA_SET_INPUT()                              gpio_direction_input(GPIO_TSP_SDA)

#define MCSDL_GPIO_SDA_IS_HIGH()                                        ((gpio_get_value(GPIO_TSP_SDA) > 0) ? 1 : 0)
#define MCSDL_GPIO_SCL_IS_HIGH()                                        ((gpio_get_value(GPIO_TSP_SCL) > 0) ? 1 : 0)

#define MCSDL_SET_GPIO_I2C()
#define MCSDL_SET_HW_I2C()



/*============================================================
//
//      Delay parameter setting
//
//      These are used on 'mcsdl_delay()'
//
//============================================================*/


#define MCSDL_DELAY_1US                                                             1
#define MCSDL_DELAY_2US                                                             2
#define MCSDL_DELAY_3US                                                             3
#define MCSDL_DELAY_5US                                                             5
#define MCSDL_DELAY_7US                                                             7
#define MCSDL_DELAY_10US                                                           10
#define MCSDL_DELAY_15US                                                           15
#define MCSDL_DELAY_20US                                                           20
#define MCSDL_DELAY_40US									40
#define MCSDL_DELAY_100US                                                         100
#define MCSDL_DELAY_150US                                                         150
#define MCSDL_DELAY_300US									300
#define MCSDL_DELAY_500US                                         500
#define MCSDL_DELAY_800US                                                         800


#define MCSDL_DELAY_1MS                                                          1000
#define MCSDL_DELAY_5MS                                                          5000
#define MCSDL_DELAY_10MS                                                        10000
#define MCSDL_DELAY_25MS                                                        25000
#define MCSDL_DELAY_30MS                                                        30000
#define MCSDL_DELAY_40MS                                                        40000
#define MCSDL_DELAY_45MS                                                        45000

#define MCSDL_DELAY_60MS                            60000

#define MCSDL_DELAY_100MS						   100000
#define MCSDL_DELAY_500MS						   500000

/*============================================================
//
//      Defence External Effect
//
//============================================================*/

#define MELFAS_DISABLE_BASEBAND_ISR()
#define MELFAS_DISABLE_WATCHDOG_TIMER_RESET()

#define MELFAS_ROLLBACK_BASEBAND_ISR()
#define MELFAS_ROLLBACK_WATCHDOG_TIMER_RESET()

#endif

