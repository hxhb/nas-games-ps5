#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT}/scripts/env.sh"

payload="${ROOT}/apps/nasgames/nasgames.elf"
version="$(awk -F '"' '/^#define NG_VERSION / {print $2}' "${ROOT}/apps/nasgames/src/common.h")"
pm_url="${PM_URL:-http://${PS5_HOST}:8084}"
slot="local/nasgames"
request_header=(-H 'X-PLDMGR-Request: 1')
http_port="${NASGAMES_PORT:-8098}"
base_url="http://${PS5_HOST}:${http_port}"

for command in curl jq sha256sum; do
    command -v "$command" >/dev/null || { printf 'Missing command: %s\n' "$command" >&2; exit 1; }
done

release_dir="${ROOT}/dist/nasgames-${version}"
mkdir -p "${release_dir}"
install -m 0755 "${payload}" "${release_dir}/nasgames.elf"
(cd "${release_dir}" && sha256sum nasgames.elf > SHA256SUMS && sha256sum -c SHA256SUMS)
printf 'Game Center %s artifacts: %s\n' "${version}" "${release_dir}"

pm_request() {
    curl -fsS --compressed --max-time 15 "${request_header[@]}" "$@"
}

process_pid() {
    pm_request "${pm_url}/processes_list" |
        jq -r '.processes | map(select(.name == "nasgames.elf") | .pid) | if length <= 1 then .[0] // empty else error("multiple Game Center processes") end'
}

state=""
pid="$(process_pid)"
if [[ -n "${pid}" ]]; then
    state="$(curl -fsS --max-time 5 "${base_url}/api/state")"
    jq -e '.ok and .app == "Game Center" and .job.state != "running" and .job.queued == 0 and (.scan.busy | not)' \
        >/dev/null <<<"${state}" || { printf 'Wait for Game Center scanning and installation to finish.\n' >&2; exit 1; }
    configured_port="$(curl -fsS --max-time 5 "${base_url}/api/settings" | jq -er '.port')"
else
    configured_port="${http_port}"
fi

PM_URL="${pm_url}" bash "${ROOT}/scripts/pm-upload.sh" "${release_dir}/nasgames.elf"

if [[ -n "${pid}" ]]; then
    if curl -fsS --max-time 5 -X POST -H 'Content-Type: application/json' \
        "${base_url}/api/shutdown" >/dev/null 2>&1; then
        printf 'Game Center is shutting down.\n'
    elif [[ "$(jq -r '.version' <<<"${state}")" == "0.1.0" ]]; then
        curl -fsS --max-time 5 "${base_url}/api/state" |
            jq -e '.ok and .version == "0.1.0" and .job.state != "running" and .job.queued == 0 and (.scan.busy | not)' >/dev/null
        [[ "$(process_pid)" == "${pid}" ]]
        pm_request -X POST "${pm_url}/process_kill?pid=${pid}" >/dev/null
    else
        printf 'Game Center refused to shut down. Check the active tasks and retry.\n' >&2
        exit 1
    fi

    stopped=0
    for _ in $(seq 1 45); do
        if [[ -z "$(process_pid)" ]]; then
            stopped=1
            break
        fi
        sleep 1
    done
    if [[ "${stopped}" != 1 ]]; then
        printf 'Game Center did not exit within 45 seconds.\n' >&2
        exit 1
    fi
fi

pm_request -X POST "${pm_url}/loadpayload:${slot//\//%2F}" >/dev/null
base_url="http://${PS5_HOST}:${configured_port}"
for _ in $(seq 1 30); do
    if curl -fsS --max-time 2 "${base_url}/api/health" 2>/dev/null |
        jq -e --arg version "${version}" '.ok and .version == $version' >/dev/null; then
        [[ -n "$(process_pid)" ]]
        printf 'Game Center %s is running at %s via Payload Manager.\n' "${version}" "${base_url}"
        exit 0
    fi
    sleep 1
done

printf 'Game Center %s did not answer on %s.\n' "${version}" "${base_url}" >&2
exit 1
