#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT}"

SDK_VERSION="v0.50"
SDK_URL="https://github.com/ps5-payload-dev/sdk/releases/download/${SDK_VERSION}/ps5-payload-sdk.zip"
SDK_SHA256="2993a69e52fea04a32bf1753318fc60b3f918ab635bdf946870e7d1849557bb2"

for cmd in curl python3 dpkg-deb apt-get sha256sum; do
    if ! command -v "${cmd}" >/dev/null 2>&1; then
        printf 'Missing required command: %s\n' "${cmd}" >&2
        exit 1
    fi
done

mkdir -p .cache/debs .toolchain

if [[ ! -f .cache/ps5-payload-sdk.zip ]] || \
   [[ "$(sha256sum .cache/ps5-payload-sdk.zip | awk '{print $1}')" != "${SDK_SHA256}" ]]; then
    printf 'Downloading ps5-payload-sdk %s...\n' "${SDK_VERSION}"
    curl -L --fail --silent --show-error -o .cache/ps5-payload-sdk.zip "${SDK_URL}"
fi

actual_sha="$(sha256sum .cache/ps5-payload-sdk.zip | awk '{print $1}')"
if [[ "${actual_sha}" != "${SDK_SHA256}" ]]; then
    printf 'SDK checksum mismatch: expected %s, got %s\n' "${SDK_SHA256}" "${actual_sha}" >&2
    exit 1
fi

printf 'Extracting SDK...\n'
python3 -m zipfile -e .cache/ps5-payload-sdk.zip .toolchain/sdk-root
chmod +x .toolchain/sdk-root/ps5-payload-sdk/bin/*

packages=(
    clang-18
    lld-18
    libclang-cpp18
    libllvm18
    libclang-common-18-dev
    libclang1-18
    llvm-18
    llvm-18-runtime
    llvm-18-linker-tools
    socat
)

for package in "${packages[@]}"; do
    matches=(.cache/debs/"${package}"_*.deb)
    if [[ ! -e "${matches[0]}" ]]; then
        printf 'Downloading %s...\n' "${package}"
        (cd .cache/debs && apt-get download "${package}")
    fi
done

printf 'Extracting local LLVM toolchain...\n'
mkdir -p .toolchain/llvm .toolchain/socat
for deb in .cache/debs/*.deb; do
    case "$(basename "${deb}")" in
        socat_*) dpkg-deb -x "${deb}" .toolchain/socat ;;
        *)       dpkg-deb -x "${deb}" .toolchain/llvm ;;
    esac
done

source scripts/env.sh

printf '\nInstalled toolchain:\n'
printf '  SDK:  %s\n' "${PS5_PAYLOAD_SDK}"
printf '  clang: %s\n' "$(prospero-clang --version | head -n1)"
printf '  lld:   %s\n' "$(prospero-lld --version | head -n1)"
printf '\nNext:\n'
printf '  make check\n'
printf '  make deploy\n'
