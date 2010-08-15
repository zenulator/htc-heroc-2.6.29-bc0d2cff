cmd_arch/arm/lib/clear_user.o := arm-eabi-gcc -Wp,-MD,arch/arm/lib/.clear_user.o.d  -nostdinc -isystem /home/jason/android-ndk-r4b/build/prebuilt/linux-x86/arm-eabi-4.4.0/bin/../lib/gcc/arm-eabi/4.4.0/include -Iinclude  -I/home/jason/heroc-kernels/stock/heroc-2.6.29-bc0d2cff/arch/arm/include -include include/linux/autoconf.h -D__KERNEL__ -mlittle-endian -Iarch/arm/mach-msm/include -D__ASSEMBLY__ -mabi=aapcs-linux -mno-thumb-interwork -D__LINUX_ARM_ARCH__=6 -march=armv6 -mtune=arm1136j-s -msoft-float -gdwarf-2     -c -o arch/arm/lib/clear_user.o arch/arm/lib/clear_user.S

deps_arch/arm/lib/clear_user.o := \
  arch/arm/lib/clear_user.S \
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
    $(wildcard include/config/smp.h) \
  /home/jason/heroc-kernels/stock/heroc-2.6.29-bc0d2cff/arch/arm/include/asm/hwcap.h \

arch/arm/lib/clear_user.o: $(deps_arch/arm/lib/clear_user.o)

$(deps_arch/arm/lib/clear_user.o):
