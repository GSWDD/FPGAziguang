module icb_to_ddr(
    input  wire                RISCV_clk          /*synthesis PAP_MARK_DEBUG="1"*/,    // 50M
    input  wire                hard_rst_n         ,

    input  wire                s7_icb_cmd_valid   /*synthesis PAP_MARK_DEBUG="1"*/,
    output wire                s7_icb_cmd_ready   /*synthesis PAP_MARK_DEBUG="1"*/,
    input  wire [31:0]         s7_icb_cmd_addr    /*synthesis PAP_MARK_DEBUG="1"*/,
    input  wire                s7_icb_cmd_read    /*synthesis PAP_MARK_DEBUG="1"*/,
    input  wire [31:0]         s7_icb_cmd_wdata   /*synthesis PAP_MARK_DEBUG="1"*/,
    input  wire [3:0]          s7_icb_cmd_wmask   /*synthesis PAP_MARK_DEBUG="1"*/,

    input  wire [31:0]         slave_lite_reg4    /*synthesis PAP_MARK_DEBUG="1"*/,    // 写ddr长度
    input  wire [31:0]         slave_lite_reg5    /*synthesis PAP_MARK_DEBUG="1"*/,    // 数据类型，只有为0x10时才使能本模块功能
    input  wire [31:0]         slave_lite_reg6    /*synthesis PAP_MARK_DEBUG="1"*/,    // 写ddr基地址

    input  wire                ddr_clk            /*synthesis PAP_MARK_DEBUG="1"*/,    // 
    input  wire                ddr_rst            /*synthesis PAP_MARK_DEBUG="1"*/,

    output      [27:0]         ddr_axi_awaddr     /*synthesis PAP_MARK_DEBUG="1"*/,
    output wire [3:0]          ddr_axi_awuser_id  /*synthesis PAP_MARK_DEBUG="1"*/,
    output wire [3:0]          ddr_axi_awlen      /*synthesis PAP_MARK_DEBUG="1"*/,
    input  wire                ddr_axi_awready    /*synthesis PAP_MARK_DEBUG="1"*/,
    output                     ddr_axi_awvalid    /*synthesis PAP_MARK_DEBUG="1"*/,
                                                  
    output      [255:0]        ddr_axi_wdata      /*synthesis PAP_MARK_DEBUG="1"*/,
    output wire [31:0]         ddr_axi_wstrb      /*synthesis PAP_MARK_DEBUG="1"*/,
    input                      ddr_axi_wready     /*synthesis PAP_MARK_DEBUG="1"*/  

);

    wire  [31:0]   wr_ddr_len            /*synthesis PAP_MARK_DEBUG="1"*/;
    reg   [31:0]   wr_ddr_len_cnt        /*synthesis PAP_MARK_DEBUG="1"*/;
    wire  [31:0]   wr_ddr_baseaddr       /*synthesis PAP_MARK_DEBUG="1"*/;

    assign wr_ddr_len = slave_lite_reg4;
    assign wr_ddr_baseaddr = slave_lite_reg6;
    assign ddr_axi_wstrb = 32'hFFFFFFFF;

    assign ddr_axi_awuser_id = 4'h0;
    assign ddr_axi_awlen = 4'h0;

    reg   [3:0]    rd_state = 0;

    reg            fifo_wr_en        /*synthesis PAP_MARK_DEBUG="1"*/;
    reg   [31:0]   fifo_wr_data      /*synthesis PAP_MARK_DEBUG="1"*/;
    wire           fifo_wr_full      /*synthesis PAP_MARK_DEBUG="1"*/;
    reg            fifo_rd_en        /*synthesis PAP_MARK_DEBUG="1"*/;
    wire  [255:0]  fifo_rd_data      /*synthesis PAP_MARK_DEBUG="1"*/;
    wire           fifo_rd_empty     /*synthesis PAP_MARK_DEBUG="1"*/;

    always @(posedge RISCV_clk) begin
        if (!hard_rst_n) begin
            fifo_wr_en <= 1'b0;
            fifo_wr_data <= 32'd0;
        end
        else begin
            if (s7_icb_cmd_valid & s7_icb_cmd_ready & !s7_icb_cmd_read & !fifo_wr_full) begin
                fifo_wr_en <= 1'b1;
                fifo_wr_data <= s7_icb_cmd_wdata;
            end
            else begin
                fifo_wr_en <= 1'b0;
                fifo_wr_data <= 32'd0;
            end
        end
    end

    always @(posedge ddr_clk) begin
        if (ddr_rst) begin
            fifo_rd_en <= 1'b0;
        end
        else begin
            if (fifo_rd_empty) begin
                fifo_rd_en <= 1'b0;
            end
            else begin
                fifo_rd_en <= 1'b1;
            end
        end
    end

    always @(posedge ddr_clk) begin
        if (ddr_rst) begin
            wr_ddr_len_cnt <= 32'd0;
            rd_state <= 4'd0;
            ddr_axi_awvalid <= 1'b0;
            ddr_axi_awaddr <= 28'd0;
            ddr_axi_wdata <= 256'd0;
        end
        else begin
            case(rd_state)
                4'd0:begin
                    if (slave_lite_reg5 == 32'h0000_0010) begin
                        wr_ddr_len_cnt <= 32'd0;
                        rd_state <= rd_state + 1'b1;
                    end 
                    else begin
                        wr_ddr_len_cnt <= 32'd0;
                        rd_state <= rd_state;
                    end
                end
                4'd1:begin
                    if (wr_ddr_len_cnt < wr_ddr_len) begin
                        if (ddr_axi_awready && ddr_axi_wready && fifo_rd_en) begin
                            ddr_axi_awvalid <= 1'b1;
                            ddr_axi_awaddr <= wr_ddr_baseaddr[27:0] + wr_ddr_len_cnt;
                            wr_ddr_len_cnt <= wr_ddr_len_cnt + 32'd32;
                            ddr_axi_wdata <= fifo_rd_data;
                            rd_state <= rd_state;
                        end
                        else begin
                            ddr_axi_awvalid <= 1'b0;
                            ddr_axi_awaddr <= 28'd0;
                            ddr_axi_wdata <= 256'd0;
                            wr_ddr_len_cnt <= wr_ddr_len_cnt;
                            rd_state <= rd_state;
                        end
                    end 
                    else begin
                        rd_state <= 4'd0;
                    end
                end
                default:;
            endcase
        end
    end

    icb_to_ddr icb_to_ddr_fifo (
        .wr_clk        (RISCV_clk),                 // input
        .wr_rst        (!hard_rst_n),               // input
        .wr_en         (fifo_wr_en),                // input
        .wr_data       (fifo_wr_data),              // input [31:0]
        .wr_full       (fifo_wr_full),              // output
        .almost_full   (),                          // output
        .rd_clk        (ddr_clk),                   // input
        .rd_rst        (ddr_rst),                   // input
        .rd_en         (fifo_rd_en),                // input
        .rd_data       (fifo_rd_data),              // output [255:0]
        .rd_empty      (fifo_rd_empty),             // output
        .almost_empty  ()                           // output
    );


endmodule