; Include serial link interface macros
;  - Serial link bit timing is performed by link service routines
#define CLKD_SERIAL
#include "utility/asyn_srl.inc"
#include "utility/link_hd.inc"


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
    movwf   status_isr      ; save off contents of STATUS register
    movf    PCLATH,W        ; Move PCLATH register into W register
    movwf   pclath_isr      ; save off contents of PCLATH register
    movlw   high BeginISR   ; Load ISR address high byte ...
    movwf   PCLATH          ; ... into PCLATH to set code block

    btfss   INTCON,T0IF     ; Skip if TMR0 overflow interrupt ...
    goto    ExitISR         ; ... else skip service routine

RunISR
    ; Re-enable the timer interrupt and reload the timer
    bcf     INTCON,T0IF     ; Reset the TMR0 Interrupt bit
    movlw   (RTCCINT - 2)   ; Allow 2 cycles for TMR0 write inhibit
    addwf   TMR0,F          ; Reload TMR0

    ; Service next controller link
    ;******************************************************************

    SrvcLink  lnkNState, serNTimer, INTLNKDLYRX, INTLNKDLYTX, EnableTxN, InitTxN, SrvcTxN, TxBreakN, EnableRxN, InitRxN, SrvcRxN

    ; Service previous controller link
    ;******************************************************************

    SrvcLink  lnkPState, serPTimer, INTLNKDLYRX, INTLNKDLYTX, EnableTxP, InitTxP, SrvcTxP, TxBreakP, EnableRxP, InitRxP, SrvcRxP

    ; Run interrupt scaling counter for second timing
    ;******************************************************************

    decfsz  intScCount,W    ; Decrement interrupt scaling counter into W
    movwf   intScCount      ; If result is not zero update the counter

    ; Run detector
    ;******************************************************************

    ; Detector is designed to float at 2.5V, just above TTL off threshold
    ; so will be on if no train present or emitter is on. Will only be off
    ; when train is present and emitter is off.

    btfss   EMTPORT,EMTBIT  ; Skip is emitter is off (active low)...
    goto    EmitterOn       ; ... else ignore sensor

EmitterOff
    btfsc   SNSPORT,SNSBIT  ; Skip if sensor is off ...
    goto    SensorOn        ; ... else jump as sensor not in correspondence

SensorOff
    ; Sensing presence of train
    incfsz  snsAcc,W        ; Increment sensor match accumulator into W
    movwf   snsAcc          ; If result is not zero update the accumulator
    goto    EmitterOffEnd

SensorOn
    ; Detecting absence of train
    decfsz  snsAcc,W        ; Decrement sensor match accumulator into W
    movwf   snsAcc          ; If result is not zero update the accumulator

EmitterOffEnd
    bcf     EMTPORT,EMTBIT  ; Turn emitter on (active low)
    goto    SensorEnd

EmitterOn
    bsf     EMTPORT,EMTBIT  ; Turn emitter off (active low)

SensorEnd

    ; Output aspect display
    ;******************************************************************

    call    GetAspectMask
    iorwf   ASPPORT,F
    iorlw   ~ASPOUTMSK
    andwf   ASPPORT,F

    ; End Interrupt Service Routine
    ;******************************************************************

    btfsc   INTCON,T0IF     ; Skip if no TMR0 overflow during ISR ...
    goto    RunISR          ; ... else run service routine again

    ; Exit Interrupt Service Routine
    ;******************************************************************

ExitISR
    movf    pclath_isr,W    ; Retrieve copy of PCLATH register
    movwf   PCLATH          ; Restore pre-isr PCLATH register contents
    swapf   status_isr,W    ; Swap copy of STATUS register into W register
    movwf   STATUS          ; Restore pre-isr STATUS register contents
    swapf   w_isr,F         ; Swap pre-isr W register value nibbles
    swapf   w_isr,W         ; Swap pre-isr W register into W register

    retfie                  ; return from Interrupt

    ; Run interrupt scaling counter for second timing
    ;******************************************************************


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

    bsf     EMTPORT,EMTBIT  ; Ensure detector emmitter is off
    bsf     DETPORT,DETBIT  ; Ensure detector indicator is off

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
    movwf   EEADR           ; Set address of EEPROM location to read
    BANKSEL EECON1
    bsf     EECON1,RD       ; Trigger EEPROM read
    BANKSEL EEDATA
    movf    EEDATA,W        ; Get EEPROM data read
    BANKSEL TMR0
    movwf   aspectTime      ; Initialise aspect interval for next signal
    movwf   nxtTimer        ; Initialise timer used to simulate next signal

    ; Initialise interrupts
    ;******************************************************************

    movlw   RTCCINT
    movwf   TMR0            ; Initialise TMR0 for timer interrupts
    clrf    INTCON          ; Disable all interrupt sources
    bsf     INTCON,T0IE     ; Enable TMR0 interrupts
    bsf     INTCON,GIE      ; Enable interrupts

    ;******************************************************************
    ; Top of main processing loop
    ;******************************************************************
