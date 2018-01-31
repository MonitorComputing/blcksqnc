;**********************************************************************
;                                                                     *
;    Description:   Controller for occupation block with positional   *
;                   train detector at exit.                           *
;                                                                     *
;                   Receives train detection state from previous (in  *
;                   rear) controller which it uses as entry detector  *
;                   for occupation block.                             *
;                   Sends value of signal aspect (increment of local  *
;                   value of signal aspect) along with special speed  *
;                   indication and block reversed to previous         *
;                   controller.                                       *
;                                                                     *
;                   Receives value of signal aspect to be displayed   *
;                   along with special speed indication and block     *
;                   reversed from next (in advance) controller.       *
;                   Sends train detection state to next controller.   *
;                                                                     *
;                   If no data is received from next controller link  *
;                   input is treated as a level input indicating      *
;                   to display a stop aspect or to cycle aspect from  *
;                   stop to clear at fixed intervals after the        *
;                   passing of a train.                               *
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
#include "blcksqnc_cod.inc"

;**********************************************************************
; Subroutine to return aspect output mask in accumulator
;**********************************************************************
GetAspectMask
    movf    aspOut,W
    btfsc   STATUS,Z
    retlw   STPMSK

    xorlw   ASPCLR
    btfsc   STATUS,Z
    retlw   CLRMSK

    btfss   aspOut,ASPCLFLG
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
