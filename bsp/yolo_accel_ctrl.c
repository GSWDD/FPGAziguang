#include "yolo_accel_ctrl.h"
#include "yolo_load_param.h"
#include "icb_ctrl.h"

#define	IMG_W	1248	// 图像宽
#define	IMG_H	624		// 图像长

char InterruptProcessed = DISABLE;

int layers_config[15][9] = {
	// conv_type, ch_in, ch_out, is_pool, feature_size, mult, shift, zp_in, zp_out
	// conv_type: 0->3x3, 1->1x1
       {0,         8,      16,        1,      416,      19290,   9,    0,     65},
	   {0,        16,      32,        1,      208,      30363,   8,   12,     86},
	   {0,        32,      64,        1,      104,      31915,   8,   22,     80},
	   {0,        64,     128,        1,       52,      16639,   7,   19,     57},
	   {0,       128,     256,        0,       26,      20025,   8,    9,     70},	// Layer8->20
	   {0,       128,     256,        1,       26,      20025,   8,    9,     70},	// Layer9
	   {0,       256,     512,        1,       13,      23363,   9,   14,     66},	// Layer10-11
	   {0,       512,    1024,        0,       13,      17866,  11,   12,     48},
	   {1,      1024,     256,        0,       13,      23354,   8,    7,     41},
	   {0,       256,     512,        0,       13,      28523,   8,    6,     63},
	   {1,       512,      24,        0,       13,      27118,   6,   11,     51},
	   {1,       256,     128,        0,       13,      25039,   8,    6,     39},	// Layer18
	   {0,       128,     128,        0,       26,          0,   0,    0,      0},	// Layer19 UpSample
	   {0,       384,     256,        0,       26,      20304,   8,    5,     49},	// Layer21
	   {1,       256,      24,        0,       26,      19131,   6,    8,    117}	// Layer22
};

uint32_t layers_tx_addr[15] = {
	0x1000000, 0x3000000, 0x3100000, 0x3000000, 0x3100000, 0x3100000, 0x3000000, 0x3100000, 0x3000000, L13_ADDR, 0x3000000, L13_ADDR, 0x3000000, 0x3100000, 0x3000000
};

uint32_t layers_rx_addr[15] = {
	0x3000000, 0x3100000, 0x3000000, 0x3100000, 0x3115200, 0x3000000, 0x3100000, 0x3000000, L13_ADDR, 0x3000000, L15_ADDR, 0x3000000, 0x3100000, 0x3000000, L22_ADDR
};

void update_tx_info()
{
	if(feature_size <= 52)
		tx_len = (feature_size*feature_size) << 3;
	else if(tx_cnt == 0) {
		tx_len = (pl_buffer_row_num * feature_size)<<3;
	}
	else if(tx_cnt == tx_cnt_end - 1)
		tx_len = (tx_last_row_num * feature_size)<<3;

	tx_addr = tx_base_addr + ((tx_row_num * feature_size)<<3)*tx_cnt + ((feature_size*feature_size)<<3)*ch_in_batch_cnt;
}

void update_rx_info()
{
	rx_addr = rx_addr + rx_len;

	if(tx_cnt == 0 && ch_out_batch_cnt == 0)
		rx_addr = rx_base_addr;

	if(feature_size <= 52) {
		if(is_pool == 0) {
			rx_len = (feature_size*feature_size) << 3;
			rx_left_row_num = 0;
		}
		else if(pool_stride == 0) {
			rx_len = ((feature_size>>1)* (feature_size>>1)) << 3;
			rx_left_row_num = 0;
		}
		else {
			rx_len = (feature_size*feature_size) << 3;
			rx_left_row_num = 0;
		}

	}
	else if(tx_cnt == 0) {
		rx_len = (((pl_buffer_row_num-1)>>1)<<3)*(feature_size>>1);
		rx_left_row_num = (pl_buffer_row_num-1)%2;
	}
	else if(tx_cnt == tx_cnt_end-1)	{
		rx_len = (((tx_last_row_num-1+rx_left_row_num)>>1)<<3)*(feature_size>>1);
	}
	else {
		rx_len = (((pl_buffer_row_num-2+rx_left_row_num)>>1)<<3)*(feature_size>>1);
		rx_left_row_num = (pl_buffer_row_num-2+rx_left_row_num)%2;
	}

}

