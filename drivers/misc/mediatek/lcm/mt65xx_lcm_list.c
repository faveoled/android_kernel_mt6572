/*
* Copyright (C) 2011-2014 MediaTek Inc.
* 
* This program is free software: you can redistribute it and/or modify it under the terms of the 
* GNU General Public License version 2 as published by the Free Software Foundation.
* 
* This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/

#include <lcm_drv.h>
#ifdef BUILD_LK
#include <platform/disp_drv_platform.h>
#else
#include <linux/delay.h>
#include <mach/mt_gpio.h>
#endif
#include <cust_gpio_usage.h>
//used to identify float ID PIN status
#define LCD_HW_ID_STATUS_LOW      0
#define LCD_HW_ID_STATUS_HIGH     1
#define LCD_HW_ID_STATUS_FLOAT 0x02
#define LCD_HW_ID_STATUS_ERROR  0x03

#ifdef BUILD_LK
#define LCD_DEBUG(fmt)  dprintf(CRITICAL,fmt)
#else
#define LCD_DEBUG(fmt)  printk(fmt)
#endif

extern LCM_DRIVER otm8018b_dsi_vdo_lcm_drv;
extern LCM_DRIVER ili9806e_wvga_dsi_vdo_lcm_drv;

LCM_DRIVER* lcm_driver_list[] = 
{ 
#if defined(OTM8018B_DSI_VDO)	
	&otm8018b_dsi_vdo_lcm_drv, 
#endif
        #if defined(ILI9806E_WVGA_DSI_VDO)	
	&ili9806e_wvga_dsi_vdo_lcm_drv, 
#endif
};

#define LCM_COMPILE_ASSERT(condition) LCM_COMPILE_ASSERT_X(condition, __LINE__)
#define LCM_COMPILE_ASSERT_X(condition, line) LCM_COMPILE_ASSERT_XX(condition, line)
#define LCM_COMPILE_ASSERT_XX(condition, line) char assertion_failed_at_line_##line[(condition)?1:-1]

unsigned int lcm_count = sizeof(lcm_driver_list)/sizeof(LCM_DRIVER*);
#if defined(NT35520_HD720_DSI_CMD_TM) | defined(NT35520_HD720_DSI_CMD_BOE) | defined(NT35521_HD720_DSI_VDO_BOE) | defined(NT35521_HD720_DSI_VIDEO_TM)
#ifdef BUILD_LK
extern void mdelay(unsigned long msec);
#endif
static unsigned char lcd_id_pins_value = 0xFF;


/******************************************************************************
Function:       which_lcd_module_triple
  Description:    read LCD ID PIN status,could identify three status:high��low��float
  Input:           none
  Output:         none
  Return:         LCD ID1|ID0 value
  Others:         
******************************************************************************/
unsigned char which_lcd_module_triple(void)
{
    unsigned char  high_read0 = 0;
    unsigned char  low_read0 = 0;
    unsigned char  high_read1 = 0;
    unsigned char  low_read1 = 0;
    unsigned char  lcd_id0 = 0;
    unsigned char  lcd_id1 = 0;
    unsigned char  lcd_id = 0;
    //Solve Coverity scan warning : check return value
    unsigned int ret = 0;
    //only recognise once
    if(0xFF != lcd_id_pins_value) 
    {
        return lcd_id_pins_value;
    }
    //Solve Coverity scan warning : check return value
    ret = mt_set_gpio_mode(GPIO_DISP_ID0_PIN, GPIO_MODE_00);
    if(0 != ret)
    {
        LCD_DEBUG("ID0 mt_set_gpio_mode fail\n");
    }
    ret = mt_set_gpio_dir(GPIO_DISP_ID0_PIN, GPIO_DIR_IN);
    if(0 != ret)
    {
        LCD_DEBUG("ID0 mt_set_gpio_dir fail\n");
    }
    ret = mt_set_gpio_pull_enable(GPIO_DISP_ID0_PIN, GPIO_PULL_ENABLE);
    if(0 != ret)
    {
        LCD_DEBUG("ID0 mt_set_gpio_pull_enable fail\n");
    }
    ret = mt_set_gpio_mode(GPIO_DISP_ID1_PIN, GPIO_MODE_00);
    if(0 != ret)
    {
        LCD_DEBUG("ID1 mt_set_gpio_mode fail\n");
    }
    ret = mt_set_gpio_dir(GPIO_DISP_ID1_PIN, GPIO_DIR_IN);
    if(0 != ret)
    {
        LCD_DEBUG("ID1 mt_set_gpio_dir fail\n");
    }
    ret = mt_set_gpio_pull_enable(GPIO_DISP_ID1_PIN, GPIO_PULL_ENABLE);
    if(0 != ret)
    {
        LCD_DEBUG("ID1 mt_set_gpio_pull_enable fail\n");
    }
    //pull down ID0 ID1 PIN    
    ret = mt_set_gpio_pull_select(GPIO_DISP_ID0_PIN,GPIO_PULL_DOWN);
    if(0 != ret)
    {
        LCD_DEBUG("ID0 mt_set_gpio_pull_select->Down fail\n");
    }
    ret = mt_set_gpio_pull_select(GPIO_DISP_ID1_PIN,GPIO_PULL_DOWN);
    if(0 != ret)
    {
        LCD_DEBUG("ID1 mt_set_gpio_pull_select->Down fail\n");
    }
    //delay 100ms , for discharging capacitance 
    mdelay(100);
    //get ID0 ID1 status
    low_read0 = mt_get_gpio_in(GPIO_DISP_ID0_PIN);
    low_read1 = mt_get_gpio_in(GPIO_DISP_ID1_PIN);
    //pull up ID0 ID1 PIN 
    ret = mt_set_gpio_pull_select(GPIO_DISP_ID0_PIN,GPIO_PULL_UP);
    if(0 != ret)
    {
        LCD_DEBUG("ID0 mt_set_gpio_pull_select->UP fail\n");
    }
    ret = mt_set_gpio_pull_select(GPIO_DISP_ID1_PIN,GPIO_PULL_UP);
    if(0 != ret)
    {
        LCD_DEBUG("ID1 mt_set_gpio_pull_select->UP fail\n");
    }
    //delay 100ms , for charging capacitance 
    mdelay(100);
    //get ID0 ID1 status
    high_read0 = mt_get_gpio_in(GPIO_DISP_ID0_PIN);
    high_read1 = mt_get_gpio_in(GPIO_DISP_ID1_PIN);

    if( low_read0 != high_read0 )
    {
        /*float status , pull down ID0 ,to prevent electric leakage*/
        ret = mt_set_gpio_pull_select(GPIO_DISP_ID0_PIN,GPIO_PULL_DOWN);
        if(0 != ret)
        {
            LCD_DEBUG("ID0 mt_set_gpio_pull_select->Down fail\n");
        }
        lcd_id0 = LCD_HW_ID_STATUS_FLOAT;
    }
    else if((LCD_HW_ID_STATUS_LOW == low_read0) && (LCD_HW_ID_STATUS_LOW == high_read0))
    {
        /*low status , pull down ID0 ,to prevent electric leakage*/
        ret = mt_set_gpio_pull_select(GPIO_DISP_ID0_PIN,GPIO_PULL_DOWN);
        if(0 != ret)
        {
            LCD_DEBUG("ID0 mt_set_gpio_pull_select->Down fail\n");
        }
        lcd_id0 = LCD_HW_ID_STATUS_LOW;
    }
    else if((LCD_HW_ID_STATUS_HIGH == low_read0) && (LCD_HW_ID_STATUS_HIGH == high_read0))
    {
        /*high status , pull up ID0 ,to prevent electric leakage*/
        ret = mt_set_gpio_pull_select(GPIO_DISP_ID0_PIN,GPIO_PULL_UP);
        if(0 != ret)
        {
            LCD_DEBUG("ID0 mt_set_gpio_pull_select->UP fail\n");
        }
        lcd_id0 = LCD_HW_ID_STATUS_HIGH;
    }
    else
    {
        LCD_DEBUG(" Read LCD_id0 error\n");
        ret = mt_set_gpio_pull_select(GPIO_DISP_ID0_PIN,GPIO_PULL_DISABLE);
        if(0 != ret)
        {
            LCD_DEBUG("ID0 mt_set_gpio_pull_select->Disbale fail\n");
        }
        lcd_id0 = LCD_HW_ID_STATUS_ERROR;
    }


    if( low_read1 != high_read1 )
    {
        /*float status , pull down ID1 ,to prevent electric leakage*/
        ret = mt_set_gpio_pull_select(GPIO_DISP_ID1_PIN,GPIO_PULL_DOWN);
        if(0 != ret)
        {
            LCD_DEBUG("ID1 mt_set_gpio_pull_select->Down fail\n");
        }
        lcd_id1 = LCD_HW_ID_STATUS_FLOAT;
    }
    else if((LCD_HW_ID_STATUS_LOW == low_read1) && (LCD_HW_ID_STATUS_LOW == high_read1))
    {
        /*low status , pull down ID1 ,to prevent electric leakage*/
        ret = mt_set_gpio_pull_select(GPIO_DISP_ID1_PIN,GPIO_PULL_DOWN);
        if(0 != ret)
        {
            LCD_DEBUG("ID1 mt_set_gpio_pull_select->Down fail\n");
        }
        lcd_id1 = LCD_HW_ID_STATUS_LOW;
    }
    else if((LCD_HW_ID_STATUS_HIGH == low_read1) && (LCD_HW_ID_STATUS_HIGH == high_read1))
    {
        /*high status , pull up ID1 ,to prevent electric leakage*/
        ret = mt_set_gpio_pull_select(GPIO_DISP_ID1_PIN,GPIO_PULL_UP);
        if(0 != ret)
        {
            LCD_DEBUG("ID1 mt_set_gpio_pull_select->UP fail\n");
        }
        lcd_id1 = LCD_HW_ID_STATUS_HIGH;
    }
    else
    {

        LCD_DEBUG(" Read LCD_id1 error\n");
        ret = mt_set_gpio_pull_select(GPIO_DISP_ID1_PIN,GPIO_PULL_DISABLE);
        if(0 != ret)
        {
            LCD_DEBUG("ID1 mt_set_gpio_pull_select->Disable fail\n");
        }
        lcd_id1 = LCD_HW_ID_STATUS_ERROR;
    }
#ifdef BUILD_LK
    dprintf(CRITICAL,"which_lcd_module_triple,lcd_id0:%d\n",lcd_id0);
    dprintf(CRITICAL,"which_lcd_module_triple,lcd_id1:%d\n",lcd_id1);
#else
    printk("which_lcd_module_triple,lcd_id0:%d\n",lcd_id0);
    printk("which_lcd_module_triple,lcd_id1:%d\n",lcd_id1);
#endif
    lcd_id =  lcd_id0 | (lcd_id1 << 2);

#ifdef BUILD_LK
    dprintf(CRITICAL,"which_lcd_module_triple,lcd_id:%d\n",lcd_id);
#else
    printk("which_lcd_module_triple,lcd_id:%d\n",lcd_id);
#endif

    lcd_id_pins_value = lcd_id;
    return lcd_id;
}
#endif
