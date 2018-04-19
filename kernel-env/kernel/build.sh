#!/bin/bash
export CROSS_COMPILE=../$1 


export ARCH=arm64
if [[ $2 = "full" ]]; then
	make mrproper
	make defconfig fet.config
fi
make KERNELRELEASE=4.4.23-linaro-lt-qcom Image dtbs  -j$[$(nproc) + 1]
#rm ./arch/arm64/boot/dts/qcom/apq8016-var-dart.dtb
#rm ./arch/arm64/boot/dts/qcom/apq8016-sbc.dtb
if [[ $2 = "full" ]]; then
	make KERNELRELEASE=4.4.23-linaro-lt-qcom modules  -j$[$(nproc) + 1] 
	make KERNELRELEASE=4.4.23-linaro-lt-qcom INSTALL_MOD_PATH=build-${ARCH}-modules modules_install -j$[$(nproc) + 1] 
	sudo find ./build-arm64-modules/ -type l -delete
	cd ./build-arm64-modules/
	pwd
fi

