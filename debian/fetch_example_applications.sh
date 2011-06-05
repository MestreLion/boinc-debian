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
version="6.12.28+dfsg-2"

for arch in ${!deb2boinc[@]}
do

  if [ -d collection/$arch ]; then
    echo "Destination directory 'collection/arch' already exiting ... skipping."
    continue
  fi

  if [ ! -r $arch.deb ]; then 
    url="$mirror/pool/main/b/boinc/boinc-server-maker_${version}_${arch}.deb"
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

  for f in collection/$arch/*
  do
    echo renaming $f to ${f}_${deb2boinc[$arch]}
    mv $f ${f}_${deb2boinc[$arch]}
  done

done

