`include "defines.v"
`include "config.v"

module top#(
	parameter MEM_ROW_ADDR_WIDTH   = 15,
	parameter MEM_COL_ADDR_WIDTH   = 10,
	parameter MEM_BADDR_WIDTH      = 3,
	parameter MEM_DQ_WIDTH         = 32,
	parameter MEM_DQS_WIDTH        = 32/8
)(

    //公共接口
    input  wire                    sys_clk_50m,    //时钟输入 50M
    input  wire                    hard_rst_n,     //外接KEY1，来自外部的复位信号，低电平有效
    output wire                    core_active,    //外接LED1，处理器活动指示，以肉眼可见速度翻转
//    output wire                    core_rst_n,

    //JTAG接口
    input  wire                    JTAG_TMS,
    input  wire                    JTAG_TDI,
    output wire                    JTAG_TDO,
    input  wire                    JTAG_TCK,

    //OV5647 coms1
    output wire                    cmos_init_done,//外接LED4，OV5640寄存器初始化完成
    inout  wire                    cmos1_scl     /*synthesis PAP_MARK_DEBUG="1"*/,//cmos1 i2c 
    inout  wire                    cmos1_sda     /*synthesis PAP_MARK_DEBUG="1"*/,//cmos1 i2c 
    input  wire                    cmos1_vsync   /*synthesis PAP_MARK_DEBUG="1"*/,//cmos1 vsync
    input  wire                    cmos1_href    /*synthesis PAP_MARK_DEBUG="1"*/,//cmos1 hsync refrence,data valid
    input  wire                    cmos1_pclk    /*synthesis PAP_MARK_DEBUG="1"*/,//cmos1 pxiel clock
    input  wire    [7:0]           cmos1_data    /*synthesis PAP_MARK_DEBUG="1"*/,//cmos1 data
    output wire                    cmos1_reset   /*synthesis PAP_MARK_DEBUG="1"*/,//cmos1 reset

    //DDR
    output wire                    mem_rst_n     ,
    output wire                    mem_ck        ,
    output wire                    mem_ck_n      ,
    output wire                    mem_cke       ,
    output wire                    mem_cs_n      ,
    output wire                    mem_ras_n     ,
    output wire                    mem_cas_n     ,
    output wire                    mem_we_n      ,
    output wire                    mem_odt       ,
    output wire     [14:0]         mem_a         ,
    output wire     [2:0]          mem_ba        ,
    inout  wire     [3:0]          mem_dqs       ,
    inout  wire     [3:0]          mem_dqs_n     ,
    inout  wire     [31:0]         mem_dq        ,
    output wire     [3:0]          mem_dm        ,
    output reg                     heart_beat_led,
    output wire                    ddr_init_done ,//外接LED3，DDR初始化完成

    //SD、TF卡接口
    output wire                    sd_clk        ,
    inout                          sd_cmd        ,
    input  wire [3:0]              sd_dat        ,//需要上拉

    //FPIOA
    inout  wire [`FPIOA_PORT_NUM-1:0]    fpioa    //处理器IO接口

   );

    //s6
    wire                 s6_icb_cmd_valid;
    wire                 s6_icb_cmd_ready;
    wire [`MemAddrBus]   s6_icb_cmd_addr ;
    wire                 s6_icb_cmd_read ;
    wire [`MemBus]       s6_icb_cmd_wdata;
    wire [3:0]           s6_icb_cmd_wmask;
    wire                 s7_icb_cmd_valid;
    wire                 s7_icb_cmd_ready;
    wire [`MemAddrBus]   s7_icb_cmd_addr ;
    wire                 s7_icb_cmd_read ;
    wire [`MemBus]       s7_icb_cmd_wdata;
    wire [3:0]           s7_icb_cmd_wmask;

    wire                sys_clk_25m    ;

    wire                cmos1_pclk_24bit /*synthesis PAP_MARK_DEBUG="1"*/;    // 25M
    wire [23:0]         cmos1_data_24bit /*synthesis PAP_MARK_DEBUG="1"*/;
    wire                cmos1_href_24bit /*synthesis PAP_MARK_DEBUG="1"*/;

    // 原始图像1280*720*3
    wire            vid_ce                ;
    wire            m_vsync               ;
    wire            m_hsync               ;
    wire            m_href                ;
    wire    [23:0]  m_data                ;

    wire            task_finish/*synthesis PAP_MARK_DEBUG="1"*/;    // 中断

    wire    [31:0]  slave_lite_reg0         ;
    wire    [31:0]  slave_lite_reg1         ;
    wire    [31:0]  slave_lite_reg2         ;
    wire    [31:0]  slave_lite_reg3         ;
    wire    [31:0]  slave_lite_reg4         ;
    wire    [31:0]  slave_lite_reg5         ;    // 32'd0为权重，激活，偏置；32'd1为特征
    wire    [31:0]  slave_lite_reg6         ;    // 存放权重，激活，偏置的DDR地址

    wire    [63:0]  s_axis_mm2s_tdata       ;       
    wire    [ 7:0]  s_axis_mm2s_tkeep       ;       
    wire            s_axis_mm2s_tvalid      ;       
    wire            s_axis_mm2s_tready      ;       
    wire            s_axis_mm2s_tlast       ;       

    wire    [63:0]  s_axis_s2mm_tdata       ;       
    wire    [ 7:0]  s_axis_s2mm_tkeep       ;       
    wire            s_axis_s2mm_tvalid      ;       
    wire            s_axis_s2mm_tready      ;       
    wire            s_axis_s2mm_tlast       ;  
     
//    wire    [63:0]  ddr_mm2s_tdata          ;       
//    wire    [ 7:0]  ddr_mm2s_tkeep          ;       
//    wire            ddr_mm2s_tvalid         ;       
//    wire            ddr_mm2s_tready         ;       
//    wire            ddr_mm2s_tlast          ;

//    assign icb_mm2s_tready = s_axis_mm2s_tready;
//    assign ddr_mm2s_tready = s_axis_mm2s_tready;

//    assign s_axis_mm2s_tdata = (slave_lite_reg5 == 4'd0) ? icb_mm2s_tdata : ddr_mm2s_tdata;
//    assign s_axis_mm2s_tkeep = (slave_lite_reg5 == 4'd0) ? icb_mm2s_tkeep : ddr_mm2s_tkeep;
//    assign s_axis_mm2s_tvalid = (slave_lite_reg5 == 4'd0) ? icb_mm2s_tvalid : ddr_mm2s_tvalid;
//    assign s_axis_mm2s_tlast = (slave_lite_reg5 == 4'd0) ? icb_mm2s_tlast : ddr_mm2s_tlast;
//        
    //PLL
    pll u_pll (
        .clkin1         (sys_clk_50m   ),//50MHz sys_clk
        .clkout0        (       ),//37.125M 720P30 hdmi
        .clkout1        (       ),//10MHz hdmi
        .clkout2        (sys_clk_25m   ),//25M ov5640
        .pll_lock       (              )
    );

    ov5640_top ov5640_top_module(
        .sys_rst_n          (hard_rst_n        ),
        .sys_clk_50m        (sys_clk_50m       ),
        .sys_clk_25m        (sys_clk_25m       ),       
        .cmos_init_done     (cmos_init_done    ),//OV5640寄存器初始化完成
        .cmos1_scl          (cmos1_scl         ),//cmos1 i2c 
        .cmos1_sda          (cmos1_sda         ),//cmos1 i2c 
        .cmos1_vsync        (cmos1_vsync       ),//cmos1 vsync
        .cmos1_href         (cmos1_href        ),//cmos1 hsync refrence,data valid
        .cmos1_pclk         (cmos1_pclk        ),//cmos1 pxiel clock
        .cmos1_data         (cmos1_data        ),//cmos1 data
        .cmos1_reset        (cmos1_reset       ),//cmos1 reset

        .vid_ce             (vid_ce ),
        .m_vsync            (m_vsync),
        .m_hsync            (m_hsync),
        .m_href             (m_href ),
        .m_data             (m_data )
    );

    sparrow_soc sparrow_soc_module(
        //公共接口
        .clk            (sys_clk_50m   ),//时钟输入
        .hard_rst_n     (hard_rst_n    ), //来自外部的复位信号，低电平有效
        .core_active    (core_active   ),//处理器活动指示，以肉眼可见速度翻转
        .core_rst_n     (              ),
        //JTAG接口
        .JTAG_TMS       (JTAG_TMS      ),
        .JTAG_TDI       (JTAG_TDI      ),
        .JTAG_TDO       (JTAG_TDO      ),
        .JTAG_TCK       (JTAG_TCK      ), //即使没有JTAG，也保留这个接口，使得约束可以通用
    
        //SD、TF卡接口
        .sd_clk         (sd_clk        ),
        .sd_cmd         (sd_cmd        ),
        .sd_dat         (sd_dat        ),//需要上拉
    
        //s6
        .s6_icb_cmd_valid    (s6_icb_cmd_valid),
        .s6_icb_cmd_ready    (s6_icb_cmd_ready),
        .s6_icb_cmd_addr     (s6_icb_cmd_addr ),
        .s6_icb_cmd_read     (s6_icb_cmd_read ),
        .s6_icb_cmd_wdata    (s6_icb_cmd_wdata),
        .s6_icb_cmd_wmask    (s6_icb_cmd_wmask),
        .s6_icb_rsp_valid    (1'b0),
        .s6_icb_rsp_ready    (),
        .s6_icb_rsp_err      (1'b0),
        .s6_icb_rsp_rdata    (32'd0),

        //s7
        .s7_icb_cmd_valid    (s7_icb_cmd_valid),
        .s7_icb_cmd_ready    (s7_icb_cmd_ready),
        .s7_icb_cmd_addr     (s7_icb_cmd_addr ),
        .s7_icb_cmd_read     (s7_icb_cmd_read ),
        .s7_icb_cmd_wdata    (s7_icb_cmd_wdata),
        .s7_icb_cmd_wmask    (s7_icb_cmd_wmask),
        .s7_icb_rsp_valid    (1'b0),
        .s7_icb_rsp_ready    (),
        .s7_icb_rsp_err      (1'b0),
        .s7_icb_rsp_rdata    (32'd0),

        .RISCV2PL            (RISCV2PL),
        .PL2RISCV            (PL2RISCV),
        //FPIOA
        .fpioa               (fpioa)
    );

    wire            init_done/*synthesis PAP_MARK_DEBUG="1"*/;

    ddr_ctl ddr_ctl_module (
        .sys_clk_50m               (sys_clk_50m        ),
        .hard_rst_n                (hard_rst_n         ),// input
        .ddr_init_done             (ddr_init_done      ),// output
        .ddrphy_clkin              (core_clk           ),// output
        .pll_lock                  (pll_lock           ),// output
        
        .cmos1_pclk                (cmos1_pclk         ),
        .m_vsync                   (m_vsync            ),
        .m_data                    (m_data             ),
        .m_href                    (m_href             ),

        .init_done                 (init_done          ),

//        .yolo_axi_araddr           (yolo_axi_araddr    ),
//        .yolo_axi_aruser_id        (4'd1               ),
//        .yolo_axi_arlen            (yolo_axi_arlen     ),
//        .yolo_axi_arready          (yolo_axi_arready   ),
//        .yolo_axi_arvalid          (yolo_axi_arvalid   ),
//        .yolo_axi_rready           (yolo_axi_rready    ),
//        .yolo_axi_rdata            (yolo_axi_rdata     ),
//        .yolo_axi_rid              (yolo_axi_rid       ),
//        .yolo_axi_rlast            (yolo_axi_rlast     ),
//        .yolo_axi_rvalid           (yolo_axi_rvalid    ),
    
        .s7_icb_cmd_valid          (s7_icb_cmd_valid   ),
        .s7_icb_cmd_ready          (s7_icb_cmd_ready   ),
        .s7_icb_cmd_addr           (s7_icb_cmd_addr    ),
        .s7_icb_cmd_read           (s7_icb_cmd_read    ),
        .s7_icb_cmd_wdata          (s7_icb_cmd_wdata   ),
        .s7_icb_cmd_wmask          (s7_icb_cmd_wmask   ),
    
        .slave_lite_reg4           (slave_lite_reg4    ),    // 写ddr长度
        .slave_lite_reg5           (slave_lite_reg5    ),    // 数据类型，只有为0x10时才使能本模块功能
        .slave_lite_reg6           (slave_lite_reg6    ),    // 写ddr基地址
    
        .s_axis_mm2s_tdata         (s_axis_mm2s_tdata  ),       
        .s_axis_mm2s_tkeep         (s_axis_mm2s_tkeep  ),       
        .s_axis_mm2s_tvalid        (s_axis_mm2s_tvalid ),       
        .s_axis_mm2s_tready        (s_axis_mm2s_tready ),       
        .s_axis_mm2s_tlast         (s_axis_mm2s_tlast  ),      

        .mem_rst_n                 (mem_rst_n          ),// output
        .mem_ck                    (mem_ck             ),// output
        .mem_ck_n                  (mem_ck_n           ),// output
        .mem_cke                   (mem_cke            ),// output
        .mem_cs_n                  (mem_cs_n           ),// output
        .mem_ras_n                 (mem_ras_n          ),// output
        .mem_cas_n                 (mem_cas_n          ),// output
        .mem_we_n                  (mem_we_n           ),// output
        .mem_odt                   (mem_odt            ),// output
        .mem_a                     (mem_a              ),// output [14:0]
        .mem_ba                    (mem_ba             ),// output [2:0]
        .mem_dqs                   (mem_dqs            ),// inout [3:0]
        .mem_dqs_n                 (mem_dqs_n          ),// inout [3:0]
        .mem_dq                    (mem_dq             ),// inout [31:0]
        .mem_dm                    (mem_dm             ) // output [3:0]
    );

//    ddr_to_yolo_axi_ctl ddr_to_yolo_axi_ctl_module(
//
//        .sysclk              (sys_clk_50m),
//        .sysrst_n            (hard_rst_n),
//        .data_rd_en          (),
//        .data_rd_len         (slave_lite_reg4    ),
//        .data_rd_araddr      (32'h03000000       ),
//        
//        .axi_clk             (core_clk),
//        .axi_araddr          (yolo_axi_araddr    ),
//        .axi_arid            (yolo_axi_aruser_id ),
//        .axi_arlen           (yolo_axi_arlen     ),
//        .axi_arsize          (               ),
//        .axi_arburst         (   ),
//        .axi_arvalid         (yolo_axi_arvalid   ),
//        .axi_arready         (yolo_axi_arready   ),
//        .axi_rready          (yolo_axi_rready    ),
//        .axi_rdata           (yolo_axi_rdata     ),
//        .axi_rvalid          (yolo_axi_rvalid    ),
//        .axi_rlast           (yolo_axi_rlast     ),
//        .axi_rid             (yolo_axi_rid       ),
//        
//        .s_axis_mm2s_tdata   (ddr_mm2s_tdata     ),       
//        .s_axis_mm2s_tkeep   (ddr_mm2s_tkeep     ),       
//        .s_axis_mm2s_tvalid  (ddr_mm2s_tvalid    ),       
//        .s_axis_mm2s_tready  (ddr_mm2s_tready    ),       
//        .s_axis_mm2s_tlast   (ddr_mm2s_tlast     )
//
//   );

    slave_lite_reg_ctrrl slave_lite_reg_ctrrl_module(
        .sys_clk_50m        (sys_clk_50m     ),
        .hard_rst_n         (hard_rst_n      ),
        .s6_icb_cmd_valid   (s6_icb_cmd_valid),
        .s6_icb_cmd_ready   (s6_icb_cmd_ready),
        .s6_icb_cmd_addr    (s6_icb_cmd_addr ),
        .s6_icb_cmd_read    (s6_icb_cmd_read ),
        .s6_icb_cmd_wdata   (s6_icb_cmd_wdata),
        .s6_icb_cmd_wmask   (s6_icb_cmd_wmask),
        
        .slave_lite_reg0    (slave_lite_reg0),
        .slave_lite_reg1    (slave_lite_reg1),
        .slave_lite_reg2    (slave_lite_reg2),
        .slave_lite_reg3    (slave_lite_reg3),
        .slave_lite_reg4    (slave_lite_reg4),
        .slave_lite_reg5    (slave_lite_reg5),
        .slave_lite_reg6    (slave_lite_reg6)
   );

    yolo_accel_top yolo_accel_top_module(
        // system signals
        .sclk                    (sys_clk_50m),      // 原工程给200M，这里给50M     
        .s_rst_n                 (hard_rst_n),       
                                 
        .slave_lite_reg0         (slave_lite_reg0),
        .slave_lite_reg1         (slave_lite_reg1),
        .slave_lite_reg2         (slave_lite_reg2),
        .slave_lite_reg3         (slave_lite_reg3),
                                 
        .s_axis_mm2s_tdata       (s_axis_mm2s_tdata ),       
        .s_axis_mm2s_tkeep       (s_axis_mm2s_tkeep ),       
        .s_axis_mm2s_tvalid      (s_axis_mm2s_tvalid),       
        .s_axis_mm2s_tready      (s_axis_mm2s_tready),       
        .s_axis_mm2s_tlast       (s_axis_mm2s_tlast ),       
                                 
        .s_axis_s2mm_tdata       (s_axis_s2mm_tdata ),       
        .s_axis_s2mm_tkeep       (s_axis_s2mm_tkeep ),       
        .s_axis_s2mm_tvalid      (s_axis_s2mm_tvalid),       
        .s_axis_s2mm_tready      (s_axis_s2mm_tready),       
        .s_axis_s2mm_tlast       (s_axis_s2mm_tlast ),       
                                 
        .task_finish             (task_finish)       
    );

    // fpioa中断握手
    reg        PL2RISCV = 1'b0       /*synthesis PAP_MARK_DEBUG="1"*/;
    wire       RISCV2PL              /*synthesis PAP_MARK_DEBUG="1"*/;
    reg  [1:0] fpioa_state = 2'd0    /*synthesis PAP_MARK_DEBUG="1"*/;

    always @(posedge sys_clk_50m) begin
        if (!hard_rst_n) begin
            PL2RISCV <= 1'b0;
        end
        else begin
            case(fpioa_state)
                2'd0: begin
                    if (init_done) begin
                        PL2RISCV <= 1'b1;
                        fpioa_state <= 2'd2;
                    end   
                    else begin
                        PL2RISCV <= 1'b0;
                        fpioa_state <= fpioa_state;
                    end
                end
                2'd1: begin
                    if (task_finish) begin
                        PL2RISCV <= 1'b1;
                        fpioa_state <= 2'd2;
                    end   
                    else begin
                        PL2RISCV <= 1'b0;
                        fpioa_state <= fpioa_state;
                    end
                end
                2'd2: begin
                    if (RISCV2PL) begin
                        PL2RISCV <= 1'b0;
                        fpioa_state <= 2'd1;
                    end
                    else begin
                        PL2RISCV <= 1'b1;
                        fpioa_state <= fpioa_state;
                    end
                end                   
            endcase
        end
    end

    parameter TH_1S = 27'd33000000;
    reg  [26:0]        cnt;
        
    //心跳信号    
    always @(posedge core_clk) begin
        if (!ddr_init_done)
            cnt <= 27'd0;
        else if ( cnt >= TH_1S )
            cnt <= 27'd0;
        else
            cnt <= cnt + 27'd1;
    end
    
    always @(posedge core_clk) begin
        if (!ddr_init_done)
            heart_beat_led <= 1'd1;
        else if ( cnt >= TH_1S )
            heart_beat_led <= ~heart_beat_led;
    end

endmodule