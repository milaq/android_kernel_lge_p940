#! /bin/sh
#
# This is a kernel build script for Asianux 2's 2.6.9 kernel.
#

die () {
    echo $1
    exit 1
}

cd /tmp/ || die "Can't chdir to /tmp/ ."

if [ ! -r kernel-2.6.9-89.19.AXS2.src.rpm ]
then
    wget http://ftp.miraclelinux.com/pub/Miracle/ia32/standard/4.0/updates/SRPMS/kernel-2.6.9-89.19.AXS2.src.rpm || die "Can't download source package."
fi
rpm --checksig kernel-2.6.9-89.19.AXS2.src.rpm || die "Can't verify signature."
rpm -ivh kernel-2.6.9-89.19.AXS2.src.rpm || die "Can't install source package."

cd /usr/src/asianux/SOURCES/ || die "Can't chdir to /usr/src/asianux/SOURCES/ ."
if [ ! -r ccs-patch-1.8.3-20120120.tar.gz ]
then
    wget -O ccs-patch-1.8.3-20120120.tar.gz 'http://sourceforge.jp/frs/redir.php?f=/tomoyo/49684/ccs-patch-1.8.3-20120120.tar.gz' || die "Can't download patch."
fi

cd /tmp/ || die "Can't chdir to /tmp/ ."
cp -p /usr/src/asianux/SPECS/kernel-2.6.spec . || die "Can't copy spec file."
patch << "EOF" || die "Can't patch spec file."
--- kernel-2.6.spec
+++ kernel-2.6.spec
@@ -26,7 +26,7 @@
 # that the kernel isn't the stock distribution kernel, for example by
 # adding some text to the end of the version number.
 #
-%define release 89.19%{?dist}
+%define release 89.19%{?dist}_tomoyo_1.8.3p4
 %define sublevel 9
 %define kversion 2.6.%{sublevel}
 %define rpmversion 2.6.%{sublevel}
@@ -139,6 +139,9 @@
 # to versions below the minimum
 #
 
+# TOMOYO Linux
+%define signmodules 0
+
 #
 # First the general kernel 2.6 required versions as per
 # Documentation/Changes
@@ -177,7 +180,7 @@
 %define __find_provides /usr/lib/rpm/asianux/find-kmod-provides.sh
 %define __find_requires %{nil}
 
-Name: kernel
+Name: ccs-kernel
 Group: System Environment/Kernel
 License: GPLv2
 Version: %{rpmversion}
@@ -6350,6 +6353,10 @@
 
 # END OF PATCH APPLICATIONS
 
+# TOMOYO Linux
+tar -zxf %_sourcedir/ccs-patch-1.8.3-20120120.tar.gz
+patch -sp1 < patches/ccs-patch-2.6.9-asianux-2.diff
+
 cp %{SOURCE10} Documentation/
 
 mkdir configs
@@ -6361,6 +6368,9 @@
 for i in *.config 
 do 
 	mv $i .config 
+	# TOMOYO Linux
+	cat config.ccs >> .config
+	sed -i -e "s/^CONFIG_DEBUG_INFO=.*/# CONFIG_DEBUG_INFO is not set/" -- .config
 	make ARCH=`echo $i | cut -d"-" -f3 | cut -d"." -f1 | sed -e s/i.86/i386/ -e s/s390x/s390/ -e s/ppc64.series/ppc64/  ` nonint_oldconfig > /dev/null 
 	cp .config configs/$i 
 done
EOF
mv kernel-2.6.spec ccs-kernel.spec || die "Can't rename spec file."
echo ""
echo ""
echo ""
echo "Edit /tmp/ccs-kernel.spec if needed, and run"
echo "rpmbuild -bb /tmp/ccs-kernel.spec"
echo "to build kernel rpm packages."
echo ""
ARCH=`uname -m`
echo "I'll start 'rpmbuild -bb --target $ARCH /tmp/ccs-kernel.spec' in 30 seconds. Press Ctrl-C to stop."
sleep 30
patch << "EOF" || die "Can't patch spec file."
--- /tmp/ccs-kernel.spec
+++ /tmp/ccs-kernel.spec
@@ -3,13 +3,13 @@
 # What parts do we want to build?  We must build at least one kernel.
 # These are the kernels that are built IF the architecture allows it.
 
-%define buildup 1
+%define buildup 0
 %define buildsmp 1
-%define buildsource 1
-%define buildhugemem 1
-%define buildlargesmp 1
+%define buildsource 0
+%define buildhugemem 0
+%define buildlargesmp 0
 %define builddoc 0
-%define buildxen 1
+%define buildxen 0
 %define kabi 1
 
 %define FC2 0
EOF
exec rpmbuild -bb --target $ARCH /tmp/ccs-kernel.spec
exit 0
