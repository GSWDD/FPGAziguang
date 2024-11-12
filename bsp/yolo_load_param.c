#include "system.h"
#include "yolo_load_param.h"
#include "yolo_accel_ctrl.h"
#include "sigmoid_exp_table.h"

//// 偏置参数文件名
//char FileBias[14][32]       = {"l0_b.bin", "l2_b.bin", "l4_b.bin", "l6_b.bin", "l8_b.bin", "l8_b.bin", "l10_b.bin", "l12_b.bin",
//                                "l13_b.bin", "l14_b.bin", "l15_b.bin", "l18_b.bin", "l21_b.bin", "l22_b.bin"};
//
//// 激活函数参数文件名
//char FileLeakyRelu[14][32]  = {"l0_r.bin", "l2_r.bin", "l4_r.bin", "l6_r.bin", "l820_r.bin", "l8_r.bin", "l10_r.bin", "l12_r.bin",
//                                "l13_r.bin", "l14_r.bin", "l15_r.bin", "l18_r.bin", "l21_r.bin", "l22_r.bin"};
//
//// 权重参数文件名
//char FileWeight[14][32]     = {"l0_w.bin", "l2_w.bin", "l4_w.bin", "l6_w.bin", "l8_w.bin", "l8_w.bin", "l10_w.bin", "l12_w.bin",
//                                "l13_w.bin", "l14_w.bin", "l15_w.bin", "l18_w.bin", "l21_w.bin", "l22_w.bin"};

// sd扇区地址
int BiasSDAddr[15] = {19984, 19996, 20016, 20060, 20212, 20212, 1032, 3344, 12572, 13092, 15404, 15436, 15520, 17256};
uint32_t BiasLen[15] = {16*4, 32*4, 64*4, 128*4, 256*4, 256*4, 512*4, 1024*4, 256*4, 512*4, 24*4, 128*4, 256*4, 24*4};

int ActSDAddr[15] = {19988, 20000, 20020, 20064, 17276, 452, 1036, 3352, 12576, 13096, 15408, 15440, 15524, 17260};
uint32_t ActLen = 256;

int WeightSDAddr[15] = {19992, 20004, 20024, 20068, 456, 456, 1040, 3356, 12580, 13100, 15412, 15444, 15528, 17264};
uint32_t WeightLen[15] = {16*8*9, 32*16*9, 64*32*9, 128*64*9, 256*128*9, 256*128*9, 512*256*9, 1024*512*9, 256*1024, 512*256*9, 24*512, 128*256, 256*384*9, 24*256};

int BiasAddr[15] = {BIAS_STORE_BASEADDR,
                    BIAS_STORE_BASEADDR+16*4,
                    BIAS_STORE_BASEADDR+(16+32)*4,
                    BIAS_STORE_BASEADDR+(16+32+64)*4,
                    BIAS_STORE_BASEADDR+(16+32+64+128)*4,
                    BIAS_STORE_BASEADDR+(16+32+64+128+256)*4,
                    BIAS_STORE_BASEADDR+(16+32+64+128+256*2)*4,
                    BIAS_STORE_BASEADDR+(16+32+64+128+256*2+512)*4,
                    BIAS_STORE_BASEADDR+(16+32+64+128+256*2+512+1024)*4,
                    BIAS_STORE_BASEADDR+(16+32+64+128+256*2+512+1024+256)*4,
                    BIAS_STORE_BASEADDR+(16+32+64+128+256*2+512+1024+256+512)*4,
                    BIAS_STORE_BASEADDR+(16+32+64+128+256*2+512+1024+256+512+24)*4,
                    BIAS_STORE_BASEADDR+(16+32+64+128+256*2+512+1024+256+512+24+128)*4,
                    BIAS_STORE_BASEADDR+(16+32+64+128+256*2+512+1024+256+512+24+128+256)*4,
                    BIAS_STORE_BASEADDR+(16+32+64+128+256*2+512+1024+256+512+24+128+256+24)*4};

int ActAddr[15] =  {ACT_STORE_BASEADDR,
                    ACT_STORE_BASEADDR+256,
                    ACT_STORE_BASEADDR+256*2,
                    ACT_STORE_BASEADDR+256*3,
                    ACT_STORE_BASEADDR+256*4,
                    ACT_STORE_BASEADDR+256*5,
                    ACT_STORE_BASEADDR+256*6,
                    ACT_STORE_BASEADDR+256*7,
                    ACT_STORE_BASEADDR+256*8,
                    ACT_STORE_BASEADDR+256*9,
                    ACT_STORE_BASEADDR+256*10,
                    ACT_STORE_BASEADDR+256*11,
                    ACT_STORE_BASEADDR+256*12,
                    ACT_STORE_BASEADDR+256*13,
                    ACT_STORE_BASEADDR+256*14};

