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
cd ./kernel
./build.sh $CROSS_COMPILE $1
cd ..
./skales/dtbTool -o dt.img -s 2048 kernel/arch/arm64/boot/dts/qcom/
./skales/mkbootimg --kernel kernel/arch/arm64/boot/Image \
                   --ramdisk ramdisk/initrd.img-4.4.23-linaro-lt-qcom \
                   --output boot-sd410.img \
                   --dt dt.img \
                   --pagesize 2048 \
                   --base 0x80000000 \
                   --cmdline "root=/dev/mmcblk0p10 rw rootwait console=ttyMSM0,115200n8"
echo "Image: boot-sd410.img. Done!"

echo "Do you want to flash 'boot-sd410.img'? [y/n]"

read -n1 ans
if [[ $ans = "y" ]]; then
    echo "Copying image to sdcard"
    sudo fastboot flash boot boot-sd410.img
    echo Success
 
fi




