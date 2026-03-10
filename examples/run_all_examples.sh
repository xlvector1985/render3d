#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
EXAMPLES_DIR="${ROOT_DIR}/examples"
OUT_DIR="${EXAMPLES_DIR}/output"

mkdir -p "${OUT_DIR}"

for scene in "${EXAMPLES_DIR}"/*.txt; do
  [ -e "${scene}" ] || continue
  base="$(basename "${scene}" .txt)"
  ppm="${OUT_DIR}/${base}_300.ppm"
  png="${OUT_DIR}/${base}_300.png"
  "${ROOT_DIR}/raytracer_bvh" 300 50 "${scene}" > "${ppm}"
  python3 "${ROOT_DIR}/ppm_to_png.py" "${ppm}" "${png}"
done
