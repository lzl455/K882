/*******************************************************************************
 * Copyright (c) 2013, SHENZHEN Co.,LTD.
 * All rights reserved.
 *
 * 文件名称: led.c
 *
 * 功能说明:
 * 修改说明:
 *******************************************************************************/
#include <string.h>
#include "om_driver.h"

#include "main.h"
#include "lvd.h"
#include "led.h"

#include "flash.h"
#include "flash_process.h"
#include "usb_cfg_info.h"
#include "matrix_key.h"

//-----------------------------------------------------------------------------;
// 常量定义区
//

//-----------------------------------------------------------------------------;
// 变量定义区
//
bool bdevice_led_status;
bool bled_bright_en;
bool bback_led_status;
bool bargb_valid;

uint8_t gtemp_i;
uint8_t gtemp_j;
uint8_t gtemp_x;

uint8_t device_conn_index = 0;
uint8_t device_hint_count;

uint8_t led_hint_mode;
uint8_t led_mode_bak;

uint8_t logo_hint_time_count;
uint8_t rgb_enable;

uint8_t pwm_value;
uint8_t pwm_count;

uint8_t nhjb_led_index;

uint16_t flash_on_time;
uint16_t flash_off_time;
uint32_t led_delay_count;

uint16_t led_ctrl_reg;
uint16_t led_status_reg;
uint8_t led_use_flag;

uint8_t led_cur_mode;
uint8_t led_pre_mode;
enum speed_level led_speed_lev;
unsigned char led_rgb_index;
unsigned char pwm_value;
enum led_level led_light_lev;

uint8_t pwm_time_count;
uint16_t delay_time_count;
uint8_t rgb_index;
uint8_t led_process_index;
uint8_t hint_time_count;

__ALIGNED(4)
uint8_t pwm_rgb_tbl[iLED_MAX][3];               // 灯效计算缓冲
__ALIGNED(4)
uint8_t pwm_out_tbl[iLED_MAX][3];               // 算完的一帧, drv_ledc_write 只发这个, 避免中断里发出算到一半的帧

// 灯效节拍: 按RTC时间推进, 与各模式调用 logo_process 的频率无关, 三种模式速度一致
// ponytail: 固定8ms按蓝牙/USB现有速度估的, 整体快慢改这里
#define iLOGO_FRAME_TICK        mMS_TO_TICK(8)

uint8_t pwm_duty_array_0[0x10];
uint8_t pwm_duty_array_1[0x10];
uint8_t pwm_duty_array_2[0x10];
uint8_t pwm_duty_array_3[0x10];

extern bool b24g_nromal;
extern uint8_t usb_connect_ok_hint;
extern uint8_t bat_press;
extern uint8_t usb_power_on;
uint8_t breach_color_index;
uint8_t on_color_index;
uint8_t start = 0xFF;
//=============================================================================;
// 代码区                                                                       ;
//=============================================================================;

/*******************************************************************************
 * 函数名称: ledc_init()
 * 功能描述:
 * 输入参数： 无
 * 输出参数： 无
 * 说    明:
 *******************************************************************************/
void ledc_init(void)
{
    ledc_config_t config = {
        .reset_cycles = 100,
        .long_pulse_cycles = 44,
        .short_pulse_cycles = 14,
    };
    drv_ledc_init(OM_LEDC, &config);
    bargb_valid = true;
}

/******************************************************************************
 * 函数名称: led_status_hint
 * 功能描述:
 * 输入参数:
 * 输出参数:
 * 说    明:
 ******************************************************************************/
