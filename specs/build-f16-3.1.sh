#! /bin/sh
#
# This is a kernel build script for Fedora 16's 3.1.0 kernel.
#

die () {
    echo $1
    exit 1
}

yum -y install wget rpm-build make gcc redhat-rpm-config xmlto asciidoc gnupg elfutils-devel zlib-devel binutils-devel newt-devel python-devel perl-ExtUtils-Embed pciutils-devel

cd /tmp/ || die "Can't chdir to /tmp/ ."

if [ ! -r kernel-3.1.9-1.fc16.src.rpm ]
then
    wget http://ftp.riken.jp/Linux/fedora/updates/16/SRPMS/kernel-3.1.9-1.fc16.src.rpm || die "Can't download source package."
fi
rpm --checksig kernel-3.1.9-1.fc16.src.rpm || die "Can't verify signature."
rpm -ivh kernel-3.1.9-1.fc16.src.rpm || die "Can't install source package."

cd /root/rpmbuild/SOURCES/ || die "Can't chdir to /root/rpmbuild/SOURCES/ ."
if [ ! -r ccs-patch-1.8.3-20120120.tar.gz ]
then
    wget -O ccs-patch-1.8.3-20120120.tar.gz 'http://sourceforge.jp/frs/redir.php?f=/tomoyo/49684/ccs-patch-1.8.3-20120120.tar.gz' || die "Can't download patch."
fi

cd /root/rpmbuild/SPECS/ || die "Can't chdir to /root/rpmbuild/SPECS/ ."
cp -p kernel.spec ccs-kernel.spec || die "Can't copy spec file."
patch << "EOF" || die "Can't patch spec file."
--- ccs-kernel.spec
+++ ccs-kernel.spec
@@ -23,7 +23,7 @@
 #
 # (Uncomment the '#' and both spaces below to set the buildid.)
 #
-# % define buildid .local
+%define buildid _tomoyo_1.8.3p4
 ###################################################################
 
 # The buildid can also be specified on the rpmbuild command line
@@ -488,6 +488,11 @@
 # to versions below the minimum
 #
 
+# TOMOYO Linux
+%define with_modsign 0
+%define _enable_debug_packages 0
+%define with_debuginfo 0
+
 #
 # First the general kernel 2.6 required versions as per
 # Documentation/Changes
@@ -547,7 +552,7 @@
 AutoProv: yes\
 %{nil}
 
-Name: kernel%{?variant}
+Name: ccs-kernel%{?variant}
 Group: System Environment/Kernel
 License: GPLv2
 URL: http://www.kernel.org/
@@ -1014,7 +1019,7 @@
 AutoReqProv: no\
 Requires(pre): /usr/bin/find\
 Requires: perl\
-%description -n kernel%{?variant}%{?1:-%{1}}-devel\
+%description -n ccs-kernel%{?variant}%{?1:-%{1}}-devel\
 This package provides kernel headers and makefiles sufficient to build modules\
 against the %{?2:%{2} }kernel package.\
 %{nil}
@@ -1614,6 +1619,10 @@
 
 # END OF PATCH APPLICATIONS
 
+# TOMOYO Linux
+tar -zxf %_sourcedir/ccs-patch-1.8.3-20120120.tar.gz
+patch -sp1 < patches/ccs-patch-3.1.0-fedora-16.diff
+
 %endif
 
 # Any further pre-build tree manipulations happen here.
@@ -1643,6 +1652,9 @@
 for i in *.config
 do
   mv $i .config
+  # TOMOYO Linux
+  cat config.ccs >> .config
+  sed -i -e 's:CONFIG_DEBUG_INFO=.*:# CONFIG_DEBUG_INFO is not set:' -- .config
   Arch=`head -1 .config | cut -b 3-`
   make ARCH=$Arch listnewconfig | grep -E '^CONFIG_' >.newoptions || true
 %if %{listnewconfig_fail}
EOF
echo ""
echo ""
echo ""
echo "Edit /root/rpmbuild/SPECS/ccs-kernel.spec if needed, and run"
echo "rpmbuild -bb /root/rpmbuild/SPECS/ccs-kernel.spec"
echo "to build kernel rpm packages."
echo ""
ARCH=`uname -m`
echo "I'll start 'rpmbuild -bb --target $ARCH --without debug --without debuginfo /root/rpmbuild/SPECS/ccs-kernel.spec' in 30 seconds. Press Ctrl-C to stop."
sleep 30
exec rpmbuild -bb --target $ARCH --without debug --without debuginfo /root/rpmbuild/SPECS/ccs-kernel.spec
exit 0
