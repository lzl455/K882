/*******************************************************************************
* 文件名称: mode_switch.h
* 功能说明: 模式切换的判断逻辑(纯函数, 不访问硬件, 可在PC上测试: tests/test_mode_switch.c)
* 使用前需定义 iDEVICE_TYPE_USB (main.h)
*
* 规则: 上电: 插着USB且允许切USB -> USB, 否则回到最后使用的无线模式(BT0~BT2/24G)
*       运行: 只看USB 5V插拔(去抖后): 插入且允许 -> 复位进USB; USB模式下拔出 -> 复位回无线
*       所有切换都通过复位完成, 由上电流程按上面规则重新选模式
*******************************************************************************/
#ifndef __MODE_SWITCH_H__
#define __MODE_SWITCH_H__

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    uint8_t     stable;                         // 去抖后的电平
    uint8_t     raw;                            // 最近一次采样
    uint32_t    t;                              // raw 最近一次变化的时刻
} vbus_debounce_t;

static inline void vbus_debounce_init(vbus_debounce_t *d, uint8_t level)
{
    d->stable = level;
    d->raw = level;
    d->t = 0;
}

/* 电平保持 hold 时长不变才接受; 返回 true 表示 stable 刚发生变化 */
static inline bool vbus_debounce(vbus_debounce_t *d, uint8_t level, uint32_t now, uint32_t hold)
{
    if(level != d->raw)
    {
        d->raw = level;
        d->t = now;
        return false;
    }
    if((level == d->stable) || ((now - d->t) < hold))
        return false;

    d->stable = level;
    return true;
}

/* 上电选择工作模式 */
static inline uint8_t mode_boot_select(uint8_t vbus, uint8_t usb_allowed, uint8_t wireless)
{
    return (vbus && usb_allowed) ? (uint8_t)iDEVICE_TYPE_USB : wireless;
}

/* USB插拔(去抖后)是否需要复位重新选模式 */
static inline bool mode_need_reset(uint8_t vbus, uint8_t usb_allowed, uint8_t cur)
{
    if(vbus)
        return usb_allowed && (iDEVICE_TYPE_USB != cur);
    return iDEVICE_TYPE_USB == cur;
}

#endif
