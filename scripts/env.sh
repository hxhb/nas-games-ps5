#!/usr/bin/env bash

# Load the local, self-contained PS5 toolchain into the current shell.
# Usage: source scripts/env.sh

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"

export PS5_PAYLOAD_SDK="${ROOT}/.toolchain/sdk-root/ps5-payload-sdk"
LLVM_ROOT="${ROOT}/.toolchain/llvm/usr/lib/llvm-18"

if [[ ! -f "${PS5_PAYLOAD_SDK}/toolchain/prospero.mk" || ! -x "${LLVM_ROOT}/bin/clang" ]]; then
    printf '%s\n' "PS5 toolchain is not installed. Run: ./scripts/bootstrap.sh" >&2
    return 1 2>/dev/null || exit 1
fi

export LLVM_CONFIG="${LLVM_ROOT}/bin/llvm-config"
export PATH="${PS5_PAYLOAD_SDK}/bin:${LLVM_ROOT}/bin:${ROOT}/.toolchain/socat/usr/bin:${PATH}"

LLVM_LIBDIR="${ROOT}/.toolchain/llvm/usr/lib/x86_64-linux-gnu"
CLANG_LIBDIR="${LLVM_ROOT}/lib"
if [[ -n "${LD_LIBRARY_PATH:-}" ]]; then
    export LD_LIBRARY_PATH="${LLVM_LIBDIR}:${CLANG_LIBDIR}:${LD_LIBRARY_PATH}"
else
    export LD_LIBRARY_PATH="${LLVM_LIBDIR}:${CLANG_LIBDIR}"
fi

# Load local target settings without overriding explicit environment values.
if [[ -f "${ROOT}/config.env" ]]; then
    while IFS='=' read -r key value || [[ -n "${key:-}" ]]; do
        [[ -z "${key}" || "${key}" == \#* ]] && continue
        key="${key//[[:space:]]/}"
        value="${value%$'\r'}"
        if [[ -z "${!key:-}" ]]; then
            printf -v "${key}" '%s' "${value}"
            export "${key}"
        fi
    done < "${ROOT}/config.env"
fi

: "${PS5_HOST:=ps5}"
: "${PS5_PORT:=9021}"
: "${PS5_WEB_PORT:=8080}"
export PS5_HOST PS5_PORT PS5_WEB_PORT
