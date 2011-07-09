#!/bin/bash

# This script shall support the initial setup 
# of a BOINC server by assisting in the download
# of example applications from Debian.
# 
# This script downloads the binaries, puts them in a 
# direcotory structure as BOINC 
# wants them( see http://boinc.berkeley.edu/trac/wiki/AppVersionNew ), 
# and then signs them.
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

# Path to your projects key file and sign_executable binary.
keyfile="/home/dhananjay/projects/test/keys/code_sign_private"
signapp=/home/dhananjay/projects/test/bin/sign_executable

mirror="http://ftp.de.debian.org/debian"
version="6.12.33+dfsg-1"
shortver=$(echo $version|cut -d . -f1-2)

for arch in ${!deb2boinc[@]}
do

    if [ ! -r $arch.deb ]; then 
	url="$mirror/pool/main/b/boinc/boinc-app-examples_${version}_${arch}.deb"
	if ! wget -O - $url > ${arch}.deb ; then
	    echo "Platform '$arch' failed to download .... skipping."
	    rm -f ${arch}.deb
	    continue
	fi
    fi

    ar xvf ${arch}.deb data.tar.gz
    echo "Untaring"
    tar xzf data.tar.gz ./usr/lib/boinc-server/apps/
    echo "Contents:"
    ls ./usr/lib/boinc-server/apps/
    mkdir -p collection
    mv ./usr/lib/boinc-server/apps collection/$arch
    mv usr deleteThisDir
    rm -r deleteThisDir
    
    echo Reorganising and signing the application binaries.
    
    for f in collection/$arch/*
    do
    	appname=`echo $f|cut -d / -f 3`
	apppath=apps/$appname/$shortver/${deb2boinc[$arch]}/
	mkdir -p  $apppath
	mv $f $apppath
	${signapp} $apppath/$appname $keyfile >> $apppath/${appname}.sig
    done
    
    
done

echo cleaning up.

if [ -d collection ]
then
    rm -r collection
fi

