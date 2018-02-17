;**********************************************************************
;                                                                     *
;    Description:                                                     *
;                                                                     *
;    Controller for multiple aspect colour light signal and           *
;    occupation block with positional train detector at block exit.   *
;                                                                     *
;    Controller provides drive to emitter and accepts input from      *
;    sensor of a reflective IR detector. Correspondance is tallied so *
;    that high and low watermarks can be used to deem when detection  *
;    takes place. Duration of detection is stretched by approximately *
;    one second to accomodate gaps between vehicles.                  *
;                                                                     *
;    Detection is indicated using an open drain output which is read  *
;    back so that an alternative detector can be connected as a wired *
;    or in addition, or as an alternative, to the built in detection. *
;                                                                     *
;    Local detection is sent to the next, in advance, controller and  *
;    hence received by this controller from the previous, in rear,    *
;    controller.                                                      *
;                                                                     *
;    The local block occupation state machine uses the local, exit,   *
;    and previous, entry, detection to determine if:                  *
;        The block is clear;                                          *
;        A train is entering the block in the forward direction;      *
;        A train is completely contained within the block;            *
;        A train is leaving the block in the forward direction;       *
;        A train is spanning the block from entry to exit;            *
;        A train is entering the block in the reverse direction;      *
;        A train is leaving the block in the reverse direction.       *
;                                                                     *
;    If no data is being received from the previous, in rear,         *
;    controller the block occupation state machine is not executed as *
;    there is be no block entry detection available.                  *
;                                                                     *
;    The block occupation state machine can automatically determine   *
;    reverse running is taking place through the local block. Also a  *
;    line reversed input allows this condition to be forced.          *
;                                                                     *
;    The block reversed condition is sent to both the next, in        *
;    advance, and previous, in rear, controllers. This allows the     *
;    reversal condition, and restoration to forward running, to be    *
;    propogated back through a chain of controllers. There is a       *
;    bidirectional input which stops this propogation at any          *
;    controller at which the input is not set.                        *
;                                                                     *
;    The controller determines the aspects to be displayed by the     *
;    signal guarding the local occupation block but that signal is    *
;    located at the previous, in rear, controller and the information *
;    is sent to that controller for output. Conversely the controller *
;    receives aspect information to be out put to the local signal    *
;    from the next, in advance, controller.                           *
;                                                                     *
;    If no data is being received from the next, in advance,          *
;    controller then the sequencing of aspects to display after a     *
;    train exits the occupation block in the forward direction is     *
;    simulated.                                                       *
;                                                                     *
;    Alternatively the input for the link to the next, in advance,    *
;    controller can be used to force display of a stop aspect by the  *
;    local signal.                                                    *
;                                                                     *
;    An input can be used to cause the aspect displayed by the local  *
;    signal to latch at stop. That is once the signal has sequenced   *
;    to a stop aspect it will not change to any other aspect unless   *
;    the input is cleared. The default for the input is clear, i.e.   *
;    the local signal is not latched at stop.                         *
;                                                                     *
;    The aspect to be displayed by the signal guarding the local      *
;    block (connected to the previous, in rear, controller) depends   *
;    on a number of factors:                                          *
;        If block is occupied, not clear, display stop;               *
;        If running is reversed in the block display stop;            *
;        Otherwise display next aspect in sequence from stop to clear *
;        after aspect received from next, in advance, controller (or  *
;        simulated if no link).                                       *
;                                                                     *
;    The aspect to be displayed by the local signal guarding the      *
;    next, in advance, block depends on a number of factors:          *
;        If signal is inhibited locally display stop;                 *
;        If signal is latched locally hold at stop once reached;      *
;        Otherwise display aspect received from next, in advance,     *
;        controller (or simulated if no link).                        *
;                                                                     *
;    There is also a special speed input which can be employed by     *
;    controller specialisations to display location specific speed    *
;    specific apsect variations.                                      *
;                                                                     *
;    Author:        Chris White                                       *
;    Company:       Monitor Computing Services Ltd.                   *
;                                                                     *
;**********************************************************************
;                                                                     *
;    Copyright (C) 2018  Monitor Computing Services Ltd.              *
;                                                                     *
;    This program is free software; you can redistribute it and/or    *
;    modify it under the terms of the GNU General Public License      *
;    as published by the Free Software Foundation; either version 2   *
;    of the License, or any later version.                            *
;                                                                     *
;    This program is distributed in the hope that it will be useful,  *
;    but WITHOUT ANY WARRANTY; without even the implied warranty of   *
;    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the    *
;    GNU General Public License for more details.                     *
;                                                                     *
;    You should have received a copy of the GNU General Public        *
;    License (http://www.gnu.org/copyleft/gpl.html) along with this   *
;    program; if not, write to:                                       *
;       The Free Software Foundation Inc.,                            *
;       59 Temple Place - Suite 330,                                  *
;       Boston, MA  02111-1307,                                       *
;       USA.                                                          *
;                                                                     *
;**********************************************************************


;**********************************************************************
; Reset vector
;**********************************************************************

            org BootVector  ; Processor reset vector

    clrf    INTCON          ; Disable interrupts
    clrf    INTCON          ; Ensure interrupts are disabled
    goto    Initialise      ; Jump to beginning of program


;**********************************************************************
; Interrupt Service Routine
;**********************************************************************

            org IntVector   ; Interrupt vector location

BeginISR
    movwf   w_isr           ; Save off current W register contents
    swapf   STATUS,W        ; Swap status register into W register
    BANKSEL TMR0            ; Ensure register page 0 is selected
    movwf   status_isr      ; Save off contents of STATUS register
    movf    FSR,W           ; Move FSR register into W register
    movwf   fsr_isr         ; Save off contents of FSR register
    movf    PCLATH,W        ; Move PCLATH register into W register
    movwf   pclath_isr      ; Save off contents of PCLATH register
    movlw   high BeginISR   ; Load ISR address high byte ...
    movwf   PCLATH          ; ... into PCLATH to set code block

    btfss   INTCON,T0IF     ; Skip if TMR0 overflow interrupt ...
    goto    ExitISR         ; ... else skip service routine

RunISR
    ; Re-enable the timer interrupt and reload the timer
    bcf     INTCON,T0IF     ; Reset the TMR0 Interrupt bit
    movlw   low RTCCINT + 3 ; Add 3 cycles for TMR0 write increment and inhibit
    addwf   TMR0,F          ; Reload TMR0

    ; Service next controller link
    ;******************************************************************

    SrvcLink  lnkNState, serNTimer, INTLNKDLYRX, INTLNKDLYTX, EnableTxN, InitTxN, SrvcTxN, TxBreakN, EnableRxN, InitRxN, SrvcRxN

    ; Service previous controller link
    ;******************************************************************

    SrvcLink  lnkPState, serPTimer, INTLNKDLYRX, INTLNKDLYTX, EnableTxP, InitTxP, SrvcTxP, TxBreakP, EnableRxP, InitRxP, SrvcRxP

    ; Run interrupt scaling counter for second based timing
    ;******************************************************************

    decfsz  intScCount,W    ; Decrement interrupt scaling counter into W
    movwf   intScCount      ; If result is not zero update the counter

    ; Run train detection
    ;******************************************************************

    ; Detector is designed to float at 2.5V, just above TTL off threshold and
    ; filtered to ignore low frequency signals in order to reject ambient light
    ; sources. Hence will be on if no train present (and interence from ambient
    ; light) or emitter is on and will only be off when train is present and
    ; emitter is off.

    btfss   EMTPORT,EMTBIT  ; Skip is emitter is off (active low)...
    goto    EmitterOn       ; ... else ignore sensor

EmitterOff
    btfsc   SNSPORT,SNSBIT  ; Skip if sensor is off ...
    goto    SensorOn        ; ... else jump as sensor not in correspondence

SensorOff
    ; Sensing presence of train
    incf    snsAcc,W        ; Increment sensor match accumulator into W
    movwf   snsAcc          ; If result is not zero update the accumulator
    btfsc   snsAcc,INPACTV  ; Skip if below on threshold ...
    bcf     DETPORT,DETBIT  ; ... else turn detector indicator on (active low)
    goto    EmitterOffEnd

SensorOn
    ; Detecting absence of train
    decf    snsAcc,W        ; Decrement sensor match accumulator into W
    btfss   STATUS,Z        ; Skip if result is zero ...
    movwf   snsAcc          ; ... else update the accumulator
    btfsc   STATUS,Z        ; Skip if above off threshold ...
    bsf     DETPORT,DETBIT  ; ... else turn detector indicator off (active low)

EmitterOffEnd
    bcf     EMTPORT,EMTBIT  ; Turn emitter on (active low)
    goto    SensorEnd

EmitterOn
    bsf     EMTPORT,EMTBIT  ; Turn emitter off (active low)

SensorEnd

    ; Output aspect display without altering non aspect bits on port
    ;******************************************************************

    call    GetAspectOutput
    iorwf   ASPPORT,F       ; Set relevant aspect output bits
    iorlw   ~ASPOUTMSK      ; Protect non aspect output bits
    andwf   ASPPORT,F       ; Clear relevant aspect output bits

    ; End Interrupt Service Routine
    ;******************************************************************

    btfsc   INTCON,T0IF     ; Skip if no TMR0 overflow during ISR ...
    goto    RunISR          ; ... else run service routine again

    ; Exit Interrupt Service Routine
    ;******************************************************************

ExitISR
    movf    pclath_isr,W    ; Retrieve copy of PCLATH register
    movwf   PCLATH          ; Restore pre-isr PCLATH register contents
    movf    fsr_isr,W       ; Retrieve copy of FSR register
    movwf   FSR             ; Restore pre-isr FSR register contents
    swapf   status_isr,W    ; Swap copy of STATUS register into W register
    movwf   STATUS          ; Restore pre-isr STATUS register contents
    swapf   w_isr,F         ; Swap pre-isr W register value nibbles
    swapf   w_isr,W         ; Swap pre-isr W register into W register

    retfie                  ; return from Interrupt


;**********************************************************************
; Subroutine to read EEPROM
;**********************************************************************

ReadEEPROM
    movwf   EEADR           ; Set address of EEPROM location to read
    BANKSEL EECON1
    bsf     EECON1,RD       ; Trigger EEPROM read
    BANKSEL EEDATA
    movf    EEDATA,W        ; Get EEPROM data read
    BANKSEL TMR0
    return


;**********************************************************************
; Main program initialisation code
;**********************************************************************

Initialise
    ; Clear I/O ports
    ;******************************************************************

    clrf    PORTA
    clrf    PORTB

    BANKSEL OPTION_REG

    ; Program I/O port bit directions
    ;******************************************************************

    movlw   PORTASTATUS
    movwf   TRISA
    movlw   PORTBSTATUS
    movwf   TRISB

    ; Set option register:
    ;******************************************************************
    ;   Prescaler assignment - watchdog timer

    clrf    OPTION_REG
    bsf     OPTION_REG,PSA

    BANKSEL TMR0

    ; Initialise ports
    ;******************************************************************

    movlw   PORTASTATUS     ; For Port A need to write one to each bit ...
    movwf   PORTA           ; ... being used for input

    bsf     EMTPORT,EMTBIT  ; Ensure detector emmitter is off (active low)
    bsf     DETPORT,DETBIT  ; Ensure detector indicator is off (active low)

    ; Initialise RAM to zero
    ;******************************************************************

    movlw   endRAM          ; Address of end of RAM
    movwf   RAM_Start       ; Ensure first byte of RAM is non zero
    movwf   FSR             ; Point indirect register to end of RAM

ClearRAM
    clrf    INDF            ; Clear byte of RAM addressed by FSR
    decf    FSR,F           ; Decrement FSR to next byte of RAM
    movf    RAM_Start,F     ; Test first byte of RAM
    btfss   STATUS,Z        ; Skip if byte of RAM now zero ...
    goto    ClearRAM        ; ... else continue to clear RAM

    incf    snsAcc,F        ; Prevent accumulator rollover down through zero

    ; Inputs are active low so initialise debounce for all off
    comf    debnce,F
    comf    inputs,F

    bsf     lnkPState,LNKDIRFLG ; Initially send to previous controller

    ; Initialise timing
    ;******************************************************************

    movlw   low EEaspectTime
    call    ReadEEPROM
    movwf   aspectTime      ; Initialise aspect interval for next signal
    movwf   nxtTimer        ; Initialise timer used to simulate next signal

    movlw   INTSCLNG        ; Initialise interrupt ...
    movwf   intScCount      ; ...  scaling counter
    movlw   low SECSCLNG    ; Initialise one second ...
    movwf   secCount        ; ... scaled interrupts counter

    ; Optional initialisation (user code should define UserInit macro)
    ;******************************************************************
#ifdef UserInit
    UserInit
#endif

    ; Initialise interrupts
    ;******************************************************************

    movlw   low RTCCINT
    movwf   TMR0            ; Initialise TMR0 for timer interrupts
    clrf    INTCON          ; Disable all interrupt sources
    bsf     INTCON,T0IE     ; Enable TMR0 interrupts
    bsf     INTCON,GIE      ; Enable interrupts


;**********************************************************************
; Top of main processing loop
;**********************************************************************

Main
    clrwdt                  ; Reset the watchdog timer

    ;******************************************************************
    ; Perform timing operations
    ;******************************************************************

Timing
    ; To keep the interrupt service routine as brief as possible timing is
    ; performed by the interrupt service routing decrementing a counter until
    ; it reaches 1.  Here in the main program loop (i.e. outside the interrupt
    ; service routine) the count is tested and if found to be 1 it is reset
    ; and the various timing operations are performed.

    ; Scale interrupts to lower than 10KHz
    ;******************************************************************

    decfsz  intScCount,W    ; Test interrupts scaling counter
    goto    TimingEnd       ; Skip if a interrupt scaling has not elapsed

    movlw   INTSCLNG        ; Reload interrupt ...
    movwf   intScCount      ; ...  scaling counter

    ; Perform input debouncing, bit state must be steady for two cycles
    ;******************************************************************

    movf    PORTB,W         ; Get current port B bits
    andlw   INPMSK          ; Isolate input bits
    movwf   FSR
    btfsc   DETPORT,DETBIT  ; Bring in detecting input, open drain output
    bsf     FSR,DETBIT

    ; First of all clear all input bits matching last read (on or off)
    ; leaving unchanged any inputs bits not matching last read
    movf    FSR,W           ; Get current read
    xorwf   debnce,W        ; Create mask for inputs changed since last read
    andwf   inputs,F        ; Clear inputs bits unchanged since last read

    ; Next set any input bits which were on for both current and last read
    ; leaving unchanged any inputs where current value doesn't match last read
    xorlw   0xFF            ; Create mask for inputs unchanged since last read
    andwf   FSR,W           ; Get current port bits on for both reads
    iorwf   inputs,F        ; Set inputs on for both reads

    movf    FSR,W           ; Save current read ...
    movwf   debnce          ; ... as last read

    ; Run link reception timeouts
    ;******************************************************************

    decfsz  lnkPTimer,W     ; Check previous controller link reception timeout
    movwf   lnkPTimer       ; Update timeout counter if not yet timed out

    decfsz  lnkNTimer,W     ; Check next controller link reception timeout
    movwf   lnkNTimer       ; Update timeout counter if not yet timed out

    ; Run one second timing
    ;******************************************************************

    decfsz  secCount,F      ; Decrement seconds scaled interrupts counter ...
    goto    TimingEnd       ; ... skipping this jump if it has reached zero

    movlw   low SECSCLNG    ; Reload one second ...
    movwf   secCount        ; ... scaled interrupts counter

    decfsz  nxtTimer,W      ; Decrement next signal simulation timer into W
    movwf   nxtTimer        ; Update timeout counter if not yet timed out

    ; Check status of train detection input (active low) for off. This
    ; is only done once a second so detection can be stretched to
    ; ignore any gaps between vehicles.
    ;******************************************************************

    btfss   inputs,DETBIT   ; Skip if detection input is off (active low) ...
    goto    TimingEnd       ; ... else skip stretching of exit detection

    btfsc   lclCntlr,STRFLG ; Skip if exit detection has not been stretched ...
    bcf     lclCntlr,EXTFLG ; ... else set exit detection state to off

    bsf     lclCntlr,STRFLG ; Set exit detection stretched flag

TimingEnd


    ;******************************************************************
    ; Check status of train detection input (active low) for on
    ;******************************************************************

    btfss   inputs,DETBIT   ; Skip if detection input is off (active low) ...
    bsf     lclCntlr,EXTFLG ; ... else set exit detection state to on
    btfss   inputs,DETBIT   ; Skip if detection input is off (active low) ...
    bcf     lclCntlr,STRFLG ; ... else clear exit detection stretched flag


    ;******************************************************************
    ; Run local occupation block state machine
    ;******************************************************************

    btfsc   lnkPState,NOPFLG ; Skip if receiving data from previous link ...
    goto    BlockEnd         ; ... else don't run block logic

    movlw   high BlockTable ; Load jump table address high byte ...
    movwf   PCLATH          ; ... into PCLATH to make jump in same code block
    movf    lclCntlr,W      ; Use current state value ...

BlockStateJump
    andlw   BLKSTATE        ; ... (after removing flag and aspect bits) ...
    addwf   PCL,F           ; ... as offset into state jump table

BlockTable
    goto    BlockClear      ; State 0 - Block clear
    goto    TrainEnteringF  ; State 1 - Train entering forward
    goto    TrainInBlock    ; State 2 - Train in block
    goto    TrainLeavingF   ; State 3 - Train leaving forward
    goto    TrainSpansBlock ; State 4 - Train spanning block
    goto    TrainEnteringR  ; State 5 - Train entering reverse
    goto    TrainLeavingR   ; State 6 - Train leaving reverse
    goto    BlockEnd        ; State 7 - Unused

#if (high BlockTable) != (high $)
    error "Block state jump table spans 8 bit boundary"
#endif

NewBlkState
    iorwf   lclCntlr,F      ; Set at least the desired state
    iorlw   ~BLKSTATE       ; Protect non state bits
    andwf   lclCntlr,F      ; Narrow to the desired state
    goto    BlockStateJump  ; Go directly to the new state


BlockClear      ; **** State 0 - Block clear ****

CheckNtrRev
    ; Check for train entering in reverse
    movlw   TRAINENTERINGR  ; Possible next state
    btfsc   lclCntlr,EXTFLG ; Skip if exit detection off ...
    goto    NewBlkState     ; ... else train entering in reverse, change state

CheckNtrFwd
    ; Check for train entering forwards
    btfss   lclCntlr,ENTFLG ; Skip if entry detection on ...
    goto    BlockEnd        ; ... else remain in current state

    ; Train at block entrance,
    ; next state = 1 - Train entering forward,
    incf    lclCntlr,F      ; Roll through to next state


TrainEnteringF  ; **** State 1 - Train entering forward ****

    bcf     lclCntlr,BRVFLG ; Clear local block reversed

ChkSpnFwd
    ; Check if train now spans this block
    movlw   BLOCKSPANNED    ; Possible next state
    btfsc   lclCntlr,EXTFLG ; Skip if exit detection off ...
    goto    NewBlkState     ; ... else train now spans this block, change state

CheckOccFwd
    ; Check if train now occupies this block
    btfsc   lclCntlr,ENTFLG ; Skip if entry detection off ...
    goto    BlockEnd        ; ... else remain in current state

    ; Train no longer at block entrance,
    ; next state = 2 - Train in block
    incf    lclCntlr,F      ; Roll through to next state


TrainInBlock   ; **** State 2 - Train in block ****

CheckExtRev
    ; Check for train exiting in reverse
    movlw   TRAINLEAVINGR   ; Possible next state
    btfsc   lclCntlr,ENTFLG ; Skip if entry detection off ...
    goto    NewBlkState     ; ... else train exiting in reverse, change state

CheckExtFwd
    btfss   lclCntlr,EXTFLG ; Skip if exit detection on ...
    goto    BlockEnd        ; ... else remain in current state

    ; Train detected at block exit,
    ; next state = 3 - Train leaving forward
    incf    lclCntlr,F      ; Roll through to next state


TrainLeavingF   ; **** State 3 - Train leaving forward ****

    bcf     lclCntlr,BRVFLG ; Clear local block reversed

    ; Check if train has left block
    btfsc   lclCntlr,EXTFLG ; Skip if exit detection off ...
    goto    ChkTrnRvd       ; ... else check if train now spans this block

    ; Train no longer at block exit,
    ; next state = 0 - Block clear
    movlw   BLOCKCLEAR      ; Next state
    goto    NewBlkState     ; Change state

ChkTrnRvd
    ; Check if train has changed direction and now spans this block
    btfss   lclCntlr,ENTFLG ; Skip if entry detection on ...
    goto    BlockEnd    ; ... else remain in current state

    bsf     lclCntlr,BRVFLG ; Set local block reversed

    ; Train detected at block entrance,
    ; next state = 4 - Train spanning block
    incf    lclCntlr,F      ; Roll through to next state


TrainSpansBlock    ; **** State 4 - Train spanning block ****

CheckTrvFwd
    ; Check for train traversal of block forwards
    movlw   TRAINLEAVINGF   ; Possible next state
    btfss   lclCntlr,ENTFLG ; Skip if entry detection on ...
    goto    NewBlkState     ; ... else train is leaving forwards, change state

CheckTrvRev
    ; Check for train traversal of block in reverse
    btfsc   lclCntlr,EXTFLG ; Skip if exit detection off ...
    goto    BlockEnd    ; ... else remain in current state

    ; Train no longer at block exit,
    ; next state = 6 - Train leaving reverse
    movlw   TRAINLEAVINGR   ; Next state
    goto    NewBlkState


TrainEnteringR  ; **** State 5 - Train entering reverse ****

    bsf     lclCntlr,BRVFLG ; Set local block reversed

ChkSpnRev
    ; Check if train now spans this block
    movlw   BLOCKSPANNED    ; Possible next state
    btfsc   lclCntlr,ENTFLG ; Skip if entry detection off ...
    goto    NewBlkState     ; ... else train at block entrance, change state

CheckOccRev
    ; Check if train now occupies this block
    btfsc   lclCntlr,EXTFLG ; Skip if exit detection off ...
    goto    BlockEnd    ; ... else remain in current state

    ; Train no longer at block exit,
    ; next state = 2 - Train in block
    movlw   BLOCKOCCUPIED   ; Next state
    goto    NewBlkState


TrainLeavingR   ; **** State 6 - Train leaving reverse ****

    bsf     lclCntlr,BRVFLG ; Set local block reversed

    ; Check if train has changed direction and now spans this block
    movlw   BLOCKSPANNED    ; Possible next state
    btfsc   lclCntlr,EXTFLG ; Skip if exit detection off ...
    goto    NewBlkState     ; ... else train at block exit, change state

    ; Check if train has left block
    btfsc   lclCntlr,ENTFLG ; Skip if entry detection off ...
    goto    BlockEnd        ; ... else remain in current state

    ; Train no longer at block entrance,
    ; next state = 0 - Block clear
    movlw   BLOCKCLEAR      ; Next state
    goto    NewBlkState     ; Change State

BlockEnd    ; End of signal block state machine


    ;******************************************************************
    ; Service link with next controller
    ;******************************************************************

    btfss   lnkNState,LNKDIRFLG ; Skip if replying to next controller ...
    goto    CheckNextRx         ; ... else skip over next controller send

    ; Send status reply to next controller
    ;******************************************************************

    movf    lclCntlr,W      ; Send local controller status

    ; Only three bits of status (exit detection, entry detection, and block
    ; reversed) are sent so as a simple error check these are sent in low
    ; nibble with ones complement in high nibble

    iorlw   0x0F            ; Status bits - high nibble, 0xF - low nibble

    movwf   FSR             ; Copy status bits - high nibble, 0xF - low nibble
    swapf   FSR,F           ; Swap, 0xF - high nibble, status bits - low nibble

    andlw   0xF0            ; Status bits - high nibble, 0 - low nibble
    xorwf   FSR,F           ; Complemented high nibble, original low nibble

    call    LinkTxN         ; Send data to next controller
    btfsc   STATUS,Z        ; Skip if data not sent ...
    bcf     lnkNState,LNKDIRFLG ; ... else resume listening to next controller

    goto    NextLinkEnd

CheckNextRx
    ; Look for status received from next controller
    ;******************************************************************

    call    LinkRxN         ; Check for data from next controller
    btfss   STATUS,Z        ; Skip if data received ...
    goto    NextRxEnd       ; ... else skip over next controller receive

    movwf   FSR             ; Save the received data

    ; As a simple error check received data is ignored unless same value
    ; received twice in succession

    xorwf   telemNxt,F      ; Test against last received data
    movwf   telemNxt        ; Replace last received data
    btfss   STATUS,Z        ; Skip if last and just received data match ...
    goto    NextLinkEnd     ; ... else ignore just received data

    ; Only four bits of status (aspect value, line reversed, and special speed)
    ; are received so as a simple error check these are sent in low nibble with
    ; ones complement in high nibble.

    swapf   FSR,W           ; Status bits - high nibble, complement - low nibble
    comf    FSR,F           ; Complement - high nibble, status bits - low nibble
    xorwf   FSR,F           ; Exclusive or received data with complement
    btfss   STATUS,Z        ; Skip if result is zero, i.e. data is ok ...
    goto    NextLinkEnd     ; ... else ignore received data

    movwf   nxtCntlr        ; Save received next controller status

    bsf     lnkNState,LNKDIRFLG ; Start replying to next controller

    MaxLw   INTLINKTMON, 1
    movwf   lnkNTimer       ; Reset link reception timeout

    ; Action line reversal state propogated from next controller
    ;******************************************************************

    btfsc   nxtCntlr,LRVFLG ; Skip if next controller line not reversed ...
    bsf     lclCntlr,BRVFLG ; ... else set local block reversed

    btfsc   nxtCntlr,LRVFLG ; Skip if next controller line not reversed ...
    goto    NextLinkEnd     ; ... else leave local block reversed

    movlw   BLKSTATE
    andwf   lclCntlr,W      ; Test block state of local block
    btfsc   STATUS,Z        ; Skip if local block not clear ...
    bcf     lclCntlr,BRVFLG ; ... else clear local block reversed

    goto    NextLinkEnd

NextRxEnd

    ; Test if next controller link has timedout
    ;******************************************************************

    decf    lnkNTimer,W     ; Check link reception timeout
    btfsc   STATUS,Z        ; Skip if link not timedout ...
    goto    NextLinkFailed  ; ... else handle link failure

    ; Signal inhibit input cannot be read until link has timed out
    bsf     inputs,INHBIT   ; Set inhibit input off (active low)
    bsf     debnce,INHBIT   ; Set inhibit input debounce off (active low)

    goto    NextLinkEnd     ; ... else keep waiting for data


NextLinkFailed
    ; Next controller link timed out, simulate next signal sequencing
    ;******************************************************************

    bcf     nxtCntlr,LRVFLG ; Clear next block line reversed flag
    bcf     nxtCntlr,SPDFLG ; Clear next signal special speed flag

    btfss   lclCntlr,BRVFLG ; Ignore exit detection if local block reversed ...
    btfsc   inputs,DETBIT   ; ... skip if exit detection on (active low) ...
    goto    RunNextSignal   ; ... else run next signal simulation

    ; Exit detection whilst normal running so set simulated next signal aspect
    ; value to stop and reset the aspect timer to simulate train traversing
    ; next block
    movlw   ~ASPMSK
    andwf   nxtCntlr,F
    goto    TimeNextSignal

RunNextSignal
    decfsz  nxtTimer,W      ; Test if aspect timer elapsed ...
    goto    NextLinkEnd     ; ... else skip next signal sequencing

    ; Simulate next signal changing aspect
    movlw   ASPINCR
    addwf   nxtCntlr,W      ; Increment to next aspect value
    btfss   STATUS,C        ; Skip if overflow, already showing clear aspect ...
    movwf   nxtCntlr        ; ... else store new aspect value

TimeNextSignal
    ; Load aspect timer for the duration of the new aspect
    movf    aspectTime,W
    movwf   nxtTimer

NextLinkEnd


    ;******************************************************************
    ; Process local signal aspect display
    ;******************************************************************

    ; Unless the local signal is latched or inhibited this controller displays
    ; the received, or simulated, signal aspect for the next block

    movlw   ASPMSK
    andwf   aspVal,W        ; Test local signal aspect value
    btfss   STATUS,Z        ; Skip if local signal aspect value = stop ...
    goto    SetLocalAspect  ; ... otherwise set local aspect

    btfss   inputs,LCHBIT   ; Skip if latch input is off (active low) ...
    goto    LocalAspectDone ; ... otherwise leave local aspect = stop

SetLocalAspect
    movlw   ASPMSK
    andwf   nxtCntlr,W      ; Get local signal aspect from next controller
    btfss   inputs,INHBIT   ; Skip if signal is not inhibited (active low) ...
    clrw                    ; ... otherwise set aspect = stop

LocalAspectDone
    movwf   aspVal          ; Save aspect display value


    ;******************************************************************
    ; Line reversal can be from input or local block reversal
    ;******************************************************************

    btfsc   lclCntlr,BRVFLG ; Skip if local block not reversed ...
    bcf     inputs,REVBIT   ; ... else set line reversed (active low)

    btfss   inputs,BIDBIT   ; Skip if line bidirectional ...
    bsf     inputs,REVBIT   ; ... else clear line reversed (active low)


    ;******************************************************************
    ; Service link with previous controller
    ;******************************************************************

    btfss   lnkPState,LNKDIRFLG ; Skip if not waiting on reply from previous
    goto    CheckPrevRx         ; ... else skip over previous controller send

    ; Send status to previous controller
    ;******************************************************************

    ; Local block's signal aspect value (displayed by previous controller)
    ; depends on the aspect value of the local signal, unless the local block
    ; is occupied (not clear) or the line is reversed.

    movlw   BLKSTATE
    andwf   lclCntlr,W      ; Test block state of local block
    btfss   STATUS,Z        ; Skip if local block clear ...
    goto    SendAspects     ; ... else signal for local block displays stop

    movlw   ASPINCR         ; Increment aspect value ...
    addwf   aspVal,W        ; ... of local signal into W
    btfsc   STATUS,C        ; Skip if no overflow ...
    movlw   ASPCLR          ; ... else send clear aspect

SendAspects
    andlw   ASPMSK          ; Clear bits other than aspect value to send
    btfss   inputs,REVBIT   ; Skip if line not (active low) reversed ...
    movlw   LRVMSK          ; ... else propagate this and display stop aspect

    btfss   inputs,SPDBIT   ; Skip if not special speed (active low) ...
    iorlw   SPDMSK          ; ... else propagate this to previous controller

    ; Only aspect value, line reversed, and special speed are sent so as a
    ; simple error check these are sent in low nibble with ones complement in
    ; high nibble.

    iorlw   0x0F            ; Status bits - high nibble, 0xF - low nibble

    movwf   FSR             ; Copy status bits - high nibble, 0xF - low nibble
    swapf   FSR,F           ; Swap, 0xF - high nibble, status bits - low nibble

    andlw   0xF0            ; Status bits - high nibble, 0 - low nibble
    xorwf   FSR,F           ; Complemented high nibble, original low nibble

    call    LinkTxP         ; Send data to previous block
    btfss   STATUS,Z        ; Skip if data was sent ...
    goto    PrevLinkEnd     ; ... else continue trying to send

    bcf     lnkPState,LNKDIRFLG ; Start waiting on reply from previous

    MaxLw   INTLINKTMOP, 1
    movwf   lnkPTimer       ; Start reception timeout

CheckPrevRx
    ; Check for status reply from previous controller
    ;******************************************************************

    decf    lnkPTimer,W     ; Check link reception timeout
    btfsc   STATUS,Z        ; Skip if link not timedout ...
    bsf     lnkPState,NOPFLG ; ... else set no reception from previous link flag
    btfsc   STATUS,Z        ; Skip if link not timedout ...
    goto    PrevRxDone      ; ... else resume sending to previous controller

    call    LinkRxP         ; Check for data from previous controller
    btfss   STATUS,Z        ; Skip if data received ...
    goto    PrevLinkEnd     ; ... else continue waiting for reply

    movwf   FSR             ; Store the received data

    ; As a simple error check received data is ignored unless same value
    ; received twice in succession

    xorwf   telemPrv,F      ; Test against last received data
    movwf   telemPrv        ; Replace last received data
    btfss   STATUS,Z        ; Skip if last and just received data match ...
    goto    PrevLinkEnd     ; ... else ignore just received data

    ; Only three bits of status (entry detection, exit detection, and block
    ; reversed) are received so as a simple error check these are sent in low
    ; nibble with ones complement in high nibble

    swapf   FSR,W           ; Status bits - high nibble, complement - low nibble
    comf    FSR,F           ; Complement - high nibble, status bits - low nibble
    xorwf   FSR,F           ; Exclusive or received data with complement
    btfss   STATUS,Z        ; Skip if result is zero, i.e. data is ok ...
    goto    PrevLinkEnd      ; ... else ignore just received data

    bcf     lnkPState,NOPFLG ; Clear no reception from previous link flag
    movwf   FSR             ; Store the received status for bit tests

    ; Previous exit detection is local entry detection
    bcf     lclCntlr,ENTFLG ; Assume entry detector is off
    btfsc   FSR,EXTFLG      ; Skip if previous exit detector is off ...
    bsf     lclCntlr,ENTFLG ; ... else entry detector is on

    btfss   FSR,BRVFLG      ; Skip if previous block reversed ...
    bcf     lclCntlr,BRVFLG ; ... else clear local block reversed

PrevRxDone
    bsf     lnkPState,LNKDIRFLG ; Resume sending to previous controller

PrevLinkEnd


    ;******************************************************************
    ; Optional main loop code (user code should define UserMain macro)
    ;******************************************************************
#ifdef UserMain
    UserMain
#endif


    ;******************************************************************
    ; End of main processing loop
    ;******************************************************************
    goto    Main


;**********************************************************************
; Instance next block interface routine macros
;**********************************************************************

EnableRxN   EnableRx  RXNTRIS, RXNPORT, RXNBIT
    return

InitRxN     InitRx  srlIfStat, serNTimer, serNBitCnt, serNReg, RXNFLG, RXNERR, RXNBREAK, RXNSTOP
    return

SrvcRxN     ServiceRx  srlIfStat, serNTimer, serNBitCnt, serNReg, serNBffr, RXNPORT, RXNBIT, INTSERINI, INTSERBIT, RXNERR, RXNBREAK, RXNSTOP, RXNFLG

SerRxN      SerialRx  srlIfStat, serNBffr, RXNFLG

EnableTxN   EnableTx  TXNTRIS, TXNPORT, TXNBIT
    return

InitTxN     InitTx  srlIfStat, serNTimer, serNBitCnt, serNReg, TXNFLG, TXNBREAK
    return

TxBreakN    TxBreak  srlIfStat, TXNBREAK
    return

SrvcTxN     ServiceTx  srlIfStat, serNTimer, serNBitCnt, serNReg, serNBffr, TXNPORT, TXNBIT, RXNPORT, RXNBIT, INTSERBIT, TXNFLG, TXNBREAK

SerTxN      SerialTx  srlIfStat, serNBffr, TXNFLG

LinkRxN     LinkRx  lnkNState, SerRxN

LinkTxN     LinkTx  lnkNState, SerTxN


;**********************************************************************
; Instance previous block interface routine macros
;**********************************************************************

EnableRxP   EnableRx  RXPTRIS, RXPPORT, RXPBIT
    return

InitRxP     InitRx  srlIfStat, serPTimer, serPBitCnt, serPReg, RXPFLG, RXPERR, RXPBREAK, RXPSTOP
    return

SrvcRxP     ServiceRx  srlIfStat, serPTimer, serPBitCnt, serPReg, serPBffr, RXPPORT, RXPBIT, INTSERINI, INTSERBIT, RXPERR, RXPBREAK, RXPSTOP, RXPFLG

SerRxP      SerialRx  srlIfStat, serPBffr, RXPFLG

EnableTxP   EnableTx  TXPTRIS, TXPPORT, TXPBIT
    return

InitTxP     InitTx  srlIfStat, serPTimer, serPBitCnt, serPReg, TXPFLG, TXPBREAK
    return

TxBreakP    TxBreak  srlIfStat, TXPBREAK
    return

SrvcTxP     ServiceTx  srlIfStat, serPTimer, serPBitCnt, serPReg, serPBffr, TXPPORT, TXPBIT, RXPPORT, RXPBIT, INTSERBIT, TXPFLG, TXPBREAK

SerTxP      SerialTx  srlIfStat, serPBffr, TXPFLG

LinkRxP     LinkRx  lnkPState, SerRxP

LinkTxP     LinkTx  lnkPState, SerTxP