void update_cmd()
{
	if(conv_type == 0)
		reg_cmd	= CONV_TYPE3|IS_PADDING;
	else
		reg_cmd = CONV_TYPE1;

	if(is_pool == 1)
		reg_cmd |= IS_POOL;
	if(pool_stride == 1)
		reg_cmd |= POOL_STRIDE1;

	if(feature_size <= 52)
		reg_cmd |= SITE_ALL;
	else if(tx_cnt == 0)
		reg_cmd |= SITE_FIRST;
	else if(tx_cnt == tx_cnt_end - 1)
		reg_cmd |= SITE_LAST;
	else
		reg_cmd |= SITE_MIDDLE;

	if(ch_in_batch_cnt == (ch_in_batch_cnt_end-1))
		reg_cmd |= BATCH_LAST;
	else if(ch_in_batch_cnt == 0)
		reg_cmd |= BATCH_FIRST;
	else
		reg_cmd |= BATCH_MIDDLE;

	reg_cmd = reg_cmd | SET_COL_TYPE(col_type);
	
	if(feature_size <= 52)
		reg_cmd = reg_cmd | SET_ROW_NUM(feature_size);
	else if(tx_cnt == tx_cnt_end-1)
		reg_cmd = reg_cmd | SET_ROW_NUM(tx_last_row_num);
	else
		reg_cmd = reg_cmd | SET_ROW_NUM(pl_buffer_row_num);
}

void update_cnt_info()
{
	if(ch_in_batch_cnt == ch_in_batch_cnt_end-1) {
		ch_in_batch_cnt = 0;
		if(tx_cnt == tx_cnt_end-1) {
			tx_cnt = 0;
			ch_out_batch_cnt++;
		}
		else
			tx_cnt++;
	}
	else
		ch_in_batch_cnt++;
//	if(tx_cnt == tx_cnt_end-1){
//		tx_cnt = 0;
////		if(ch_out_batch_cnt == ch_out_batch_cnt_end-1)
////			ch_out_batch_cnt = 0;
////		else
//		ch_out_batch_cnt++;
//	}
//	else
//		tx_cnt++;

}


void wait_pl_finish()
{
    // fpioa中断
//    uint32_t PL2RISCV_reg = 0;
    uint32_t fpioa_nio_reg = 0;

//    fpioa_nio_reg = fpioa_nio_din_read();
//    PL2RISCV_reg = fpioa_nio_reg & 0x00000020;
//    printf("PL2RISCV_reg = 0x%08x \n", PL2RISCV_reg);

    while((fpioa_nio_din_read() & 0x00000020) == 0)
    {
//        fpioa_nio_reg = fpioa_nio_din_read();
//        PL2RISCV_reg = fpioa_nio_reg & 0x00000020;
//        if (PL2RISCV_reg == 0x20) {
//            fpioa_nio_dout_write(fpioa_nio_reg | 0x00000010);
//            printf("PL2RISCV_reg = 0x%08x \n", PL2RISCV_reg);
//        }
    }

    fpioa_nio_reg = fpioa_nio_din_read();
    fpioa_nio_dout_write(fpioa_nio_reg & 0xFFFFFFEF);

//    printf("PL2RISCV_reg = 0x%08x \n", PL2RISCV_reg);
}

