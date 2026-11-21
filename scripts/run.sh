#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT}/scripts/env.sh"

if [[ "$#" -eq 0 ]]; then
    printf 'Usage: %s COMMAND [ARG...]\n' "$0" >&2
    exit 2
fi

exec "$@"
