# verilator_emulator

##error fix:\
    `implicit conversion to enum 'enum{}$unit::operation_t' from 'logic' `\
    `op_in_r is an operation, not bit`\
    `op_in_r <= '0 -> op_in_r <= nop`\ 

##verilator commands\
    `verilator cc alu.sv`\
    `ls -l obj_dir/`\
    `verilator -Wall --trace -cc alu.sv --exe tb_alu.cpp`\
    `make -C obj_dir -f Valu.mk Valu`\
    `./obj_dir/Valu`\
    `ls`\
    `sudo apt-get install gtkwave`\
    `gtkwave waveform.vcd`\

##vim commands\
    one more after end of the line\
    `:set ve+=onemore`\
    move\
    `alt + h j k l`\
    select\
    `V v`\
    copy cut paste\
    `y yy 3yy y$ yiw`\
    `d dd 3dd d$`\
    `P p`\

  ![alu](https://github.com/xTech-01/verilator_emulator/blob/main/gtkwave_add_sub_alu.png)\
  ![Ref](https://itsembedded.com/dhd/verilator_1/)
