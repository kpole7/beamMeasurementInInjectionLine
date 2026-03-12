#!/usr/bin/env bash

set -euo pipefail

# Usage:
#   ./run-my--clang-tidy.sh [build-dir]
# Example:
#   ./run-my--clang-tidy.sh build/pico-release
BUILD_DIR="${1:-build/pico-release}"
COMPILE_DB="${BUILD_DIR}/compile_commands.json"

if [[ ! -f "${COMPILE_DB}" ]]; then
  echo "Brak ${COMPILE_DB}. Najpierw skonfiguruj projekt (np. cmake --preset pico-release)." >&2
  exit 1
fi

# Process only translation units in source/.
FILE_REGEX='(^|.*/)source/.*\.(c|cc|cpp|cxx)$'

# Show diagnostics only for source/ and include/ headers.
# This excludes freemodbus/ and portfreemodbus/ diagnostics.
HEADER_FILTER='(^|.*/)(source|include)/.*'

if ! command -v arm-none-eabi-gcc >/dev/null 2>&1; then
  echo "Brak arm-none-eabi-gcc w PATH." >&2
  exit 1
fi

TOOLCHAIN_INSTALL_DIR="$(arm-none-eabi-gcc -print-search-dirs | sed -n 's/^install: //p' | head -n 1)"
SYSROOT_DIR="$(realpath "${TOOLCHAIN_INSTALL_DIR}/../../../arm-none-eabi")"

if [[ ! -d "${SYSROOT_DIR}/include" ]]; then
  echo "Nie znaleziono sysroot arm-none-eabi (oczekiwano: ${SYSROOT_DIR}/include)." >&2
  exit 1
fi

run-clang-tidy \
  -p "${BUILD_DIR}" \
  -j "$(nproc)" \
  -header-filter="${HEADER_FILTER}" \
  -extra-arg=--target=arm-none-eabi \
  -extra-arg="--sysroot=${SYSROOT_DIR}" \
  "${FILE_REGEX}"
