module ddr_ctl(

    input  wire                sys_clk_50m       ,
    input  wire                hard_rst_n        ,
    output wire                ddr_init_done     ,
    output wire                ddrphy_clkin      ,
    output wire                pll_lock          ,

    output wire                mem_rst_n         ,
    output wire                mem_ck            ,
    output wire                mem_ck_n          ,
    output wire                mem_cke           ,
    output wire                mem_cs_n          ,
    output wire                mem_ras_n         ,
    output wire                mem_cas_n         ,
    output wire                mem_we_n          ,
    output wire                mem_odt           ,
    output wire [14:0]         mem_a             ,
    output wire [2:0]          mem_ba            ,
    inout  wire [3:0]          mem_dqs           ,
    inout  wire [3:0]          mem_dqs_n         ,
    inout  wire [31:0]         mem_dq            ,
    output wire [3:0]          mem_dm            ,
    output reg                 heart_beat_led    ,

    //ov5640 data_in 1280*720*3
    input  wire                cmos1_pclk        ,
    input  wire                m_vsync           ,
    input  wire                m_href            ,
    input  wire [23:0]         m_data            ,

    output wire                init_done         ,

    // RISCV
    input  wire                s7_icb_cmd_valid       /*synthesis PAP_MARK_DEBUG="1"*/,
    output wire                s7_icb_cmd_ready       /*synthesis PAP_MARK_DEBUG="1"*/,
    input  wire [31:0]         s7_icb_cmd_addr        /*synthesis PAP_MARK_DEBUG="1"*/,
    input  wire                s7_icb_cmd_read        /*synthesis PAP_MARK_DEBUG="1"*/,
    input  wire [31:0]         s7_icb_cmd_wdata       /*synthesis PAP_MARK_DEBUG="1"*/,
    input  wire [3:0]          s7_icb_cmd_wmask       /*synthesis PAP_MARK_DEBUG="1"*/,

    input  wire [31:0]         slave_lite_reg4        /*synthesis PAP_MARK_DEBUG="1"*/,
    input  wire [31:0]         slave_lite_reg5        /*synthesis PAP_MARK_DEBUG="1"*/,
    input  wire [31:0]         slave_lite_reg6        /*synthesis PAP_MARK_DEBUG="1"*/,

    output wire [63:0]         s_axis_mm2s_tdata       /*synthesis PAP_MARK_DEBUG="1"*/,       
    output wire [ 7:0]         s_axis_mm2s_tkeep       /*synthesis PAP_MARK_DEBUG="1"*/,       
    output wire                s_axis_mm2s_tvalid      /*synthesis PAP_MARK_DEBUG="1"*/,       
    input  wire                s_axis_mm2s_tready      /*synthesis PAP_MARK_DEBUG="1"*/,       
    output wire                s_axis_mm2s_tlast       /*synthesis PAP_MARK_DEBUG="1"*/

);

    //========================================================================\
    // =========== Define Parameter and Internal signals =========== 
    //========================================================================/

    wire [27:0]         ov5640_axi_awaddr        /*synthesis PAP_MARK_DEBUG="1"*/;
    wire [3:0]          ov5640_axi_awuser_id     /*synthesis PAP_MARK_DEBUG="1"*/;
    wire [3:0]          ov5640_axi_awlen         /*synthesis PAP_MARK_DEBUG="1"*/;
    wire                ov5640_axi_awready       /*synthesis PAP_MARK_DEBUG="1"*/;
    wire                ov5640_axi_awvalid       /*synthesis PAP_MARK_DEBUG="1"*/;
    wire [255:0]        ov5640_axi_wdata         /*synthesis PAP_MARK_DEBUG="1"*/;
    wire [31:0]         ov5640_axi_wstrb         /*synthesis PAP_MARK_DEBUG="1"*/;
    wire                ov5640_axi_wready        /*synthesis PAP_MARK_DEBUG="1"*/;
    wire                ov5640_axi_wusero_last   /*synthesis PAP_MARK_DEBUG="1"*/;

    wire [27:0]         ov5640_axi_araddr        /*synthesis PAP_MARK_DEBUG="1"*/;
    wire [3:0]          ov5640_axi_aruser_id     /*synthesis PAP_MARK_DEBUG="1"*/;
    wire [3:0]          ov5640_axi_arlen         /*synthesis PAP_MARK_DEBUG="1"*/;
    wire                ov5640_axi_arready       /*synthesis PAP_MARK_DEBUG="1"*/;
    wire                ov5640_axi_arvalid       /*synthesis PAP_MARK_DEBUG="1"*/;
    wire [255:0]        ov5640_axi_rdata         /*synthesis PAP_MARK_DEBUG="1"*/;
    wire [3:0]          ov5640_axi_rid           /*synthesis PAP_MARK_DEBUG="1"*/;
    wire                ov5640_axi_rlast         /*synthesis PAP_MARK_DEBUG="1"*/;
    wire                ov5640_axi_rvalid        /*synthesis PAP_MARK_DEBUG="1"*/;

    wire [27:0]         s7_axi_awaddr     /*synthesis PAP_MARK_DEBUG="1"*/;
    wire [3:0]          s7_axi_awuser_id  /*synthesis PAP_MARK_DEBUG="1"*/;
    wire [3:0]          s7_axi_awlen      /*synthesis PAP_MARK_DEBUG="1"*/;
    wire                s7_axi_awready    /*synthesis PAP_MARK_DEBUG="1"*/;
    wire                s7_axi_awvalid    /*synthesis PAP_MARK_DEBUG="1"*/;
                                           
    wire [255:0]        s7_axi_wdata      /*synthesis PAP_MARK_DEBUG="1"*/;
    wire [31:0]         s7_axi_wstrb      /*synthesis PAP_MARK_DEBUG="1"*/;
    wire                s7_axi_wready     /*synthesis PAP_MARK_DEBUG="1"*/; 
    wire                s7_axi_wusero_last/*synthesis PAP_MARK_DEBUG="1"*/; 

    wire [27:0]         yolo_axi_araddr   /*synthesis PAP_MARK_DEBUG="1"*/;
    wire [3:0]          yolo_axi_aruser_id/*synthesis PAP_MARK_DEBUG="1"*/;
    wire [3:0]          yolo_axi_arlen    /*synthesis PAP_MARK_DEBUG="1"*/;
    wire                yolo_axi_arready  /*synthesis PAP_MARK_DEBUG="1"*/;
    wire                yolo_axi_arvalid  /*synthesis PAP_MARK_DEBUG="1"*/;
    wire [255:0]        yolo_axi_rdata    /*synthesis PAP_MARK_DEBUG="1"*/;
    wire [3:0]          yolo_axi_rid      /*synthesis PAP_MARK_DEBUG="1"*/;
    wire                yolo_axi_rlast    /*synthesis PAP_MARK_DEBUG="1"*/;
    wire                yolo_axi_rvalid   /*synthesis PAP_MARK_DEBUG="1"*/;

    wire [27:0]         axi_awaddr        /*synthesis PAP_MARK_DEBUG="1"*/;
    wire [3:0]          axi_awuser_id     /*synthesis PAP_MARK_DEBUG="1"*/;
    wire [3:0]          axi_awlen         /*synthesis PAP_MARK_DEBUG="1"*/;
    wire                axi_awready       /*synthesis PAP_MARK_DEBUG="1"*/;
    wire                axi_awvalid       /*synthesis PAP_MARK_DEBUG="1"*/;
    wire [255:0]        axi_wdata         /*synthesis PAP_MARK_DEBUG="1"*/;
    wire [31:0]         axi_wstrb         /*synthesis PAP_MARK_DEBUG="1"*/;
    wire                axi_wready        /*synthesis PAP_MARK_DEBUG="1"*/;
    wire                axi_wusero_last   /*synthesis PAP_MARK_DEBUG="1"*/;

    wire [27:0]         axi_araddr        /*synthesis PAP_MARK_DEBUG="1"*/;
    wire [3:0]          axi_aruser_id     /*synthesis PAP_MARK_DEBUG="1"*/;
    wire [3:0]          axi_arlen         /*synthesis PAP_MARK_DEBUG="1"*/;
    wire                axi_arready       /*synthesis PAP_MARK_DEBUG="1"*/;
    wire                axi_arvalid       /*synthesis PAP_MARK_DEBUG="1"*/;
    wire [255:0]        axi_rdata         /*synthesis PAP_MARK_DEBUG="1"*/;
    wire [3:0]          axi_rid           /*synthesis PAP_MARK_DEBUG="1"*/;
    wire                axi_rlast         /*synthesis PAP_MARK_DEBUG="1"*/;
    wire                axi_rvalid        /*synthesis PAP_MARK_DEBUG="1"*/;

    assign axi_awaddr            = (slave_lite_reg5 == 32'h00000010) ? s7_axi_awaddr : ov5640_axi_awaddr;
    assign axi_awuser_id         = (slave_lite_reg5 == 32'h00000010) ? s7_axi_awuser_id : ov5640_axi_awuser_id;
    assign axi_awlen             = (slave_lite_reg5 == 32'h00000010) ? s7_axi_awlen : ov5640_axi_awlen;
    assign axi_awvalid           = (slave_lite_reg5 == 32'h00000010) ? s7_axi_awvalid : ov5640_axi_awvalid;
    assign axi_wdata             = (slave_lite_reg5 == 32'h00000010) ? s7_axi_wdata : ov5640_axi_wdata;
    assign axi_wstrb             = (slave_lite_reg5 == 32'h00000010) ? s7_axi_wstrb : ov5640_axi_wstrb;
    assign s7_axi_awready        = axi_awready;
    assign s7_axi_wready         = axi_wready;
    assign s7_axi_wusero_last    = axi_wusero_last;
    assign ov5640_axi_awready    = axi_awready;
    assign ov5640_axi_wready     = axi_wready;
    assign ov5640_axi_wusero_last = axi_wusero_last;

    assign yolo_axi_arready  = axi_arready;
    assign yolo_axi_rdata    = axi_rdata;
    assign yolo_axi_rid      = axi_rid;
    assign yolo_axi_rlast    = axi_rlast;
    assign yolo_axi_rvalid   = axi_rvalid;
    assign axi_aruser_id     = (slave_lite_reg5 == 32'h00000020) ? yolo_axi_aruser_id : 4'd0;
    assign axi_araddr        = (slave_lite_reg5 == 32'h00000020) ? yolo_axi_araddr : 4'd0;
    assign axi_arlen         = (slave_lite_reg5 == 32'h00000020) ? yolo_axi_arlen : 4'd0;
    assign axi_arvalid       = (slave_lite_reg5 == 32'h00000020) ? yolo_axi_arvalid : 4'd0;

    wire            core_clk;

    //�޸�ddr��дģ��v1
    fram_buf fram_buf_module(
        .ddr_clk        (  core_clk             ),//input                         ddr_clk,
        .ddr_rstn       (  ddr_init_done        ),//input                         ddr_rstn,

        //ov5640 data_in
        .cmos1_pclk     (cmos1_pclk),
        .m_vsync_720p   (m_vsync   ),
        .m_href_720p    (m_href    ),
        .m_data_720p    (m_data    ),   
                  
        //data_out
        .vout_clk_720p       (  cmos1_pclk   ),//input                         vout_clk,
        .rd_fsync_720p       (  1'b0         ),//input                         rd_fsync,
        .rd_en_720p          (  1'b0         ),//input                         rd_en,
        .vout_de_720p        (               ),//output                        vout_de,
        .vout_data_720p      (               ),//output [PIX_WIDTH- 1'b1 : 0]  vout_data,

        .init_done      (  init_done    ),//output reg                    init_done,

        //axi bus
        .axi_awaddr     (  ov5640_axi_awaddr           ),// output[27:0]
        .axi_awid       (  ov5640_axi_awuser_id        ),// output[3:0]
        .axi_awlen      (  ov5640_axi_awlen            ),// output[3:0]
        .axi_awsize     (                       ),// output[2:0]
        .axi_awburst    (                       ),// output[1:0]
        .axi_awready    (  ov5640_axi_awready          ),// input
        .axi_awvalid    (  ov5640_axi_awvalid          ),// output               
        .axi_wdata      (  ov5640_axi_wdata            ),// output[255:0]
        .axi_wstrb      (  ov5640_axi_wstrb            ),// output[31:0]
        .axi_wlast      (  ov5640_axi_wusero_last      ),// input
        .axi_wvalid     (                       ),// output
        .axi_wready     (  ov5640_axi_wready           ),// input
        .axi_bid        (  4'd0                 ),// input[3:0]
        .axi_araddr     (  ov5640_axi_araddr           ),// output[27:0]
        .axi_arid       (  ov5640_axi_aruser_id        ),// output[3:0]
        .axi_arlen      (  ov5640_axi_arlen            ),// output[3:0]
        .axi_arsize     (                       ),// output[2:0]
        .axi_arburst    (                       ),// output[1:0]
        .axi_arvalid    (  ov5640_axi_arvalid          ),// output
        .axi_arready    (  ov5640_axi_arready          ),// input
        .axi_rready     (                       ),// output
        .axi_rdata      (  ov5640_axi_rdata            ),// input[255:0]
        .axi_rvalid     (  ov5640_axi_rvalid           ),// input
        .axi_rlast      (  ov5640_axi_rlast            ),// input
        .axi_rid        (  ov5640_axi_rid              ) // input[3:0]         
    );

    s7_to_ddr s7_to_ddr_module(
        .RISCV_clk          (sys_clk_50m),    // 50M
        .hard_rst_n         (hard_rst_n),
    
        .s7_icb_cmd_valid   (s7_icb_cmd_valid ),
        .s7_icb_cmd_ready   (s7_icb_cmd_ready ),
        .s7_icb_cmd_addr    (s7_icb_cmd_addr  ),
        .s7_icb_cmd_read    (s7_icb_cmd_read  ),
        .s7_icb_cmd_wdata   (s7_icb_cmd_wdata ),
        .s7_icb_cmd_wmask   (s7_icb_cmd_wmask ),
    
        .slave_lite_reg4    (slave_lite_reg4  ),    // 写ddr长度
        .slave_lite_reg5    (slave_lite_reg5  ),    // 数据类型，只有为0x10时才使能本模块功能
        .slave_lite_reg6    (slave_lite_reg6  ),    // 写ddr基地址
       
        .ddr_clk            (core_clk         ),
        .ddr_rst            (1'b0             ),
    
        .ddr_axi_awaddr     (s7_axi_awaddr   ),
        .ddr_axi_awuser_id  (s7_axi_awuser_id),
        .ddr_axi_awlen      (s7_axi_awlen    ),
        .ddr_axi_awready    (s7_axi_awready  ),
        .ddr_axi_awvalid    (s7_axi_awvalid  ),
        .ddr_axi_wdata      (s7_axi_wdata    ),
        .ddr_axi_wstrb      (s7_axi_wstrb    ),
        .ddr_axi_wready     (s7_axi_wready   ),
        .ddr_axi_wusero_last(s7_axi_wusero_last) 
    );

    ddr_to_yolo_axi_ctl ddr_to_yolo_axi_ctl_module(

        .sysclk              (sys_clk_50m        ),
        .sysrst_n            (hard_rst_n         ),
//        .data_rd_en          (1'b0),
        .slave_lite_reg5     (slave_lite_reg5    ),    // 数据类型，只有为0x20时才使能本模块功能
        .data_rd_len         (slave_lite_reg4    ),    // 读ddr长度
        .data_rd_araddr      (slave_lite_reg6    ),    // 读ddr基地址
        
        .axi_clk             (core_clk           ),
        .axi_rst             (1'b0               ),

        .axi_araddr          (yolo_axi_araddr    ),
        .axi_aruser_id       (yolo_axi_aruser_id ),
        .axi_arlen           (yolo_axi_arlen     ),
        .axi_arvalid         (yolo_axi_arvalid   ),
        .axi_arready         (yolo_axi_arready   ),
        .axi_rdata           (yolo_axi_rdata     ),
        .axi_rvalid          (yolo_axi_rvalid    ),
        .axi_rlast           (yolo_axi_rlast     ),
        .axi_rid             (yolo_axi_rid       ),
        
        .s_axis_mm2s_tdata   (s_axis_mm2s_tdata  ),       
        .s_axis_mm2s_tkeep   (s_axis_mm2s_tkeep  ),       
        .s_axis_mm2s_tvalid  (s_axis_mm2s_tvalid ),       
        .s_axis_mm2s_tready  (s_axis_mm2s_tready ),       
        .s_axis_mm2s_tlast   (s_axis_mm2s_tlast  )

   );

    DDR3_50H u_DDR3_50H (
        .ref_clk                   (sys_clk_50m        ),
        .resetn                    (hard_rst_n         ),// input
        .ddr_init_done             (ddr_init_done      ),// output
        .ddrphy_clkin              (core_clk           ),// output
        .pll_lock                  (pll_lock           ),// output
        
        .axi_awaddr                (axi_awaddr         ),// input [27:0]
        .axi_awuser_ap             (1'b0               ),// input
        .axi_awuser_id             (axi_awuser_id      ),// input [3:0]
        .axi_awlen                 (axi_awlen          ),// input [3:0]
        .axi_awready               (axi_awready        ),// output
        .axi_awvalid               (axi_awvalid        ),// input
        .axi_wdata                 (axi_wdata          ),
        .axi_wstrb                 (axi_wstrb          ),// input [31:0]
        .axi_wready                (axi_wready         ),// output
        .axi_wusero_id             (                   ),// output [3:0]
        .axi_wusero_last           (axi_wusero_last    ),// output

        .axi_araddr                (axi_araddr         ),// input [27:0]
        .axi_aruser_ap             (1'b0               ),// input
        .axi_aruser_id             (axi_aruser_id      ),// input [3:0]
        .axi_arlen                 (axi_arlen          ),// input [3:0]
        .axi_arready               (axi_arready        ),// output
        .axi_arvalid               (axi_arvalid        ),// input
        .axi_rdata                 (axi_rdata          ),// output [255:0]
        .axi_rid                   (axi_rid            ),// output [3:0]
        .axi_rlast                 (axi_rlast          ),// output
        .axi_rvalid                (axi_rvalid         ),// output
        
        .apb_clk                   (1'b0               ),// input
        .apb_rst_n                 (1'b1               ),// input
        .apb_sel                   (1'b0               ),// input
        .apb_enable                (1'b0               ),// input
        .apb_addr                  (8'b0               ),// input [7:0]
        .apb_write                 (1'b0               ),// input
        .apb_ready                 (                   ), // output
        .apb_wdata                 (16'b0              ),// input [15:0]
        .apb_rdata                 (                   ),// output [15:0]
        .apb_int                   (                   ),// output
        
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
        .mem_dm                    (mem_dm             ),// output [3:0]
        //debug
        .debug_data                (                   ),// output [135:0]
        .debug_slice_state         (                   ),// output [51:0]
        .debug_calib_ctrl          (                   ),// output [21:0]
        .ck_dly_set_bin            (                   ),// output [7:0]
        .force_ck_dly_en           (1'b0               ),// input
        .force_ck_dly_set_bin      (8'h05              ),// input [7:0]
        .dll_step                  (                   ),// output [7:0]
        .dll_lock                  (                   ),// output
        .init_read_clk_ctrl        (2'b0               ),// input [1:0]
        .init_slip_step            (4'b0               ),// input [3:0]
        .force_read_clk_ctrl       (1'b0               ),// input
        .ddrphy_gate_update_en     (1'b0               ),// input
        .update_com_val_err_flag   (                   ),// output [3:0]
        .rd_fake_stop              (1'b0               ) // input
    );


endmodule