Main
    clrwdt                  ; Reset the watchdog timer

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

    movlw   INTSCLNG        ; Reload interrupt scaling counter
    movwf   intScCount

    ; Perform input debouncing
    ;******************************************************************

    movf    PORTA,W         ; Get current port A bits
    andlw   0xF0            ; Discard bits 0 to 3
    movwf   FSR             ; Save current bits 4 to 7

    movf    PORTB,W         ; Get current port B bits
    andlw   0x0F            ; Discard bits 4 to 7
    iorwf   FSR,F           ; Combine with saved port A bits

    ; First of all clear all input bits where current and last port reads match
    ; leaving unchanged any inputs where current doesn't match last port read
    movf    FSR,W           ; Get combined current port bits
    xorwf   debnce,W        ; Create mask for inputs changed since last read
    andwf   inputs,F        ; Clear inputs unchanged since last read

    ; Next set any input bits on for both current and last port read
    ; leaving unchanged any inputs where current doesn't match last port read
    xorlw   0xFF            ; Create mask for inputs unchanged since last read
    andwf   FSR,W           ; Get current port bits unchanged since last read
    iorwf   inputs,F        ; Set inputs unchanged since last read

    movf    FSR,W           ; Save combined current port bits ...
    movwf   debnce          ; ... as last read value for next debounce cycle

    ; Run link reception timeouts
    ;******************************************************************

    decf    lnkPTimer,W     ; Check previous controller link reception timeout
    btfss   STATUS,Z        ; Skip if link timedout ...
    movwf   lnkPTimer       ; ... else update timeout counter

    decf    lnkNTimer,W     ; Check next controller link reception timeout
    btfss   STATUS,Z        ; Skip if link timedout ...
    movwf   lnkNTimer       ; ... else update timeout counter

    ; Run one second timing
    ;******************************************************************

    decfsz  secCount,F      ; Decrement seconds scaled interrupts counter ...
    goto    TimingEnd       ; ... skipping this jump if it has reached zero

    movlw   SECSCLNG        ; Reload one second ...
    movwf   secCount        ; ... scaled interrupts counter low byte

    decfsz  nxtTimer,W      ; Decrement next signal simulation timer into W
    movwf   nxtTimer        ; If result is not zero update the timer

    ; Check status of train detection input (active low) for off
    ; (this is only done once a second so detection can be stretched to
    ;  ignore any gaps between vehicles)
    ;******************************************************************

    btfss   inputs,DETBIT   ; Skip if detection input is off (active low) ...
    goto    TimingEnd       ; ... else skip stretching of exit detection

    btfsc   lclCntlr,STRFLG ; Skip if exit detection has not been stretched ...
    bcf     lclCntlr,EXTFLG ; ... else set exit detection state to off

    bsf     lclCntlr,STRFLG ; Set exit detection stretched flag

