cmd_arch/arm/kernel/entry-common.o := arm-eabi-gcc -Wp,-MD,arch/arm/kernel/.entry-common.o.d  -nostdinc -isystem /home/jason/android-ndk-r4b/build/prebuilt/linux-x86/arm-eabi-4.4.0/bin/../lib/gcc/arm-eabi/4.4.0/include -Iinclude  -I/home/jason/heroc-kernels/stock/heroc-2.6.29-bc0d2cff/arch/arm/include -include include/linux/autoconf.h -D__KERNEL__ -mlittle-endian -Iarch/arm/mach-msm/include -D__ASSEMBLY__ -mabi=aapcs-linux -mno-thumb-interwork -D__LINUX_ARM_ARCH__=6 -march=armv6 -mtune=arm1136j-s -msoft-float -gdwarf-2     -c -o arch/arm/kernel/entry-common.o arch/arm/kernel/entry-common.S

deps_arch/arm/kernel/entry-common.o := \
  arch/arm/kernel/entry-common.S \
    $(wildcard include/config/function/tracer.h) \
    $(wildcard include/config/dynamic/ftrace.h) \
    $(wildcard include/config/cpu/arm710.h) \
    $(wildcard include/config/oabi/compat.h) \
    $(wildcard include/config/arm/thumb.h) \
    $(wildcard include/config/aeabi.h) \
    $(wildcard include/config/alignment/trap.h) \
  /home/jason/heroc-kernels/stock/heroc-2.6.29-bc0d2cff/arch/arm/include/asm/unistd.h \
  /home/jason/heroc-kernels/stock/heroc-2.6.29-bc0d2cff/arch/arm/include/asm/ftrace.h \
  arch/arm/mach-msm/include/mach/entry-macro.S \
  arch/arm/mach-msm/include/mach/msm_iomap.h \
    $(wildcard include/config/arch/msm/arm11.h) \
    $(wildcard include/config/arch/msm/scorpion.h) \
    $(wildcard include/config/arch/qsd8x50.h) \
    $(wildcard include/config/arch/msm7225.h) \
    $(wildcard include/config/arch/msm7227.h) \
    $(wildcard include/config/msm/debug/uart.h) \
    $(wildcard include/config/cache/l2x0.h) \
  /home/jason/heroc-kernels/stock/heroc-2.6.29-bc0d2cff/arch/arm/include/asm/sizes.h \
  arch/arm/kernel/entry-header.S \
    $(wildcard include/config/frame/pointer.h) \
  include/linux/init.h \
    $(wildcard include/config/modules.h) \
    $(wildcard include/config/hotplug.h) \
  include/linux/compiler.h \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
  include/linux/linkage.h \
  /home/jason/heroc-kernels/stock/heroc-2.6.29-bc0d2cff/arch/arm/include/asm/linkage.h \
  /home/jason/heroc-kernels/stock/heroc-2.6.29-bc0d2cff/arch/arm/include/asm/assembler.h \
    $(wildcard include/config/cpu/feroceon.h) \
  /home/jason/heroc-kernels/stock/heroc-2.6.29-bc0d2cff/arch/arm/include/asm/ptrace.h \
    $(wildcard include/config/smp.h) \
  /home/jason/heroc-kernels/stock/heroc-2.6.29-bc0d2cff/arch/arm/include/asm/hwcap.h \
  include/asm/asm-offsets.h \
  /home/jason/heroc-kernels/stock/heroc-2.6.29-bc0d2cff/arch/arm/include/asm/errno.h \
  include/asm-generic/errno.h \
  include/asm-generic/errno-base.h \
  /home/jason/heroc-kernels/stock/heroc-2.6.29-bc0d2cff/arch/arm/include/asm/thread_info.h \
    $(wildcard include/config/arm/thumbee.h) \
  /home/jason/heroc-kernels/stock/heroc-2.6.29-bc0d2cff/arch/arm/include/asm/fpstate.h \
    $(wildcard include/config/vfpv3.h) \
    $(wildcard include/config/iwmmxt.h) \
  arch/arm/kernel/calls.S \

arch/arm/kernel/entry-common.o: $(deps_arch/arm/kernel/entry-common.o)

$(deps_arch/arm/kernel/entry-common.o):
