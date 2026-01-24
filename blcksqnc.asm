;**********************************************************************
;                                                                     *
;    Description:   Controller for multiple aspect colour light       *
;                   signal and occupation block with positional train *
;                   detector at block exit.                           *
;                                                                     *
;                   This is a generic specialisation to test common   *
;                   code.                                             *
;                                                                     *
;    Author:        Chris White                                       *
;    Company:       Monitor Computing Services Ltd.                   *
;                                                                     * 
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
;                                                                     *
;                            +---+ +---+                              *
;    Warning 2 Aspect  <- RA2|1  |_| 18|RA1 -> Warning 1 Aspect       *
;        Clear Aspect  <- RA3|2      17|RA0 -> Stop Aspect            *
;          !Detecting <-> RA4|3      16|                              *
;                            |4      15|                              *
;                            |5      14|                              *
;     !Latch Signal On -> RB0|6      13|RB7 <-> Next / <- !Inhibit    *
;       !Line reversed -> RB1|7      12|RB6 <-> Previous              *
;   Line bidirectional -> RB2|8      11|RB5 ->  !Emitter              *
;         Normal speed -> RB3|9      10|RB4 <-  Sensor                *
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
#if RAM_End < endRAM
    error "This program ran out of RAM!"
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
