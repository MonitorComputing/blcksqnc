;**********************************************************************
;                                                                     *
; Description: Controller for multiple aspect colour light signal and *
;              occupation block with positional train detector at     *
;              block exit.                                            *
;                                                                     *
;              This is a generic specialisation to test common code.  *
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
;                                                                     *
;                            +---+ +---+                              *
;    Warning 2 Aspect  <- RA2|1  |_| 18|RA1 -> Warning 1 Aspect       *
;        Clear Aspect  <- RA3|2      17|RA0 -> Stop Aspect            *
;          !Detecting <-> RA4|3      16|RA7 -> !Emitter               *
;               Sensor -> RA5|4      15|RA6 -> !Block occupied        *
;                            |5      14|                              *
;     !Latch Signal On -> RB0|6      13|RB7 <-> Next / <- !Inhibit    *
;       !Line reversed -> RB1|7      12|RB6 <-> Previous              *
;   Line bidirectional -> RB2|8      11|RB5                           *
;         Normal speed -> RB3|9      10|RB4                           *
;                            +---------+                              *
;                                                                     *
;**********************************************************************


;**********************************************************************
; Configuration directives and constant definitions
;**********************************************************************
#include "blcksqnc_def.inc"

; Aspect output constants
STPMSK      EQU     B'00000001' ; Mask for stop aspect output bit
WR1MSK      EQU     B'00000010' ; Mask for warning 1 aspect output bit
WR2MSK      EQU     B'00000100' ; Mask for warning 2 aspect output bit
CLRMSK      EQU     B'00001000' ; Mask for clear aspect output bit
BLANKMSK    EQU     B'00000000' ; Mask for blank aspect output


;**********************************************************************
; Variable registers
;**********************************************************************
#include "blcksqnc_ram.inc"
afterRAM
            endc
endRAM      EQU afterRAM - 1
#if RAM0_End < endRAM
    error "This program ran out of Bank 0 RAM!"
#endif

;**********************************************************************
; EEPROM initialisation
;**********************************************************************
#include "blcksqnc_rom.inc"

;**********************************************************************
; Code
;**********************************************************************
; Include serial link interface macros
;  - Serial link bit timing is performed by link service routines
#define CLKD_SERIAL
#include "utility_pic/asyn_srl.inc"
#include "utility_pic/link_hd.inc"
#include "blcksqnc_cod.inc"

;**********************************************************************
; Subroutine to return aspect output value in accumulator
;**********************************************************************
GetAspectOutput
    movf    aspVal,W        ; Get output value, may contain spurious bits
    andlw   ASPMSK          ; Isolate aspect value
    btfsc   STATUS,Z
    retlw   STPMSK

    xorlw   ASPCLR
    btfsc   STATUS,Z
    retlw   CLRMSK

    movlw   HALFSEC
    andwf   secCount,W      ; Test for flashing aspect blanking period

    movlw   WR2MSK
    btfss   aspVal,ASPW2FLG
    movlw   WR1MSK
    btfsc   nxtCntlr,SPDFLG ; Skip if next signal not at normal speed ...
    return                  ; ... else display warning aspect
    btfss   STATUS,Z        ; Skip if aspect blanking period ...
    return                  ; ... else display warning aspect
    retlw   BLANKMSK        ; Blank aspect display


;**********************************************************************
; End of source code
;**********************************************************************

#if CodeEnd < $
    error "This program is just too big!"
#endif
    end     ; directive 'end of program'