TimingEnd

    ; Check status of train detector
    ;******************************************************************

    decf    snsAcc,W        ; Test detector correspondence accumulator
    btfsc   STATUS,Z        ; Skip if above off threshold ...
    bsf     DETPORT,DETBIT  ; ... else turn detector indicator off (active low)

    btfsc   snsAcc,INPACTV  ; Skip if below on threshold ...
    bcf     DETPORT,DETBIT  ; ... else turn detector indicator on (active low)

    ; Check status of train detection input (active low) for on
    ;******************************************************************

    btfss   inputs,DETBIT   ; Skip if detection input is off (active low) ...
    bsf     lclCntlr,EXTFLG ; ... else set exit detection state to on
    btfss   inputs,DETBIT   ; Skip if detection input is off (active low) ...
    bcf     lclCntlr,STRFLG ; ... else clear exit detection stretched flag

    ; Check status of special speed input (active low)
    ;******************************************************************

    btfsc   inputs,SPDBIT   ; Skip if special speed is on (active low) ...
    bcf     prvCntlr,SPDFLG ; ... else set speed state to normal
    btfss   inputs,SPDBIT   ; Skip if special speed is off (active low) ...
    bsf     prvCntlr,SPDFLG ; ... else set speed state to special

    ; Service link with previous controller
    ;******************************************************************

    btfss   lnkPState,LNKDIRFLG ; Skip if not waiting on reply from previous
    goto    CheckPrevRx         ; ... else skip over previous controller send

    ; Send status to previous controller
    ;******************************************************************

    ; Only four bits of signalling status need to be sent so as a simple error
    ; check these are sent in low nibble with ones complement in high nibble

    movf    prvCntlr,W      ; Get status for previous controller

    btfsc   nxtCntlr,REVFLG ; Test if next block line reversed flag is set ...
    iorlw   REVMSK          ; ... if so propagate this to previous controller

    iorlw   0x0F            ; Set up for ones complement high nibble later

    movwf   FSR             ; Save local signalling status
    swapf   FSR,F           ; Swap nibbles, signalling status - low, 0xF - high

    andlw   0xF0            ; Isolate signalling status for ones complement
    xorwf   FSR,F           ; Ones complement high and signalling status in low

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

    ; Only four bits of signalling status need to be sent so as a simple error
    ; check these are sent in low nibble with ones complement in high nibble

    movf    FSR,W           ; Signalling in low nibble, ones complement in high
    swapf   FSR,F           ; Swap nibbles of the received data
    comf    FSR,F           ; Ones complement the swapped received data
    xorwf   FSR,F           ; Exclusive or complemented and swapped data
    btfss   STATUS,Z        ; Skip if result is zero, i.e. data is ok ...
    goto    PrevLinkEnd      ; ... else ignore just received data

    andlw   0x0F            ; Clear ones complement from received data
    iorwf   prvCntlr,F      ; OR received data into previous controller status
    iorlw   0xF0            ; Protect high nibble of controller status
    andwf   prvCntlr,F      ; Mask received data in previous controller status

PrevRxDone
    bsf     lnkPState,LNKDIRFLG ; Resume sending to previous controller

PrevLinkEnd

    ; Service link with next controller
    ;******************************************************************

    btfss   lnkNState,LNKDIRFLG ; Skip if replying to next controller ...
    goto    CheckNextRx         ; ... else skip over next controller send

    ; Send status reply to next controller
    ;******************************************************************

    ; Only four bits of signalling status need to be sent so as a simple error
    ; check these are sent in low nibble with ones complement in high nibble

    movf    lclCntlr,W      ; Get local controller status

    iorlw   0x0F            ; Set up for ones complement high nibble later

    movwf   FSR             ; Save local signalling status
    swapf   FSR,F           ; Swap nibbles, signalling status - low, 0xF - high

    andlw   0xF0            ; Isolate signalling status for ones complement
    xorwf   FSR,F           ; Ones complement high and signalling status in low

    call    LinkTxN         ; Send data to next controller
    btfss   STATUS,Z        ; Skip if data sent ...
    goto    NextLinkEnd     ; ... else continue trying to send

    bcf     lnkNState,LNKDIRFLG ; Resume listening to next controller

CheckNextRx
    ; Look for status received from next controller
    ;******************************************************************

    call    LinkRxN         ; Check for data from next controller
    btfss   STATUS,Z        ; Skip if data received ...
    goto    NextRxEnd       ; ... else skip over next controller receive

    movwf   FSR             ; Store the received data

    ; As a simple error check received data is ignored unless same value
    ; received twice in succession

    xorwf   telemNxt,F      ; Test against last received data
    movwf   telemNxt        ; Replace last received data
    btfss   STATUS,Z        ; Skip if last and just received data match ...
    goto    NextLinkEnd     ; ... else ignore just received data

    ; Only four bits of signalling status need to be sent so as a simple error
    ; check these are sent in low nibble with ones complement in high nibble

    swapf   FSR,W           ; Signalling in high nibble, ones complement in low
    comf    FSR,F           ; Ones complement the received data
    xorwf   FSR,F           ; Exclusive or complemented and swapped data
    btfss   STATUS,Z        ; Skip if result is zero, i.e. data is ok ...
    goto    NextLinkEnd     ; ... else ignore received data

    movwf   nxtCntlr        ; Save received data as next controller status

    bsf     lnkNState,LNKDIRFLG ; Start replying to next controller

    MaxLw   INTLINKTMON, 1
    movwf   lnkNTimer       ; Reset link reception timeout

    goto    NextLinkEnd

NextRxEnd

    ; Test if next controller link has timedout
    ;******************************************************************

    decf    lnkNTimer,W     ; Check link reception timeout
    btfsc   STATUS,Z        ; Skip if link not timedout ...
    goto    NextLinkFailed  ; ... else handle link failure

    ; Signal inhibit input cannot be read until link has timed out
    bcf     lclCntlr,INHFLG
    bsf     inputs,INHBIT
    bsf     debnce,INHBIT

    goto    NextLinkEnd     ; ... else keep waiting for data


