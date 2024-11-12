module slave_lite_reg_ctrrl(

    input  wire           sys_clk_50m        /*synthesis PAP_MARK_DEBUG="1"*/,
    input  wire           hard_rst_n         ,

    input  wire           s6_icb_cmd_valid   /*synthesis PAP_MARK_DEBUG="1"*/,
    output wire           s6_icb_cmd_ready   /*synthesis PAP_MARK_DEBUG="1"*/,
    input  wire [31:0]    s6_icb_cmd_addr    /*synthesis PAP_MARK_DEBUG="1"*/,
    input  wire           s6_icb_cmd_read    /*synthesis PAP_MARK_DEBUG="1"*/,
    input  wire [31:0]    s6_icb_cmd_wdata   /*synthesis PAP_MARK_DEBUG="1"*/,
    input  wire [3:0]     s6_icb_cmd_wmask   /*synthesis PAP_MARK_DEBUG="1"*/,

    output reg  [31:0]    slave_lite_reg0    /*synthesis PAP_MARK_DEBUG="1"*/,
    output reg  [31:0]    slave_lite_reg1    /*synthesis PAP_MARK_DEBUG="1"*/,
    output reg  [31:0]    slave_lite_reg2    /*synthesis PAP_MARK_DEBUG="1"*/,
    output reg  [31:0]    slave_lite_reg3    /*synthesis PAP_MARK_DEBUG="1"*/,

    // slave_lite_reg4为当前写入yolo数据长度，包括偏置、激活、权重、特征数据
    // slave_lite_reg5为当前写入数据类型，其中0x00000010为偏置、激活、权重，0x0为写特征数据，0x00000020为读偏置、激活、权重、读特征数据到yolo加速模块
    // slave_lite_reg6为当前写入偏置、激活、权重的ddr地址，或读取偏置、激活、权重、特征数据的ddr地址

    output reg  [31:0]    slave_lite_reg4    /*synthesis PAP_MARK_DEBUG="1"*/,
    output reg  [31:0]    slave_lite_reg5    /*synthesis PAP_MARK_DEBUG="1"*/,
    output reg  [31:0]    slave_lite_reg6    /*synthesis PAP_MARK_DEBUG="1"*/

    );

    localparam lite_reg0 = 32'hC0000000  ;
    localparam lite_reg1 = 32'hC0000004  ;
    localparam lite_reg2 = 32'hC0000008  ;
    localparam lite_reg3 = 32'hC000000C  ;
    localparam lite_reg4 = 32'hC0000010  ;
    localparam lite_reg5 = 32'hC0000014  ;
    localparam lite_reg6 = 32'hC0000018  ;

    assign s6_icb_cmd_ready = 1'b1;

    always @(posedge sys_clk_50m) begin
        if (!hard_rst_n) begin
            slave_lite_reg0 <= 32'd0;
            slave_lite_reg1 <= 32'd0;
            slave_lite_reg2 <= 32'd0;
            slave_lite_reg3 <= 32'd0;
            slave_lite_reg4 <= 32'd0;
            slave_lite_reg5 <= 32'd0;
            slave_lite_reg6 <= 32'd0;
        end
        else begin
            if (s6_icb_cmd_ready && s6_icb_cmd_valid && s6_icb_cmd_read == 1'b0) begin
                case(s6_icb_cmd_addr)
                    lite_reg0: begin
                        slave_lite_reg0 <= s6_icb_cmd_wdata;
                    end
                    lite_reg1: begin
                        slave_lite_reg1 <= s6_icb_cmd_wdata;
                    end
                    lite_reg2: begin
                        slave_lite_reg2 <= s6_icb_cmd_wdata;
                    end
                    lite_reg3: begin
                        slave_lite_reg3 <= s6_icb_cmd_wdata;
                    end
                    lite_reg4: begin
                        slave_lite_reg4 <= s6_icb_cmd_wdata;
                    end
                    lite_reg5: begin
                        slave_lite_reg5 <= s6_icb_cmd_wdata;
                    end
                    lite_reg6: begin
                        slave_lite_reg6 <= s6_icb_cmd_wdata;
                    end
               endcase
            end
            else begin
                slave_lite_reg0 <= slave_lite_reg0;
                slave_lite_reg1 <= slave_lite_reg1;
                slave_lite_reg2 <= slave_lite_reg2;
                slave_lite_reg3 <= slave_lite_reg3;
                slave_lite_reg4 <= slave_lite_reg4;
                slave_lite_reg5 <= slave_lite_reg5;
                slave_lite_reg6 <= slave_lite_reg6;
            end
        end 
    end

endmodule