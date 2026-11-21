#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT}/scripts/env.sh"

PORT="${NASGAMES_PORT:-8098}"
BASE_URL="http://${PS5_HOST}:${PORT}"
APP_ELF="${ROOT}/apps/nasgames/nasgames.elf"
PREPARE_ELF="${ROOT}/apps/nasgames/build/release-prepare.elf"

for command in curl jq; do
    if ! command -v "${command}" >/dev/null 2>&1; then
        printf 'Missing required command: %s\n' "${command}" >&2
        exit 1
    fi
done

if [[ ! -f "${APP_ELF}" || ! -f "${PREPARE_ELF}" ]]; then
    printf 'Release payloads are missing. Build with: make nasgames && make -C apps/nasgames release-prepare\n' >&2
    exit 1
fi

if current="$(curl -fsS --max-time 5 "${BASE_URL}/api/state" 2>/dev/null)"; then
    jq -e '.ok and .app == "NAS Games" and .job.state != "running" and .job.queued == 0 and (.scan.busy | not)' \
        >/dev/null <<<"${current}" || { printf 'Wait for NAS Games scanning and installation to finish.\n' >&2; exit 1; }
fi

printf 'Preparing NAS Games release for %s\n' "${PS5_HOST}"
bash "${ROOT}/scripts/deploy.sh" "${PREPARE_ELF}"

for _ in $(seq 1 20); do
    if ! curl -fsS --max-time 1 "${BASE_URL}/api/health" >/dev/null 2>&1; then
        break
    fi
    sleep 0.5
done
if curl -fsS --max-time 1 "${BASE_URL}/api/health" >/dev/null 2>&1; then
    printf 'The previous NAS Games instance is still listening on %s\n' "${BASE_URL}" >&2
    exit 1
fi
sleep 2

printf 'Deploying %s\n' "${APP_ELF}"
bash "${ROOT}/scripts/deploy.sh" "${APP_ELF}"

ready=0
for _ in $(seq 1 40); do
    if curl -fsS --max-time 2 "${BASE_URL}/api/health" >/dev/null 2>&1; then
        ready=1
        break
    fi
    sleep 1
done
if [[ "${ready}" -ne 1 ]]; then
    printf 'NAS Games did not become ready at %s\n' "${BASE_URL}" >&2
    exit 1
fi

settings="$(curl -fsS --max-time 5 "${BASE_URL}/api/settings")"
body="$(
    jq '{
        version,
        port,
        auto_scan,
        scan_interval_seconds,
        max_scan_depth,
        open_browser_on_start,
        keep_awake,
        sources: [
            .sources[]
            | select((.id == "localtest" and .localRoot == "/data/nasgames/test-plain") | not)
        ]
    }' <<<"${settings}"
)"
curl -fsS --max-time 10 -X PUT \
    -H 'Content-Type: application/json' \
    --data-binary "${body}" \
    "${BASE_URL}/api/settings" >/dev/null
curl -fsS --max-time 5 -X POST -H 'Content-Type: application/json' "${BASE_URL}/api/scan" >/dev/null

remaining="$(
    curl -fsS --max-time 5 "${BASE_URL}/api/settings" |
        jq -r '[.sources[] | select(.id == "localtest" and .localRoot == "/data/nasgames/test-plain")] | length'
)"
if [[ "${remaining}" != "0" ]]; then
    printf 'Temporary source configuration was not removed\n' >&2
    exit 1
fi

printf 'NAS Games is ready at %s with clean release data.\n' "${BASE_URL}"