int WeightAddr[15] =   {WEIGHT_STORE_BASEADDR,
                        WEIGHT_STORE_BASEADDR+16*8*9,
                        WEIGHT_STORE_BASEADDR+16*8*9+32*16*9,
                        WEIGHT_STORE_BASEADDR+16*8*9+32*16*9+64*32*9,
                        WEIGHT_STORE_BASEADDR+16*8*9+32*16*9+64*32*9+128*64*9,
                        WEIGHT_STORE_BASEADDR+16*8*9+32*16*9+64*32*9+128*64*9+256*128*9,
                        WEIGHT_STORE_BASEADDR+16*8*9+32*16*9+64*32*9+128*64*9+256*128*9*2,
                        WEIGHT_STORE_BASEADDR+16*8*9+32*16*9+64*32*9+128*64*9+256*128*9*2+512*256*9,
                        WEIGHT_STORE_BASEADDR+16*8*9+32*16*9+64*32*9+128*64*9+256*128*9*2+512*256*9+1024*512*9,
                        WEIGHT_STORE_BASEADDR+16*8*9+32*16*9+64*32*9+128*64*9+256*128*9*2+512*256*9+1024*512*9+256*1024,
                        WEIGHT_STORE_BASEADDR+16*8*9+32*16*9+64*32*9+128*64*9+256*128*9*2+512*256*9+1024*512*9+256*1024+512*256*9,
                        WEIGHT_STORE_BASEADDR+16*8*9+32*16*9+64*32*9+128*64*9+256*128*9*2+512*256*9+1024*512*9+256*1024+512*256*9+24*512,
                        WEIGHT_STORE_BASEADDR+16*8*9+32*16*9+64*32*9+128*64*9+256*128*9*2+512*256*9+1024*512*9+256*1024+512*256*9+24*512+128*256,
                        WEIGHT_STORE_BASEADDR+16*8*9+32*16*9+64*32*9+128*64*9+256*128*9*2+512*256*9+1024*512*9+256*1024+512*256*9+24*512+128*256+256*384*9,
                        WEIGHT_STORE_BASEADDR+16*8*9+32*16*9+64*32*9+128*64*9+256*128*9*2+512*256*9+1024*512*9+256*1024+512*256*9+24*512+128*256+256*384*9+24*256};

void UART_Init()
{
    init_uart0_printf(115200,1);    //设置波特率
}

void SD_Init()
{
    uint8_t tmp;
    while(sdrd_busy_chk());     //等待启动完成
    tmp = sdrd_init_state_read();
    printf("SDRD state:0x%x \n", tmp);
    tmp = sdrd_busy_chk();
    printf("SDRD busy:0x%x \n", tmp);
}

void FPIOA_Init()
{
    fpioa_perips_in_set(NIO, 4);
    fpioa_nio_mode_write(NIO_4, NIO_MODE_OE_PP);
}

void Trap_Init()
{
    trap_global_ctrl(ENABLE);//打开全局中断
    trap_en_ctrl(TRAP_SOFT, ENABLE);//打开软件中断
//    delay_mtime_us(1);//等待1us
//    core_soft_interrupt();//触发软件中断
//    delay_mtime_us(10);//等待10us
//    printf("soft_trap=%lu\n",trap_mip_state(TRAP_SOFT));//软件已经响应了，返回0
}