#define RGB_R (1 << 0)
#define RGB_G (1 << 1)
#define RGB_B (1 << 2)
uint8_t RGB_Value;
uint8_t usb_switch_count1;
uint16_t usb_switch_count2;
extern uint8_t RGB_bak;
extern uint32_t usb_into_time;
void led_status_hint(void)
{
    mLED_VDD_DIS;
    if (hint_led_status & bled_hint_mode)
    {
        switch (hint_led_status & 0x0F)
        {
        case bled_bt0:
            RGB_Value = RGB_B;
            break;
        case bled_bt1:
            RGB_Value = RGB_B;
            break;
        case bled_bt2:
            RGB_Value = RGB_B;
            break;
        case bled_24g:
            RGB_Value = (RGB_R | RGB_G);
            break;
        case bled_bt0 | bled_bt1 | bled_bt2 | bled_24g:
            RGB_Value = (RGB_R | RGB_G | RGB_B);
            mLED_WIN_LOW
                mLED_CAPS_LOW break;
        default:
            RGB_Value = 0;
            mLED_WIN_HIGH
                mLED_CAPS_HIGH break;
        }
        if (0 == (hint_led_status & (bled_bt0 | bled_bt1 | bled_bt2 | bled_24g)))
            return;
    }
    else
    {
        if (key_ctrl & bwin_lock_status)
            mLED_WIN_LOW else mLED_WIN_HIGH if (kb_led_status & bcaps_lock)
                mLED_CAPS_LOW else mLED_CAPS_HIGH

                if (usb_power_on)
            {
                if (mMS_TO_TICK(5 * 1000) > drv_rtc_read(OM_RTC) - usb_into_time)
                {
                    RGB_Value = RGB_R;
                }
                else
                {
                    usb_power_on = 0;
                }
            }
        else if (usb_connect_ok_hint)
        {
            usb_switch_count2++;
            if (usb_switch_count2 < 50)
            {
                RGB_Value = (RGB_R | RGB_B);
            }
            else if (usb_switch_count2 < 100)
            {
                RGB_Value = 0;
            }
            else
            {
                usb_switch_count2 = 0;
                usb_switch_count1++;
            }
            if (usb_switch_count1 >= 3)
            {
                usb_switch_count1 = 0;
                usb_switch_count2 = 0;
                usb_connect_ok_hint = 0;
                flash_save_status();
            }
        }
        else
        {
            switch ((uint8_t)device_type)
            {
            case iDEVICE_TYPE_USB:
                RGB_Value = (RGB_R | RGB_B);
                break;
            case iDEVICE_TYPE_BT0:
                RGB_Value = RGB_B;
                break;
            case iDEVICE_TYPE_BT1:
                RGB_Value = RGB_B;
                break;
            case iDEVICE_TYPE_BT2:
                RGB_Value = RGB_B;
                break;
            case iDEVICE_TYPE_24G:
                RGB_Value = (RGB_R | RGB_G);
                break;
            }
        }
    }

    if (bat_press || (iLOW_POWER >= bat_value))
    {
        RGB_Value = RGB_bak;
    }

    if (RGB_Value & 0x01)
        mLED_LOWR_LOW else mLED_LOWR_HIGH if (RGB_Value & 0x02)
            mLED_LOWG_LOW else mLED_LOWG_HIGH;
    if (RGB_Value & 0x04)
        mLED_LOWB_LOW else mLED_LOWB_HIGH

            // mLED_LOWR_HIGH
            // mLED_LOWG_HIGH
            // mLED_LOWB_LOW

            mLED_VDD_EN;
}

/*******************************************************************************
 * 函数名称: device_led_on()
 * 功能描述:
 * 输入参数： 无
 * 输出参数： 无
 * 说    明:
 *******************************************************************************/
void device_led_on(void)
{
    bdevice_led_status = true;

    hint_led_status &= 0xF0;
    if (iDEVICE_TYPE_BT0 == device_type)
        hint_led_status |= bled_bt0;
    else if (iDEVICE_TYPE_BT1 == device_type)
        hint_led_status |= bled_bt1;
    else if (iDEVICE_TYPE_BT2 == device_type)
        hint_led_status |= bled_bt2;
    else if (iDEVICE_TYPE_24G == device_type)
        hint_led_status |= bled_24g;
}

/*******************************************************************************
 * 函数名称: device_led_off()
 * 功能描述:
 * 输入参数： 无
 * 输出参数： 无
 * 说    明:
 *******************************************************************************/
void device_led_off(void)
{
    bdevice_led_status = false;

    hint_led_status &= 0xF0;
}

void logo_off(void)
{
    bargb_valid = false;

    DRV_RCC_CLOCK_ENABLE(RCC_CLK_LEDC, 0U);
    OM_GPIO0->DATAOUTSET = (1 << ARGB_OUT_PIN);
    OM_GPIO0->DATAOUTCLR = (1 << ARGB_OFF_PIN);
}

void clr_rgb(void)
{
    memset(pwm_rgb_tbl, 0x00, iLED_MAX * 3);
}

void rgb_pwm0_w(void)
{
    if (rgb_enable & iPWM_R)
        pwm_rgb_tbl[gtemp_j][iR_INDEX] = pwm_duty_array_0[gtemp_x];
    else
        pwm_rgb_tbl[gtemp_j][iR_INDEX] = 0;
    if (rgb_enable & iPWM_G)
        pwm_rgb_tbl[gtemp_j][iG_INDEX] = pwm_duty_array_0[gtemp_x];
    else
        pwm_rgb_tbl[gtemp_j][iG_INDEX] = 0;
    if (rgb_enable & iPWM_B)
        pwm_rgb_tbl[gtemp_j][iB_INDEX] = pwm_duty_array_0[gtemp_x];
    else
        pwm_rgb_tbl[gtemp_j][iB_INDEX] = 0;
}

