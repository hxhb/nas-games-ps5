#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT}/scripts/env.sh"

printf 'SDK:       %s\n' "${PS5_PAYLOAD_SDK}"
printf 'PS5 target: %s:%s\n' "${PS5_HOST}" "${PS5_PORT}"

printf 'Compiler:  '
prospero-clang --version | head -n1
printf 'Linker:    '
prospero-lld --version | head -n1

if command -v getent >/dev/null 2>&1; then
    printf 'Resolved:  '
    getent hosts "${PS5_HOST}" || printf '%s\n' "unresolved"
fi

if ! timeout 3 nc -z -w 2 "${PS5_HOST}" "${PS5_PORT}" >/dev/null 2>&1; then
    printf '\nCannot reach %s:%s.\n' "${PS5_HOST}" "${PS5_PORT}" >&2
    printf '%s\n' 'Make sure elfldr is running and that WSL can route to the PS5 IP.' >&2
    printf '%s\n' 'Override the target with: make check PS5_HOST=192.168.1.50' >&2
    exit 1
fi

printf '\nTCP target is reachable.\n'
