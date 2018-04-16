#!/bin/bash
rm dt.img
read -p "Do you want to remove old 'boot-sd410.img'? [Y/n] " ans

case $ans in [yY][eE][sS]|[yY]|[jJ]|'') 

    echo
    echo "Yes"
    rm boot-sd410.img
    echo
    ;;
    *)
    echo
    echo "No"
    echo
    ;;
esac

export CROSS_COMPILE=gcc-linaro-6.3.1-2017.02-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-
export ARCH=arm64
export PATH=~/bin:$PATH
export PATH=~/bin/skales:$PATH
cd ./kernel
./build.sh $CROSS_COMPILE $1
cd ..
./skales/dtbTool -o dt.img -s 2048 kernel/arch/arm64/boot/dts/qcom/
export cmdline="root=/dev/mmcblk0p10 rw rootwait console=tty0 console=ttyMSM0,115200n8 drm.debug=15 loglevel=10"
./skales/mkbootimg --kernel kernel/arch/arm64/boot/Image \
          --ramdisk ramdisk/initrd.img-4.9.56-linaro-lt-qcom \
          --output boot-sd410.img \
          --dt dt.img \
          --pagesize 2048 \
          --base 0x80000000 \
          --cmdline "$cmdline"
echo "Image: boot-sd410.img. Done!"

echo "Do you want to copy 'boot-sd410.img' to sdcard? [y/n]"

read -n1 ans
if [[ $ans = "y" ]]; then
    echo "Copying image to sdcard"
    sudo cp boot-sd410.img /media/$USER/OSes/Debian/
    sudo umount /media/$USER/OSes && sudo umount /media/$USER/rootfs
    rm dt.img
    echo Success
 
fi




