#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/.." && pwd)"

default_files=(
    "${repo_root}/data/vita/romfs/sce_sys/icon0.png"
    "${repo_root}/data/vita/romfs/sce_sys/pic0.png"
    "${repo_root}/data/vita/romfs/sce_sys/livearea/contents/bg.png"
    "${repo_root}/data/vita/romfs/sce_sys/livearea/contents/startup.png"
)

usage() {
    cat <<'EOF'
Usage:
  scripts/fix-vita-package-pngs.sh
  scripts/fix-vita-package-pngs.sh path/to/file.png [...]

Without arguments, the script rewrites the Vita packaging PNGs in-place:
  - data/vita/romfs/sce_sys/icon0.png
  - data/vita/romfs/sce_sys/pic0.png
  - data/vita/romfs/sce_sys/livearea/contents/bg.png
  - data/vita/romfs/sce_sys/livearea/contents/startup.png

Each PNG is converted in-place to a stripped indexed PNG with an 8-bit palette.
This avoids Vita installer issues caused by truecolor PNGs with extra metadata.
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage
    exit 0
fi

if ! command -v magick >/dev/null 2>&1; then
    echo "error: ImageMagick 'magick' command not found" >&2
    exit 1
fi

if [[ "$#" -eq 0 ]]; then
    files=("${default_files[@]}")
else
    files=("$@")
fi

for path in "${files[@]}"; do
    if [[ ! -f "${path}" ]]; then
        echo "error: file not found: ${path}" >&2
        exit 1
    fi

    tmp_dir="$(dirname "${path}")"
    tmp="$(mktemp "${tmp_dir}/.vita-png-XXXXXX.png")"
    trap 'rm -f "${tmp}"' EXIT

    magick "${path}" -strip -colors 256 "PNG8:${tmp}"
    mv "${tmp}" "${path}"
    trap - EXIT

    echo "rewrote ${path}"
    file "${path}"
done
