cmd_arch/arm/mm/proc-v6.o := arm-eabi-gcc -Wp,-MD,arch/arm/mm/.proc-v6.o.d  -nostdinc -isystem /home/jason/android-ndk-r4b/build/prebuilt/linux-x86/arm-eabi-4.4.0/bin/../lib/gcc/arm-eabi/4.4.0/include -Iinclude  -I/home/jason/heroc-kernels/stock/heroc-2.6.29-bc0d2cff/arch/arm/include -include include/linux/autoconf.h -D__KERNEL__ -mlittle-endian -Iarch/arm/mach-msm/include -D__ASSEMBLY__ -mabi=aapcs-linux -mno-thumb-interwork -D__LINUX_ARM_ARCH__=6 -march=armv6 -mtune=arm1136j-s -msoft-float -gdwarf-2     -c -o arch/arm/mm/proc-v6.o arch/arm/mm/proc-v6.S

deps_arch/arm/mm/proc-v6.o := \
  arch/arm/mm/proc-v6.S \
    $(wildcard include/config/smp.h) \
    $(wildcard include/config/mmu.h) \
  include/linux/linkage.h \
  include/linux/compiler.h \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
  /home/jason/heroc-kernels/stock/heroc-2.6.29-bc0d2cff/arch/arm/include/asm/linkage.h \
  /home/jason/heroc-kernels/stock/heroc-2.6.29-bc0d2cff/arch/arm/include/asm/assembler.h \
    $(wildcard include/config/cpu/feroceon.h) \
  /home/jason/heroc-kernels/stock/heroc-2.6.29-bc0d2cff/arch/arm/include/asm/ptrace.h \
    $(wildcard include/config/arm/thumb.h) \
  /home/jason/heroc-kernels/stock/heroc-2.6.29-bc0d2cff/arch/arm/include/asm/hwcap.h \
  include/asm/asm-offsets.h \
  /home/jason/heroc-kernels/stock/heroc-2.6.29-bc0d2cff/arch/arm/include/asm/pgtable-hwdef.h \
  /home/jason/heroc-kernels/stock/heroc-2.6.29-bc0d2cff/arch/arm/include/asm/pgtable.h \
  include/asm-generic/4level-fixup.h \
  /home/jason/heroc-kernels/stock/heroc-2.6.29-bc0d2cff/arch/arm/include/asm/proc-fns.h \
    $(wildcard include/config/cpu/32.h) \
    $(wildcard include/config/cpu/arm610.h) \
    $(wildcard include/config/cpu/arm7tdmi.h) \
    $(wildcard include/config/cpu/arm710.h) \
    $(wildcard include/config/cpu/arm720t.h) \
    $(wildcard include/config/cpu/arm740t.h) \
    $(wildcard include/config/cpu/arm9tdmi.h) \
    $(wildcard include/config/cpu/arm920t.h) \
    $(wildcard include/config/cpu/arm922t.h) \
    $(wildcard include/config/cpu/arm925t.h) \
    $(wildcard include/config/cpu/arm926t.h) \
    $(wildcard include/config/cpu/arm940t.h) \
    $(wildcard include/config/cpu/arm946e.h) \
    $(wildcard include/config/cpu/sa110.h) \
    $(wildcard include/config/cpu/sa1100.h) \
    $(wildcard include/config/cpu/arm1020.h) \
    $(wildcard include/config/cpu/arm1020e.h) \
    $(wildcard include/config/cpu/arm1022.h) \
    $(wildcard include/config/cpu/arm1026.h) \
    $(wildcard include/config/cpu/xscale.h) \
    $(wildcard include/config/cpu/xsc3.h) \
    $(wildcard include/config/cpu/v6.h) \
    $(wildcard include/config/cpu/v7.h) \
  /home/jason/heroc-kernels/stock/heroc-2.6.29-bc0d2cff/arch/arm/include/asm/memory.h \
    $(wildcard include/config/page/offset.h) \
    $(wildcard include/config/dram/size.h) \
    $(wildcard include/config/dram/base.h) \
    $(wildcard include/config/zone/dma.h) \
    $(wildcard include/config/discontigmem.h) \
  include/linux/const.h \
  arch/arm/mach-msm/include/mach/memory.h \
    $(wildcard include/config/arch/qsd8x50.h) \
    $(wildcard include/config/arch/msm7225.h) \
    $(wildcard include/config/arch/msm7200a.h) \
    $(wildcard include/config/arch/msm7201a.h) \
    $(wildcard include/config/arch/msm7x00a.h) \
    $(wildcard include/config/mach/desirec.h) \
    $(wildcard include/config/arch/msm7227.h) \
    $(wildcard include/config/arch/msm/arm11.h) \
    $(wildcard include/config/cache/l2x0.h) \
    $(wildcard include/config/arch/msm/scorpion.h) \
  /home/jason/heroc-kernels/stock/heroc-2.6.29-bc0d2cff/arch/arm/include/asm/sizes.h \
  include/asm-generic/memory_model.h \
    $(wildcard include/config/flatmem.h) \
    $(wildcard include/config/sparsemem/vmemmap.h) \
    $(wildcard include/config/sparsemem.h) \
  arch/arm/mach-msm/include/mach/vmalloc.h \
  arch/arm/mm/proc-macros.S \
    $(wildcard include/config/cpu/dcache/writethrough.h) \
  /home/jason/heroc-kernels/stock/heroc-2.6.29-bc0d2cff/arch/arm/include/asm/thread_info.h \
    $(wildcard include/config/arm/thumbee.h) \
  /home/jason/heroc-kernels/stock/heroc-2.6.29-bc0d2cff/arch/arm/include/asm/fpstate.h \
    $(wildcard include/config/vfpv3.h) \
    $(wildcard include/config/iwmmxt.h) \

arch/arm/mm/proc-v6.o: $(deps_arch/arm/mm/proc-v6.o)

$(deps_arch/arm/mm/proc-v6.o):
