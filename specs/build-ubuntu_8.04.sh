#! /bin/sh
#
# This is kernel build script for ubuntu 8.04's 2.6.24 kernel.
#

die () {
    echo $1
    exit 1
}

#VERSION=`uname -r | cut -d - -f 1,2`
VERSION=`apt-cache search ^linux-image-2.6.24-..- | cut -b 13-21 | awk ' { print $1 }' | sort -r | uniq | head -n 1`
export CONCURRENCY_LEVEL=`grep -c '^processor' /proc/cpuinfo` || die "Can't export."

apt-get -y install wget

# Download TOMOYO Linux patches.
mkdir -p /usr/src/rpm/SOURCES/
cd /usr/src/rpm/SOURCES/ || die "Can't chdir to /usr/src/rpm/SOURCES/ ."
if [ ! -r ccs-patch-1.8.3-20120120.tar.gz ]
then
    wget -O ccs-patch-1.8.3-20120120.tar.gz 'http://sourceforge.jp/frs/redir.php?f=/tomoyo/49684/ccs-patch-1.8.3-20120120.tar.gz' || die "Can't download patch."
fi

# Install kernel source packages.
cd /usr/src/ || die "Can't chdir to /usr/src/ ."
apt-get -y install linux-kernel-devel fakeroot build-essential || die "Can't install packages."
apt-get build-dep linux-image-${VERSION}-generic || die "Can't install packages."
apt-get source linux-image-${VERSION}-generic || die "Can't install kernel source."
apt-get -y install linux-headers-${VERSION} || die "Can't install packages."
apt-get build-dep linux-ubuntu-modules-${VERSION}-generic || die "Can't install packages."
apt-get source linux-ubuntu-modules-${VERSION}-generic || die "Can't install kernel source."
apt-get build-dep linux-restricted-modules-${VERSION}-generic || die "Can't install packages."
apt-get source linux-restricted-modules-${VERSION}-generic || die "Can't install kernel source."
for i in `awk ' { if ( $1 != "Build-Depends:") next; $1 = ""; n = split($0, a, ","); for (i = 1; i <= n; i++) { split(a[i], b, " "); print b[1]; } } ' linux-2.6.24/debian/control`; do apt-get -y install $i; done

# Copy patches and create kernel config.
cd linux-2.6.24/ || die "Can't chdir to linux-2.6.24/ ."
mkdir -p debian/binary-custom.d/ccs/patchset || die "Can't create debian/binary-custom.d/ccs/patchset ."
mkdir -p ccs-patch/ || die "Can't create directory."
cd ccs-patch/ || die "Can't chdir to ccs-patch/ ."
tar -zxf /usr/src/rpm/SOURCES/ccs-patch-1.8.3-20120120.tar.gz || die "Can't extract patch."
cp -p patches/ccs-patch-2.6.24-ubuntu-8.04.diff ../debian/binary-custom.d/ccs/patchset/ubuntu-8.04.patch || die "Can't copy patch."
rm -fR specs/ patches/ || die "Can't delete directory."
for i in `find . -type f`; do diff -u /dev/null $i; done > ../debian/binary-custom.d/ccs/patchset/ccs.patch
cd ../ || die "Can't chdir to ../ ."
cd debian/binary-custom.d/ccs/ || die "Can't chdir to debian/binary-custom.d/ccs/ ."
cat ../../config/i386/config ../../config/i386/config.generic ../../../ccs-patch/config.ccs > config.i386 || die "Can't create config."
sed -i -e 's:CONFIG_DEBUG_INFO=.*:# CONFIG_DEBUG_INFO is not set:' -- config.i386 || die "Can't edit config."
touch rules || die "Can't create file."
cd ../../../ || die "Can't chdir to ../../../ ."
rm -fR ccs-patch || die "Can't delete directory."
awk ' BEGIN { flag = 0; print ""; } { if ($1 == "Package:" ) { if (index($0, "-generic") > 0) flag = 1; else flag = 0; }; if (flag) print $0; } ' debian/control.stub | sed -e 's:-generic:-ccs:' > debian/control.stub.ccs || die "Can't create file."
cat debian/control.stub.ccs >> debian/control.stub || die "Can't edit file."
debian/rules debian/control || die "Can't run control."

# Start compilation.
debian/rules custom-binary-ccs || die "Failed to build kernel package."
cd ../ || die "Can't chdir to ../ ."

# Install header package for compiling additional modules.
dpkg -i linux-headers-*-ccs*.deb || die "Can't install packages."
cd linux-ubuntu-modules-2.6.24-2.6.24 || die "Can't chdir to linux-ubuntu-modules-2.6.24-2.6.24 ."
awk ' BEGIN { flag = 0; print ""; } { if ($1 == "Package:" ) { if (index($0, "-generic") > 0) flag = 1; else flag = 0; }; if (flag) print $0; } ' debian/control.stub | sed -e 's:-generic:-ccs:' > debian/control.stub.ccs || die "Can't create file."
cat debian/control.stub.ccs >> debian/control.stub || die "Can't edit file."
debian/rules debian/control || die "Can't run control."
sed -i -e 's:virtual:virtual ccs:' debian/rules.d/i386.mk || die "Can't edit file."
debian/rules binary-indep binary-arch || die "Failed to build kernel package."
cd .. || die "Can't chdir to ../ ."

cd linux-restricted-modules-2.6.24-2.6.24.18/ || die "Can't chdir to linux-restricted-modules-2.6.24-2.6.24.18/ ."
awk ' BEGIN { flag = 0; print ""; } { if ( $1 == "Package:") { if ( index($2, "-generic") > 0) flag = 1; else flag = 0; }; if (flag) print $0; } ' debian/control.stub.in | sed -e 's:-generic:-ccs:g' > debian/control.stub.in.tmp || die "Can't create file."
cat debian/control.stub.in.tmp >> debian/control.stub.in || die "Can't edit file."
sed -i -e 's/,generic/,ccs generic/' debian/rules || die "Can't edit file."
debian/rules debian/control || die "Can't run control."
debian/rules binary || die "Failed to build kernel package."

# Generate meta packages.
cd /usr/src/
rm -fR linux-meta-*/
apt-get source linux-meta
cd linux-meta-*/
sed -e 's/generic/ccs/g' -- debian/control.d/generic > debian/ccs
rm -f debian/control.d/*
mv debian/ccs debian/control.d/ccs
debian/rules binary-arch
cd ../
rm -fR linux-meta-*/

exit 0