// 初始化yolo模型各参数
void yolo_forward_init(int layer_config[], int i)
{

	conv_type = layer_config[0];	// 卷积类型（0表示3x3卷积，1表示1x1卷积）
	ch_in = layer_config[1];		// 输入通道数
	ch_out = layer_config[2];		// 输出通道数
	is_pool = layer_config[3];		// 是否进行池化操作
	feature_size = layer_config[4];	// 特征图的大小
	mult = layer_config[5];			// 乘法因子
	shift = layer_config[6];		// 移位值
	zp_in = layer_config[7];		// 输入零点
	zp_out = layer_config[8];		// 输出零点

	if(i == 6)		// 判断当前执行的层是否为Layer10,Layer11
		pool_stride	= 1;	// 如果是，设置池化步幅为1
	else
		pool_stride = 0;	// 否则，设置池化步幅为0

	///////////////////////////////////////////////////////////////////
	// 获取计数器相关的值
	tx_cnt = 0;
	ch_out_batch_cnt = 0;
	ch_out_batch_cnt_end = ch_out>>3;

	ch_in_batch_cnt = 0;
	ch_in_batch_cnt_end = ch_in>>3;

	if(feature_size <= 52) {
		pl_buffer_row_num = feature_size;
		tx_row_num = feature_size;
		tx_last_row_num = 0;
	}
	else {
		pl_buffer_row_num = PL_BUFFER_LEN / feature_size; 	// PL端数据BUFFER能存储特征图数据的最大行数量 	// PL_BUFFER_LEN = 4096
		tx_row_num = pl_buffer_row_num - 2;					// PS端每次实际发送特征图数据的行数量
		tx_last_row_num = feature_size % tx_row_num;		// PS端最后一次发送特征图数据的行数量
	}

	// 计算发送计数器结束值,确定向前传播发送多少次数据
	if(tx_last_row_num == 0)
		tx_cnt_end = feature_size / tx_row_num;
	else
		tx_cnt_end = feature_size / tx_row_num + 1;

	///////////////////////////////////////////////////////////////////
	// 初始化权重和偏置地址
	bias_buffer_rd_addr = 0;
	weight_buffer_rd_addr = 0;
	if(i<12) {
		weight_tx_addr = WeightAddr[i];
		bias_tx_addr = BiasAddr[i];
		act_tx_addr = ActAddr[i];
	}
	else {	// 12层后使用前一层的权重、偏置、激活
		weight_tx_addr = WeightAddr[i-1];
		bias_tx_addr = BiasAddr[i-1];
		act_tx_addr = ActAddr[i-1];
	}
	weight_batch_cnt = 0;
	bias_len = ch_out*4;	// 偏置长度,通常为输出通道数乘以4(假设为32位浮点数)

	// 计算权重长度weight_len
	if(conv_type == 0) {	// 3x3卷积
		if(i == 13)
			weight_len = 110592; //384*8*4*9 = 110592;
		else if(ch_in * ch_out <= 16384)
			weight_len = ch_in * ch_out * 9;
		else
			weight_len = 16384 * 9;
	}
	else {	// 1x1卷积
		if(ch_in * ch_out <= 16384)
			weight_len = ch_in * ch_out;
		else
			weight_len = 16384;
	}
	///////////////////////////////////////////////////////////////////
	tx_base_addr = layers_tx_addr[i];	// 设置发送地址
	rx_base_addr = layers_rx_addr[i];	// 设置接收地址
	///////////////////////////////////////////////////////////////////
	if(feature_size == 416)
		col_type = 0;
	else if(feature_size == 208)
		col_type = 1;
	else if(feature_size == 104)
		col_type = 2;
	else if(feature_size == 52)
		col_type = 3;
	else if(feature_size == 26)
		col_type = 4;
	else if(feature_size == 13)
		col_type = 5;
}

