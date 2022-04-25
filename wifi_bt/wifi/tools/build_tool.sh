#!/bin/bash

set -e

readonly CUR_PATH="$(readlink -f $(dirname $0))"

KERNEL_VERSION_LIST=(
"5.4"
"4.9"
)

KERNEL_ARCH_LIST=(
"arm64"
"arm"
)

WIFI_DRIVER_LIST=()

WIFI_MODULE_LIST=()

PRODUCT_LIST=()

function get_android_root()
{
	local TopFile=build/core/envsetup.mk
	local HERE=`pwd`
	local T=$HERE
	while [[ ( ! ( -f $TopFile ) ) && ( `pwd` != "/" ) ]]; do
		cd ..
		T=`pwd`
	done
	cd $HERE
	if [[ -f $T/$TopFile ]]; then
		echo $T
	else
		echo ""
	fi
}

function process_exit()
{
  local exit_code=$1
  local msg=$2

  if [[ $exit_code -eq 0 ]]; then
    if [[ x${msg} == x ]]; then
      msg="done!"
    fi
    echo -e "\n\e[32m"${msg}"\e[0m\n"
  else
    if [[ x${msg} == x ]]; then
      msg="failed!"
    fi
    echo -e "\n\e[31m"${msg}"\e[0m\n"
  fi
  kill -SIGINT $$
}

cd $CUR_PATH
readonly ROOT_DIR=$(get_android_root)
if [[ x$ROOT_DIR == x ]]; then
  process_exit -1 "Couldn't locate the android root directory!"
fi

readonly MAKE_DIR=${ROOT_DIR}/vendor/amlogic/common/wifi_bt/wifi/tools

