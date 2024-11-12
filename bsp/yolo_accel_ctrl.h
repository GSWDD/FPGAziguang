#ifndef YOLO_ACCEL_CTRL_H
#define YOLO_ACCEL_CTRL_H

//#include "dma_ctrl.h"
#include "yolo_load_param.h"
#include "sigmoid_exp_table.h"

#define	S_IDLE			0
#define S_FEATURE_CONV	1
#define	S_DMA_RX		2
#define	S_FINISH		3

// s6基地址:0xC0000000u
#define XPAR_AXI4_LITE_V1_0_0_BASEADDR           0xC0000000u

#define	Lite_Reg0			XPAR_AXI4_LITE_V1_0_0_BASEADDR
#define	Lite_Reg1			XPAR_AXI4_LITE_V1_0_0_BASEADDR+0x4
#define	Lite_Reg2			XPAR_AXI4_LITE_V1_0_0_BASEADDR+0x8

#define Lite_Reg4           XPAR_AXI4_LITE_V1_0_0_BASEADDR+0x10
#define Lite_Reg5           XPAR_AXI4_LITE_V1_0_0_BASEADDR+0x14
#define Lite_Reg6           XPAR_AXI4_LITE_V1_0_0_BASEADDR+0x18

#define XPAR_AXI4_LITE_BOX_V1_0_0_BASEADDR       0xC0001000u
#define	BOX_X1_ADDR			XPAR_AXI4_LITE_BOX_V1_0_0_BASEADDR
#define	BOX_Y1_ADDR			XPAR_AXI4_LITE_BOX_V1_0_0_BASEADDR+0x4
#define	BOX_X2_ADDR			XPAR_AXI4_LITE_BOX_V1_0_0_BASEADDR+0x8
#define	BOX_Y2_ADDR			XPAR_AXI4_LITE_BOX_V1_0_0_BASEADDR+0xC

// s7基地址:0xE0000000u
#define S7_BASEADDR           0xE0000000u

//////////////////////////////////////////////////////////////////
// Define Command
#define	WRITE_BIAS			0x21
#define	WRITE_LEAKY_RELU	0x31
#define	WRITE_WEIGHT		0x11
#define	WRITE_FEATURE		0x01

#define	READ_START			0x02
#define	CONV_START			0x04
#define	UPSAMPLE_START		0x08

#define	CONV_TYPE1			0x40
#define	CONV_TYPE3			0x00
#define	IS_PADDING			0x80
#define	IS_POOL				0x100
#define	POOL_STRIDE1		0x2000

#define	SITE_FIRST			0x000
#define	SITE_MIDDLE			0x200
#define	SITE_LAST			0x400
#define SITE_ALL			0x600

#define	BATCH_FIRST			0x0000
#define	BATCH_MIDDLE		0x0800
#define	BATCH_LAST			0x1000

#define SET_COL_TYPE(x)     (x<<14)

#define	SET_ROW_NUM(x)		((x-1)<<17)

//////////////////////////////////////////////////////////////////

#define	PL_BUFFER_LEN		4096

#define SET_MULT_BUFFER_RDADDR(x,y,z)		((x<<16)|(y<<8)|(z))
#define SET_SHIFT_ZP(x,y,z)					((x<<24)|(y<<8)|(z))

#define	L13_ADDR			0x3200000
#define	L15_ADDR			0x3300000
#define	L22_ADDR			0x3400000

//////////////////////////////////////////////////////////////////
int layers_config[15][9];

uint8_t     conv_type ;
uint16_t    ch_in;
uint16_t	ch_out;
uint8_t	    is_pool;
uint16_t	feature_size;
uint16_t	mult;
uint8_t	    shift;
uint8_t	    zp_in;
uint8_t	    zp_out;


uint8_t     pool_stride;
uint8_t     col_type;
uint8_t     ch_in_batch_cnt;
uint8_t     ch_in_batch_cnt_end;
uint8_t     ch_out_batch_cnt;
uint8_t     ch_out_batch_cnt_end;
uint8_t     tx_cnt;
uint8_t     tx_cnt_end;
uint8_t	    batch_cnt;
uint8_t	    batch_cnt_end;
uint8_t     pl_buffer_row_num;
uint8_t     tx_row_num;
uint8_t     tx_last_row_num;
uint8_t     rx_left_row_num;


uint8_t     bias_buffer_rd_addr;
uint8_t     weight_buffer_rd_addr;
uint32_t    weight_tx_addr;
uint32_t    bias_tx_addr;
uint32_t    act_tx_addr;
uint32_t    weight_batch_cnt;
uint32_t    layers_tx_addr[15];
uint32_t    layers_rx_addr[15];
uint32_t    tx_base_addr;
uint32_t    rx_base_addr;

uint32_t    reg_cmd;

uint32_t    tx_addr;
uint32_t    tx_len;

uint32_t    rx_addr;
uint32_t    rx_len;

uint32_t    bias_len;
uint32_t    weight_len;


char state;
char InterruptProcessed;

void update_tx_info();
void update_rx_info();
void update_cmd();
void update_cnt_info();
void yolo_forward();
void yolo_forward_init(int layer_config[], int i);
void wait_pl_finish();
void upsample_task();
void yolo_detect();

#endif
