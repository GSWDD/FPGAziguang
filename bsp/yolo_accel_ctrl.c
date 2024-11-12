#include "yolo_accel_ctrl.h"
#include "yolo_load_param.h"
#include "icb_ctrl.h"

#define	IMG_W	1248	// ͼ���
#define	IMG_H	624		// ͼ��

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
    // fpioa�ж�
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

// ��ʼ��yoloģ�͸�����
void yolo_forward_init(int layer_config[], int i)
{

	conv_type = layer_config[0];	// ������ͣ�0��ʾ3x3�����1��ʾ1x1�����
	ch_in = layer_config[1];		// ����ͨ����
	ch_out = layer_config[2];		// ���ͨ����
	is_pool = layer_config[3];		// �Ƿ���гػ�����
	feature_size = layer_config[4];	// ����ͼ�Ĵ�С
	mult = layer_config[5];			// �˷�����
	shift = layer_config[6];		// ��λֵ
	zp_in = layer_config[7];		// �������
	zp_out = layer_config[8];		// ������

	if(i == 6)		// �жϵ�ǰִ�еĲ��Ƿ�ΪLayer10,Layer11
		pool_stride	= 1;	// ����ǣ����óػ�����Ϊ1
	else
		pool_stride = 0;	// �������óػ�����Ϊ0

	///////////////////////////////////////////////////////////////////
	// ��ȡ��������ص�ֵ
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
		pl_buffer_row_num = PL_BUFFER_LEN / feature_size; 	// PL������BUFFER�ܴ洢����ͼ���ݵ���������� 	// PL_BUFFER_LEN = 4096
		tx_row_num = pl_buffer_row_num - 2;					// PS��ÿ��ʵ�ʷ�������ͼ���ݵ�������
		tx_last_row_num = feature_size % tx_row_num;		// PS�����һ�η�������ͼ���ݵ�������
	}

	// ���㷢�ͼ���������ֵ,ȷ����ǰ�������Ͷ��ٴ�����
	if(tx_last_row_num == 0)
		tx_cnt_end = feature_size / tx_row_num;
	else
		tx_cnt_end = feature_size / tx_row_num + 1;

	///////////////////////////////////////////////////////////////////
	// ��ʼ��Ȩ�غ�ƫ�õ�ַ
	bias_buffer_rd_addr = 0;
	weight_buffer_rd_addr = 0;
	if(i<12) {
		weight_tx_addr = WeightAddr[i];
		bias_tx_addr = BiasAddr[i];
		act_tx_addr = ActAddr[i];
	}
	else {	// 12���ʹ��ǰһ���Ȩ�ء�ƫ�á�����
		weight_tx_addr = WeightAddr[i-1];
		bias_tx_addr = BiasAddr[i-1];
		act_tx_addr = ActAddr[i-1];
	}
	weight_batch_cnt = 0;
	bias_len = ch_out*4;	// ƫ�ó���,ͨ��Ϊ���ͨ��������4(����Ϊ32λ������)

	// ����Ȩ�س���weight_len
	if(conv_type == 0) {	// 3x3���
		if(i == 13)
			weight_len = 110592; //384*8*4*9 = 110592;
		else if(ch_in * ch_out <= 16384)
			weight_len = ch_in * ch_out * 9;
		else
			weight_len = 16384 * 9;
	}
	else {	// 1x1���
		if(ch_in * ch_out <= 16384)
			weight_len = ch_in * ch_out;
		else
			weight_len = 16384;
	}
	///////////////////////////////////////////////////////////////////
	tx_base_addr = layers_tx_addr[i];	// ���÷��͵�ַ
	rx_base_addr = layers_rx_addr[i];	// ���ý��յ�ַ
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
	for(int i=0; i<15; i++) {	// 15������

		yolo_forward_init(layers_config[i], i);		// ��ʼ��yoloÿһ��Ĳ���

		if(i==12)	// UpSample
			upsample_task();	// �����ϲ�����
		else {
			state = S_IDLE;
			while(state != S_FINISH) {
				switch(state) {
					case S_IDLE:	// ׼��ƫ�á����Ȩ������

					    printf("%s", "S_IDLE \n");

					    SYS_RWMEM_W(Lite_Reg2) = SET_SHIFT_ZP(shift,zp_out, zp_in); // ������������
					    SYS_RWMEM_W(Lite_Reg1) = SET_MULT_BUFFER_RDADDR(mult, bias_buffer_rd_addr, weight_buffer_rd_addr);  // �����˴��ĸ���ַ��ȡƫ�ú�Ȩ������
					    printf("Lite_Reg2 = 0x%08X \n", SET_SHIFT_ZP(shift,zp_out, zp_in));
					    printf("Lite_Reg1 = 0x%08X \n", SET_MULT_BUFFER_RDADDR(mult, bias_buffer_rd_addr, weight_buffer_rd_addr));

						// ����
						printf("%s", "ActAddr \n");

                        SYS_RWMEM_W(Lite_Reg6) = act_tx_addr; // ��ʾ��ƫ��/Ȩ��/�����ddr��ַ��ddr_to_yoloģ��
                        SYS_RWMEM_W(Lite_Reg0) = WRITE_LEAKY_RELU; // ����Act�������yoloģ��
                        SYS_RWMEM_W(Lite_Reg4) = 256;   // ����Act���ݳ���256
                        SYS_RWMEM_W(Lite_Reg5) = 0x00000020; // ��ʾ�Ƕ�ȡddrƫ��/Ȩ��/���������yoloģ��
                        printf("act_tx_addr = 0x%08X \n", act_tx_addr);

						wait_pl_finish();							// �ȴ�PL��������ж�

						SYS_RWMEM_W(Lite_Reg5) = 0x00000000;

						// Ȩ��
						printf("%s", "WeightAddr \n");

                        SYS_RWMEM_W(Lite_Reg6) = weight_tx_addr; // ��ʾ��ƫ��/Ȩ��/�����ddr��ַ��ddr_to_yoloģ��

						if(conv_type == 0)
						    SYS_RWMEM_W(Lite_Reg0) = WRITE_WEIGHT;              // ���;��3x3��Weight��������
						else
						    SYS_RWMEM_W(Lite_Reg0) = WRITE_WEIGHT|CONV_TYPE1;   // ���;��1x1Weight��������

						SYS_RWMEM_W(Lite_Reg4) = weight_len; // ����Ȩ�����ݳ���
						SYS_RWMEM_W(Lite_Reg5) = 0x00000020; // ��ʾ�Ƕ�ȡddrƫ��/Ȩ��/���������yoloģ��

						printf("weight_tx_addr = 0x%08X \n", weight_tx_addr);
						printf("weight_len = 0x%08X \n", weight_len);

						wait_pl_finish();							// �ȴ�PL��������ж�
						SYS_RWMEM_W(Lite_Reg5) = 0x00000000;

                        // ƫ��
                        printf("%s", "BiasAddr \n");

                        SYS_RWMEM_W(Lite_Reg6) = bias_tx_addr; // ��ʾ��ƫ��/Ȩ��/�����ddr��ַ��ddr_to_yoloģ��
                        SYS_RWMEM_W(Lite_Reg0) = WRITE_BIAS; // ����bias�������yoloģ��
                        SYS_RWMEM_W(Lite_Reg4) = bias_len;   // ����bias���ݳ���
                        SYS_RWMEM_W(Lite_Reg5) = 0x00000020; // ��ʾ�Ƕ�ȡddrƫ��/Ȩ��/���������yoloģ��

                        printf("bias_tx_addr = 0x%08X \n", bias_tx_addr);
                        printf("bias_len = 0x%08X \n", bias_len);

                        wait_pl_finish();                           // �ȴ�PL��������ж�
                        SYS_RWMEM_W(Lite_Reg5) = 0x00000000;

						state = S_FEATURE_CONV;
						break;

					case S_FEATURE_CONV:	// �������

					    printf("%s", "S_FEATURE_CONV \n");
					    SYS_RWMEM_W(Lite_Reg4) = tx_len; // �����������ݳ���
					    printf("feature_len = %u \n", tx_len);

						update_tx_info();
						update_cmd();

						SYS_RWMEM_W(Lite_Reg0) = reg_cmd|WRITE_FEATURE;     // ����Feature��������

//                        SYS_RWMEM_W(Lite_Reg6) = tx_addr; // ��ʾ��������ddr��ַ��ddr_to_yoloģ��
						SYS_RWMEM_W(Lite_Reg6) = 0x00000000; // ��ʾ��������ddr��ַ��ddr_to_yoloģ��
                        SYS_RWMEM_W(Lite_Reg4) = tx_len;   // ���������������ݳ���
                        SYS_RWMEM_W(Lite_Reg5) = 0x00000020; // ��ʾ�Ƕ�ȡddrƫ��/Ȩ��/���������yoloģ��
                        printf("tx_addr = 0x%08X \n", tx_addr);
                        printf("tx_len = 0x%08X \n", tx_len);

//						ICB_Tx(tx_addr, tx_len);

						wait_pl_finish();

						SYS_RWMEM_W(Lite_Reg5) = 0x00000000;

						SYS_RWMEM_W(Lite_Reg0) = reg_cmd|CONV_START;    // �������
						wait_pl_finish();

						if(ch_in_batch_cnt == ch_in_batch_cnt_end-1)	// ���������Ѵ�����
							state = S_DMA_RX;
						else {
							update_cnt_info();				// ����״̬
							state = S_FEATURE_CONV;

							//���ݵ�ǰ������� i������Ȩ�ػ������Ķ�ȡ��ַ�����ڵ� 13 �㣬ʹ�� 192 ��Ϊƫ���������������㣬ʹ�� 8 �����ƣ����� 256����Ϊƫ����
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
						SYS_RWMEM_W(Lite_Reg0) = reg_cmd|READ_START;    // ��PL�˵����ݴ���PS��
						ICB_Rx(rx_addr, rx_len);					// 9������+1�����=10������ --->�����8��416��������---�������ػ��󣬱��4*208---> �ܹ�8��ͨ����������������4*208*8=6656
						wait_pl_finish();
//						Xil_DCacheFlushRange(rx_addr, rx_len);		// ˢ�����ݻ���

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
									    SYS_RWMEM_W(Lite_Reg0) = WRITE_WEIGHT;      // ���;��3x3��Weight����
									else
									    SYS_RWMEM_W(Lite_Reg0) = WRITE_WEIGHT|CONV_TYPE1;   // ���;��1x1Weight����
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

// �����ϲ�����
void upsample_task()
{
	tx_addr = tx_base_addr;	// ��ʾ�ϲ���������Ҫ������������ݵ���ʼ��ַ
	tx_len = 0x548;		// ��ʾҪ������������ݵĳ��ȣ�1368 �ֽڣ�

	rx_addr = rx_base_addr;	// ��ʾ�ϲ��������н������ݵ���ʼ��ַ
	rx_len = 0x1520;	// ��ʾҪ���յ����ݳ��ȣ�5392 �ֽڣ�

	SYS_RWMEM_W(Lite_Reg2) = SET_SHIFT_ZP(shift,zp_out, zp_in);

	// 16���ϲ���
	for(int i=0; i<16; i++) {
	    SYS_RWMEM_W(Lite_Reg0) = WRITE_FEATURE;     // ����Feature����
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
	// Layer15 ����
	uint8_t *layer15_addr;
	uint8_t p_obj, p_class, p_obj_max, p_class_max;
	uint16_t layer15_score=0, layer15_score_max=0;
	uint8_t layer15_kmax, layer15_rmax, layer15_cmax;
	float layer15_score_f;

	// ͨ������ѭ�������� 15 ���ÿ��λ�ã�3 ������ͼ��ÿ������ͼ 13x13��������ÿ��λ�õ�Ŀ����� p_obj �������� p_class
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
	// Layer22 ����
	uint8_t *layer22_addr;
	uint16_t layer22_score=0, layer22_score_max=0;
	uint8_t layer22_kmax, layer22_rmax, layer22_cmax;
	float layer22_score_f;

	// ʹ������ѭ�������� 22 ���ÿ��λ�ã�3 ������ͼ��ÿ������ͼ 26x26��������ÿ��λ�õ�Ŀ����ʺ������ʣ����������÷ּ����Ӧ������
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

	// ���ݵ� 22 ��͵� 15 ������յ÷֣�ѡ��÷ָ��ߵĲ���б߽��ļ���
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

	// ����߽����������� (box_x, box_y) �Ϳ�� (box_w, box_h)�������ݲ�ͬ�� k ֵ����Ӧ�ĵ�ַ��ȡ����
	// ���ݼ���õ�����������Ϳ�ߣ�����߽������Ͻ� (box_x1, box_y1) �����½� (box_x2, box_y2) ������
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
