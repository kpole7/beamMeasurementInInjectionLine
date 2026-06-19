#!/usr/bin/env bash

set -euo pipefail

DEFAULT_ELF="build/pico-debug/beamMeasurementInInjectionLine.elf"
TOTAL_RAM_BYTES=$((264 * 1024))

usage() {
    cat <<'EOF'
Usage:
  ./run-ram-usage.sh [path/to/file.elf] [total_ram_bytes]

Examples:
  ./run-ram-usage.sh
  ./run-ram-usage.sh build/pico-debug/beamMeasurementInInjectionLine.elf
  ./run-ram-usage.sh build/pico-debug/beamMeasurementInInjectionLine.elf 270336
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    usage
    exit 0
fi

ELF_PATH="${1:-$DEFAULT_ELF}"
if [[ -n "${2:-}" ]]; then
    TOTAL_RAM_BYTES="$2"
fi

if [[ ! -f "$ELF_PATH" ]]; then
    echo "Error: ELF file not found: $ELF_PATH" >&2
    exit 1
fi

if ! command -v arm-none-eabi-size >/dev/null 2>&1; then
    echo "Error: arm-none-eabi-size is not available in PATH." >&2
    exit 1
fi

if [[ ! "$TOTAL_RAM_BYTES" =~ ^[0-9]+$ ]] || [[ "$TOTAL_RAM_BYTES" -le 0 ]]; then
    echo "Error: total_ram_bytes must be a positive integer." >&2
    exit 1
fi

read_section_size() {
    local section_name="$1"
    arm-none-eabi-size -A "$ELF_PATH" | awk -v sec="$section_name" '$1 == sec {print $2}'
}

RAM_VECTOR_TABLE=$(read_section_size ".ram_vector_table")
DATA_SIZE=$(read_section_size ".data")
BSS_SIZE=$(read_section_size ".bss")
HEAP_SIZE=$(read_section_size ".heap")
STACK_SIZE=$(read_section_size ".stack_dummy")

RAM_VECTOR_TABLE=${RAM_VECTOR_TABLE:-0}
DATA_SIZE=${DATA_SIZE:-0}
BSS_SIZE=${BSS_SIZE:-0}
HEAP_SIZE=${HEAP_SIZE:-0}
STACK_SIZE=${STACK_SIZE:-0}

STATIC_BYTES=$((RAM_VECTOR_TABLE + DATA_SIZE + BSS_SIZE))
STATIC_STACK_BYTES=$((STATIC_BYTES + STACK_SIZE))
LINKER_BUDGET_BYTES=$((STATIC_STACK_BYTES + HEAP_SIZE))
FREE_AFTER_LINKER_BUDGET=$((TOTAL_RAM_BYTES - LINKER_BUDGET_BYTES))

pct() {
    local numerator="$1"
    awk -v n="$numerator" -v d="$TOTAL_RAM_BYTES" 'BEGIN { printf "%.3f", (n / d) * 100.0 }'
}

STATIC_PCT=$(pct "$STATIC_BYTES")
STATIC_STACK_PCT=$(pct "$STATIC_STACK_BYTES")
LINKER_BUDGET_PCT=$(pct "$LINKER_BUDGET_BYTES")

echo "ELF: $ELF_PATH"
echo "TOTAL_RAM_BYTES: $TOTAL_RAM_BYTES"
echo
echo "Section sizes [B]:"
echo "  .ram_vector_table : $RAM_VECTOR_TABLE"
echo "  .data             : $DATA_SIZE"
echo "  .bss              : $BSS_SIZE"
echo "  .stack_dummy      : $STACK_SIZE"
echo "  .heap             : $HEAP_SIZE"
echo
echo "Calculated usage:"
echo "  Static (.ram_vector_table + .data + .bss): $STATIC_BYTES B (${STATIC_PCT}%)"
echo "  Static + stack_dummy                    : $STATIC_STACK_BYTES B (${STATIC_STACK_PCT}%)"
echo "  Linker budget (+ heap)                  : $LINKER_BUDGET_BYTES B (${LINKER_BUDGET_PCT}%)"
echo "  Free after linker budget                : $FREE_AFTER_LINKER_BUDGET B"