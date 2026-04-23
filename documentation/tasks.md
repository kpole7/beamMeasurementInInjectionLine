# Project AppForFaradayCups
## Main tasks

```
The software performs several tasks:
1. The Modbus server.
2. Reading logic inputs.
3. High-level control.
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
        Input signals must be filtered to prevent bounce. Filtering means that a change in logic state is 
        only detected once the readings have been stable for 5 ms (the constant should be defined as 
        a preprocessor directive).
        The input ports are associated with the Modbus registers that have the entry “LogicInputs" in 
        the “Right to modify” column in the table of Modbus registers.
    Input data: physical ports.
    Output data: Cup1Switch, ExternalInhibition, Cup2Switch, Cup3Switch1, Cup3Switch2.
    Implementation and testing progress.
        Not implemented.

Add. 3  Mnemonic: HighLevelCtrl
    Brief description.
        This module controls all high-level software operations. It oversees the operation of the auxiliary 
        state machines that control the individual Faraday cups. The module checks whether the device is 
        working properly. 
    Detailed description.
        The module operates according to the following list of rules:
        Rule 1. If the external lock is active, perform the “lock the beam” action.
        Rule 2. If any of the auxiliary FSMs is in a failure state, perform the “respond to failure” action,
                otherwise, perform the "return to normal" action.
        Rule 3. For all the auxiliary FSMs: if an auxiliary FSM is in a steady state other than 
                the state reqested by the user, set the request flag for that auxiliary FSM and force 
                the output port to the appropriate state; illustrative example for cup #1:
                if (Cup1Steady && (Cup1Inserted != Cup1Control)) than { Cup1RequestedState = Cup1Control;
                    force the appropriate output signal; }
        This module reads input data from Modbus registers and writes the result of the operation to Modbus 
        registers.
    Input data:
        Cup1Control, Cup2Control, Cup3Control, ExternalInhibition,
        Cup1Error, Cup2Error, Cup3Error, Cup1Steady, Cup2Steady, Cup3Steady, 
        Cup1Inserted, Cup2Inserted, Cup3Inserted, 
    Output data:
        ErrorCode, LastError, ErrorStorage, ActiveCup, 
        Cup1RequestedState, Cup2RequestedState, Cup3RequestedState
    Implementation and testing progress.
        Not implemented.

Add. 4  Mnemonic: AuxiliaryFSMs
    Brief description.
        This module contains 3 auxiliary state machines that handle the installed Faraday cups (in the range
        from 1 to 3). Each of the three state machines controls the Faraday cup (inserts or withdraws it), 
        checks whether the mechanism is operating correctly, and determines whether the mechanism is in 
        a steady state or a transient state.
    Detailed description.
        Each finite-state machine (auxiliary FSM) operates independently of the others. The mechanism for inserting 
        and removing individual cups can be pneumatic or motor-driven.
        1.  The pneumatic mechanism has a single limit switch. When the cup is withdrawn, the signal from the 
            limit switch has the value CupNSwitch = false (where N is the cup index); otherwise, when the cup 
            is inserted, CupNSwitch = true; the actuator is a solenoid valve; the solenoid valve coil is 
            powered continuously when the cup is to be inserted; the coil is powered if CupNRequestedState = 
            true.
        2.  The motor mechanism has two limit switches. The limit switches serve a dual purpose: they 
            transmit information about the cup’s position and shut off the motor, keeping the cup within 
            the permissible range of motion. The mechanism can be in one of four states:
            -   cup withdrawn; mechanism control signal CupNRequestedState = false; 
                limit switch signals: CupNSwitch1 = true, CupNSwitch2 = false;
            -   cup slides in; mechanism control signal CupNRequestedState = true; 
                limit switch signals: CupNSwitch1 = false, CupNSwitch2 = false;
            -   cup inserted; mechanism control signal CupNRequestedState = true; 
                limit switch signals: CupNSwitch1 = false, CupNSwitch2 = true;
            -   cup slides out; mechanism control signal CupNRequestedState = false; 
                limit switch signals: CupNSwitch1 = false, CupNSwitch2 = false;
        The auxiliary FSM must measure the duration of a transition state and check whether it has exceeded 
        the limit.
        At every stage of the mechanism's operation, the signals from the limit switches must be checked for 
        correctness. 
        The rules of correctness will be updated in the future.
    Input data:

    Output data:

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
        Implemented to about 20%.

Add. 6  Mnemonic: DebugTerminal
    Brief description.
        This module provides a debugging tool using the USB connection. It allows you displaying and 
        evaluating certain Modbus registers.
    Detailed description.
        This module acts as a kind of terminal; this tool is designed to work within the cutecom application.
    Implementation and testing progress.
        The task has been implemented and tested only with regard to a few Modbus registers.


```

