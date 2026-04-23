# Project AppForFaradayCups
## Main tasks

```
The software performs several tasks:
1. The Modbus server.
2. Reading logic inputs.
3. The main state machine (FSM).
4. Auxiliary state machines that control individual Faraday cups and monitor their operation.
5. Analog measurements handler.
6. Debugging terminal handler.

Description
Add. 1  Mnemonic: ModbusServer
    Brief description.
        The Modbus server is implemented in Free Modbus module.
        It supports the Modbus RTU server via a serial port in accordance with the standard. It supports
        also the LED indicating Modbus transmission. The server (slave) supports only one Modbus device 
        address.
    Detailed description.
        The register address space is presented in the Modbus registers table.
    Implementation and testing progress.
        The task has been implemented and tested only with regard to a few Modbus registers.

Add. 2  Mnemonic: LogicInputs
    Brief description.
        The logic inputs contain the external lock signal and signals from the limit switches located near 
        the Faraday cups. The module periodically reads the logical input ports and writes the data to 
        the certain Modbus registers.
    Detailed description.
        The input ports are associated with the Modbus registers that have the entry “LogicInputs" in 
        the “Right to modify” column in the table of Modbus registers.
    Implementation and testing progress.
        Not implemented.

Add. 3  Mnemonic: MainFSM
    Brief description.
        The main state machine controls all high-level software operations. It oversees the operation of the 
        auxiliary state machines that control the individual Faraday cups. The main FSM checks whether the 
        device is working properly. 
    Detailed description.
        The main state machine operates according to the following list of rules:
        Rule 1. If the external lock is active, perform the “lock the beam” action.
        Rule 2. If any of the auxiliary FSMs is in a failure state, perform the “respond to failure” action.
        Rule 3. For all the auxiliary FSMs: if an auxiliary FSM is in a steady state other than 
                the state reqested by the user, set the request flag for that auxiliary FSM and force 
                the output port to the appropriate state; illustrative example for cup #1:
                if (Cup1Steady && (Cup1Inserted != Cup1Control)) than Cup1RequestedState = Cup1Control
    Implementation and testing progress.
        Not implemented.

Add. 4  Mnemonic: AuxiliaryFSMs
    Brief description.
        This module contains 3 auxiliary state machines that handle the installed Faraday cups (in the range
        from 1 to 3). Each of the three state machines controls the Faraday cup (inserts or withdraws it), 
        checks whether the mechanism is operating correctly, and determines whether the mechanism is in 
        a steady state or a transient state.
    Detailed description.
        Each state machine (FSM) measures the duration of a transition state and checks whether it has
        exceeded the limit. Each FSM checks the status of the limit switches assigned to a given Faraday cup.
    Implementation and testing progress.
        Not implemented.

Add. 5  Mnemonic: AnalogInputs
    Brief description.
        This module measures analog signals from a multichannel input amplifier. 
    Detailed description.
        The module controls the multiplexer that selects the Faraday cup (indicated by the ActiveCup 
        register). It periodically measures the currents flowing through the electrodes of an active Faraday 
        cup. It selects the appropriate amplification depending on the signal level. It calculates the
        normalized values of the signals.
    Implementation and testing progress.
        Not implemented.

Add. 6  Mnemonic: DebugTerminal
    Brief description.
        This module provides a debugging tool using the USB connection. It allows you displaying and 
        evaluating certain Modbus registers.
    Detailed description.
        This module acts as a kind of terminal; this tool is designed to work within the cutecom application.
    Implementation and testing progress.
        The task has been implemented and tested only with regard to a few Modbus registers.


```


