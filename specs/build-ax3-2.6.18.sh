#! /bin/sh
#
# This is a kernel build script for Asianux 3's 2.6.18 kernel.
#

die () {
    echo $1
    exit 1
}

cd /tmp/ || die "Can't chdir to /tmp/ ."

if [ ! -r kernel-2.6.18-274.3.AXS3.src.rpm ]
then
    wget http://ftp.miraclelinux.com/pub/Asianux/Server/3.0/updates/src/kernel-2.6.18-274.3.AXS3.src.rpm || die "Can't download source package."
fi
rpm --checksig kernel-2.6.18-274.3.AXS3.src.rpm || die "Can't verify signature."
rpm -ivh kernel-2.6.18-274.3.AXS3.src.rpm || die "Can't install source package."

cd /usr/src/asianux/SOURCES/ || die "Can't chdir to /usr/src/asianux/SOURCES/ ."
if [ ! -r ccs-patch-1.8.3-20120120.tar.gz ]
then
    wget -O ccs-patch-1.8.3-20120120.tar.gz 'http://sourceforge.jp/frs/redir.php?f=/tomoyo/49684/ccs-patch-1.8.3-20120120.tar.gz' || die "Can't download patch."
fi

cd /tmp/ || die "Can't chdir to /tmp/ ."
cp -p /usr/src/asianux/SPECS/kernel.spec . || die "Can't copy spec file."
patch << "EOF" || die "Can't patch spec file."
--- kernel.spec
+++ kernel.spec
@@ -68,7 +68,7 @@
 %define kversion 2.6.%{sublevel}.%{stablerev}
 %define rpmversion 2.6.%{sublevel}
 # %dist is defined in Asianux VPBS
-%define release %{revision}.3%{dist}
+%define release %{revision}.3%{dist}_tomoyo_1.8.3p4
 %define signmodules 0
 %define xen_hv_cset 15502
 %define xen_abi_ver 3.1
@@ -277,6 +277,9 @@
 # to versions below the minimum
 #
 
+# TOMOYO Linux
+%define signmodules 0
+
 #
 # First the general kernel 2.6 required versions as per
 # Documentation/Changes
@@ -307,7 +310,7 @@
 #
 %define kernel_prereq  fileutils, module-init-tools, initscripts >= 8.11.1-1, mkinitrd >= 4.2.21-1
 
-Name: kernel
+Name: ccs-kernel
 Group: System Environment/Kernel
 License: GPLv2
 URL: http://www.kernel.org/
@@ -1045,6 +1048,10 @@
 %patch200100 -p1
 # end of Asianux patches
 
+# TOMOYO Linux
+tar -zxf %_sourcedir/ccs-patch-1.8.3-20120120.tar.gz
+patch -sp1 < patches/ccs-patch-2.6.18-asianux-3.diff
+
 cp %{SOURCE10} Documentation/
 
 mkdir configs
@@ -1096,6 +1103,9 @@
 for i in `ls *86*.config`
 do
   mv $i .config
+  # TOMOYO Linux
+  cat config.ccs >> .config
+  sed -i -e "s/^CONFIG_DEBUG_INFO=.*/# CONFIG_DEBUG_INFO is not set/" -- .config 
   Arch=`head -1 .config | cut -b 3-`
   make ARCH=$Arch nonint_oldconfig > /dev/null
   echo "# $Arch" > configs/$i
EOF
mv kernel.spec ccs-kernel.spec || die "Can't rename spec file."
echo ""
echo ""
echo ""
echo "Edit /tmp/ccs-kernel.spec if needed, and run"
echo "rpmbuild -bb /tmp/ccs-kernel.spec"
echo "to build kernel rpm packages."
echo ""
ARCH=`uname -m`
echo "I'll start 'rpmbuild -bb --target $ARCH --with baseonly --without debug --without debuginfo --without source /tmp/ccs-kernel.spec' in 30 seconds. Press Ctrl-C to stop."
sleep 30
exec rpmbuild -bb --target $ARCH --with baseonly --without debug --without debuginfo --without source /tmp/ccs-kernel.spec
exit 0