void load_SD_param()
{


    printf("%s", "WR_Bias_DDR \n");

    for (int cnt1 = 0; cnt1 < 1; ++cnt1) {

        SYS_RWMEM_W(Lite_Reg6) = BiasAddr[cnt1]; // 偏置ddr地址
        SYS_RWMEM_W(Lite_Reg4) = BiasLen[cnt1]; // 发送bias数据长度
        SYS_RWMEM_W(Lite_Reg5) = 0x00000010; // 表示是偏置/权重/激活

        printf("BiasAddr = 0x%08X \n", BiasAddr[cnt1]);
        printf("BiasLen = 0x%08X \n", BiasLen[cnt1]);

        //bias扇区
        uint32_t sdrd_sector_cnt;   // 当前文件占用扇区个数
        if (BiasLen[cnt1] % 512 == 0) {
            sdrd_sector_cnt = BiasLen[cnt1] / 512;
        }
        else {
            sdrd_sector_cnt = BiasLen[cnt1] / 512 + 1;
        }

        for (int sector_cnt = 0; sector_cnt < sdrd_sector_cnt; ++sector_cnt) {
            sdrd_sector_set(BiasSDAddr[cnt1]+sector_cnt+2048);
            delay_mtime_us(1);
            while(sdrd_busy_chk());

            uint32_t current_sector_len;
            if (sector_cnt == sdrd_sector_cnt - 1){ // 最后一扇区不一定满
                current_sector_len = BiasLen[cnt1] - sector_cnt * 512;
            }
            else {
                current_sector_len = 512;
            }

            printf("current_sector_len = %u \n", current_sector_len);

            for (int cnt = 0; cnt < current_sector_len / 4; ++cnt) {
                uint32_t REG8 = 0;
                uint32_t REG32 = 0;
                for (int byte_cnt = 0; byte_cnt < 4; ++byte_cnt) {
                    REG8 = sdrd_buffer_read(cnt * 4 + byte_cnt);
                    REG32 += (REG8 << (byte_cnt * 8));
                }
                printf("REG32 = 0x%08X \n", REG32);
                SYS_RWMEM_W(S7_BASEADDR) = REG32;
            }
        }

        SYS_RWMEM_W(Lite_Reg5) = 0x00000000;
    }

    printf("%s", "WR_Act_DDR \n");

    for (int cnt1 = 0; cnt1 < 1; ++cnt1) {

        SYS_RWMEM_W(Lite_Reg6) = ActAddr[cnt1]; // 偏置ddr地址
        SYS_RWMEM_W(Lite_Reg4) = ActLen; // 发送Act数据长度
        SYS_RWMEM_W(Lite_Reg5) = 0x00000010; // 表示是偏置/权重/激活

        printf("ActAddr = 0x%08X \n", ActAddr[cnt1]);
        printf("ActLen = 0x%08X \n", ActLen);

        //Act扇区
        uint32_t sdrd_sector_cnt;   // 当前文件占用扇区个数
        if (ActLen % 512 == 0) {
            sdrd_sector_cnt = ActLen / 512;
        }
        else {
            sdrd_sector_cnt = ActLen / 512 + 1;
        }

        for (int sector_cnt = 0; sector_cnt < sdrd_sector_cnt; ++sector_cnt) {
            sdrd_sector_set(ActSDAddr[cnt1]+sector_cnt+2048);
            delay_mtime_us(1);
            while(sdrd_busy_chk());

            uint32_t current_sector_len;
            if (sector_cnt == sdrd_sector_cnt - 1){ // 最后一扇区不一定满
                current_sector_len = ActLen - sector_cnt * 512;
            }
            else {
                current_sector_len = 512;
            }

            printf("current_sector_len = %u \n", current_sector_len);

            for (int cnt = 0; cnt < current_sector_len / 4; ++cnt) {
                uint32_t REG8 = 0;
                uint32_t REG32 = 0;
                for (int byte_cnt = 0; byte_cnt < 4; ++byte_cnt) {
                    REG8 = sdrd_buffer_read(cnt * 4 + byte_cnt);
                    REG32 += (REG8 << (byte_cnt * 8));
                }
                printf("REG32 = 0x%08X \n", REG32);
                SYS_RWMEM_W(S7_BASEADDR) = REG32;
            }
        }

        SYS_RWMEM_W(Lite_Reg5) = 0x00000000;
    }

    printf("%s", "WR_Weight_DDR \n");

    for (int cnt1 = 0; cnt1 < 1; ++cnt1) {

        SYS_RWMEM_W(Lite_Reg6) = WeightAddr[cnt1]; // 偏置ddr地址
        SYS_RWMEM_W(Lite_Reg4) = WeightLen[cnt1]; // 发送Weight数据长度
        SYS_RWMEM_W(Lite_Reg5) = 0x00000010; // 表示是偏置/权重/激活

        printf("WeightAddr = 0x%08X \n", WeightAddr[cnt1]);
        printf("WeightLen = 0x%08X \n", WeightLen[cnt1]);

        //Weight扇区
        uint32_t sdrd_sector_cnt;   // 当前文件占用扇区个数
        if (WeightLen[cnt1] % 512 == 0) {
            sdrd_sector_cnt = WeightLen[cnt1] / 512;
        }
        else {
            sdrd_sector_cnt = WeightLen[cnt1] / 512 + 1;
        }

        for (int sector_cnt = 0; sector_cnt < sdrd_sector_cnt; ++sector_cnt) {
            sdrd_sector_set(WeightSDAddr[cnt1]+sector_cnt+2048);
            delay_mtime_us(1);
            while(sdrd_busy_chk());

            uint32_t current_sector_len;
            if (sector_cnt == sdrd_sector_cnt - 1){ // 最后一扇区不一定满
                current_sector_len = WeightLen[cnt1] - sector_cnt * 512;
            }
            else {
                current_sector_len = 512;
            }

            printf("current_sector_len = %u \n", current_sector_len);

            for (int cnt = 0; cnt < current_sector_len / 4; ++cnt) {
                uint32_t REG8 = 0;
                uint32_t REG32 = 0;
                for (int byte_cnt = 0; byte_cnt < 4; ++byte_cnt) {
                    REG8 = sdrd_buffer_read(cnt * 4 + byte_cnt);
                    REG32 += (REG8 << (byte_cnt * 8));
                }
                printf("REG32 = 0x%08X \n", REG32);
                SYS_RWMEM_W(S7_BASEADDR) = REG32;
            }
        }

        SYS_RWMEM_W(Lite_Reg5) = 0x00000000;
    }

    SYS_RWMEM_W(Lite_Reg4) = 0x00000000;

}

void Time_Init()
{
    timer_div_set(0);
    timer_cmpol_ctrl(1);
    timer_cmpval_set(10, 15);
    timer_overflow_set(20);
    delay_mtime_us(100);
}
