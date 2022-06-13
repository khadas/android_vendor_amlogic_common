#!/bin/sh
############################################################
# File : bridge-u16.sh
#
# This script makes labtool linux build.
#
# It takes the release platform etc. as an input parameters.
#

printUsage() {
    echo ""
    echo "Please go to root directory."
    echo ""
    echo "Usage: bridge-u16 <version>"
    echo ""
}

############################################################
# Main
#

echo kernel=`uname -r`
echo kver=$(echo $kernel | cut -c1-3)
echo  
#if [ ! $kver = "3.1" ]; then
#    echo "Please make the release on Fedora 3.1"
#    printUsage
#    exit 1
#fi

MfgToolVer=$1


BridgeVer=$(echo $(grep -r "#define BRIDGE_VERSION" mfgbridge.c) | cut -d "\"" -f 2)
echo $BridgeVer

RelLabel=Bridge_app_$BridgeVer
BridgeLabel=Bridge_$BridgeVer
RootDir=`pwd`
TargetDir=$RootDir/$RelLabel
RelDir=$RootDir/release/$RelLabel
echo TargetDir $TargetDir
echo ReleaseDir $RelDir
echo RootDir $RootDir

############################################################
echo # Copy RELEASENOTES to the release directory
#
#rm -rf $RelDir
#mkdir -p release
#mkdir $RelDir
############################################################
echo # Copy MFG tools source code and tar to release directory
#
#rm -rf $TargetDir
#mkdir $TargetDir
#mkdir $TargetDir/bridge
#cp -r *.* $TargetDir/bridge/
#tar cvzf $RelDir/$BridgeLabel-src.tgz $TargetDir
#cp -r bridge $TargetDir

SRCDIR=src_mfgbridge
rm -r-f $SRCDIR
mkdir -p $SRCDIR/bridge

cp *.c $SRCDIR/bridge
cp *.h $SRCDIR/bridge
cp *.mk $SRCDIR/bridge
cp *.sh $SRCDIR/bridge
cp bridge_init.conf $SRCDIR/bridge
cp Makefile $SRCDIR/bridge

mkdir -p $SRCDIR/drvwrapper
cp ../drvwrapper/*.c $SRCDIR/drvwrapper
cp ../drvwrapper/*.h $SRCDIR/drvwrapper

rm -r-f $BridgeLabel-src.tgz
tar cvzf $BridgeLabel-src.tgz src_mfgbridge



############################################################
# Make the build
#
make clean
make all

BINDIR=bin_mfgbridge
mkdir -p $BINDIR
cp mfgbridge $BINDIR
file -b *.c
file -b bridge_init.conf
dos2unix bridge_init.conf
cp bridge_init.conf $BINDIR
#make clean

############################################################
echo # Tar the binary to release directory
#
cd $RootDir
rm -r -f $BridgeLabel-bin.tgz
tar cvzf $BridgeLabel-bin.tgz bin_mfgbridge

rm -r -f release
mkdir release
cp $BridgeLabel-src.tgz \release
cp $BridgeLabel-bin.tgz \release
rm $BridgeLabel-src.tgz
rm $BridgeLabel-bin.tgz

rm mfgbridge.o
rm mfgdebug.o
rm -r -f bin_mfgbridge
rm -r -f src_mfgbridge

#echo $RelDir 
#ls $RelDir 
#echo $RelLabel/bin_labtool
#echo cp -r  $RelLabel/bin_labtool $RelDir 
#tar cvzf $RelDir/$LabtoolLabel-bin.tgz $RelLabel/bin_labtool


############################################################
echo # Remove target directory 
#
#rm -rf $TargetDir

############################################################
echo "DONE"
echo "Release is available in release/ directory"
echo ""
