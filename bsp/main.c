#include "system.h"
#include "yolo_load_param.h"
#include "yolo_accel_ctrl.h"
#include "sigmoid_exp_table.h"
#include "icb_ctrl.h"

int main()
{

    UART_Init();        // 初始化UART
    printf("%s", "Hello world SparrowRV\n");

    SD_Init();          // 初始化SD卡
    Trap_Init();        // 初始化中断
    FPIOA_Init();       // 初始化fpioa
    Time_Init();        // 初始化定时器
    load_SD_param();    // 加载YOLO参数，参数存放在SD卡中

    uint32_t time_tmp;  // 定时器计数

    while(1) {
        timer_en_ctrl(ENABLE);      // 使能定时器

//        wait_pl_finish();   // 等待图像存储ddr结束中断

        yolo_forward();             // 执行YOLO前向传播，处理输入数据

        time_tmp = timer_cnt_val_read();
        printf("timer_cnt = %u\n",time_tmp);

        yolo_detect();                      // 执行YOLO检测，处理前向传播的结果
    }
    return 0;

}

//**********代修改**********//
//wait_pl_finish
//Xil_DCacheFlushRange
