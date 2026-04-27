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
        only detected once the readings have been stable for 5 ms (it should be defined as a constant using 
        the “#define” directive).
    Input data: 
        lower layer (physical ports)
    Output data: 
        Cup1Switch, ExternalInhibition, Cup2Switch, Cup3Switch1, Cup3Switch2.
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
        Rule 2. If any module is in a failure state, perform the “respond to failure” action,
                otherwise, perform the "return to normal" action.
        Rule 3. For all the auxiliary FSMs: if an auxiliary FSM is in a steady state other than 
                the state reqested by the user, set the request flag for that auxiliary FSM and force 
                the output port to the appropriate state; illustrative example for cup #1:
                if (Cup1Steady && (Cup1Inserted != Cup1Control)) than { Cup1RequestedState = Cup1Control;
                    force the appropriate output signal }
        Rule 4. Assign the ActiveCup register the index of the first Faraday cup inserted, as viewed from 
                the ion source; if none is inserted, retain the previous value; 
        In any case, all rules must be processed.
        If the rules conflict on a particular issue, the rule with the lower number takes precedence.
        The numbering of the Faraday cups (for InstalledCups = 3) is as follows:
            Cup #1 is closest to the ion source;
            Cup #2 is between Cup #1 and Cup #3;
            Cup #3 is farthest from the ion source and closest to the cyclotron;
        "Lock the beam" action = Hold the cup closest to the cyclotron (cup #InstalledCups) in an inserted 
            position to block the ion beam.
        “Respond to failure” action = { backup ErrorCode in LastError register; update ErrorCode register;
            update ErrorStorage register }
        "Return to normal" action = { if ErrorCode!=0, then { backup ErrorCode in LastError register}; 
            update ErrorCode register }
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
        This module contains 3 auxiliary state machines (auxiliary FSM) that handle the installed Faraday 
        cups (in the range from 1 to 3). Each of the three state machines controls the Faraday cup (inserts 
        or withdraws it), checks whether the mechanism is operating correctly, and determines whether 
        the mechanism is in a steady state or a transient state.
    Detailed description.
        Each auxiliary FSM operates independently of the others. The mechanism for inserting and removing 
        individual cups can be pneumatic or motor-driven. The cup type is specified in the Cup1Type, 
        Cup2Type, Cup3Type registers.
        1.  The pneumatic mechanism has a single limit switch. When the cup is withdrawn, the signal from the 
            limit switch has the value CupNSwitch = false (where N is the cup index); otherwise, when the cup 
            is inserted, CupNSwitch = true; the actuator is a solenoid valve; the solenoid valve coil is 
            powered continuously when the cup is to be inserted; the coil is powered if CupNRequestedState = 
            true.
            The mechanism can be in one of four states:
            -   The cup is stationary and withdrawn; mechanism control signal CupNRequestedState = false; 
                limit switch signal: CupNSwitch = false;
            -   The cup is being inserted; mechanism control signal CupNRequestedState = true; 
                limit switch signal: CupNSwitch = false;
            -   The cup is stationary and inserted; mechanism control signal CupNRequestedState = true; 
                limit switch signal: CupNSwitch = true;
            -   The cup is being withdrawn; mechanism control signal CupNRequestedState = false; 
                limit switch signal: CupNSwitch = false;
        2.  The motor mechanism has two limit switches. The limit switches serve a dual purpose: they 
            transmit information about the cup position and shut off the motor, keeping the cup within 
            the permissible range of motion. The motor shut-off mechanism operates autonomously. 
            The mechanism can be in one of four states:
            -   The cup is stationary and withdrawn; mechanism control signal CupNRequestedState = false; 
                limit switch signals: CupNSwitch1 = true, CupNSwitch2 = false;
            -   The cup is being inserted; mechanism control signal CupNRequestedState = true; 
                limit switch signals: CupNSwitch1 = false, CupNSwitch2 = false;
            -   The cup is stationary and inserted; mechanism control signal CupNRequestedState = true; 
                limit switch signals: CupNSwitch1 = false, CupNSwitch2 = true;
            -   The cup is being withdrawn; mechanism control signal CupNRequestedState = false; 
                limit switch signals: CupNSwitch1 = false, CupNSwitch2 = false;
        The auxiliary FSM must measure the duration of a transition state and check whether it has exceeded 
        the limit. At every stage of the mechanism's operation, the signals from the limit switches must be 
        checked for correctness (the rules for checking correctness should be derived from the description 
        above).
    Input data:
        Cup1Switch, ExternalInhibition, Cup2Switch, Cup3Switch1, Cup3Switch2,
        Cup1RequestedState, Cup2RequestedState, Cup3RequestedState.
    Output data:
        signals on physical output ports, Cup1Error, Cup2Error, Cup3Error, 
        Cup1LastError, Cup2LastError, Cup3LastError, Cup1ErrorStorage, Cup2ErrorStorage, Cup3ErrorStorage.
    Implementation and testing progress.
        Not implemented.

Add. 5  Mnemonic: AnalogInputs
    Brief description.
        This module measures analog signals from a multichannel input amplifier. 
    Detailed description.
        The module is implemented as a simple state machine, called cyclically at a frequency of 10 Hz 
        (an approximate value that should be defined as a constant using the “#define” directive). 
        Static local variables used by the module:
            LocalActiveCup - the local equivalent of the ActiveCup register; the ActiveCup register can be 
                            modified in another module at any time; if the value of ActiveCup changes, 
                            LocalActiveCup should take on the value of ActiveCup after the current module’s 
                            cycle has completed;
            ActiveChannel - the index of the currently measured channel (in other words, the index of 
                            the electrode inside the Faraday cup whose current we want to measure); 
                            ActiveChannel is a module state variable; 
                            for example, in the case of Cup 1, the ActiveChannel variable takes on values 
                            ranging from 0 to ElectrodesInsideCup1 - 1;
        During a cyclic call, the module performs the following steps:
        1.  The module reads the values of ADC0 and ADC1 (these are digital values corresponding to the same 
            analog signal, with the difference that the analog signal in the ADC1 channel is amplified 10 
            times more than in the ADC0 channel); ADC0 and ADC1 are the special registers in 
            the microcontroller.
        2.  Analog input value is selected for further calculations, as follows:
            if ADC1 < RangeChangeThreshold, then {LargeGain = true}, else {LargeGain = false}
            if LargeGain, then { X = ADC1} else { X = ADC0}
            where LargeGain and X are module variables.
        2.  For the given values of LocalActiveCup and ActiveChannel the module calculates the electrode 
            current using the formula Y = (uint16)((FactorCoef * Factor) * (X - Offset)), where Factor is 
            selected from the registers: Cup1Channel1Gain1Factor ...Cup3Channel4Gain2Factor, and Offset is 
            selected from the registers: Cup1Channel1Gain1Offset ...Cup3Channel4Gain2Offset, 
            FactorCoef is a constant (of type "double") that should be defined using the “#define” directive; 
            the result of the calculation (that is Y) is inserted into the appropriate register from 
            the range Cup1Channel1Sample ...Cup3Channel4Sample; at this point, the unit of current has not 
            been specified (e.g., 0.1 μA), but it will be specified after the experiments are conducted.
        3.  The result of the calculation (the module variable Y) is compared with the corresponding lower 
            limit from the registers Cup1Channel1LowerLimit ...Cup3Channel4LowerLimit and with the 
            corresponding upper limit from the registers Cup1Channel1UpperLimit ...Cup3Channel4UpperLimit. 
            If the result falls within the specified limits, the corresponding error register 
            (Cup3ChannelError, Cup2ChannelError, or Cup3ChannelError) is cleared; otherwise, an error code 
            is written to the error register. 
        4.  The module begins preparing the next measurement. The variable LocalActiveCup takes on the value 
            ActiveCup as follows:
            if LocalActiveCup == ActiveCup, then {ActiveCupChanged = false} 
            else {ActiveCupChanged = true; LocalActiveCup = ActiveCup}
            where ActiveCupChanged is a module variable;
        5.  The module controls the multiplexer that selects the Faraday cup for given value of 
            LocalActiveCup.
        6.  Set the ActiveChannel value as follows:
            if ActiveCupChanged then { ActiveChannel=0 } 
            else { ActiveChannel++; if ActiveChannel==ElectrodesInsideCupN then { ActiveChannel=0 }}
        7.  The module controls a multiplexer that selects the channel (i.e., which electrode to measure 
            current from) for a given value of the ActiveChannel parameter.
    Input data:
        lower layer (physical ports),
        ActiveCup, RangeChangeThreshold, Cup1Channel1Gain1Factor ...Cup3Channel4Gain2Factor, 
        Cup1Channel1Gain1Offset ...Cup3Channel4Gain2Offset, Cup1Channel1LowerLimit ...Cup3Channel4LowerLimit
    Output data:
        Cup1Channel1Sample ...Cup3Channel4Sample
    Implementation and testing progress.
        Implemented to about 10%.

Add. 6  Mnemonic: DebugTerminal
    Brief description.
        This module provides a debugging tool using the USB connection. It allows you displaying and 
        evaluating certain Modbus registers.
    Detailed description.
        This module acts as a kind of terminal; this tool is designed to work within the cutecom application.
    Implementation and testing progress.
        The task has been implemented and tested only with regard to a few Modbus registers.

```

