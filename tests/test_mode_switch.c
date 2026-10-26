/* 模式切换逻辑的PC端测试(不编进固件)
 * 运行: gcc -std=c99 -Wall -I../projects/mouse_demo/periph test_mode_switch.c -o test_mode_switch && ./test_mode_switch
 */
#include <assert.h>
#include <stdio.h>

enum { iDEVICE_TYPE_BT0, iDEVICE_TYPE_BT1, iDEVICE_TYPE_BT2, iDEVICE_TYPE_24G, iDEVICE_TYPE_USB };
#include "mode_switch.h"

int main(void)
{
    vbus_debounce_t d;

    /* 上电选模式 */
    assert(iDEVICE_TYPE_USB == mode_boot_select(1, 1, iDEVICE_TYPE_24G));
    assert(iDEVICE_TYPE_BT1 == mode_boot_select(1, 0, iDEVICE_TYPE_BT1));   /* 插着线按键切到无线: 留在无线 */
    assert(iDEVICE_TYPE_24G == mode_boot_select(0, 1, iDEVICE_TYPE_24G));   /* 拔线: 回最后的无线模式 */

    /* 运行中插拔 */
    assert( mode_need_reset(1, 1, iDEVICE_TYPE_24G));                       /* 无线下插入 -> USB */
    assert(!mode_need_reset(1, 0, iDEVICE_TYPE_BT0));                       /* 不允许切USB */
    assert(!mode_need_reset(1, 1, iDEVICE_TYPE_USB));                       /* 已经是USB */
    assert( mode_need_reset(0, 1, iDEVICE_TYPE_USB));                       /* USB下拔出 -> 无线 */
    assert(!mode_need_reset(0, 1, iDEVICE_TYPE_BT2));                       /* 无线下拔出(充电线) */

    /* 去抖 */
    vbus_debounce_init(&d, 0);
    assert(!vbus_debounce(&d, 1, 100, 50));                                 /* 变化, 开始计时 */
    assert(!vbus_debounce(&d, 1, 149, 50));                                 /* 未满50 */
    assert(!vbus_debounce(&d, 0, 150, 50));                                 /* 抖回0 */
    assert(!vbus_debounce(&d, 1, 160, 50));                                 /* 重新计时 */
    assert( vbus_debounce(&d, 1, 210, 50) && 1 == d.stable);                /* 稳定50 -> 接受 */
    assert(!vbus_debounce(&d, 1, 300, 50));                                 /* 只报一次 */
    assert(!vbus_debounce(&d, 0, 400, 50) && !vbus_debounce(&d, 1, 401, 50)
           && !vbus_debounce(&d, 1, 999, 50) && 1 == d.stable);              /* 短暂掉电不算拔出 */

    vbus_debounce_init(&d, 1);                                              /* RTC计数回绕 */
    assert(!vbus_debounce(&d, 0, 0xFFFFFFF0u, 50));
    assert( vbus_debounce(&d, 0, 0x30u, 50) && 0 == d.stable);

    puts("mode_switch: all tests passed");
    return 0;
}