NextLinkFailed
    ; Next controller link timed out, simulate it
    ;******************************************************************

    bcf     nxtCntlr,REVFLG ; Clear next block line reversed flag
    bcf     nxtCntlr,SPDFLG ; Clear next signal special speed flag

    decfsz  nxtTimer,W      ; Test if aspect timer elapsed ...
    goto    NextSignalEnd   ; ... else skip next signal sequencing

    ; Simulate signal changing aspect
    movlw   ASPINCR
    addwf   nxtCntlr,W      ; Increment to next aspect value
    btfss   STATUS,C        ; Skip if overflow, already showing 'green' ...
    movwf   nxtCntlr        ; ... else store new aspect value

    ; Load aspect timer for the duration of the new aspect
    movf    aspectTime,W
    movwf   nxtTimer

NextSignalEnd

    ; Next controller link timed out, check signal inhibit input (active low)
    ;******************************************************************

    btfsc   inputs,INHBIT   ; Skip if inhibit input is on (active low) ...
    bcf     lclCntlr,INHFLG ; ... else set signal inhibit to off
    btfss   inputs,INHBIT   ; Skip if inhibit input is off (active low) ...
    bsf     lclCntlr,INHFLG ; ... else set signal inhibit to on

NextLinkEnd

    ; Unless the local signal is inhibited or latched this controller normally
    ; displays the signal aspect for the next block (received or simulated)

    movlw   ASPSTATE
    andwf   lclCntlr,W      ; Test local signal aspect value
    btfss   STATUS,Z        ; Skip if local signal aspect value = stop ...
    goto    SetLocalAspect  ; ... otherwise set local aspect

    btfss   inputs,LCHBIT   ; Skip if latch input is off (active low) ...
    goto    SkipLocalAspect ; ... otherwise skip setting local aspect

SetLocalAspect
    movlw   ~ASPSTATE
    andwf   lclCntlr,F      ; Clear local signal aspect value bits

    movlw   ASPSTATE
    andwf   nxtCntlr,W      ; Get local signal aspect from next controller

    btfss   lclCntlr,INHFLG ; Skip if signal is inhibited ...
    iorwf   lclCntlr,F      ; ... else use the signal aspect for display

SkipLocalAspect

    ; This block's signal aspect value (displayed by previous controller)
    ; depends on the aspect value of the local signal.

    movlw   ~ASPSTATE
    andwf   prvCntlr,F      ; Clear signal aspect value bits (= stop aspect)

    movlw   ASPINCR
    addwf   lclCntlr,W      ; Increment local signal aspect value into W
    btfsc   STATUS,C        ; Skip if no overflow ...
    movlw   ASPCLR          ; ... else set for clear aspect
    andlw   ASPSTATE        ; Isolate new aspect value bits   

    btfss   nxtCntlr,REVFLG ; Skip if next block is line reversed ...
    iorwf   prvCntlr,F      ; ... else set new aspect value

    ; Run this occupation block state machine
    ;******************************************************************

    ; The signal aspect for this block is dependant on the aspect of the next
    ; signal but for the purpose of this block it doesn't matter if these have
    ; been received or simulated.

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

NewBlkState
    iorwf   lclCntlr,F      ; Set at least the desired state
    iorlw   ~BLKSTATE       ; Protect non state bits
    andwf   lclCntlr,F      ; Narrow to the desired state
    goto    BlockStateJump  ; Go directly to the new state


BlockClear      ; State 0 - Block clear

    bcf     prvCntlr,REVFLG ; Clear line reversed flag for this block

CheckNtrRev
    ; Check for train entering in reverse
    movlw   TRAINENTERINGR
    btfsc   lclCntlr,EXTFLG ; Skip if exit detection off ...
    goto    NewBlkState     ; ... else train entering in reverse, change state

CheckNtrFwd
    ; Check for train entering forwards
    btfss   prvCntlr,ENTFLG ; Skip if entry detection on ...
    goto    BlockEnd        ; ... else remain in current state

    ; Train at block entrance,
    ; next state = 1 - Train entering forward,
    incf    lclCntlr,F


TrainEnteringF  ; State 1 - Train entering forward

ChkSpnFwd
    ; Check if train now spans this block
    movlw   BLOCKSPANNED
    btfsc   lclCntlr,EXTFLG ; Skip if exit detection off ...
    goto    NewBlkState     ; ... else train now spans this block, change state

