#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT}/scripts/env.sh"

if [[ "$#" -lt 1 ]]; then
    printf 'Usage: %s PAYLOAD.elf [--autoload]\n' "$0" >&2
    exit 2
fi

payload="$1"
shift
autoload=0
for arg in "$@"; do
    case "${arg}" in
        --autoload) autoload=1 ;;
        *)
            printf 'Unknown option: %s\n' "${arg}" >&2
            exit 2
            ;;
    esac
done

if [[ ! -f "${payload}" ]]; then
    printf 'Payload not found: %s\n' "${payload}" >&2
    exit 1
fi

pm_url="${PM_URL:-http://${PS5_HOST}:8084}"
filename="$(basename -- "${payload}")"
stem="${filename%.*}"
slot="local/${stem}"
request_header=(-H "X-PLDMGR-Request: 1")

if ! curl -fsS --compressed --max-time 5 "${request_header[@]}" \
    "${pm_url}/version" >/dev/null; then
    printf 'Payload Manager is not reachable at %s\n' "${pm_url}" >&2
    exit 1
fi

check="$(curl -fsS --compressed --max-time 10 "${request_header[@]}" \
    "${pm_url}/manage:check?filename=${filename}")"
existing="$(printf '%s' "${check}" | python3 -c '
import json, sys
data = json.load(sys.stdin)
print(1 if (data.get("existing") or data.get("file_exists") or
            data.get("folder_exists")) else 0)
')"

if [[ "${existing}" == "1" ]]; then
    mode=replace
else
    mode=new
fi

printf 'Uploading %s to Payload Manager (%s)...\n' "${filename}" "${slot}"
curl -fsS --compressed --max-time 180 -X POST "${request_header[@]}" \
    -H "Content-Type: application/octet-stream" \
    --data-binary "@${payload}" \
    "${pm_url}/manage:upload?filename=${filename}&mode=${mode}" >/dev/null

if [[ "${autoload}" == "1" ]]; then
    config="$(curl -fsS --compressed --max-time 10 "${request_header[@]}" \
        "${pm_url}/get_config")"
    body="$(printf '%s' "${config}" | python3 -c '
import json, sys
slot = sys.argv[1]
config = json.load(sys.stdin)
items = [item for item in str(config.get("AUTOLOAD_LIST", "")).split(",") if item]
if slot not in items:
    items.append(slot)
print(json.dumps({
    "AUTOLOAD_ENABLED": True,
    "AUTOLOAD_LIST": ",".join(items),
}))
' "${slot}")"
    curl -fsS --compressed --max-time 10 -X POST "${request_header[@]}" \
        -H "Content-Type: application/json" \
        --data-binary "${body}" "${pm_url}/set_config" >/dev/null
    printf 'Enabled Autoload for %s.\n' "${slot}"
fi

printf 'Payload Manager now manages %s.\n' "${slot}"
