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
;       Clear2 Aspect  <- RA2|1  |_| 18|RA1 -> Clear1 Aspect          *
;        Clear Aspect  <- RA3|2      17|RA0 -> Stop Aspect            *
;          !Detecting <-> RA4|3      16|                              *
;                            |4      15|                              *
;                            |5      14|                              *
;     !Latch Signal On -> RB0|6      13|RB7 <-> Next / <- !Inhibit    *
;       !Line reversed -> RB1|7      12|RB6 <-> Previous              *
;   Line bidirectional -> RB2|8      11|RB5 ->  !Emitter              *
;       !Special speed -> RB3|9      10|RB4 <-  Sensor                *
;                            +---------+                              *
;                                                                     *
;**********************************************************************


;**********************************************************************
; Configuration directives and constant definitions
;**********************************************************************
#include "blcksqnc_def.inc"
  
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
#include "utility/asyn_srl.inc"
#include "utility/link_hd.inc"
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

    btfss   aspVal,ASPCLFLG
    retlw   CL1MSK
    retlw   CL2MSK
    return

;**********************************************************************
; End of source code
;**********************************************************************

#if CodeEnd < $
    error "This program is just too big!"
#endif
    end     ; directive 'end of program'