CheckOccFwd
    ; Check if train now occupies this block
    btfsc   prvCntlr,ENTFLG ; Skip if entry detection off ...
    goto    BlockOccupied   ; ... else remain in current state

    ; Train no longer at block entrance,
    ; next state = 2 - Train in block
    incf    lclCntlr,F


TrainInBlock   ; State 2 - Train in block

CheckExtRev
    ; Check for train exiting in reverse
    movlw   TRAINLEAVINGR
    btfsc   prvCntlr,ENTFLG ; Skip if entry detection off ...
    goto    NewBlkState     ; ... else train exiting in reverse, change state

CheckExtFwd
    btfss   lclCntlr,EXTFLG ; Skip if exit detection on ...
    goto    BlockOccupied   ; ... else remain in current state

    ; Train detected at block exit,
    ; next state = 3 - Train leaving forward
    incf    lclCntlr,F


TrainLeavingF   ; State 3 - Train leaving forward

    ; Check if train has left block
    btfsc   lclCntlr,EXTFLG ; Skip if exit detection off ...
    goto    ChkTrnRvd       ; ... else check if train now spans this block

    ; Train has left block

    ; In case simulating next controller set next signal aspect value to red
    ; and reset the aspect timer to simulate train traversing next block
    movlw   ~ASPSTATE
    andwf   lclCntlr,F      ; Ensure local signal continues to display stop
    andwf   nxtCntlr,F

    movf    aspectTime,W
    movwf   nxtTimer

    ; Train no longer at block exit,
    ; next state = 0 - Block clear
    movlw   BLOCKCLEAR
    goto    NewBlkState     ; Change state

ChkTrnRvd
    ; Check if train has changed direction and now spans this block
    btfss   prvCntlr,ENTFLG ; Skip if entry detection on ...
    goto    BothOccupied    ; ... else remain in current state

    bsf     prvCntlr,REVFLG ; Set line reversed flag for this block

    ; Train detected at block entrance,
    ; next state = 4 - Train spanning block
    incf    lclCntlr,F


TrainSpansBlock    ; State 4 - Train spanning block

CheckTrvFwd
    ; Check for train traversal of block forwards
    movlw   TRAINLEAVINGF
    btfss   prvCntlr,ENTFLG ; Skip if entry detection on ...
    goto    NewBlkState     ; ... else train is leaving forwards, change state

CheckTrvRev
    ; Check for train traversal of block in reverse
    btfsc   lclCntlr,EXTFLG ; Skip if exit detection off ...
    goto    BothOccupied    ; ... else remain in current state

    ; Train no longer at block exit,
    ; next state = 6 - Train leaving reverse
    movlw   TRAINLEAVINGR
    goto    NewBlkState


TrainEnteringR  ; State 5 - Train entering reverse

    bsf     prvCntlr,REVFLG ; Set line reversed flag for this block

ChkSpnRev
    ; Check if train now spans this block
    movlw   BLOCKSPANNED
    btfsc   prvCntlr,ENTFLG ; Skip if entry detection off ...
    goto    NewBlkState     ; ... else train at block entrance, change state

CheckOccRev
    ; Check if train now occupies this block
    btfsc   lclCntlr,EXTFLG ; Skip if exit detection off ...
    goto    BothOccupied    ; ... else remain in current state

    ; Train no longer at block exit,
    ; next state = 2 - Train in block
    movlw   BLOCKOCCUPIED
    goto    NewBlkState


TrainLeavingR   ; State 6 - Train leaving reverse

    bsf     prvCntlr,REVFLG ; Set line reversed flag for this block

    ; Check if train has changed direction and now spans this block
    movlw   BLOCKSPANNED
    btfsc   lclCntlr,EXTFLG ; Skip if exit detection off ...
    goto    NewBlkState     ; ... else train at block exit, change state

    ; Check if train has left block
    btfsc   prvCntlr,ENTFLG ; Skip if entry detection off ...
    goto    BlockOccupied   ; ... else remain in current state

    ; Train no longer at block entrance,
    ; next state = 0 - Block clear
    movlw   BLOCKCLEAR
    goto    NewBlkState     ; Change State

BothOccupied ; End of block state machine, next & this blocks occupied
    movlw   ~ASPSTATE
    andwf   lclCntlr,F      ; Clear next block's signal aspect value (= stop)

BlockOccupied ; End of block state machine, this block occupied
    movlw   ~ASPSTATE
    andwf   prvCntlr,F      ; Clear this block's signal aspect value (= stop)

BlockEnd    ; End of signal block state machine

    ; Get aspect display value
    ;******************************************************************

    movf    lclCntlr,W      ; Get controller status
    andlw   ASPSTATE        ; Isolate aspect value
    movwf   aspOut          ; Save as aspect display value


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