void rgb_w_max(void)
{
    if (rgb_enable & iPWM_R)
        pwm_rgb_tbl[gtemp_j][iR_INDEX] = iPWM_MAX;
    else
        pwm_rgb_tbl[gtemp_j][iR_INDEX] = 0;
    if (rgb_enable & iPWM_G)
        pwm_rgb_tbl[gtemp_j][iG_INDEX] = iPWM_MAX;
    else
        pwm_rgb_tbl[gtemp_j][iG_INDEX] = 0;
    if (rgb_enable & iPWM_B)
        pwm_rgb_tbl[gtemp_j][iB_INDEX] = iPWM_MAX;
    else
        pwm_rgb_tbl[gtemp_j][iB_INDEX] = 0;
}

extern void key_into_fcc_test(void);

/******************************************************************************
 * 函数名称: logo_process
 * 功能描述:
 * 输入参数:
 * 输出参数:
 * 说    明:
 ******************************************************************************/
void device_led_process(void)
{
    //-----------------------------------------------------------------------------
    // 模式提示
    //
    if (0 != hint_time_count)
    {
        hint_led_status |= bled_hint_mode;
        if (0 != (hint_time_count & 0x08))
        {
            bdevice_led_status = true;
            hint_led_status |= (bled_hint_mode | bled_24g | bled_bt2 | bled_bt1 | bled_bt0);
        }
        else
        {
            device_led_off();
        }

        hint_time_count--;
        if (2 > hint_time_count)
        {
            hint_time_count = 0;
            hint_led_status = 0x00;
        }

        return;
    }

    rtc_cur_value = drv_rtc_read(OM_RTC);
    if (iDEVICE_TYPE_USB != device_type)
    {
        if (ble_24g_ctrl & bom6239_conning)
        {
            hint_led_status |= bled_hint_mode;
            if (0 == device_conn_index)
            {
                device_conn_index++;
                led_delay_count = rtc_cur_value;
            }
            else if (1 == device_conn_index)
            {
                if (ble_24g_ctrl & bom6239_conn_ok)
                {
                    device_conn_index++;
                    led_delay_count = rtc_cur_value;
                    device_hint_count = 0;
                }

                key_into_fcc_test();
                if (false == bdevice_led_status)
                {
                    device_led_off();
                    if (mMS_TO_TICK(250) < (rtc_cur_value - led_delay_count))
                    {
                        bdevice_led_status = true;
                        led_delay_count = rtc_cur_value;
                    }
                }
                else
                {
                    device_led_on();
                    if (mMS_TO_TICK(250) < (rtc_cur_value - led_delay_count))
                    {
                        bdevice_led_status = false;
                        led_delay_count = rtc_cur_value;
                    }
                }
            }
            else if (2 == device_conn_index)
            {
                device_conn_index = 0;
                ble_24g_ctrl &= ~bom6239_conning;
            }
            return;
        }
        else if (ble_24g_ctrl & bom6239_sync_ing)
        {
            if (true == b24g_nromal)
                return;

            hint_led_status |= bled_hint_mode;
            if (0 == device_conn_index)
            {
                device_conn_index++;
                led_delay_count = rtc_cur_value;
            }
            else if (1 == device_conn_index)
            {
                if (ble_24g_ctrl & bom6239_sync_ok)
                {
                    device_conn_index++;
                    led_delay_count = rtc_cur_value;
                    device_hint_count = 0;
                }

                if (false == bdevice_led_status)
                {
                    device_led_off();
                    if (mMS_TO_TICK(1000) < (rtc_cur_value - led_delay_count))
                    {
                        bdevice_led_status = true;
                        led_delay_count = rtc_cur_value;
                    }
                }
                else
                {
                    device_led_on();
                    if (mMS_TO_TICK(1000) < (rtc_cur_value - led_delay_count))
                    {
                        bdevice_led_status = false;
                        led_delay_count = rtc_cur_value;
                    }
                }
            }
            else if (2 == device_conn_index)
            {
                device_conn_index = 0;
                ble_24g_ctrl &= ~bom6239_sync_ing;
            }
            return;
        }
        hint_led_status &= ~bled_hint_mode;
        device_conn_index = 0;
    }
    else
    {
        hint_led_status &= ~bled_hint_mode;
        if (0 != (usb_ctrl_flag & busb_suspend_resume))
            return;
    }
}

static void logo_frame(void);

/******************************************************************************
 * 函数名称: logo_process
 * 功能描述: 计算一帧灯效, 算完再交给 drv_ledc_write 发送
 * 说    明: 24G/USB 下 drv_ledc_write 在中断里定时调用, 直接发计算缓冲会发出
 *           算到一半的帧(如单点跑马先全灭再点亮, 会闪一下)
 ******************************************************************************/
