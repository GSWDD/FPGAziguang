module  ov5640_top(
    // system signals
    input  wire            sys_rst_n               ,
    input  wire            sys_clk_50m             ,
    input  wire            sys_clk_25m             ,       
    // COMS Interfaces
    output wire            cmos_init_done          ,//OV5640寄存器初始化完成
    inout  wire            cmos1_scl               ,//cmos1 i2c 
    inout  wire            cmos1_sda               ,//cmos1 i2c 
    input  wire            cmos1_vsync             /*synthesis PAP_MARK_DEBUG="1"*/,//cmos1 vsync
    input  wire            cmos1_href              /*synthesis PAP_MARK_DEBUG="1"*/,//cmos1 hsync refrence,data valid
    input  wire            cmos1_pclk              /*synthesis PAP_MARK_DEBUG="1"*/,//cmos1 pxiel clock
    input  wire    [7:0]   cmos1_data              /*synthesis PAP_MARK_DEBUG="1"*/,//cmos1 data
    output wire            cmos1_reset             ,//cmos1 reset
    // User Interfacescmos1

    // 原始图像1280*720*3
    output wire            vid_ce                  /*synthesis PAP_MARK_DEBUG="1"*/,
    output wire            m_vsync                 /*synthesis PAP_MARK_DEBUG="1"*/,
    output wire            m_hsync                 /*synthesis PAP_MARK_DEBUG="1"*/,
    output wire            m_href                  /*synthesis PAP_MARK_DEBUG="1"*/,
    output wire    [23:0]  m_data                  /*synthesis PAP_MARK_DEBUG="1"*/
    
);

    wire                cmos_scl          ;//cmos i2c clock
    wire                cmos_sda          ;//cmos i2c data
    wire                cmos_vsync        ;//cmos vsync
    wire                cmos_href         ;//cmos hsync refrence,data valid
    wire                cmos_pclk         ;//cmos pxiel clock
    wire [7:0]          cmos_data         ;//cmos data
    wire                cmos_reset        ;//cmos reset
    wire                initial_en        ;
    reg  [7:0]          cmos1_d_d0        ;
    reg                 cmos1_href_d0     ;
    reg                 cmos1_vsync_d0    ;


    //配置CMOS///////////////////////////////////////////////////////////////////////////////////
    //OV5640 register configure enable
    power_on_delay	power_on_delay_inst(
        .clk_50M         (sys_clk_50m    ),//input
        .reset_n         (sys_rst_n      ),//input
        .camera1_rstn    (cmos1_reset    ),//output
        .camera2_rstn    (               ),//output
        .camera_pwnd     (               ),//output
        .initial_en      (initial_en     ) //output
    );
    //CMOS1 Camera 
    reg_config	coms1_reg_config(
        .clk_25M         (sys_clk_25m    ),//input
        .camera_rstn     (cmos1_reset    ),//input
        .initial_en      (initial_en     ),//input		
        .i2c_sclk        (cmos1_scl      ),//output
        .i2c_sdat        (cmos1_sda      ),//inout
        .reg_conf_done   (cmos_init_done ),//output config_finished
        .reg_index       (               ),//output reg [8:0]
        .clock_20k       (               ) //output reg
    );


    ov5640_data     ov5640_data_inst(
        // system signals
        .s_rst_n                (sys_rst_n    ),
        // OV5640
        .ov5640_pclk            (cmos1_pclk   ),
        .ov5640_href            (cmos1_href   ),
        .ov5640_vsync           (cmos1_vsync  ),
        .ov5640_data            (cmos1_data   ),
        // User Interfaces
        .vid_ce                 (vid_ce       ),
        .m_vsync                (m_vsync      ),
        .m_hsync                (m_hsync      ),
        .m_href                 (m_href       ),
        .m_data                 (m_data       )       
    );

endmodule