Build Kernel 3.4.67 MT6572

```sh
export ARCH=arm
export CROSS_COMPILE=/home/user/arm-linu-androideabi-4.6/bin/arm-linux-androideabi-
cd android_kernel_mt6572
make muse72_s4_kk_debug_defconfig
make -j4
```
