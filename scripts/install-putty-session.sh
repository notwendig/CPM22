#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
session_src="${project_dir}/doc/putty/CPM"
session_dir="${HOME}/.putty/sessions"
session_dst="${session_dir}/CPM"

mkdir -p "${session_dir}"
install -m 0644 "${session_src}" "${session_dst}"
printf 'Installed PuTTY session: %s\n' "${session_dst}"
