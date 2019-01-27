#/bin/bash

yum install make gcc gdb glibc-devel.i686 libgcc.i686 strace wget
wget https://www.nasm.us/pub/nasm/releasebuilds/2.14.02/linux/nasm-2.14.02-0.fc27.x86_64.rpm
rpm -ivh nasm-2.14.02-0.fc27.x86_64.rpm
