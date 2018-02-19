;**********************************************************************
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
; Configuration directives
;**********************************************************************

    list      r=dec,p=16F84

#include "p16f84.inc"
; Configuration word
;  - Code Protection Off
;  - Watchdog timer enabled
;  - Power up timer enabled
;  - Crystal (resonator) oscillator

    __config   _CP_OFF & _WDT_OFF & _PWRTE_ON & _XT_OSC

; '__config' directive is used to embed configuration data within .asm file.
; The lables following the directive are located in the respective .inc file.
; See respective data sheet for additional information on configuration word.


#ifndef maxlwdefined
#define maxlwdefined
;**********************************************************************
;                                                                     *
; Macro:     MaxLw                                                    *
;                                                                     *
;            Utility to load W with numerically greater of arguments  *
;                                                                     *
;**********************************************************************
MaxLw   macro   arg1, arg2

#if (arg1 > arg2)
    movlw   arg1
#else
    movlw   arg2
#endif

    endm
#endif


;**********************************************************************
; Constant definitions
;**********************************************************************

; Address defintions
BootVector  EQU     0x000
CodeEnd     EQU     0x33F
EEpromStart EQU     0x2100
IntVector   EQU     0x004
RAM_Start   EQU     0x0C
RAM_End     EQU     0x4F

; I/O port direction masks
PORTASTATUS EQU     B'00000000' ; Configure all as outputs (RB4 open collector)
PORTBSTATUS EQU     B'00011111' ; RB0 - RB4 inputs, all others outputs

; Interrupt & timing constants
RTCCINT     EQU     256 - 200   ; 5KHz = ((4MHz / 4) / 200) (0.2 mSec tick)

; Timing constants
INTSCLNG    EQU     40 + 1      ; Interrupts scaling to 125Hz (8 mSec tick)
SECSCLNG    EQU     125 + 1     ; Down scaled interrupts per second
HALFSEC     EQU     B'11000000' ; Seconds timer roughly half second mask

INTSERINI   EQU     6           ; Interrupts per initial Rx serial bit @ 1K25
INTSERBIT   EQU     4           ; Interrupts per serial bit @ 1K25 baud
INTLNKDLYRX EQU     0           ; Interrupts for link Rx turnaround delay
INTLNKDLYTX EQU     10          ; Interrupts for link Tx turnaround delay

INTLINKTMOP EQU     5           ; Down scaled interrupts, previous link timeout
INTLINKTMON EQU     50          ; Down scaled interrupts, next link timeout

; Next controller link serial interface ports
RXNTRIS     EQU     TRISB       ; Rx port direction register
RXNPORT     EQU     PORTB       ; Rx port data register
RXNBIT      EQU     7           ; Rx input bit
;  Half duplex so Rx and Tx use same port
TXNTRIS     EQU     RXNTRIS     ; Tx port direction register
TXNPORT     EQU     RXNPORT     ; Tx port data register
TXNBIT      EQU     RXNBIT      ; Tx output bit

; Previous controller link serial interface ports
RXPTRIS     EQU     TRISB       ; Rx port direction register
RXPPORT     EQU     PORTB       ; Rx port data register
RXPBIT      EQU     6           ; Rx input bit
;  Half duplex so Rx and Tx use same port
TXPTRIS     EQU     RXPTRIS     ; Tx port direction register
TXPPORT     EQU     RXPPORT     ; Tx port data register
TXPBIT      EQU     RXPBIT      ; Tx output bit

; Serial interface status flags (see 'asyn_srl.inc')

;  Next controller link
;   bit 0 - Rx buffer full, Tx buffer clear
;   bit 1 - Rx error
;   bit 2 - Received or sending break
;   bit 3 - Seeking stop bit
RXNFLG      EQU     0           ; Receive byte buffer 'loaded' status bit
RXNERR      EQU     1           ; Receive error status bit
RXNBREAK    EQU     2           ; Received 'break' status bit
RXNSTOP     EQU     3           ; Seeking stop bit status bit
;  Half duplex so Rx and Tx flags share bits
TXNFLG      EQU     RXNFLG      ; Transmit byte buffer 'clear' status bit
TXNBREAK    EQU     RXNBREAK    ; Send 'break' status bit

