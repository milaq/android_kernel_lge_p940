#! /bin/sh
#
# This is kernel build script for ubuntu 10.10's 2.6.35 kernel.
#

die () {
    echo $1
    exit 1
}

ORIGINAL_FLAVOUR=`uname -r | cut -d - -f 3-` # e.g. generic generic-pae server
NEW_FLAVOUR=${ORIGINAL_FLAVOUR}-ccs
echo "Building "${NEW_FLAVOUR}" from "${ORIGINAL_FLAVOUR}"."

export CONCURRENCY_LEVEL=`grep -c '^processor' /proc/cpuinfo` || die "Can't export."

apt-get -y install wget

# Download TOMOYO Linux patches.
mkdir -p /root/rpmbuild/SOURCES/
cd /root/rpmbuild/SOURCES/ || die "Can't chdir to /root/rpmbuild/SOURCES/ ."
if [ ! -r ccs-patch-1.8.3-20120120.tar.gz ]
then
    wget -O ccs-patch-1.8.3-20120120.tar.gz 'http://sourceforge.jp/frs/redir.php?f=/tomoyo/49684/ccs-patch-1.8.3-20120120.tar.gz' || die "Can't download patch."
fi

# Install kernel source packages.
cd /usr/src/ || die "Can't chdir to /usr/src/ ."
apt-get -y install fakeroot build-essential || die "Can't install packages."
apt-get build-dep linux || die "Can't install packages."
apt-get source linux-source-2.6.35 || die "Can't install kernel source."
for i in `awk ' { if ( $1 != "Build-Depends:") next; $1 = ""; n = split($0, a, ","); for (i = 1; i <= n; i++) { split(a[i], b, " "); print b[1]; } } ' linux-2.6.35/debian/control`; do apt-get -y install $i; done

# Apply patches and create kernel config.
cd linux-2.6.35/ || die "Can't chdir to linux-2.6.35/ ."
tar -zxf /root/rpmbuild/SOURCES/ccs-patch-1.8.3-20120120.tar.gz || die "Can't extract patch."
patch -p1 < patches/ccs-patch-2.6.35-ubuntu-10.10.diff || die "Can't apply patch."
rm -fR patches/ specs/ || die "Can't delete patch."
for i in `find debian.master/ -type f -name '*'${ORIGINAL_FLAVOUR}'*'`; do cp -p $i `echo $i | sed -e 's/'${ORIGINAL_FLAVOUR}'/'${NEW_FLAVOUR}'/g'`; done
for i in debian.master/config/*/config.common.*; do cat config.ccs >> $i; done
rm debian.master/control.stub || die "Can't delete control.stub."
make -f debian/rules debian.master/control.stub || die "Can't update control.stub."
rm debian/control || die "Can't delete control."
debian/rules debian/control || die "Can't update control."

# Make modified header files go into local header package.
patch -p0 << "EOF" || die "Can't patch link-headers."
--- debian/scripts/link-headers
+++ debian/scripts/link-headers
@@ -39,4 +39,19 @@
 done
 )
 
+if [ $flavour == "NEW_FLAVOUR" ]
+then
+    cd $hdrdir/../../../../$symdir/usr/src/$symdir/include/linux/
+    for i in sched.h init_task.h security.h ccsecurity.h
+    do
+	rm -f $hdrdir/include/linux/$i
+	cp -p $i $hdrdir/include/linux/$i
+    done
+    rm -f $hdrdir/include/net $hdrdir/security
+    cd ../
+    tar -cf - net | tar -xf - -C $hdrdir/include/
+    cd ../
+    tar -cf - security | tar -xf - -C $hdrdir
+fi
+
 exit
EOF
sed -i -e 's/NEW_FLAVOUR/'${NEW_FLAVOUR}'/' debian/scripts/link-headers || die "Can't patch link-headers."

# Start compilation.
debian/rules binary-headers || die "Failed to build kernel package."
debian/rules binary-debs flavours=${NEW_FLAVOUR} || die "Failed to build kernel package."

# Generate meta packages.
cd /usr/src/
rm -fR linux-meta-*/
apt-get source linux-meta
cd linux-meta-*/
sed -e 's/'${ORIGINAL_FLAVOUR}'/'${NEW_FLAVOUR}'/g' -- debian/control.d/${ORIGINAL_FLAVOUR} > debian/${NEW_FLAVOUR}
rm -f debian/control.d/*
mv debian/${NEW_FLAVOUR} debian/control.d/${NEW_FLAVOUR}
debian/rules binary-arch
cd ../
rm -fR linux-meta-*/

exit 0