void logo_process(void)
{
    logo_frame();

    OM_CRITICAL_BEGIN();
    memcpy(pwm_out_tbl, pwm_rgb_tbl, sizeof(pwm_out_tbl));
    OM_CRITICAL_END();
}

static void logo_frame(void)
{
    uint8_t i;

    if (true == bsave_status)
    {
        bsave_status = false;
        flash_save_status();
    }
    if (true == bsave_cfg_dpi)
    {
        bsave_cfg_dpi = false;
        flash_save_cfg_dpi();
    }
    if (iDEVICE_TYPE_24G == device_type)
    {
        if (true == bflash_ctrl)
            return;
    }

    if (iDEVICE_TYPE_USB == device_type)
    {
        if (0 != (usb_ctrl_flag & busb_suspend_resume))
        {
            logo_off();
            return;
        }
    }
    else
    {
        if (ble_24g_ctrl & bom6239_sync_ing)
        {
            logo_off();
            return;
        }
    }

    // if((false == bled_bright_en) || (iLED_LEV_MIN==led_light_lev) || (iLOW_POWER>=bat_value))
    if ((false == bled_bright_en) || (iLOW_POWER >= bat_value))
    {
        clr_rgb();
        logo_off();
        return;
    }

    if ((bat_status & bcharge_ok) == 0)
    {
        if (iLOW_POWER >= bat_value)
        {
            clr_rgb();
            logo_off();
            return;
        }
    }

    if (false == bargb_valid)
    {
        OM_GPIO0->DATAOUTSET = (1 << ARGB_OFF_PIN);
        ledc_init();
        OM_GPIO0->DATAOUTCLR = (1 << ARGB_OUT_PIN);
    }

    if (led_pre_mode != led_cur_mode)
    {
        led_pre_mode = led_cur_mode;

        led_process_index = 0;
        rgb_index = 0;

        clr_rgb();
        memset(pwm_duty_array_0, 0x00, 16);
        memset(pwm_duty_array_1, 0x00, 16);
        memset(pwm_duty_array_2, 0x00, 16);
        memset(pwm_duty_array_3, 0x00, 16);

        switch (led_cur_mode)
        {
        case iMODE_LED_OFF:
            break;

        case iMODE_BULLET_LIGHT:
            delay_time_count = 0xFF;
            bullet_light();
            break;

        case iMODE_QICAI_SHENG_SHUO:
            delay_time_count = 0xFF;
            qicai_sheng_shuo();
            break;

        case iMODE_LED_NH:
            delay_time_count = 0xFF;
            led_mode_nhjb();
            break;

        case iMODE_LED_QICAI_BREATH:
            pwm_time_count = 0;
            delay_time_count = 0xFF;
            rgb_enable = rgb_en_tbl[led_rgb_index & 0x0F];
            break;

        case iMODE_LED_LYKS:
            led_mode_LYKS(start);
            start = 0xFF;
            break;

        case iMODE_LED_ON:
            delay_time_count = 0xff;
            break;

        case iMODE_LED_QCLG:
            delay_time_count = 0xFF;
            led_mode_QCLG();
            drv_ledc_write();
            break;

        case iMODE_LED_BBDY:
            delay_time_count = 0xFF;
            led_mode_BBDY();
            break;
        }
    }
    else
    {
        static uint32_t next_frame;
        uint32_t now = drv_rtc_read(OM_RTC);

        if ((int32_t)(now - next_frame) < 0)
            return;                                 // 未到下一帧
        next_frame += iLOGO_FRAME_TICK;
        if ((int32_t)(now - next_frame) >= 0)
            next_frame = now + iLOGO_FRAME_TICK;    // 落后超过一帧(休眠/关灯后)直接对齐, 不追帧

        switch (led_cur_mode)
        {
        case iMODE_LED_OFF:
            break;

        case iMODE_BULLET_LIGHT:
            bullet_light();
            bullet_light();
            break;

        case iMODE_QICAI_SHENG_SHUO:
            qicai_sheng_shuo();
            qicai_sheng_shuo();
            break;

        case iMODE_LED_NH:
            led_mode_nhjb();
            break;

        case iMODE_LED_QICAI_BREATH:
            led_mode_qicai_breath();
            break;

        case iMODE_LED_LYKS:
            led_mode_LYKS(start);
            start = 0xFF;
            break;

        case iMODE_LED_ON:
            led_mode_on();
            break;

        case iMODE_LED_QCLG:
            led_mode_QCLG();
            break;

        case iMODE_LED_BBDY:
            led_mode_BBDY();
            break;
        }
    }
}
