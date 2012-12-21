#! /bin/sh
#
# This is a kernel build script for VineLinux 6.0's 2.6.35 kernel.
#

die () {
    echo $1
    exit 1
}

cd /tmp/ || die "Can't chdir to /tmp/ ."

if [ ! -r kernel-2.6.35-21vl6.src.rpm ]
then
    wget http://updates.vinelinux.org/Vine-6.0/updates/SRPMS/kernel-2.6.35-21vl6.src.rpm || die "Can't download source package."
fi
rpm --checksig kernel-2.6.35-21vl6.src.rpm || die "Can't verify signature."
rpm -ivh kernel-2.6.35-21vl6.src.rpm || die "Can't install source package."

cd /root/rpm/SOURCES/ || die "Can't chdir to /root/rpm/SOURCES/ ."
if [ ! -r ccs-patch-1.8.3-20120120.tar.gz ]
then
    wget -O ccs-patch-1.8.3-20120120.tar.gz 'http://sourceforge.jp/frs/redir.php?f=/tomoyo/49684/ccs-patch-1.8.3-20120120.tar.gz' || die "Can't download patch."
fi

cd /tmp/ || die "Can't chdir to /tmp/ ."
cp -p /root/rpm/SPECS/kernel-vl.spec . || die "Can't copy spec file."
patch << "EOF" || die "Can't patch spec file."
--- kernel-vl.spec
+++ kernel-vl.spec
@@ -28,7 +28,7 @@
 %define patchlevel 13
 %define kversion 2.6.%{sublevel}
 %define rpmversion 2.6.%{sublevel}
-%define release 21%{?_dist_release}
+%define release 21%{?_dist_release}_tomoyo_1.8.3p4
 
 %define make_target bzImage
 %define hdrarch %_target_cpu
@@ -120,6 +120,9 @@
 # to versions below the minimum
 #
 
+# TOMOYO Linux
+%define signmodules 0
+
 #
 # First the general kernel 2.6 required versions as per
 # Documentation/Changes
@@ -152,7 +155,7 @@
 #
 %define kernel_prereq  fileutils, module-init-tools >= 3.6, initscripts >= 8.80, mkinitrd >= 6.0.93, linux-firmware >= 20110601-1
 
-Name: kernel
+Name: ccs-kernel
 Group: System Environment/Kernel
 License: GPLv2
 Version: %{rpmversion}
@@ -651,6 +654,10 @@
 
 # END OF PATCH APPLICATIONS
 
+# TOMOYO Linux
+tar -zxf %_sourcedir/ccs-patch-1.8.3-20120120.tar.gz
+patch -sp1 < patches/ccs-patch-2.6.35-vine-linux-6.0.diff
+
 cp %{SOURCE10} Documentation/
 
 # put Vine logo
@@ -669,6 +676,9 @@
 for i in *.config
 do 
 	mv $i .config 
+	# TOMOYO Linux
+	cat config.ccs >> .config
+	sed -i -e "s/^CONFIG_DEBUG_INFO=.*/# CONFIG_DEBUG_INFO is not set/" -- .config
 	Arch=`head -1 .config | cut -b 3-`
 	echo "# $Arch" > configs/$i
 	cat .config >> configs/$i 
EOF
mv kernel-vl.spec ccs-kernel.spec || die "Can't rename spec file."
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
exec rpmbuild -bb --target $ARCH /tmp/ccs-kernel.spec
exit 0
