mbpoll -m rtu -a 1 -o 0.1 -t 4 -0 -r 1000 /dev/ttyUSB0 1
sleep 0.2
mbpoll -m rtu -a 1 -o 0.1 -t 4 -0 -r 1001 /dev/ttyUSB0 18364
sleep 0.2
mbpoll -m rtu -a 1 -o 0.1 -t 4 -0 -r 1001 /dev/ttyUSB0 1000
