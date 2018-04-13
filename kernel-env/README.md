# NewHorusLinuxKernel

## Requirements(Ubuntu - 16.04)
### 1 - Build repo


$ sudo apt-get update

$ sudo apt-get install curl git <br/>
$ mkdir /home/$USER/bin  <br/>
$ export PATH=/home/$USER/bin:$PATH  <br/>
$ curl https://storage.googleapis.com/git-repo-downloads/repo > /home/$USER/bin/repo  <br/>
$ chmod a+x /home/$USER/bin/repo  <br/>
$ cd /home/$USER/bin  <br/>
$ mkdir skales  <br/>
$ git clone git://codeaurora.org/quic/kernel/skales skales  <br/>
$ export PATH=/home/$USER/bin/skales:$PATH  <br/>


### 2 - Installing Device Tree Compiler


$ sudo apt-get install device-tree-compiler libfdt-dev  <br/>

### 3 - Setting git user

$ git config --global user.email "you@example.com"  <br/>
$ git config --global user.name "Your Name"  <br/>

### 4 - Installing fastboot and ADB

$ sudo add-apt-repository ppa:nilarimogard/webupd8  <br/>
$ sudo apt-get update  <br/>
$ sudo apt-get install android-tools-adb android-tools-fastboot  <br/>


### 5 - Setting up environment


$ mkdir /home/$USER/sd410 <br/>
$ cd /home/$USER/sd410 <br/><br/>

$ mkdir Debian <br/>
$ cd Debian <br/>

$ git clone https://github.com/institutoorion/NewHorusLinuxKernel.git kernel-env<br/><br/>

$ cd kernel-env<br/><br/>

$ wget ftp://dart-sd410:varSD410@ftp.variscite.com/Software/Debian/gcc-linaro-6.3.1-2017.02-x86_64_aarch64-linux-gnu.tar.xz <br/><br/>
$ tar -xvf gcc-linaro*.tar.xz <br/>

## makebootimg.sh usage:
### Plug your sdcard and run:

$ chmod u+x makebootimg.sh  <br/>
#### Full kernel compilation and modules
$./makebootimg.sh full  <br/>
#### Only dts
$./makebootimg.sh  <br/>
