;**********************************************************************
;                                                                     *
; Author: Chris White (whitecf69@gmail.com)                           *
;                                                                     *
; Copyright (C) 2018 by Monitor Computing Services Limited, licensed  *
; under CC BY-NC-SA 4.0. To view a copy of this license, visit        *
; https://creativecommons.org/licenses/by-nc-sa/4.0/                  *
;                                                                     *
; This program is distributed in the hope that it will be useful, but *
; WITHOUT ANY WARRANTY; without even the implied warranty of          *
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                *
;                                                                     *
;**********************************************************************


;**********************************************************************
; Variable registers
;**********************************************************************

            cblock  RAM0_Start

; Status and accumulator storage during interrupt
w_isr           ; 'w' register, accumulator, store during ISR
status_isr      ; STATUS register store during ISR
fsr_isr         ; FSR register store during ISR
pclath_isr      ; PCLATH register store during ISR


; Serial interface
srlIfStat       ; Serial I/F status flags (see 'asyn_srl.inc')


; Next controller interface
serNTimer       ; Interrupt counter for serial bit timing
serNBitCnt      ; Bit down counter
serNReg         ; Data shift register
serNBffr        ; Data byte buffer

lnkNTimer       ; Rx timeout timer
lnkNState       ; Link state register (see 'link_hd.inc')
                ;   bits 0,1 - Current state
                ;     0  - Switching to Rx
                ;     1  - Receiving data
                ;     2  - Waiting for far end to turn around
                ;     3  - Transmiting data
                ;   bits 2,3 - Unused
                ;   bit  4 - Tx enabled
                ;   bit  5 - Rx enabled
                ;   bit  6 - Synchronise, Tx or Rx a break
                ;   bit  7 - Required direction, set = Tx, clear = Rx

telemNxt        ; Data previously received from next controller


; Previous controller interface
serPTimer       ; Interrupt counter for serial bit timing
serPBitCnt      ; Bit down counter
serPReg         ; Data shift register
serPBffr        ; Data byte buffer

lnkPTimer       ; Rx timeout timer
lnkPState       ; Link state register (see 'link_hd.inc')
                ;   bits 0,1 - Current state
                ;     0  - Switching to Rx
                ;     1  - Receiving data
                ;     2  - Waiting for far end to turn around
                ;     3  - Transmiting data
                ;   bits 2 - No reception from previous link
                ;   bits 3 - Unused
                ;   bit  4 - Tx enabled
                ;   bit  5 - Rx enabled
                ;   bit  6 - Synchronise, Tx or Rx a break
                ;   bit  7 - Required direction, set = Tx, clear = Rx

telemPrv        ; Data previously received from previous controller


; Timing
intScCount      ; Interrupt counter for down scaled timing
secCount        ; Down scaled interrupts counter for second timing

aspectTime      ; Aspect interval for simulating next signal
nxtTimer        ; Second counter for simulating next signal


; Inputs
debnce          ; Previous read values used to debounce inputs
inputs          ; Input values after debouncing:
                ;   bit 0 - Latch local signal on (active low)
                ;   bit 1 - Line reversed (active low)
                ;   bit 2 - Line is bidirectional, else suppress line reversal
                ;   bit 3 - Local signal to show normal speed aspects
                ;   bit 4 - Detecting (active low), open drain output (wired or)
                ;   bit 7 - Inhibit, display stop aspect (active low)

snsAcc          ; Detector sensor match (emitter state) accumulator


; Outputs
aspVal          ; Value indicating aspects to display (not actual ouptut)


; Block and signal control
nxtCntlr        ; Status received from next (in advance) controller:
                ;   bits 0 - 3 - Ignored bits 4 to 7 ones complement
                ;   bit 4      - Next signal displaying normal speed aspects
                ;   bit 5      - Line reversed
                ;   bits 6,7   - Aspect value, local signal guards next block
                ;     0 - Stop
                ;     1 - Clear1
                ;     2 - Clear2
                ;     3 - Clear

lclCntlr        ; Status of this controller:
                ;   bits 0 - 2 - Local occupation block state
                ;     0 - Block clear
                ;     1 - Train entering forward
                ;     2 - Train in block
                ;     3 - Train leaving forward
                ;     4 - Train spanning block
                ;     5 - Train entering reverse
                ;     6 - Train leaving reverse
                ;   bit 3 - Exit detection stretched
                ;   bit 4 - Exit detection
                ;   bit 5 - Entry detection (Exit detection from previous)
                ;   bit 6 - Local block reversed
                ;   bit 7 - Unused

seenRvsl        ; Line and block reversal seen for next and previous controllers
                ;   bits 0 - 2 - Unused
                ;   bit 5      - Next line reversed
                ;   bit 6      - Previous block reversed
                ;   bit 7      - Unused
