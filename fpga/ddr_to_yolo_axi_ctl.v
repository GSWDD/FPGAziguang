module ddr_to_yolo_axi_ctl(

    input    wire                sysclk         /*synthesis PAP_MARK_DEBUG="1"*/,
    input    wire                sysrst_n       ,

//    input    wire                data_rd_en     /*synthesis PAP_MARK_DEBUG="1"*/,
    input    wire    [31:0]      slave_lite_reg5/*synthesis PAP_MARK_DEBUG="1"*/,    // 数据类型，只有为0x20时才使能本模块功能
    input    wire    [31:0]      data_rd_len    /*synthesis PAP_MARK_DEBUG="1"*/,
    input    wire    [31:0]      data_rd_araddr /*synthesis PAP_MARK_DEBUG="1"*/,

    input    wire                axi_clk        /*synthesis PAP_MARK_DEBUG="1"*/,
    input    wire                axi_rst        /*synthesis PAP_MARK_DEBUG="1"*/,

    output   reg     [31:0]      axi_araddr     /*synthesis PAP_MARK_DEBUG="1"*/,
    output   wire    [3:0]       axi_aruser_id  /*synthesis PAP_MARK_DEBUG="1"*/,
    output   wire    [3:0]       axi_arlen      /*synthesis PAP_MARK_DEBUG="1"*/,
    output   reg                 axi_arvalid    /*synthesis PAP_MARK_DEBUG="1"*/,
    input                        axi_arready    /*synthesis PAP_MARK_DEBUG="1"*/,
                                                  
    input    wire    [255:0]     axi_rdata      /*synthesis PAP_MARK_DEBUG="1"*/,
    input    wire                axi_rvalid     /*synthesis PAP_MARK_DEBUG="1"*/,
    input    wire                axi_rlast      /*synthesis PAP_MARK_DEBUG="1"*/,
    input    wire    [3:0]       axi_rid        /*synthesis PAP_MARK_DEBUG="1"*/,

    output   wire    [63:0]      s_axis_mm2s_tdata   /*synthesis PAP_MARK_DEBUG="1"*/,       
    output   wire    [7:0]       s_axis_mm2s_tkeep   /*synthesis PAP_MARK_DEBUG="1"*/,       
    output   reg                 s_axis_mm2s_tvalid  /*synthesis PAP_MARK_DEBUG="1"*/,       
    input    wire                s_axis_mm2s_tready  /*synthesis PAP_MARK_DEBUG="1"*/,       
    output   reg                 s_axis_mm2s_tlast   /*synthesis PAP_MARK_DEBUG="1"*/

   );

    wire  [31:0]   rd_ddr_len            /*synthesis PAP_MARK_DEBUG="1"*/;
    reg   [31:0]   rd_ddr_len_cnt        /*synthesis PAP_MARK_DEBUG="1"*/;
    wire  [31:0]   rd_ddr_baseaddr       /*synthesis PAP_MARK_DEBUG="1"*/;

    assign rd_ddr_len = data_rd_len;
    assign rd_ddr_baseaddr = data_rd_araddr;

    assign axi_aruser_id = 4'h0;
    assign axi_arlen = 4'h0;

    assign s_axis_mm2s_tkeep = 8'hFF;

    reg   [3:0]    wr_state = 0      /*synthesis PAP_MARK_DEBUG="1"*/;

    reg            fifo_wr_en        /*synthesis PAP_MARK_DEBUG="1"*/;
    reg   [255:0]  fifo_wr_data      /*synthesis PAP_MARK_DEBUG="1"*/;
    wire           fifo_wr_full      /*synthesis PAP_MARK_DEBUG="1"*/;
    wire           fifo_rd_en        /*synthesis PAP_MARK_DEBUG="1"*/;
    wire  [63:0]   fifo_rd_data      /*synthesis PAP_MARK_DEBUG="1"*/;
    wire           fifo_rd_empty     /*synthesis PAP_MARK_DEBUG="1"*/;

    assign fifo_rd_en = !fifo_rd_empty;
    assign s_axis_mm2s_tdata = fifo_rd_data;

    reg   [3:0]    wr_axis_state = 0 /*synthesis PAP_MARK_DEBUG="1"*/;

    reg   [31:0]   rd_fifo_len_cnt = 0 /*synthesis PAP_MARK_DEBUG="1"*/;

    always @(posedge sysclk) begin
        if (!sysrst_n) begin
            rd_fifo_len_cnt <= 32'd0;
            s_axis_mm2s_tvalid <= 1'b0;
            s_axis_mm2s_tlast <= 1'b0;
            wr_axis_state <= 4'd0;
        end
        else begin
            case(wr_axis_state)
                4'd0:begin
                    rd_fifo_len_cnt <= 32'd0;
                    s_axis_mm2s_tvalid <= 1'b0;
                    s_axis_mm2s_tlast <= 1'b0;
                    if (slave_lite_reg5 == 32'h00000020) begin
                        wr_axis_state <= wr_axis_state + 1'b1;
                    end
                    else begin
                        wr_axis_state <= wr_axis_state;
                    end
                end
                4'd1:begin
                    if (s_axis_mm2s_tready && fifo_rd_en) begin
                        if (rd_fifo_len_cnt < data_rd_len - 32'd8) begin
                            s_axis_mm2s_tvalid <= 1'b1;
                            s_axis_mm2s_tlast <= 1'b0;
                            rd_fifo_len_cnt <= rd_fifo_len_cnt + 32'd8;
                            wr_axis_state <= wr_axis_state;
                        end
                        else if (rd_fifo_len_cnt == data_rd_len - 32'd8) begin
                            s_axis_mm2s_tvalid <= 1'b1;
                            s_axis_mm2s_tlast <= 1'b1;
                            rd_fifo_len_cnt <= 32'd0;
                            wr_axis_state <= wr_axis_state + 1'b1;
                        end
                    end
                    else begin
                        rd_fifo_len_cnt <= rd_fifo_len_cnt;
                        s_axis_mm2s_tvalid <= 1'b0;
                        s_axis_mm2s_tlast <= 1'b0;
                        wr_axis_state <= wr_axis_state;
                    end
                end
                4'd2:begin
                    rd_fifo_len_cnt <= 32'd0;
                    s_axis_mm2s_tvalid <= 1'b0;
                    s_axis_mm2s_tlast <= 1'b0;
                    if (slave_lite_reg5 == 32'd0) begin
                        wr_axis_state <= 4'd0;
                    end
                    else begin
                        wr_axis_state <= wr_axis_state;
                    end
                end
            endcase
        end
    end

    always @(posedge axi_clk) begin
        if (axi_rst) begin
            rd_ddr_len_cnt <= 32'd0;
            wr_state <= 4'd0;
            axi_arvalid <= 1'b0;
            fifo_wr_en <= 1'b0;
            fifo_wr_data <= axi_rdata;
            axi_araddr <= 28'd0;
        end
        else begin
            case(wr_state)
                4'd0:begin
                    if (slave_lite_reg5 == 32'h0000_0020) begin
                        rd_ddr_len_cnt <= rd_ddr_len_cnt;
                        fifo_wr_en <= 1'b0;       
                        fifo_wr_data <= 256'd0;
                        wr_state <= wr_state + 1'b1;
                    end
                    else begin
                        rd_ddr_len_cnt <= 32'd0;
                        wr_state <= wr_state;
                    end
                end
                4'd1:begin

                    fifo_wr_en <= 1'b0;
                    if (rd_ddr_len_cnt < rd_ddr_len) begin
                        if (fifo_rd_empty) begin
                            axi_arvalid <= 1'b1;
                            axi_araddr <= rd_ddr_baseaddr[27:0] + rd_ddr_len_cnt;
                            rd_ddr_len_cnt <= rd_ddr_len_cnt + 32'd32;
                            wr_state <= wr_state + 1'b1;                 
                        end
                        else begin
                            wr_state <= wr_state;
                        end
                    end
                    else begin
                        wr_state <= 4'd0;
                    end
                end
                4'd2:begin
                    if (axi_arready && axi_arvalid) begin
                        axi_arvalid <= 1'b0;
                        wr_state <= wr_state + 1'b1;
                    end
                    else begin
                        axi_arvalid <= axi_arvalid;
                        wr_state <= wr_state;
                    end
                end
                4'd3:begin
                    if (axi_rvalid && axi_rlast) begin
                        fifo_wr_en <= 1'b1;
                        fifo_wr_data <= axi_rdata;
                        wr_state <= 4'd1;
                    end 
                    else begin
                        fifo_wr_en <= 1'b0;
                        fifo_wr_data <= axi_rdata;
                        wr_state <= wr_state;
                    end
                end
                default:;
            endcase
        end
    end

    ddr_to_yolo ddr_to_yolo_fifo (
        .wr_clk        (axi_clk),                   // input
        .wr_rst        (1'b0),                      // input
        .wr_en         (fifo_wr_en),                // input
        .wr_data       (fifo_wr_data),              // input [255:0]
        .wr_full       (fifo_wr_full),              // output
        .almost_full   (),                          // output
        .rd_clk        (sysclk),                    // input
        .rd_rst        (!sysrst_n),                 // input
        .rd_en         (fifo_rd_en),                // input
        .rd_data       (fifo_rd_data),              // output [63:0]
        .rd_empty      (fifo_rd_empty),             // output
        .almost_empty  ()                           // output
    );

endmodule