void yolo_forward()
{
	for(int i=0; i<15; i++) {	// 15层网络

		yolo_forward_init(layers_config[i], i);		// 初始化yolo每一层的参数

		if(i==12)	// UpSample
			upsample_task();	// 处理上采样层
		else {
			state = S_IDLE;
			while(state != S_FINISH) {
				switch(state) {
					case S_IDLE:	// 准备偏置、激活、权重数据

					    printf("%s", "S_IDLE \n");

					    SYS_RWMEM_W(Lite_Reg2) = SET_SHIFT_ZP(shift,zp_out, zp_in); // 配置量化参数
					    SYS_RWMEM_W(Lite_Reg1) = SET_MULT_BUFFER_RDADDR(mult, bias_buffer_rd_addr, weight_buffer_rd_addr);  // 配置了从哪个地址读取偏置和权重数据
					    printf("Lite_Reg2 = 0x%08X \n", SET_SHIFT_ZP(shift,zp_out, zp_in));
					    printf("Lite_Reg1 = 0x%08X \n", SET_MULT_BUFFER_RDADDR(mult, bias_buffer_rd_addr, weight_buffer_rd_addr));

						// 激活
						printf("%s", "ActAddr \n");

                        SYS_RWMEM_W(Lite_Reg6) = act_tx_addr; // 表示是偏置/权重/激活的ddr地址，ddr_to_yolo模块
                        SYS_RWMEM_W(Lite_Reg0) = WRITE_LEAKY_RELU; // 发送Act数据命令，yolo模块
                        SYS_RWMEM_W(Lite_Reg4) = 256;   // 发送Act数据长度256
                        SYS_RWMEM_W(Lite_Reg5) = 0x00000020; // 表示是读取ddr偏置/权重/激活参数至yolo模块
                        printf("act_tx_addr = 0x%08X \n", act_tx_addr);

						wait_pl_finish();							// 等待PL处理结束中断

						SYS_RWMEM_W(Lite_Reg5) = 0x00000000;

						// 权重
						printf("%s", "WeightAddr \n");

                        SYS_RWMEM_W(Lite_Reg6) = weight_tx_addr; // 表示是偏置/权重/激活的ddr地址，ddr_to_yolo模块

						if(conv_type == 0)
						    SYS_RWMEM_W(Lite_Reg0) = WRITE_WEIGHT;              // 发送卷积3x3的Weight数据命令
						else
						    SYS_RWMEM_W(Lite_Reg0) = WRITE_WEIGHT|CONV_TYPE1;   // 发送卷积1x1Weight数据命令

						SYS_RWMEM_W(Lite_Reg4) = weight_len; // 发送权重数据长度
						SYS_RWMEM_W(Lite_Reg5) = 0x00000020; // 表示是读取ddr偏置/权重/激活参数至yolo模块

						printf("weight_tx_addr = 0x%08X \n", weight_tx_addr);
						printf("weight_len = 0x%08X \n", weight_len);

						wait_pl_finish();							// 等待PL处理结束中断
						SYS_RWMEM_W(Lite_Reg5) = 0x00000000;

                        // 偏置
                        printf("%s", "BiasAddr \n");

                        SYS_RWMEM_W(Lite_Reg6) = bias_tx_addr; // 表示是偏置/权重/激活的ddr地址，ddr_to_yolo模块
                        SYS_RWMEM_W(Lite_Reg0) = WRITE_BIAS; // 发送bias数据命令，yolo模块
                        SYS_RWMEM_W(Lite_Reg4) = bias_len;   // 发送bias数据长度
                        SYS_RWMEM_W(Lite_Reg5) = 0x00000020; // 表示是读取ddr偏置/权重/激活参数至yolo模块

                        printf("bias_tx_addr = 0x%08X \n", bias_tx_addr);
                        printf("bias_len = 0x%08X \n", bias_len);

                        wait_pl_finish();                           // 等待PL处理结束中断
                        SYS_RWMEM_W(Lite_Reg5) = 0x00000000;

						state = S_FEATURE_CONV;
						break;

					case S_FEATURE_CONV:	// 卷积处理

					    printf("%s", "S_FEATURE_CONV \n");
					    SYS_RWMEM_W(Lite_Reg4) = tx_len; // 发送特征数据长度
					    printf("feature_len = %u \n", tx_len);

						update_tx_info();
						update_cmd();

						SYS_RWMEM_W(Lite_Reg0) = reg_cmd|WRITE_FEATURE;     // 发送Feature数据命令

//                        SYS_RWMEM_W(Lite_Reg6) = tx_addr; // 表示特征数据ddr地址，ddr_to_yolo模块
						SYS_RWMEM_W(Lite_Reg6) = 0x00000000; // 表示特征数据ddr地址，ddr_to_yolo模块
                        SYS_RWMEM_W(Lite_Reg4) = tx_len;   // 发送特征数据数据长度
                        SYS_RWMEM_W(Lite_Reg5) = 0x00000020; // 表示是读取ddr偏置/权重/激活参数至yolo模块
                        printf("tx_addr = 0x%08X \n", tx_addr);
                        printf("tx_len = 0x%08X \n", tx_len);

//						ICB_Tx(tx_addr, tx_len);

						wait_pl_finish();

						SYS_RWMEM_W(Lite_Reg5) = 0x00000000;

						SYS_RWMEM_W(Lite_Reg0) = reg_cmd|CONV_START;    // 卷积计算
						wait_pl_finish();

						if(ch_in_batch_cnt == ch_in_batch_cnt_end-1)	// 所有数据已处理完
							state = S_DMA_RX;
						else {
							update_cnt_info();				// 更新状态
							state = S_FEATURE_CONV;

							//根据当前层的索引 i，计算权重缓冲区的读取地址。对于第 13 层，使用 192 作为偏移量；对于其他层，使用 8 的左移（乘以 256）作为偏移量
							if(i==13)
								weight_buffer_rd_addr=ch_in_batch_cnt+ch_out_batch_cnt*ch_in_batch_cnt_end - (weight_batch_cnt*192);
							else
								weight_buffer_rd_addr=ch_in_batch_cnt+ch_out_batch_cnt*ch_in_batch_cnt_end - (weight_batch_cnt<<8);

							SYS_RWMEM_W(Lite_Reg1) = SET_MULT_BUFFER_RDADDR(mult, bias_buffer_rd_addr, weight_buffer_rd_addr);
						}
						break;


					case S_DMA_RX:

					    printf("%s", "S_DMA_RX \n");

						update_rx_info();
						SYS_RWMEM_W(Lite_Reg0) = reg_cmd|READ_START;    // 将PL端的数据传至PS端
						ICB_Rx(rx_addr, rx_len);					// 9行数据+1行填充=10行数据 --->卷积后，8个416的数据量---》经过池化后，变成4*208---> 总共8个通道，即最终数据量4*208*8=6656
						wait_pl_finish();
//						Xil_DCacheFlushRange(rx_addr, rx_len);		// 刷新数据缓存

						if(tx_cnt == tx_cnt_end-1 && ch_out_batch_cnt == ch_out_batch_cnt_end-1 && ch_in_batch_cnt == ch_in_batch_cnt_end-1)
							state = S_FINISH;
						else {
							state = S_FEATURE_CONV;
							update_cnt_info();
							if(i==13) {
								if((ch_out_batch_cnt*ch_in_batch_cnt_end)%192==0 && ch_in_batch_cnt == 0 && tx_cnt == 0) {
									weight_tx_addr = weight_tx_addr + weight_len;
									SYS_RWMEM_W(Lite_Reg0) = WRITE_WEIGHT;
									ICB_Tx(weight_tx_addr, weight_len);
									wait_pl_finish();
									weight_batch_cnt++;
								}
								weight_buffer_rd_addr=ch_in_batch_cnt+ch_out_batch_cnt*ch_in_batch_cnt_end - (weight_batch_cnt*192);
							}
							else {
								if((ch_out_batch_cnt*ch_in_batch_cnt_end)%256==0 && ch_in_batch_cnt == 0 && tx_cnt == 0){
									weight_tx_addr = weight_tx_addr + weight_len;
									if(conv_type == 0)
									    SYS_RWMEM_W(Lite_Reg0) = WRITE_WEIGHT;      // 发送卷积3x3的Weight数据
									else
									    SYS_RWMEM_W(Lite_Reg0) = WRITE_WEIGHT|CONV_TYPE1;   // 发送卷积1x1Weight数据
									ICB_Tx(weight_tx_addr, weight_len);
									wait_pl_finish();
									weight_batch_cnt++;
								}
								weight_buffer_rd_addr=ch_in_batch_cnt+ch_out_batch_cnt*ch_in_batch_cnt_end - (weight_batch_cnt<<8);
							}
							bias_buffer_rd_addr = ch_out_batch_cnt;
							SYS_RWMEM_W(Lite_Reg1) = SET_MULT_BUFFER_RDADDR(mult, bias_buffer_rd_addr, weight_buffer_rd_addr);
						}
						break;


					case S_FINISH: break;

					default: break;
				}
			}
		}
	}
}

