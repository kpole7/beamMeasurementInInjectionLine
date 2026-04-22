# AppForFaradayCups
The goal of this project is to develop software for the Raspberry Pi Pico.
The software is written in C based on RaspberryPi Pico SDK.

```
The software performs several tasks:
1. The Modbus server.
2. Reading logic inputs.
3. The main state machine handler.
4. Auxiliary state machines control individual Faraday cups and monitor their operation.
4. Analog measurements handler.
5. Calculation of ionic currents.
6. Debugging terminal handler.

1. Free Modbus module.
    Brief description.
        Support for the Modbus RTU server via a serial port in accordance with the standard. Support for the
        LED indicating Modbus transmission. The server (slave) supports only one Modbus device address.
    Detailed description.
        The register address space is presented in the Modbus registers table.
    Implementation and testing progress.
        The task has been implemented and tested to the extent possible at this stage.

2. Reading logic inputs.
    Brief description.
        Periodically reading the logical input ports and writing the data to the certain Modbus registers.
    Detailed description.
        The input ports are are associated with the Modbus registers that have the entry “Reading logic
        inputs” in the “Right to modify” column in the table of Modbus registers.

3. The main state machine handler.
    Brief description.
        Implementation of a finite-state machine; execution of a set of rules that determine transitions 
        between states. Among other things, checking whether the device is working properly. 
    Detailed description.
        The task consists of the following components:
        - Responding to a user request (stored in Modbus registers) to insert or eject a Faraday cup. 
        The response involves writing to the appropriate Modbus registers (corresponding to 
        the parameters: actuator 1 control, ... actuator 3 control).
        - Measuring the duration of transient states for all actuators and checking whether timeouts 
        have been exceeded. A transient state is defined as the time from the moment the actuator control 
        changes until the new position of the cup and its associated limit switches is established.
        *******
        List of items that need to be clarified:
        - list of Modbus registers,
        - list of rules.
        *******
    Implementation and testing progress.
        Not implemented.

3. Periodic measurement of currents.
    Brief description.
        Control the multiplexer that selects the Faraday cup according to the Modbus register “active Faraday
        cup”. Periodic measurement of the currents flowing through the electrodes of an active Faraday cup. 
        Selection of the appropriate amplification.
    Detailed description.
        
    Implementation and testing progress.


3.1. Control of a 2-bit Faraday cup selection multiplexer, according to the value of the ActiveFaradayCup 
     variable, set by the Modbus master (i.e., by the user). 
3.2. Cyclic measurements of currents in the active cup; currents from 3 or 4 electrodes are measured.
3.2.1.  Control of a 2-bit 
This task is not implemented.

4. Calculation of ionic currents

6. Execution of user commands transmitted via 1-bit Modbus registers (coils).

7. Support for debug mode (without connection to external devices: 

```