;  Previous controller link
;   bit 4 - Rx buffer full, Tx buffer clear
;   bit 5 - Rx error
;   bit 6 - Received or sending break
;   bit 7 - Seeking stop bit
RXPFLG      EQU     4           ; Receive byte buffer 'loaded' status bit
RXPERR      EQU     5           ; Receive error status bit
RXPBREAK    EQU     6           ; Received 'break' status bit
RXPSTOP     EQU     7           ; Seeking stop bit
;  Half duplex so Rx and Tx flags share bits
TXPFLG      EQU     RXPFLG      ; Transmit byte buffer 'clear' status bit
TXPBREAK    EQU     RXPBREAK    ; Send 'break' status bit

NOPFLG      EQU     2           ; No reception from previous link status bit

; Block state values
BLOCKCLEAR     EQU  0           ; Block clear
TRAINENTERINGF EQU  1           ; Train entering forward
BLOCKOCCUPIED  EQU  2           ; Train in block
TRAINLEAVINGF  EQU  3           ; Train leaving forward
BLOCKSPANNED   EQU  4           ; Train spanning block
TRAINENTERINGR EQU  5           ; Train entering reverse
TRAINLEAVINGR  EQU  6           ; Train leaving reverse
BLKSTATE       EQU  B'00000111' ; Mask to isolate block state value

; Aspect values
ASPSTP      EQU     B'00000000' ; Stop
ASPWR1      EQU     B'01000000' ; Warning 1, next signal at stop
ASPWR2      EQU     B'10000000' ; Warning 2, next signal at warning 1
ASPCLR      EQU     B'11000000' ; Clear, next signal at warning 1 or clear
ASPW2FLG    EQU     7           ; Warning 2 aspect value flag bit
ASPMSK      EQU     B'11000000' ; Aspect value mask
ASPINCR     EQU     B'01000000' ; Aspect value increment

; Aspect output constants
ASPPORT     EQU     PORTA       ; Aspect output port
ASPOUTMSK   EQU     B'00001111' ; Mask for aspect output bits

; Input bits, PORTB
LCHBIT      EQU     0           ; Latch stop aspect (active low)
REVBIT      EQU     1           ; Line reversed, force stop aspect (active low)
BIDBIT      EQU     2           ; Running line is bidirectional
SPDBIT      EQU     3           ; Display normal speed aspects
INHBIT      EQU     7           ; Inhibit, forced stop aspect (active low)
INPMSK      EQU     B'10001111' ; Mask for input bits

; Detector I/O constants
EMTPORT     EQU     PORTB       ; Emitter drive port
EMTBIT      EQU     5           ; Emmitter drive bit (active low)
SNSPORT     EQU     PORTB       ; Sensor input port
SNSBIT      EQU     4           ; Sensor input bit (active high)
DETPORT     EQU     PORTA       ; Detection indicator port
DETBIT      EQU     4           ; Detection indicator bit (active low)
INPACTV     EQU     7           ; Correspondance accumulator >= high water bit

; Local controller status flags
STRFLG      EQU     3           ; Exit detection stretched bit in status byte
STRMSK      EQU     B'00001000' ; Exit detection stretched state bit mask
EXTFLG      EQU     4           ; Exit detection bit in status byte
EXTMSK      EQU     B'00010000' ; Exit detection state bit mask
ENTFLG      EQU     5           ; Entrance detection bit in status byte
ENTMSK      EQU     B'00100000' ; Entrance detection state bit mask
BRVFLG      EQU     6           ; Block reversed bit in status byte
BRVMSK      EQU     B'01000000' ; Block reversed state bit mask

; Next and Previous controllers common (received and sent) status flags
SPDFLG      EQU     4           ; Normal speed bit in status byte
SPDMSK      EQU     B'00010000' ; Normal speed state bit mask
LRVFLG      EQU     5           ; Line reversed bit in status byte
LRVMSK      EQU     B'00100000' ; Line reversed state bit mask
