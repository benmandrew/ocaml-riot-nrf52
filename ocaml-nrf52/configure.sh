#!/bin/sh

prog_NAME="$(basename $0)"

err()
{
    echo "${prog_NAME}: ERROR: $@" 1>&2
}

die()
{
    echo "${prog_NAME}: ERROR: $@" 1>&2
    exit 1
}

warn()
{
    echo "${prog_NAME}: WARNING: $@" 1>&2
}

usage()
{
    cat <<EOM 1>&2
usage: ${prog_NAME} [ OPTIONS ]
Configures the ocaml-nrf52 build system.
Options:
    --prefix=DIR:
        Installation prefix (default: /usr/local).
    --toolchain=TOOLCHAIN
        Solo5 toolchain flags to use.
EOM
    exit 1
}

MAKECONF_PREFIX=/usr/local
while [ $# -gt 0 ]; do
    OPT="$1"

    case "${OPT}" in
        --toolchain=*)
            CONFIG_TOOLCHAIN="${OPT##*=}"
            ;;
        --prefix=*)
            MAKECONF_PREFIX="${OPT##*=}"
            ;;
        --help)
            usage
            ;;
        *)
            err "Unknown option: '${OPT}'"
            usage
            ;;
    esac

    shift
done

# [ -z "${CONFIG_TOOLCHAIN}" ] && die "The --toolchain option needs to be specified."

ocamlfind query ocaml-src >/dev/null || exit 1

# MAKECONF_CFLAGS="$(solo5-config --toolchain=$CONFIG_TOOLCHAIN --cflags)"
# MAKECONF_CC="$(solo5-config --toolchain=$CONFIG_TOOLCHAIN --cc)"
# MAKECONF_LD="$(solo5-config --toolchain=$CONFIG_TOOLCHAIN --ld)"

# MAKECONF_CFLAGS="-DDEVELHELP -Werror -Wno-format-nonliteral -mno-thumb-interwork -mcpu=cortex-m4 -mlittle-endian \
# -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -ffunction-sections -fdata-sections -fno-builtin \
# -fshort-enums -ggdb -g3 -Os -DCPU_MODEL_NRF52840XXAA -DCPU_CORE_CORTEX_M4F -DRIOT_APPLICATION=\"ocaml-runtime\" \
# -DBOARD_NRF52840_MDK=\"nrf52840-mdk\" -DRIOT_BOARD=BOARD_NRF52840_MDK -DCPU_FAM_NRF52 \
# -DCPU_NRF52=\"nrf52\" -DRIOT_CPU=CPU_NRF52 -DMCU_NRF52=\"nrf52\" -DRIOT_MCU=MCU_NRF52 \
# -fwrapv -fno-common -ffunction-sections -fdata-sections -fno-delete-null-pointer-checks \
# -fdiagnostics-color -gz -Wformat=2 -isystem /Users/ben/Desktop/toolchains/gcc-arm-none-eabi-10-2020-q4-major/arm-none-eabi/include/newlib-nano"

MAKECONF_CFLAGS="-Wno-format-nonliteral -mno-thumb-interwork -mcpu=cortex-m4 -mlittle-endian \
-mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -ffunction-sections -fdata-sections -fno-builtin \
-fshort-enums -ggdb -g3 -Os -DCPU_MODEL_NRF52840XXAA -DCPU_CORE_CORTEX_M4F -DRIOT_APPLICATION=\"ocaml-nrf52\" \
-DBOARD_NRF52840_MDK=\"nrf52840-mdk\" -DRIOT_BOARD=BOARD_NRF52840_MDK -DCPU_FAM_NRF52 \
-DCPU_NRF52=\"nrf52\" -DRIOT_CPU=CPU_NRF52 -DMCU_NRF52=\"nrf52\" -DRIOT_MCU=MCU_NRF52 \
-fwrapv -fno-common -ffunction-sections -fdata-sections -fno-delete-null-pointer-checks \
-fdiagnostics-color -gz -Wformat=2 -I /Users/ben/Desktop/Projects/riot-ocaml/sys/newlib_syscalls_default/"
MAKECONF_CC="arm-none-eabi-gcc"
MAKECONF_LD="arm-none-eabi-gcc"

# BUILD_ARCH="$(uname -m)"
OCAML_BUILD_ARCH=
AS=

# Canonicalize BUILD_ARCH and set OCAML_BUILD_ARCH. The former is for autoconf,
# the latter for the rest of the OCaml build system.
case "${MAKECONF_CC}" in
    amd64-*|x86_64-*)
        BUILD_ARCH="x86_64"
        OCAML_BUILD_ARCH="amd64"
        ;;
    aarch64-*)
        OCAML_BUILD_ARCH="arm64"
        ;;
    arm-*)
        BUILD_ARCH="arm"
        OCAML_BUILD_ARCH="arm"
        ;;
    *)
        echo "ERROR: Unsupported architecture: ${BUILD_ARCH}" 1>&2
        exit 1
        ;;
esac

case "$($MAKECONF_CC -dumpmachine)" in
    *-*-freebsd*|*-*-openbsd*|arm-*-eabi*)
        AS="$MAKECONF_CC -c"
        ;;
    *)
        AS=as
        ;;
esac

PKG_CONFIG_EXTRA_LIBS=
if [ "${BUILD_ARCH}" = "aarch64" ]; then
    PKG_CONFIG_EXTRA_LIBS="$PKG_CONFIG_EXTRA_LIBS $($MAKECONF_CC -print-libgcc-file-name)" || exit 1
fi

cat <<EOM >Makeconf
MAKECONF_PREFIX=${MAKECONF_PREFIX}
MAKECONF_CFLAGS=${MAKECONF_CFLAGS}
MAKECONF_CC=${MAKECONF_CC}
MAKECONF_LD=${MAKECONF_LD}
MAKECONF_BUILD_ARCH=${BUILD_ARCH}
MAKECONF_OCAML_BUILD_ARCH=${OCAML_BUILD_ARCH}
MAKECONF_PKG_CONFIG_EXTRA_LIBS=${PKG_CONFIG_EXTRA_LIBS}
MAKECONF_AS=${AS}
EOM
