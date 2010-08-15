cmd_kernel/trace/built-in.o :=  arm-eabi-ld -EL    -r -o kernel/trace/built-in.o kernel/trace/ring_buffer.o kernel/trace/trace.o kernel/trace/trace_nop.o 
