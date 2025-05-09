#include <stdlib.h>
#include <iostream>
#include <verilated.h>
#include <verilated_vcd_c.h>
#include "Valu.h"
#include "Valu___024unit.h"
#include <cstdlib>

#define MAX_SIM_TIME 300
#define VERIF_START_TIME 7 //after reset
vluint64_t sim_time = 0;
// vluint64_t posedge_cnt = 0;

void dut_reset(Valu *dut, vluint64_t &sim_time){
	// always_comb begin
	//     dut.rst = 1'b0;
	//     if (sim_time >= 3 && sim_time < 6) begin
	//         dut.rst = 1'b1;
	//     end
	// end
	dut->rst = 0;
	if(sim_time > 1 && sim_time < 5){
		dut->rst = 1;
		dut->a_in = 0;
		dut->b_in = 0;
		dut->op_in = 0;
		dut->in_valid = 0;
	}
}

void check_out_valid(Valu *dut, vluint64_t &sim_time){
    static unsigned char in_valid = 0; // current cycle
    static unsigned char in_valid_d = 0; //delayed
    static unsigned char out_valid_exp = 0; //expected

    if (sim_time >= VERIF_START_TIME) {
		// test: out_valid <= in_valid_r;
        out_valid_exp = in_valid_d;
        in_valid_d = in_valid;
        in_valid = dut->in_valid;
        if (out_valid_exp != dut->out_valid) {
            std::cout << "ERROR: out_valid mismatch, "
                << "exp: " << (int)(out_valid_exp)
                << " recv: " << (int)(dut->out_valid)
                << " simtime: " << sim_time << std::endl;
        }
    }
	// input reset posedge up input/output updates
	// always_comb begin
	//     in_valid = 0;
	//     initial posedge_cnt <= '0;
	// 	   always_ff @ (posedge clk, posedge rst) begin
	// 	   posedge_cnt <= posedge_cnt + 1'b1;
	// end

	// dut->in_valid = 0
	// posedge_cnt++;
	//  if (posedge_cnt == 5) // 5th clock cyle cc
	//      in_valid = 1;

	//  if (posedge_cnt == 7)
	//      assert (out_valid == 1) else $error("ERROR!")
}

void set_rnd_in_out_valid(Valu *dut, vluint64_t &sim_time){
    if (sim_time >= VERIF_START_TIME) {
        dut->in_valid = rand() % 2; // 0 and 1
    }
}


int main(int argc, char** argv, char** env){
	srand (time(NULL));
	Verilated::commandArgs(argc, argv);
	Valu *dut = new Valu;

	Verilated::traceEverOn(true);
	VerilatedVcdC *m_trace = new VerilatedVcdC;

	dut->trace(m_trace, 5);
	m_trace->open("waveform.vcd");

	while (sim_time < MAX_SIM_TIME) {
		dut_reset(dut, sim_time);

		dut->clk ^= 1; //invert OR ~clk
	    dut->eval(); // evulate for tracing

		if(dut->clk == 1){
			set_rnd_in_out_valid(dut, sim_time);
			check_out_valid(dut, sim_time);  
		}

       	m_trace->dump(sim_time);
 		sim_time++;
	}
	
	m_trace->close();
	delete dut;
	exit(EXIT_SUCCESS);
}