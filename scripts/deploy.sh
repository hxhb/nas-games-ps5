#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT}/scripts/env.sh"

if [[ "$#" -ne 1 ]]; then
    printf 'Usage: %s PAYLOAD.elf\n' "$0" >&2
    exit 2
fi

payload="$1"
if [[ ! -f "${payload}" ]]; then
    printf 'Payload not found: %s\n' "${payload}" >&2
    exit 1
fi

if ! timeout 3 nc -z -w 2 "${PS5_HOST}" "${PS5_PORT}" >/dev/null 2>&1; then
    printf 'Cannot reach %s:%s. Is elfldr running?\n' "${PS5_HOST}" "${PS5_PORT}" >&2
    exit 1
fi

printf 'Sending %s to %s:%s...\n' "${payload}" "${PS5_HOST}" "${PS5_PORT}"
timeout 60 python3 "${ROOT}/scripts/send_payload.py" \
    "${PS5_HOST}" "${PS5_PORT}" "${payload}"
printf 'Payload sent.\n'
