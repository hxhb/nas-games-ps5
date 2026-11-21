#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT}/scripts/env.sh"

VERSION="${LIBSMB2_VERSION:-6.2}"
DEPS="${ROOT}/.deps"
SRC="${LIBSMB2_SRC:-${DEPS}/libsmb2-${VERSION}}"
OUT="${1:-${ROOT}/apps/nasgames/build/libsmb2}"
ARCHIVE="${DEPS}/libsmb2-${VERSION}.tar.gz"
URL="https://codeload.github.com/sahlberg/libsmb2/tar.gz/refs/tags/libsmb2-${VERSION}"

mkdir -p "${DEPS}" "${OUT}"
if [[ ! -f "${SRC}/lib/libsmb2.c" ]]; then
    if [[ -e "${SRC}" ]]; then
        printf 'Missing libsmb2 source at %s\n' "${SRC}" >&2
        exit 1
    fi
    if [[ ! -f "${ARCHIVE}" ]]; then
        printf 'Downloading libsmb2 %s…\n' "${VERSION}"
        curl -fL --retry 3 --connect-timeout 20 -o "${ARCHIVE}.part" "${URL}"
        mv "${ARCHIVE}.part" "${ARCHIVE}"
    fi
    if [[ "${VERSION}" == 6.2 ]]; then
        printf '%s  %s\n' '8e1f9efc6b2e0f6546f0fe121ac0ddf4fc2f0908ae5a6bd1f185be7c9e0bcbb3' "${ARCHIVE}" | sha256sum -c --status
    fi
    staging="$(mktemp -d "${DEPS}/extract-${VERSION}.XXXXXX")"
    tar -xzf "${ARCHIVE}" -C "${staging}" --strip-components=1
    mv "${staging}" "${SRC}"
fi

COMMON_CFLAGS=(
    -c -O2 -g
    -I"${ROOT}/apps/nasgames/third_party/libsmb2"
    -I"${SRC}/include"
    -I"${SRC}/include/smb2"
    -I"${SRC}/lib"
    -DHAVE_CONFIG_H
    '-D_U_=__attribute__((unused))'
)

objects=()
while IFS= read -r source; do
    name="$(basename "${source}" .c)"
    [[ "${name}" == "krb5-wrapper" ]] && continue
    object="${OUT}/${name}.o"
    prospero-clang "${COMMON_CFLAGS[@]}" -o "${object}" "${source}"
    objects+=("${object}")
done < <(find "${SRC}/lib" -maxdepth 1 -name '*.c' | sort)

rm -f "${OUT}/libsmb2.a"
"${PS5_PAYLOAD_SDK}/bin/prospero-ar" rcs "${OUT}/libsmb2.a" "${objects[@]}"
rm -rf "${OUT}/include"
cp -R "${SRC}/include" "${OUT}/include"
printf 'libsmb2 ready: %s (%d objects)\n' "${OUT}/libsmb2.a" "${#objects[@]}"