function setup_env_for_kernel_4_9()
{
  if [[ $KERNEL_ARCH == "arm" ]]; then
    process_exit -1 "Currently, 32-bit kernel 4.9 compilation is not supported!"
  elif [[ $KERNEL_ARCH == "arm64" ]]; then
    ARCH=arm64
    BRANCH=android-4.9
    CROSS_COMPILE=aarch64-linux-android-
    DEFCONFIG=meson64_defconfig
    CLANG_TRIPLE=aarch64-linux-gnu-
    KERNEL_DIR=vendor/amlogic/common/kernel/common
    LINUX_GCC_CROSS_COMPILE_PREBUILTS_BIN=vendor/amlogic/common/kernel/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin
  fi
  INSTALL_MOD_STRIP=1
  export COMMON_OUT_DIR=$(readlink -m ${OUT_DIR:-${ROOT_DIR}/out/${BRANCH}})
  export OUT_DIR=$(readlink -m ${COMMON_OUT_DIR}/${KERNEL_DIR})
  export DIST_DIR=$(readlink -m ${DIST_DIR:-${COMMON_OUT_DIR}/dist})
  PREBUILTS_PATHS="
    LINUX_GCC_CROSS_COMPILE_PREBUILTS_BIN
    LINUX_GCC_CROSS_COMPILE_ARM32_PREBUILTS_BIN
    CLANG_PREBUILT_BIN
    LZ4_PREBUILTS_BIN
    DTC_PREBUILTS_BIN
    LIBUFDT_PREBUILTS_BIN
  "
  for PREBUILT_BIN in ${PREBUILTS_PATHS}; do
    PREBUILT_BIN=\${${PREBUILT_BIN}}
    eval PREBUILT_BIN="${PREBUILT_BIN}"
    if [ -n "${PREBUILT_BIN}" ]; then
      PATH=${PATH//"${ROOT_DIR}/${PREBUILT_BIN}:"}
      PATH=${ROOT_DIR}/${PREBUILT_BIN}:${PATH}
    fi
  done
  export PATH
  export CROSS_COMPILE ARCH
  export MODULES_STAGING_DIR=$(readlink -m ${COMMON_OUT_DIR}/staging)
  export MODULES_PRIVATE_DIR=$(readlink -m ${COMMON_OUT_DIR}/private)
  export UNSTRIPPED_DIR=${DIST_DIR}/unstripped
  export KERNEL_UAPI_HEADERS_DIR=$(readlink -m ${COMMON_OUT_DIR}/kernel_uapi_headers)
  KERNEL_SRC=${ROOT_DIR}/${KERNEL_DIR}
  O=${OUT_DIR}
  INSTALL_MOD_PATH=${MODULES_STAGING_DIR}
}

function build_wifi_driver_for_kernel_4_9()
{
  setup_env_for_kernel_4_9 $@
  set -x
  make -C ${MAKE_DIR} \
    KERNEL_SRC=${KERNEL_SRC} O=${OUT_DIR} CONFIG_WIFI_BUILD_DRIVER=$WIFI_DRIVER CONFIG_K_VERSION=$KERNEL_VERSION modules
  set +x

  set -x
  make -C ${MAKE_DIR} \
    KERNEL_SRC=${KERNEL_SRC} O=${OUT_DIR} INSTALL_MOD_STRIP=${INSTALL_MOD_STRIP} INSTALL_MOD_PATH=${INSTALL_MOD_PATH} \
    CONFIG_WIFI_BUILD_DRIVER=$WIFI_DRIVER CONFIG_K_VERSION=$KERNEL_VERSION modules_install
  set +x
}

function setup_env_for_kernel_5_4()
{
  . $ROOT_DIR/common/build.config.common
  if [[ $KERNEL_ARCH == "arm" ]]; then
    . $ROOT_DIR/common/build.config.arm
  elif [[ $KERNEL_ARCH == "arm64" ]]; then
    . $ROOT_DIR/common/build.config.aarch64
  fi
  . $ROOT_DIR/device/khadas/common/kernelbuild/build.config.common
  export COMMON_OUT_DIR=$(readlink -m ${OUT_DIR:-${ROOT_DIR}/out${OUT_DIR_SUFFIX}/${BRANCH}})
  export OUT_DIR=$(readlink -m ${COMMON_OUT_DIR}/${KERNEL_DIR})
  export DIST_DIR=$(readlink -m ${DIST_DIR:-${COMMON_OUT_DIR}/dist})
  export CLANG_TRIPLE CROSS_COMPILE ARCH
  export MODULES_STAGING_DIR=$(readlink -m ${COMMON_OUT_DIR}/staging)
  export MODULES_PRIVATE_DIR=$(readlink -m ${COMMON_OUT_DIR}/private)
  export UNSTRIPPED_DIR=${DIST_DIR}/unstripped
  export KERNEL_UAPI_HEADERS_DIR=$(readlink -m ${COMMON_OUT_DIR}/kernel_uapi_headers)
  export INITRAMFS_STAGING_DIR=${MODULES_STAGING_DIR}/initramfs_staging
  PREBUILTS_PATHS=(
    LINUX_GCC_CROSS_COMPILE_PREBUILTS_BIN
    LINUX_GCC_CROSS_COMPILE_ARM32_PREBUILTS_BIN
    CLANG_PREBUILT_BIN
    LZ4_PREBUILTS_BIN
    DTC_PREBUILTS_BIN
    LIBUFDT_PREBUILTS_BIN
    BUILDTOOLS_PREBUILT_BIN
  )
  for PREBUILT_BIN in "${PREBUILTS_PATHS[@]}"; do
    PREBUILT_BIN=\${${PREBUILT_BIN}}
    eval PREBUILT_BIN="${PREBUILT_BIN}"
    if [ -n "${PREBUILT_BIN}" ]; then
      PATH=${PATH//"${ROOT_DIR}\/${PREBUILT_BIN}:"}
      PATH=${ROOT_DIR}/${PREBUILT_BIN}:${PATH}
    fi
  done
  HOSTCC=$CC
  KERNEL_SRC=${ROOT_DIR}/${KERNEL_DIR}
  O=${OUT_DIR}
  INSTALL_MOD_PATH=${MODULES_STAGING_DIR}
}

function build_wifi_driver_for_kernel_5_4()
{
  setup_env_for_kernel_5_4 $@
  set -x
  make -C ${MAKE_DIR} \
    KERNEL_SRC=${KERNEL_SRC} O=${OUT_DIR} CC=${CC} HOSTCC=${HOSTCC} LD=${LD} NM=${NM} OBJCOPY=${OBJCOPY} \
	CONFIG_WIFI_BUILD_DRIVER=$WIFI_DRIVER CONFIG_K_VERSION=$KERNEL_VERSION modules
  set +x

  set -x
  make -C ${MAKE_DIR} \
    KERNEL_SRC=${KERNEL_SRC} O=${OUT_DIR} CC=${CC} HOSTCC=${HOSTCC} LD=${LD} NM=${NM} OBJCOPY=${OBJCOPY} \
    INSTALL_MOD_PATH=${INSTALL_MOD_PATH} CONFIG_WIFI_BUILD_DRIVER=$WIFI_DRIVER CONFIG_K_VERSION=$KERNEL_VERSION modules_install
  set +x
}

ANSWER=()
function get_answer()
{
  local title=$1
  eval local list=(\${$2[@]})
  local answer=
  local m=

  echo -e "\n\e[47;30mplease choice $title:\e[0m"
  n=1
  for i in "${list[@]}"; do
    if [[ $n -lt 10 ]]; then
      echo "     $n.  $i"
	else
	  echo "     $n. $i"
	fi
    n=$[n+1]
  done
  n=$[n-1]
  if [[ $n -eq 0 ]]; then
    process_exit -1 "The list of supported $title is empty."
  fi
  echo "Which would you like? (1-${#list[@]})"
  echo -n "-> "
  stty erase ^H
  read answer
  if [[ x$(echo -n $answer | grep -e "^[0-9][0-9]*$") != x ]]; then
    m=$[answer-1]
    if [[ $m -ge 0 && $m -lt ${#list[@]} ]] ; then
      ANSWER=${list[$m]}
    else
      process_exit -1 "Invalid input: $answer"
    fi
  else
    if [[ x$(echo -n "${list[@]}" | grep -w "$answer" 2>/dev/null) == x ]]; then
      process_exit -1 "Invalid input: $answer"
    else
      ANSWER=$answer
    fi
  fi
  if [[ x$ANSWER == x ]]; then
    process_exit -1 "Invalid input!"
  fi
}

function usage() {
  cat << EOF

  Usage:
    $(basename $0) --help

    wifi driver standalone build script.

    If you do not specify the following options, a list will be shown for you to select.

    Build driver options:
    1. -v kernel version: 4.9/5.4
        ./$(basename $0) -v 5.4

    2. -a kernel arch: arm/arm64
        ./$(basename $0) -a arm64

    3. --driver wifi driver name:
        ./$(basename $0) --driver qca6174

    you can use different params at the same time

    Example:
    ./$(basename $0) -v 5.4 -a arm64 --driver qca6174

    Other options:
    1. --get_modules get wifi supported modules:
        ./$(basename $0) --get_modules

    2. --get_drivers get wifi supported drivers:
        ./$(basename $0) --get_drivers

    Example:
    ./$(basename $0) -v 5.4 --get_drivers
    ./$(basename $0) -v 5.4 --get_modules

EOF
}

KERNEL_VERSION=
function pick_kernel_version()
{
  if [[ x${KERNEL_VERSION} == x ]]; then
    get_answer "kernel version" KERNEL_VERSION_LIST
    KERNEL_VERSION=$ANSWER
  fi
  echo pick kernel version: $KERNEL_VERSION
  if [[ $KERNEL_VERSION == "4.9" ]]; then
    KERNEL_VERSION=4_9
  elif [[ $KERNEL_VERSION == "5.4" ]]; then
    KERNEL_VERSION=5_4
  fi
}

KERNEL_ARCH=
function pick_kernel_arch()
{
  if [[ x${KERNEL_ARCH} == x ]]; then
    get_answer "kernel arch" KERNEL_ARCH_LIST
    KERNEL_ARCH=$ANSWER
  fi
  echo pick kernel arch: $KERNEL_ARCH
}

WIFI_DRIVER=
function pick_wifi_driver()
{
  if [[ x${WIFI_DRIVER} == x ]]; then
    get_answer "wifi driver" WIFI_DRIVER_LIST
    WIFI_DRIVER=$ANSWER
  else
    if [[ x$(echo -n "${WIFI_DRIVER_LIST[@]}" | grep -w "$WIFI_DRIVER" 2>/dev/null) == x ]]; then
      usage
      process_exit -1 "Invalid input wifi driver: '$WIFI_DRIVER', you can use --get_drivers get supported drivers"
    fi
  fi
  if [[ $WIFI_DRIVER == \(*\)* ]];then
    WIFI_DRIVER=`echo $WIFI_DRIVER | awk '{print substr($1,4)}'`
  fi
  echo pick wifi driver: $WIFI_DRIVER
}

function make_get()
{
  local opt=$1
  local result=()
  local n=0

  for one in `make -C ${MAKE_DIR} CONFIG_K_VERSION=$KERNEL_VERSION $opt`;do
    if [[ $n -gt 3 ]]; then
      result[n]=$one
    fi
    n=$[n+1]
  done
  for((i=1;i<=4;i++));do
    unset result[n-i]
  done
  echo ${result[*]}
}

function build_wifi_driver()
{
  echo -e "\n######build wifi driver######\n"
  pick_kernel_version $@
  pick_kernel_arch $@
  WIFI_DRIVER_LIST=($(make_get "supported_drivers"))
  pick_wifi_driver $@
  eval export ${WIFI_DRIVER}_build=true
  if [[ $KERNEL_VERSION == "4_9" ]]; then
    build_wifi_driver_for_kernel_4_9 $@
  elif [[ $KERNEL_VERSION == "5_4" ]]; then
    build_wifi_driver_for_kernel_5_4 $@
  fi
}

function get_supported_drivers()
{
  pick_kernel_version $@
  WIFI_DRIVER_LIST=($(make_get "supported_drivers"))
  echo -e "\n\e[47;30mdriver list:\e[0m"
  for i in "${WIFI_DRIVER_LIST[@]}"; do
    echo "$i"
  done
}

function get_supported_modules()
{
  pick_kernel_version $@
  WIFI_MODULE_LIST=($(make_get "supported_modules"))
  echo -e "\n\e[47;30mmodules list:\e[0m"
  for i in "${WIFI_MODULE_LIST[@]}"; do
    echo "$i"
  done
}

i=1
opt=
while [[ $i -le $# ]]; do
  eval arg=\$$i
  i=$[i+1]
  if [[ x$(echo -n "$arg" | grep -w "\--help" 2>/dev/null) != x ]]; then
    usage
    exit 0
  elif [[ x$(echo -n "$arg" | grep -w "\--get_modules" 2>/dev/null) != x ]]; then
    opt="get_modules"
    continue
  elif [[ x$(echo -n "$arg" | grep -w "\--get_drivers" 2>/dev/null) != x ]]; then
    opt="get_drivers"
    continue
  elif [[ x$(echo -n "$arg" | grep -w "\--driver" 2>/dev/null) != x ]]; then
    eval WIFI_DRIVER=\$$i
    i=$[i+1]
    continue
  elif [[ x$(echo -n "$arg" | grep -w "\-v" 2>/dev/null) != x ]]; then
    eval KERNEL_VERSION=\$$i
    if [[ x$(echo -n "${KERNEL_VERSION_LIST[@]}" | grep -w "$KERNEL_VERSION" 2>/dev/null) == x ]]; then
      usage
      process_exit -1 "Invalid input kernel version: $KERNEL_VERSION"
    fi
    i=$[i+1]
    continue
  elif [[ x$(echo -n "$arg" | grep -w "\-a" 2>/dev/null) != x ]]; then
    eval KERNEL_ARCH=\$$i
    if [[ x$(echo -n "${KERNEL_ARCH_LIST[@]}" | grep -w "$KERNEL_ARCH" 2>/dev/null) == x ]]; then
      usage
      process_exit -1 "Invalid input kernel arch: $KERNEL_ARCH"
    fi
    i=$[i+1]
    continue
  else
    usage
    process_exit -1 "Invalid options: $arg"
  fi
done

if [[ x$opt == xget_modules ]];then
  get_supported_modules $@
elif [[ x$opt == xget_drivers ]];then
  get_supported_drivers $@
else
  build_wifi_driver $@
fi

