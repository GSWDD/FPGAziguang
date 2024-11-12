module  ov5640_data(
        // system signals
        input                   s_rst_n                 ,       
        // OV5640
        input                   ov5640_pclk             ,       
        input                   ov5640_href             ,       
        input                   ov5640_vsync            ,       
        input           [ 7:0]  ov5640_data             ,       
        // User Interfaces
        output  wire            vid_ce                  ,
        output  wire            m_vsync                 ,
        output  wire            m_hsync                 ,
        output  wire            m_href                  ,
        output  wire    [23:0]  m_data                  
            
);

//========================================================================\
// =========== Define Parameter and Internal signals =========== 
//========================================================================/
wire                            ov5640_vsync_pos                ;
reg                             ov5640_vsync_r1                 ;

reg     [ 3:0]                  frame_cnt                       ;
wire                            frame_vaild                     ;
reg     [ 7:0]                  ov5640_data_r1                  ;
reg                             m_wr_en                         ;  
wire    [15:0]                  pixel_data                      ;   
//=============================================================================
//**************    Main Code   **************
//=============================================================================


always  @(posedge ov5640_pclk) begin
        ov5640_vsync_r1 <=      ov5640_vsync;
        ov5640_data_r1  <=      ov5640_data;
end

always  @(posedge ov5640_pclk or negedge s_rst_n) begin
        if(s_rst_n == 1'b0)
                frame_cnt       <=      'd0;
        else if(frame_vaild == 1'b0 && ov5640_vsync_pos == 1'b1)
                frame_cnt       <=      frame_cnt + 1'b1;
end

always  @(posedge ov5640_pclk or negedge s_rst_n) begin
        if(s_rst_n == 1'b0)
                m_wr_en <=      1'b0;
        else if(frame_vaild == 1'b1 && ov5640_href == 1'b1)
                m_wr_en <=      ~m_wr_en;
        else
                m_wr_en <=      1'b0;
end

assign  pixel_data              =       {ov5640_data_r1, ov5640_data};
assign  m_data                  =       {pixel_data[15:11], 3'h0, pixel_data[10:05], 2'h0, pixel_data[04:00], 3'h0};
assign  m_vsync                 =       (frame_vaild == 1'b1) ? ov5640_vsync : 1'b0;
assign  m_href                  =       (frame_vaild == 1'b1) ? ov5640_href : 1'b0;
assign  m_hsync                 =        ~m_href;
assign  frame_vaild             =       (frame_cnt >= 'd10) ? 1'b1 : 1'b0;
assign  ov5640_vsync_pos        =       ov5640_vsync & ~ov5640_vsync_r1;
assign  vid_ce                  =       m_hsync | m_wr_en;

endmodule