// 处理上采样层
void upsample_task()
{
	tx_addr = tx_base_addr;	// 表示上采样操作中要传输的特征数据的起始地址
	tx_len = 0x548;		// 表示要传输的特征数据的长度（1368 字节）

	rx_addr = rx_base_addr;	// 表示上采样操作中接收数据的起始地址
	rx_len = 0x1520;	// 表示要接收的数据长度（5392 字节）

	SYS_RWMEM_W(Lite_Reg2) = SET_SHIFT_ZP(shift,zp_out, zp_in);

	// 16次上采样
	for(int i=0; i<16; i++) {
	    SYS_RWMEM_W(Lite_Reg0) = WRITE_FEATURE;     // 发送Feature数据
		ICB_Tx(tx_addr, tx_len);
		wait_pl_finish();

		SYS_RWMEM_W(Lite_Reg0) = UPSAMPLE_START;    // UpSample
		ICB_Rx(rx_addr, rx_len);
		wait_pl_finish();

		tx_addr = tx_addr + tx_len;
		rx_addr = rx_addr + rx_len;
	}
}

void yolo_detect()
{
	////////////////////////////////////////
	// Layer15 解析
	uint8_t *layer15_addr;
	uint8_t p_obj, p_class, p_obj_max, p_class_max;
	uint16_t layer15_score=0, layer15_score_max=0;
	uint8_t layer15_kmax, layer15_rmax, layer15_cmax;
	float layer15_score_f;

	// 通过三重循环遍历第 15 层的每个位置（3 个特征图，每个特征图 13x13），计算每个位置的目标概率 p_obj 和类别概率 p_class
	for(uint8_t k=0;k<3;k++) {
		for(uint8_t r=0;r<13;r++) {
			for(uint8_t c=0;c<13;c++) {
				if(k==0) {
					layer15_addr = L15_ADDR+4+(c<<3)+r*104;
					p_obj = *layer15_addr;
					p_class = *(layer15_addr+1);
				}
				else if(k==1) {
					layer15_addr = L15_ADDR+2+(c<<3)+r*104+1352;
					p_obj = *layer15_addr;
					p_class = *(layer15_addr+1);
				}
				else {
					layer15_addr = L15_ADDR+(c<<3)+r*104+2704;
					p_obj = *layer15_addr;
					p_class = *(layer15_addr+1);
				}
				layer15_score = p_obj*p_class;
				if(layer15_score >= layer15_score_max) {
					layer15_score_max = layer15_score;
					layer15_kmax = k;
					layer15_rmax = r;
					layer15_cmax = c;
					p_obj_max = p_obj;
					p_class_max = p_class;
				}
			}
		}
	}

	layer15_score_f = l15_sigmoid_table[p_obj_max]*l15_sigmoid_table[p_class_max];
	////////////////////////////////////////
	// Layer22 解析
	uint8_t *layer22_addr;
	uint16_t layer22_score=0, layer22_score_max=0;
	uint8_t layer22_kmax, layer22_rmax, layer22_cmax;
	float layer22_score_f;

	// 使用三重循环遍历第 22 层的每个位置（3 个特征图，每个特征图 26x26），计算每个位置的目标概率和类别概率，并更新最大得分及其对应的索引
	for(uint8_t k=0;k<3;k++) {
		for(uint8_t r=0;r<26;r++) {
			for(uint8_t c=0;c<26;c++) {
				if(k==0) {
					layer22_addr = L22_ADDR+4+(c<<3)+r*208;
					p_obj = *layer22_addr;
					p_class = *(layer22_addr+1);
				}
				else if(k==1) {
					layer22_addr = L22_ADDR+2+(c<<3)+r*208+5408;
					p_obj = *layer22_addr;
					p_class = *(layer22_addr+1);
				}
				else {
					layer22_addr = L22_ADDR+(c<<3)+r*104+10816;
					p_obj = *layer22_addr;
					p_class = *(layer22_addr+1);
				}
				layer22_score = p_obj*p_class;
				if(layer22_score >= layer22_score_max) {
					layer22_score_max = layer22_score;
					layer22_kmax = k;
					layer22_rmax = r;
					layer22_cmax = c;
					p_obj_max = p_obj;
					p_class_max = p_class;
				}
			}
		}
	}
	layer22_score_f = l22_sigmoid_table[p_obj_max]*l22_sigmoid_table[p_class_max];

	float box_x, box_y, box_w, box_h;
	uint8_t data_temp;

	// 根据第 22 层和第 15 层的最终得分，选择得分更高的层进行边界框的计算
	if(layer22_score_f > layer15_score_f) {
		if(layer22_kmax==0) {
			layer22_addr = (layer22_cmax<<3)+layer22_rmax*208+L22_ADDR;
			data_temp = *layer22_addr;
			box_x = (l22_sigmoid_table[data_temp] + layer22_cmax)/26;
			data_temp = *(layer22_addr+1);
			box_y = (l22_sigmoid_table[data_temp] + layer22_rmax)/26;
			data_temp = *(layer22_addr+2);
			box_w = l22_w0_table[data_temp];
			data_temp = *(layer22_addr+3);
			box_h = l22_h0_table[data_temp];
		}
		else if(layer22_kmax==1) {
			layer22_addr = 6+(layer22_cmax<<3)+layer22_rmax*208+L22_ADDR;
			data_temp = *layer22_addr;
			box_x = (l22_sigmoid_table[data_temp] + layer22_cmax)/26;
			data_temp = *(layer22_addr+1);
			box_y = (l22_sigmoid_table[data_temp] + layer22_rmax)/26;
			data_temp = *(layer22_addr+5402);
			box_w = l22_w1_table[data_temp];
			data_temp = *(layer22_addr+5403);
			box_h = l22_h1_table[data_temp];
		}
		else {
			layer22_addr = 4+(layer22_cmax<<3)+layer22_rmax*208+5408+L22_ADDR;
			data_temp = *layer22_addr;
			box_x = (l22_sigmoid_table[data_temp] + layer22_cmax)/26;
			data_temp = *(layer22_addr+1);
			box_y = (l22_sigmoid_table[data_temp] + layer22_rmax)/26;
			data_temp = *(layer22_addr+2);
			box_w = l22_w2_table[data_temp];
			data_temp = *(layer22_addr+3);
			box_h = l22_h2_table[data_temp];
		}
	}
	else {
		if(layer15_kmax==0) {
			layer15_addr = (layer15_cmax<<3)+layer15_rmax*104+L15_ADDR;
			data_temp = *layer15_addr;
			box_x = (l15_sigmoid_table[data_temp] + layer15_cmax)/13;
			data_temp = *(layer15_addr+1);
			box_y = (l15_sigmoid_table[data_temp] + layer15_rmax)/13;
			data_temp = *(layer15_addr+2);
			box_w = l15_w0_table[data_temp];
			data_temp = *(layer15_addr+3);
			box_h = l15_h0_table[data_temp];
		}
		else if(layer15_kmax==1) {
			layer15_addr = 6+(layer15_cmax<<3)+layer15_rmax*104+L15_ADDR;
			data_temp = *layer15_addr;
			box_x = (l15_sigmoid_table[data_temp] + layer15_cmax)/13;
			data_temp = *(layer15_addr+1);
			box_y = (l15_sigmoid_table[data_temp] + layer15_rmax)/13;
			data_temp = *(layer15_addr+1346);
			box_w = l15_w1_table[data_temp];
			data_temp = *(layer15_addr+1347);
			box_h = l15_h1_table[data_temp];
		}
		else {
			layer15_addr = 4+(layer15_cmax<<3)+layer15_rmax*104+1352+L15_ADDR;
			data_temp = *layer15_addr;
			box_x = (l15_sigmoid_table[data_temp] + layer15_cmax)/13;
			data_temp = *(layer15_addr+1);
			box_y = (l15_sigmoid_table[data_temp] + layer15_rmax)/13;
			data_temp = *(layer15_addr+2);
			box_w = l15_w2_table[data_temp];
			data_temp = *(layer15_addr+3);
			box_h = l15_h2_table[data_temp];
		}
	}

	// 计算边界框的中心坐标 (box_x, box_y) 和宽高 (box_w, box_h)，并根据不同的 k 值从相应的地址读取数据
	// 根据计算得到的中心坐标和宽高，计算边界框的左上角 (box_x1, box_y1) 和右下角 (box_x2, box_y2) 的坐标
	int box_x1, box_y1, box_x2, box_y2;
	box_x1 = (box_x-box_w/2) * IMG_W;
	box_x2 = (box_x+box_w/2) * IMG_W;
	box_y1 = (box_y-box_h/2) * IMG_H;
	box_y2 = (box_y+box_h/2) * IMG_H;

	SYS_RWMEM_W(BOX_X1_ADDR) = box_x1;
	SYS_RWMEM_W(BOX_Y1_ADDR) = box_y1;
	SYS_RWMEM_W(BOX_X2_ADDR) = box_x2;
	SYS_RWMEM_W(BOX_Y2_ADDR) = box_y2;
}
