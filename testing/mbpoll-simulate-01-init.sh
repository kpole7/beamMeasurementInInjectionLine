#!/bin/bash

# Commands for simulator; 2026-07-02

run_mbpoll() {
    echo "Wykonuję: $*"

    if ! "$@"; then
        echo "BŁĄD! Polecenie nie powiodło się."
        exit 1
    fi

    sleep 0.01
}

PORT="/dev/ttyS5"

if ! command -v mbpoll >/dev/null 2>&1; then
    echo "Błąd: mbpoll nie jest zainstalowany."
    exit 1
fi

if [ ! -e "$PORT" ]; then
    echo "Błąd: nie znaleziono portu $PORT"
    exit 1
fi

# samples for the cup 1 channel 1 ... 4
run_mbpoll mbpoll -q -m rtu -a 1 -o 0.1 -t 4 -0 -r 1144 "$PORT" 123
run_mbpoll mbpoll -q -m rtu -a 1 -o 0.1 -t 4 -0 -r 1145 "$PORT" 4567
run_mbpoll mbpoll -q -m rtu -a 1 -o 0.1 -t 4 -0 -r 1146 "$PORT" 8901
run_mbpoll mbpoll -q -m rtu -a 1 -o 0.1 -t 4 -0 -r 1147 "$PORT" 23456

# samples for the cup 2 channel 1 ... 4
run_mbpoll mbpoll -q -m rtu -a 1 -o 0.1 -t 4 -0 -r 1148 "$PORT" 12345
run_mbpoll mbpoll -q -m rtu -a 1 -o 0.1 -t 4 -0 -r 1149 "$PORT" 6789
run_mbpoll mbpoll -q -m rtu -a 1 -o 0.1 -t 4 -0 -r 1150 "$PORT" 123
run_mbpoll mbpoll -q -m rtu -a 1 -o 0.1 -t 4 -0 -r 1151 "$PORT" 45

# samples for the cup 3 channel 1 ... 4
run_mbpoll mbpoll -q -m rtu -a 1 -o 0.1 -t 4 -0 -r 1152 "$PORT" 222
run_mbpoll mbpoll -q -m rtu -a 1 -o 0.1 -t 4 -0 -r 1153 "$PORT" 3333
run_mbpoll mbpoll -q -m rtu -a 1 -o 0.1 -t 4 -0 -r 1154 "$PORT" 44
run_mbpoll mbpoll -q -m rtu -a 1 -o 0.1 -t 4 -0 -r 1155 "$PORT" 555

#randomization parameters
run_mbpoll mbpoll -q -m rtu -a 1 -o 0.1 -t 4 -0 -r 1156 "$PORT" 0
run_mbpoll mbpoll -q -m rtu -a 1 -o 0.1 -t 4 -0 -r 1157 "$PORT" 5

echo "Skrypt wykonano poprawnie"

