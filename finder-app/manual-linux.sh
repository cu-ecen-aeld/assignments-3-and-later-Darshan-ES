#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR="/home/darshanes/tm"
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-
# packages
PACKAGES=(
    flex
    bison
    build-essential
    libssl-dev
    bc
    u-boot-tools
    qemu
    cpio
    device-tree-compiler
)

# Update and install packages
sudo apt update && sudo apt install -y "${PACKAGES[@]}"


if [ $# -lt 1 ]; then
    echo "Using default directory ${OUTDIR} for output"
else
    OUTDIR=$1
    echo "Using passed directory ${OUTDIR} for output"
fi


if ! mkdir -p "${OUTDIR}"; then
    echo "Directory Not Created ${OUTDIR}"
    exit 1
fi

cd "$OUTDIR"

# Clone Linux Kernel
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    echo "Cloning Linux kernel version ${KERNEL_VERSION}"
    git clone ${KERNEL_REPO} --depth 1 --branch ${KERNEL_VERSION}
fi
#This 'nproc' was based on content at [https://www.geeksforgeeks.org/nproc-command-in-linux-with-examples/] with modifications #[ 'nproc ' is use for parallel compilation ].
# Build the Kernel
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    make -j$(nproc) ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
    make -j$(nproc) ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
    make -j$(nproc) ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
    cp -r arch/${ARCH}/boot/Image ${OUTDIR}/
    
fi

echo "Adding the Image to outdir"

# Prepare Root Filesystem
cd "$OUTDIR"
echo "Creating root filesystem"
sudo rm -rf ${OUTDIR}/rootfs
mkdir -p rootfs/{bin,dev,etc,home,lib,lib64,proc,sbin,sys,tmp,usr,var}
mkdir -p rootfs/usr/{bin,lib,sbin}
mkdir -p rootfs/var/log

# Clone and Build BusyBox
if [ ! -d "${OUTDIR}/busybox" ]; then
    git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    
else
    cd busybox
fi
make distclean
make defconfig
make -j$(nproc) ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

#This 'chmod' was based on content at [https://www.liquidweb.com/blog/how-do-i-set-up-setuid-setgid-and-sticky-bits-on-linux/] with modifications #[ 'Chmod ' is use to set UID for busybox ].
#set User ID for Busybox
sudo chmod u+s ${OUTDIR}/rootfs/bin/busybox

# Check Library Dependencies
echo "Library dependencies:"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "Shared library"

# Copy Libraries
SYSROOT=$(${CROSS_COMPILE}gcc --print-sysroot)
cp -a ${SYSROOT}/lib/*.so* ${OUTDIR}/rootfs/lib/
cp -a ${SYSROOT}/lib64/*.so* ${OUTDIR}/rootfs/lib64/

# Create Device Nodes
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/null c 1 3
sudo mknod -m 600 ${OUTDIR}/rootfs/dev/console c 5 1

# clean and build Finder App
cd ${FINDER_APP_DIR}
make clean
make CROSS_COMPILE=${CROSS_COMPILE}
cp writer ${OUTDIR}/rootfs/home/

# Copy files Finder App 
cp -r ${FINDER_APP_DIR}/finder-test.sh ${OUTDIR}/rootfs/home/
cp -r ${FINDER_APP_DIR}/finder.sh ${OUTDIR}/rootfs/home/
cp -r ${FINDER_APP_DIR}/autorun-qemu.sh ${OUTDIR}/rootfs/home/
cp -r ${FINDER_APP_DIR}/writer ${OUTDIR}/rootfs/home/
cp -r ${FINDER_APP_DIR}/conf/ ${OUTDIR}/rootfs/home/

#This 'sed -i' was based on content at [https://www.theunixschool.com/2012/03/sed-replace-or-substitute-file-contents.html] with modifications #[ 'sed -i' is used to replace the string in place without creating a new file ].
# Fix Configuration Path
sed -i 's|\.\./conf/assignment.txt|conf/assignment.txt|g' ${OUTDIR}/rootfs/home/finder-test.sh

#This 'chown' was based on content at [https://linuxkernal.com/posts/linux-chown-command-guide] with modifications #[ 'Chown' is use to change the ownership ].
# Change Ownership
sudo chown -R root:root ${OUTDIR}/rootfs/


# Create initramfs
cd ${OUTDIR}/rootfs
find . | cpio -H newc -ov --owner=root:root > ${OUTDIR}/initramfs.cpio
gzip -f ${OUTDIR}/initramfs.cpio
