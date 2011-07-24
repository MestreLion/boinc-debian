#!/bin/bash

# This script shall support the initial setup 
# of a BOINC server by assisting in the download
# of example applications from Debian.
#
# This script is released under the same license
# as BOINC, created and copyright by
# Steffen Moeller <moeller@debian.org>

set -e

declare -A deb2boinc
deb2boinc[armel]="armel-linux-gnu"
deb2boinc[alpha]="alpha-linux-gnu"
deb2boinc[amd64]="x86_64-pc-linux-gnu"
deb2boinc[i386]="i686-pc-linux-gnu"
deb2boinc[powerpc]="powerpc-linux-gnu"
deb2boinc[powerpc64]="ppc64-linux-gnu"
deb2boinc[ia64]="ia64-linux-gnu"
deb2boinc[sparc]="sparc-linux-gnu"
deb2boinc[sparc64]="sparc64-linux-gnu"
deb2boinc[mips]="mips-linux-gnu"
deb2boinc[s390]="s390-linux-gnu"

mirror="http://ftp.de.debian.org/debian"
version="6.12.33+dfsg-1"

projectroot=""

sign="$projectroot/bin/sign_executable"
key="$projectroot/keys/code_sign_private"

shortver=$(echo $version|cut -d . -f-2)

appsdir="apps"
downloaddir="collection"

if [ -d "$appsdir"]; then
	echo "Directory '$appsdir' is already existing. Please clean this up first."
	exit
fi

echo "I: Retrieving application for all of Debian's architectures."

for arch in ${!deb2boinc[@]}
do

  if [ -d $downloaddir/$arch ]; then
    echo "W: Destination directory '$downloaddir/$arch' already exiting ... skipping."
    continue
  fi

  if [ ! -r $arch.deb ]; then 
    url="$mirror/pool/main/b/boinc/boinc-app-examples_${version}_${arch}.deb"
    if ! wget --quiet -O - $url > ${arch}.deb ; then
       echo "E: Platform '$arch' failed to download .... skipping."
       rm -f ${arch}.deb
       continue
    fi
  fi

  ar xvf ${arch}.deb data.tar.gz
  echo "I: Untaring for architecture ${arch}"
  tar xzf data.tar.gz ./usr/lib/boinc-server/apps/
  echo -n "I: Contents:"
  ls ./usr/lib/boinc-server/apps/
  mkdir -p $downloaddir
  mv ./usr/lib/boinc-server/apps $downloaddir/$arch
  mv usr deleteThisDir
  rm -rf deleteThisDir data.tar.gz ${arch}.deb
done

if [ -d "$appsdir" ]
then
    echo "E: App directory '$appsdir' already exists, exiting!!"
    exit
fi

echo "Creating directories for all applications in folder '$downloaddir' now in folder '$appsdir'."
for f in `find collection -type f | xargs -r -l basename| sort -u`
do
    appname=`echo $f|cut -d / -f3`
    mkdir -p $appsdir/$appname
done

for app in $appsdir/*
do 
    appname=$(basename $app)
    echo "I: Copying files for application '$appname'"
    echo -n "  "
    for folder in $downloaddir/*
    do 
	archname=$(echo $folder|cut -d / -f2)
        echo -n " $archname"
	#echo Copying $folder/$appname $appsdir/$appname/${appname}_${shortver}_${deb2boinc[$archname]}
	cp $folder/$appname $appsdir/$appname/${appname}_${shortver}_${deb2boinc[$archname]}
    done
    echo
done

if [ ! -x "$sign" ]; then
	echo "I: You need to sign the applictions, still. This is to be performed specifically for your project."
	cat <<EOINSTRUCTIONS
   Follow the following scheme:
     sign="\$projectroot/bin/sign_executable"
     key="\$projectroot/keys/code_sign_private"
     for binary in $appsdir/*/*
     do
        \$sign \$binary \$key > ${binary}.sig
     done
EOINSTRUCTIONS
else
	for binary in $appsdir/*/*
	do
	    echo Signing $binary
	    $sign $binary $key > ${binary}.sig
	done
fi

