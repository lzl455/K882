/*******************************************************************************
* Copyright (c) 2013, SHENZHEN SEMITEK Co.,LTD.
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
#include "led.h"
#include "led_logo_tbl.h"

//-----------------------------------------------------------------------------;
// 常量定义区
//

//-----------------------------------------------------------------------------;
// 变量定义区
//

#define iLFSR_MULTINOMIAL       0xF521

bool                            bled_run_dir;

uint16_t                        lfsr_value;
uint8_t                         start_dec_count;
uint8_t                         snake_led_index;
extern uint8_t                  breach_color_index;
extern uint8_t                  on_color_index;

//-----------------------------------------------------------------------------;
// 表格定义区
//

//=============================================================================;
// 代码区                                                                      ;
//=============================================================================;

void bullet_light(void) {  //Bullet Light  子弹灯  彩虹小马跑  // 效果入口：单点跑马，按序切色
    static unsigned char inited;                                 // 是否已初始化（进入模式后首帧设置一次）
    static unsigned char stage;                                  // 当前颜色阶段索引（0..6：红绿蓝紫黄青白）
    static int16_t pos;                                          // 当前点亮的灯珠位置（0..iLED_MAX-1）
    static unsigned char manual_lock;                            // 颜色手动锁定标志（按下 Fn+Right 后为 1）
    static unsigned char prev_led_rgb_index;                     // 上一帧记录的颜色索引（用于检测手动切色）
    static bool prev_dir;                                        // 上一帧记录的方向（用于检测方向切换）
    static unsigned char init_speed_val;
    static unsigned char init_slow;


    // 数据全局变量
    if (init_slow && (led_speed_lev != init_speed_val)) init_slow = 0;
    {
        unsigned char speed_lev = (unsigned char)led_speed_lev;
        if (speed_lev > iSPEED_LEV_MAX) {speed_lev = iSPEED_LEV_MAX;}
        speed_lev = (unsigned char)(speed_lev + 2);
        unsigned char eff_lev = iSPEED_LEV_MIN + (((unsigned char)(speed_lev - iSPEED_LEV_MIN)) << 1);
        unsigned short threshold = (unsigned short)(init_slow ? (eff_lev << 5) : (eff_lev << 3));
        if (led_speed_lev == iSPEED_LEV_MAX) {
            threshold = (unsigned short)(threshold >> 2);
            if (threshold == 0) threshold = 1;        
        }
        if (threshold > delay_time_count) {
            delay_time_count++;
            return;
        }
    }
    delay_time_count = 0;                                        // 到推进时机：清零延时计数

    if (!inited || (delay_time_count == 0xFF)) {                 // 首次进入或模式切换首帧（入口复位）
        inited = 1;                                              // 标记已初始化
        stage = 0;                                               // 从序列第一个颜色开始（红）
        led_speed_lev = iSPEED_LEV_2;
        init_speed_val = led_speed_lev;
        init_slow = 1;
        // pos = bled_run_dir ?  0  :   (iLED_MAX - 1) ;              // 根据方向设置起始灯位：右→左从最右开始；左→右从最左开始
            pos = bled_run_dir ? (iLED_MAX - 1) : 0;              // 根据方向设置起始灯位：右→左从最右开始；左→右从最左开始
        manual_lock = 0;                                         // 初始不锁定颜色（自动遍历颜色序列）
        prev_led_rgb_index = led_rgb_index;                      // 记录当前颜色索引（用于侦测 Fn+Right 手动切色）
        prev_dir = bled_run_dir;                                 // 记录当前方向（用于侦测 Fn+Left 切方向）
    }

    if (prev_led_rgb_index != led_rgb_index) {                   // 侦测到颜色索引变化（Fn+Right 手动切色）  led_rgb_index颜色全局变量
        stage = (led_rgb_index & 0x07);                          // 切到手动选择的颜色阶段（0..6）
        manual_lock = 1;                                         // 锁定颜色，不再自动遍历
        prev_led_rgb_index = led_rgb_index;                      // 更新上一帧颜色索引
    }

    if (prev_dir != bled_run_dir) {                              // 侦测到方向变化（Fn+Left 切方向）  方向全局变量
        manual_lock = 0;                                         // 解除颜色锁定（恢复自动遍历颜色）
        prev_dir = bled_run_dir;                                 // 更新上一帧方向
    }

    static const unsigned char seq[7] = {                        // 颜色序列：红、绿、蓝、黄 紫、、青、白
        iPWM_R,
        iPWM_G,
        iPWM_B,
        (iPWM_R|iPWM_B),  //紫色
        (iPWM_R|iPWM_G),  //黄色
        (iPWM_G | iPWM_B),
        (iPWM_R | iPWM_G | iPWM_B)
    };

    rgb_enable = seq[stage];                                     // 选中当前阶段颜色作为“单点”显示的通道掩码

    for (gtemp_x = 0; gtemp_x < iLED_MAX; gtemp_x++) {           // 先清空整排灯珠的 RGB（确保“上一灯灭，下一灯亮”）
        pwm_rgb_tbl[gtemp_x][iR_INDEX] = 0;
        pwm_rgb_tbl[gtemp_x][iG_INDEX] = 0;
        pwm_rgb_tbl[gtemp_x][iB_INDEX] = 0;
    }

    if (pos >= 0 && pos < iLED_MAX) {
        unsigned char i0 = (unsigned char)pos;
        pwm_rgb_tbl[i0][iR_INDEX] = (rgb_enable & iPWM_R) ? iPWM_MAX : 0;
        pwm_rgb_tbl[i0][iG_INDEX] = (rgb_enable & iPWM_G) ? iPWM_MAX : 0;
        pwm_rgb_tbl[i0][iB_INDEX] = (rgb_enable & iPWM_B) ? iPWM_MAX : 0;
    }

    if (bled_run_dir) {
        pos += 1;
        if (pos >= iLED_MAX) {
            pos = 0;
            if (!manual_lock) {
                stage++;
                if (stage >= 7) stage = 0;
            }
        }
    } else {
        pos -= 1;
        if (pos < 0) {
            pos = (iLED_MAX - 1);
            if (!manual_lock) {
                stage++;
                if (stage >= 7) stage = 0;
            }
        }
    }
}


void qicai_sheng_shuo(void) {
    static unsigned char inited;
    static unsigned char stage;
    static unsigned char manual_lock;
    static unsigned char prev_led_rgb_index;
    static bool prev_dir;
    static unsigned char phase;
    static unsigned char count;
    static unsigned char init_speed_val;
    static unsigned char init_slow;
    unsigned char speed_lev = led_speed_lev;
    if (speed_lev == iSPEED_LEV_MIN) speed_lev = iSPEED_LEV_2;
    if (init_slow && (speed_lev != init_speed_val)) init_slow = 0;
    {
        unsigned short threshold = (unsigned short)(speed_lev * (init_slow ? 20 : 10));
        if(threshold > delay_time_count){
            delay_time_count++;
            return;
        }
    }
    delay_time_count = 0;

    if(!inited || (delay_time_count == 0xFF)){
        inited = 1;
        stage = 0;
        manual_lock = 0;
        prev_led_rgb_index = led_rgb_index;
        prev_dir = bled_run_dir;
        phase = 0;
        count = 0;
        led_speed_lev = iSPEED_LEV_2;



        init_speed_val = led_speed_lev;
        init_slow = 1;
    }

    if (prev_led_rgb_index != led_rgb_index) {
        stage = (led_rgb_index & 0x07);
        manual_lock = 1;
        prev_led_rgb_index = led_rgb_index;
    }

    if (prev_dir != bled_run_dir) {
        manual_lock = 0;
        prev_dir = bled_run_dir;
        phase = 0;
        count = 0;
    }


    static const unsigned char seq[8] = {                        // 八色序列：蓝、青、绿、紫、红、紫、蓝、青
        iPWM_B,                    // 蓝
        (iPWM_G|iPWM_B),           // 青
        iPWM_G,                    // 绿
        (iPWM_R|iPWM_B),           // 紫
        iPWM_R,                    // 红
        (iPWM_R|iPWM_B),           // 紫（重复）
        iPWM_B,                    // 蓝（重复）
        (iPWM_G|iPWM_B)            // 青（重复）
    };

    


    rgb_enable = seq[stage];

    for (gtemp_x = 0; gtemp_x < iLED_MAX; gtemp_x++) {
        pwm_rgb_tbl[gtemp_x][iR_INDEX] = 0;
        pwm_rgb_tbl[gtemp_x][iG_INDEX] = 0;
        pwm_rgb_tbl[gtemp_x][iB_INDEX] = 0;
    }

    if(phase == 0){
        if(count < iLED_MAX) count++;
        else phase = 1;
    } else {
        if(count > 0) count--;
        else {
            phase = 0;
            if (!manual_lock) { stage++; if (stage >= 8) stage = 0; }
        }
    }

    if(count){
        int16_t start, end;
        if(bled_run_dir){                    // 方向：1→左→右；0→右→左
            start = 0;
            end = (int16_t)count - 1;
        } else {
            start = (int16_t)iLED_MAX - (int16_t)count;
            end = (int16_t)iLED_MAX - 1;
        }
        if(start < 0) start = 0;
        if(end >= (int16_t)iLED_MAX) end = (int16_t)iLED_MAX - 1;
        for(gtemp_x = (unsigned char)start; gtemp_x <= (unsigned char)end; gtemp_x++){
            if (rgb_enable & iPWM_R) pwm_rgb_tbl[gtemp_x][iR_INDEX] = iPWM_MAX; else pwm_rgb_tbl[gtemp_x][iR_INDEX] = 0;
            if (rgb_enable & iPWM_G) pwm_rgb_tbl[gtemp_x][iG_INDEX] = iPWM_MAX; else pwm_rgb_tbl[gtemp_x][iG_INDEX] = 0;
            if (rgb_enable & iPWM_B) pwm_rgb_tbl[gtemp_x][iB_INDEX] = iPWM_MAX; else pwm_rgb_tbl[gtemp_x][iB_INDEX] = 0;
        }
    }
}

void led_mode_nhjb(void)
{
    unsigned char i;

    if(led_speed_lev>delay_time_count)
    {
		delay_time_count++;
        return;
	}
    delay_time_count = 0;

    if(0 == led_process_index)
    {
        pwm_duty_array_0[1]++;
        if(iPWM_MAX <= pwm_duty_array_0[1])
            led_process_index++;
    }
    else if(1 == led_process_index)
    {
        pwm_duty_array_0[0]--;
        if(0 == pwm_duty_array_0[0])
            led_process_index++;
    }
    else if(2 == led_process_index)
    {
        pwm_duty_array_0[2]++;
        if(iPWM_MAX <= pwm_duty_array_0[2])
            led_process_index++;
    }
    else if(3 == led_process_index)
    {
        pwm_duty_array_0[1]--;
        if(0 == pwm_duty_array_0[1])
            led_process_index++;
    }
    else if(4 == led_process_index)
    {
        pwm_duty_array_0[0]++;
        if(iPWM_MAX <= pwm_duty_array_0[0])
            led_process_index++;
    }
    else //if(5 == led_process_index)
    {
        pwm_duty_array_0[2]--;
        if(0 == pwm_duty_array_0[2])
            led_process_index = 0;
    }

    for(i=0; i<iLED_MAX; i++)
    {
        pwm_rgb_tbl[i][0] = pwm_duty_array_0[iR_INDEX];
        pwm_rgb_tbl[i][1] = pwm_duty_array_0[iG_INDEX];
        pwm_rgb_tbl[i][2] = pwm_duty_array_0[iB_INDEX];
    }
}


void led_mode_qicai_breath(void)
{
    static unsigned char inited;
    static unsigned char manual_lock;
    static unsigned char stage;
    static unsigned char prev_led_rgb_index;
    static bool prev_dir;
    unsigned char i;

    if(!inited || (delay_time_count == 0xFF)){
        inited = 1;
        manual_lock = 0;
        stage = 0;
        prev_led_rgb_index = led_rgb_index;
        prev_dir = bled_run_dir;
        rgb_index = 0;
        pwm_time_count = 0;
        bback_led_status = 0;
    }

        // 确保呼吸速度有下限，避免过快
    unsigned char effective_speed = led_speed_lev;
    if (effective_speed < 2) effective_speed = 2; // 最低速度为 iSPEED_LEV_2
    
    if(effective_speed>delay_time_count)
    {
        delay_time_count++;
        return;
    }
    delay_time_count = 0;

    if(!bback_led_status){
        if(pwm_time_count >= iPWM_MAX)
            pwm_value = 0;
        else
            pwm_value = (iPWM_MAX - pwm_time_count);
    }
    else{
        if(pwm_time_count >= iPWM_MAX)
            pwm_value = iPWM_MAX;
        else
            pwm_value = pwm_time_count;
    }

    if(pwm_time_count == iPWM_MAX){
        pwm_time_count = 0;
        bback_led_status = !bback_led_status;
        if(bback_led_status){
            if(!manual_lock){
                rgb_index++;
                if(rgb_index >= 7)
                    rgb_index = 0;
            }
        }
    }

    if(prev_led_rgb_index != led_rgb_index){
        stage = (led_rgb_index & 0x07);
        manual_lock = 1;
        prev_led_rgb_index = led_rgb_index;
    }

    if(manual_lock && (prev_dir != bled_run_dir)){
        manual_lock = 0;
        prev_dir = bled_run_dir;
    }

    {
        static const unsigned char seq[8] = {  
        iPWM_R,                // 红
        iPWM_G,                // 绿
        iPWM_B,                    // 蓝
        (iPWM_R|iPWM_B),           // 紫
        (iPWM_R|iPWM_G),           //黄色
        (iPWM_G|iPWM_B),           // 青
        (iPWM_G|iPWM_B|iPWM_R)      //白
    };

        uint8_t step = manual_lock ? stage : (rgb_index % 7);
        rgb_enable = seq[step];
    }

    for(i=0; i<iLED_MAX; i++)
    {
        pwm_rgb_tbl[i][iR_INDEX] = (rgb_enable&iPWM_R) ? pwm_value : 0;
        pwm_rgb_tbl[i][iG_INDEX] = (rgb_enable&iPWM_G) ? pwm_value : 0;
        pwm_rgb_tbl[i][iB_INDEX] = (rgb_enable&iPWM_B) ? pwm_value : 0;
    }

    pwm_time_count++;
}

void led_mode_LYKS(unsigned char start){

    static unsigned char inited; // 初始化标志，首次或复位时置1
    static unsigned char stage; // 当前颜色阶段索引，0..6循环
    static int16_t rip_start[8]; // 波源起始位置数组
    static unsigned char rip_radius[8]; // 外圈半径，右侧扩散指针
    static unsigned char rip_active[8]; // 波源是否激活标志
    static uint16_t rip_tick[8]; // 每个波源的节拍累加计数
    static unsigned char rip_state[8]; // 每个波源的颜色阶段索引
    static int16_t rip_pos[8]; // 内圈位置，左侧扩散指针

    unsigned char evt = 0; // 是否有触发事件

    if(!inited || (delay_time_count == 0xFF)){ // 首次进入或计数异常时，执行初始化
        inited = 1; // 标记已初始化
        stage = 0; // 颜色阶段复位到起始（红）
        for(unsigned char i=0; i<8; i++){ // 清空所有波源的状态
            rip_active[i] = 0; // 标记为空闲
            rip_radius[i] = 0; // 半径复位
            rip_tick[i] = 0; // 节拍复位
            rip_state[i] = 0; // 颜色阶段复位
            rip_pos[i] = 0; // 内圈位置复位
        }
        for(gtemp_x=0; gtemp_x<iLED_MAX; gtemp_x++){ // 清空显示缓冲，所有灯灭
            pwm_rgb_tbl[gtemp_x][iR_INDEX] = 0; // R通道关
            pwm_rgb_tbl[gtemp_x][iG_INDEX] = 0; // G通道关
            pwm_rgb_tbl[gtemp_x][iB_INDEX] = 0; // B通道关
        }
    }

    if(start < iLED_MAX){ // 有合法的触发位置（波源）
        unsigned char idx = 0xFF; // 预置无可用槽
        for(unsigned char i=0; i<8; i++){ // 从并行槽中寻找空闲波源
            if(!rip_active[i]){ idx = i; break; } // 找到空槽，记录并退出
        }
        if(idx != 0xFF){ // 找到可用槽，初始化该波源
            rip_start[idx] = (int16_t)start; // 记录波源起点
            rip_pos[idx] = (int16_t)start - 1; // 内圈从起点左侧一格开始
            rip_radius[idx] = (unsigned char)((unsigned char)start + 1); // 外圈从起点右侧一格开始
            rip_tick[idx] = 0; // 清零节拍
            rip_state[idx] = stage; // 使用当前颜色阶段
            rip_active[idx] = 1; // 激活该波源
            stage++; if(stage >= 7) stage = 0; // 下一次触发时切到下一颜色
        }
        evt = 1; // 记录本帧有触发事件
    }

    if(!evt){ // 无触发事件时，按速度档位做节流
        if(led_speed_lev>delay_time_count){ // 尚未到更新时机
            delay_time_count++; // 累加延时计数
            return; // 暂不刷新
        }
    }
    delay_time_count = 0; // 到刷新时机，清零延时计数

    static const unsigned char color_seq[7] = { // 七色序列，依次红黄绿青蓝品红白    红蓝紫白绿黄青		
        iPWM_R, // 红
        iPWM_B, // 蓝
        (iPWM_R|iPWM_B), // 品红
        (iPWM_R|iPWM_G|iPWM_B), // 白（全开）
        iPWM_G, // 绿
        (iPWM_R|iPWM_G), // 黄
        (iPWM_G|iPWM_B) // 青
    };
    

    for(gtemp_x=0; gtemp_x<iLED_MAX; gtemp_x++){ // 每次绘制前清屏
        pwm_rgb_tbl[gtemp_x][iR_INDEX] = 0; // R清零
        pwm_rgb_tbl[gtemp_x][iG_INDEX] = 0; // G清零
        pwm_rgb_tbl[gtemp_x][iB_INDEX] = 0; // B清零
    }

    { // 绘制每个激活的涟漪波源
        for(unsigned char i=0; i<8; i++){ // 遍历所有并行波源
            if(!rip_active[i]) continue; // 跳过未激活的波源
            {
                int16_t lp = rip_pos[i]; // 当前内圈位置（向左扩散）
                int16_t rp = (int16_t)rip_radius[i]; // 当前外圈位置（向右扩散）
                unsigned char mask = color_seq[rip_state[i]]; // 波源使用的颜色掩码
                unsigned char val = (unsigned char)(((unsigned int)iPWM_MAX * (unsigned int)(5)) / (unsigned int)5); // 按亮度档映射到PWM值
                unsigned char val2 = val; // 备份一份亮度值
                if(lp >= 0 && lp < (int16_t)iLED_MAX){ // 内圈在有效范围
                    if(mask & iPWM_R) pwm_rgb_tbl[(unsigned char)lp][iR_INDEX] = val2; else pwm_rgb_tbl[(unsigned char)lp][iR_INDEX] = 0; // 内圈R通道赋值/清零
                    if(mask & iPWM_G) pwm_rgb_tbl[(unsigned char)lp][iG_INDEX] = val2; else pwm_rgb_tbl[(unsigned char)lp][iG_INDEX] = 0; // 内圈G通道赋值/清零
                    if(mask & iPWM_B) pwm_rgb_tbl[(unsigned char)lp][iB_INDEX] = val2; else pwm_rgb_tbl[(unsigned char)lp][iB_INDEX] = 0; // 内圈B通道赋值/清零
                }
                if(rp >= 0 && rp < (int16_t)iLED_MAX){ // 外圈在有效范围
                    if(mask & iPWM_R) pwm_rgb_tbl[(unsigned char)rp][iR_INDEX] = val2; else pwm_rgb_tbl[(unsigned char)rp][iR_INDEX] = 0; // 外圈R通道赋值/清零
                    if(mask & iPWM_G) pwm_rgb_tbl[(unsigned char)rp][iG_INDEX] = val2; else pwm_rgb_tbl[(unsigned char)rp][iG_INDEX] = 0; // 外圈G通道赋值/清零
                    if(mask & iPWM_B) pwm_rgb_tbl[(unsigned char)rp][iB_INDEX] = val2; else pwm_rgb_tbl[(unsigned char)rp][iB_INDEX] = 0; // 外圈B通道赋值/清零
                }
            }
            rip_tick[i] += 2; // 节拍累加（每帧推进2）
            if(rip_tick[i] >= (unsigned short)(((iPWM_MAX >> 1) * 10) / 11)){ // 达到推进门限时移动波源
                rip_tick[i] = 0; // 清零节拍
                rip_pos[i]--;  // 内圈向左移动一步
                rip_radius[i]++;  // 外圈向右移动一步
                if((rip_pos[i] < 0) && (rip_radius[i] >= iLED_MAX)){ // 两端都越界，认为该波源寿终正寝
                    rip_active[i] = 0; // 关闭该波源    
                }
            }
        }
    }
}

void led_mode_on(void)
{
    static const unsigned char seq[7] = {
        iPWM_R,
        iPWM_G,
        iPWM_B,
        (iPWM_R|iPWM_B), 
        (iPWM_R|iPWM_G), 
        (iPWM_G | iPWM_B),
        (iPWM_R | iPWM_G | iPWM_B)
    };
    
    rgb_enable = seq[led_rgb_index % 7 ];  

    for(gtemp_j=0; gtemp_j<iLED_MAX; gtemp_j++)
    {
        rgb_w_max();
    }
}

void led_mode_QCLG (void ){         //蓝色和青色之间会短
    static unsigned char inited; // 初始化标志
    static unsigned char offset; // 颜色序列偏移量（整体平移）
    static uint16_t step_tick; // 步进计数

    if((led_speed_lev >> 2) > delay_time_count) // 未到推进时机则节流（速度放大两倍）
    {
        delay_time_count++; // 递增延时计数
        return; // 暂不更新
    }
    delay_time_count = 0; // 到推进时机，清零计数

    if(!inited || (delay_time_count == 0xFF)) // 首次或异常重置
    {
        inited = 1; // 标记已初始化
        offset = 0; // 偏移从0开始
        step_tick = 0; // 步进计数清零
    }

    {
        static const unsigned char color_seq[7] = { // 七色序列，用于滚动
            iPWM_R|iPWM_G,   // 黄
            iPWM_R,          // 橙 (红)
            iPWM_R|iPWM_B,   // 品红
            iPWM_R|iPWM_B,   // 紫 (同品红)
            iPWM_B,          // 蓝
            iPWM_G|iPWM_B,   // 青
            iPWM_G           // 绿
        };
        uint16_t unit_count = (iLED_MAX >> 1);
        uint16_t phase_step = unit_count ? (iPWM_MAX / unit_count) : iPWM_MAX;
        if (phase_step == 0) phase_step = 1;

        {
            uint16_t add = (led_speed_lev == 0) ? 2 : 1;
            step_tick += add;
            if (step_tick >= iPWM_MAX) {
                step_tick -= iPWM_MAX;
                if (bled_run_dir == 0) {
                    offset++;
                    if (offset >= 7) offset = 0;
                } else {
                    if (offset == 0) {
                        offset = 6;
                    } else {
                        offset--;
                    }
                }
            }
        }

        // 先清空所有LED
        for(gtemp_x = 0; gtemp_x < iLED_MAX; gtemp_x++) {
            pwm_rgb_tbl[gtemp_x][iR_INDEX] = 0;
            pwm_rgb_tbl[gtemp_x][iG_INDEX] = 0;
            pwm_rgb_tbl[gtemp_x][iB_INDEX] = 0;
        }

        // 每2个LED为一组，设置颜色并实现相邻单元间的渐变
        for(gtemp_x = 0; gtemp_x < iLED_MAX; gtemp_x += 2) {
            // 当前单元颜色和下一个单元颜色
            unsigned char unit_index = gtemp_x / 2; // 当前单元索引
            uint32_t phase_acc = (uint32_t)step_tick + ((uint32_t)unit_index * phase_step);
            uint16_t t = (uint16_t)(phase_acc % iPWM_MAX);
            unsigned char color_index = (unsigned char)((unit_index + offset + (phase_acc / iPWM_MAX)) % 7);
            unsigned char color_curr = color_seq[color_index]; // 当前单元颜色
            unsigned char color_next = color_seq[(color_index + 1) % 7]; // 下一个单元颜色

            // 获取当前单元和下一个单元的RGB值
            uint16_t curr_r = (color_curr & iPWM_R) ? iPWM_MAX : 0;
            uint16_t curr_g = (color_curr & iPWM_G) ? iPWM_MAX : 0;
            uint16_t curr_b = (color_curr & iPWM_B) ? iPWM_MAX : 0;
            uint16_t next_r = (color_next & iPWM_R) ? iPWM_MAX : 0;
            uint16_t next_g = (color_next & iPWM_G) ? iPWM_MAX : 0;
            uint16_t next_b = (color_next & iPWM_B) ? iPWM_MAX : 0;

            // 插值因子：0表示完全使用当前颜色，iPWM_MAX表示完全使用下一个颜色
            uint16_t inv = iPWM_MAX - t;

            // 计算插值后的颜色值
            unsigned char duty_r = (unsigned char)((inv * curr_r + t * next_r) / iPWM_MAX);
            unsigned char duty_g = (unsigned char)((inv * curr_g + t * next_g) / iPWM_MAX);
            unsigned char duty_b = (unsigned char)((inv * curr_b + t * next_b) / iPWM_MAX);

            // 第1个LED：使用插值后的颜色
            pwm_rgb_tbl[gtemp_x][iR_INDEX] = duty_r;
            pwm_rgb_tbl[gtemp_x][iG_INDEX] = duty_g;
            pwm_rgb_tbl[gtemp_x][iB_INDEX] = duty_b;

            // 第2个LED（与第1个颜色相同）
            if((gtemp_x + 1) < iLED_MAX) {
                pwm_rgb_tbl[gtemp_x + 1][iR_INDEX] = duty_r;
                pwm_rgb_tbl[gtemp_x + 1][iG_INDEX] = duty_g;
                pwm_rgb_tbl[gtemp_x + 1][iB_INDEX] = duty_b;
            }
        }
    }

}

void led_mode_BBDY(void){
    static unsigned char inited; // 初始化标志
    static unsigned char offset; // 颜色序列偏移量
    static uint16_t bbdy_step_tick; // 步进计数（用于控制滚动节奏）

    if(led_speed_lev>delay_time_count) // 未到推进时机则节流
    {
        delay_time_count++; // 延时计数+1
        return; // 暂不更新
    }
    delay_time_count = 0; // 达到推进时机，清零

    if(!inited || (delay_time_count == 0xFF)) // 首次或异常重置
    {
        inited = 1; // 标记已初始化
        offset = 0; // 序列偏移复位
        bbdy_step_tick = 0; // 步进计数清零
    }

    {

        static const unsigned char color_seq[6] = { // 八色序列：蓝、青、绿、紫、红、紫、蓝、青
            iPWM_B,                    // 蓝
            (iPWM_G|iPWM_B),           // 青
            iPWM_G,                    // 绿
            (iPWM_R|iPWM_B),           // 紫
            iPWM_R,                    // 红
            (iPWM_R|iPWM_B),           // 紫（重复）

        };

        for(gtemp_x = 0; gtemp_x < iLED_MAX; gtemp_x++) // 遍历所有灯位
        {
            unsigned char m = color_seq[(gtemp_x + offset) % 6]; // 当前离散颜色掩码
            if(m & iPWM_R) pwm_rgb_tbl[gtemp_x][iR_INDEX] = iPWM_MAX; else pwm_rgb_tbl[gtemp_x][iR_INDEX] = 0; // R全亮或全灭
            if(m & iPWM_G) pwm_rgb_tbl[gtemp_x][iG_INDEX] = iPWM_MAX; else pwm_rgb_tbl[gtemp_x][iG_INDEX] = 0; // G全亮或全灭
            if(m & iPWM_B) pwm_rgb_tbl[gtemp_x][iB_INDEX] = iPWM_MAX; else pwm_rgb_tbl[gtemp_x][iB_INDEX] = 0; // B全亮或全灭
        }
    }

    bbdy_step_tick += (led_speed_lev == iSPEED_LEV_MIN) ? 2 : 1;
    if(bbdy_step_tick < iPWM_MAX) // 未满一轮则返回
        return; // 下一帧继续
    bbdy_step_tick = 0; // 满一轮清零

    offset++; // 偏移+1，实现滚动
    if(offset >=6 ) offset = 0; // 环回到起点
}

