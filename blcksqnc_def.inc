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
; Configuration directives
;**********************************************************************

    list      r=dec,p=16F627

#include "p16f627.inc"
; Configuration word
;  - Code Protection Off
;  - Data code Protection Off
;  - MCLR disabled, use RA5 for IO
;  - Power up timer enabled
;  - Watchdog timer disabled
;  - Internal oscillator, use RA6 and 7 for IO

    __config   _CP_OFF & _CPD_OFF & _LVP_OFF & _MCLRE_OFF & _PWRTE_ON & _WDT_ON & _INTRC_OSC_NOCLKOUT

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
CodeEnd     EQU     0x3FF
EEpromStart EQU     0x2100
EEpromEnd   EQU     0x217F
IntVector   EQU     0x004
RAM0_Start  EQU     0x20
RAM0_End    EQU     0x7F
RAM1_Start  EQU     0xA0
RAM1_End    EQU     0xEF
RAM2_Start  EQU     0x120
RAM2_End    EQU     0x14F

; I/O port direction masks
PORTASTATUS EQU     B'00100000' ; All outputs (RB4 open drain) except RB5
PORTBSTATUS EQU     B'11001111' ; 0 - 3, 6, 7 inputs, 4 and 5 outputs

; Interrupt & timing constants
RTCCINT     EQU     256 - 100   ; 10KHz = ((4MHz / 4) / 100) (0.1 mSec tick)

; Timing constants
INTSCLNG    EQU     80 + 1      ; Interrupts scaling to 125Hz (8 mSec tick)
SECSCLNG    EQU     125 + 1     ; Down scaled interrupts per second
HALFSEC     EQU     B'11000000' ; Seconds timer roughly half second mask

INTSERINI   EQU     6           ; Interrupts per initial Rx serial bit @ 2K5
INTSERBIT   EQU     4           ; Interrupts per serial bit @ 2K5 baud
INTLNKDLYRX EQU     0           ; Interrupts for link Rx turnaround delay
INTLNKDLYTX EQU     10          ; Interrupts for link Tx turnaround delay

INTLINKTMOP EQU     5           ; Down scaled interrupts, previous link timeout
INTLINKTMON EQU     50          ; Down scaled interrupts, next link timeout

; Next controller link serial interface ports
RXNTRIS     EQU     TRISB       ; Rx port direction register
RXNPORT     EQU     PORTB       ; Rx port data register
RXNBIT      EQU     5           ; Rx input bit
;  Half duplex so Rx and Tx use same port
TXNTRIS     EQU     RXNTRIS     ; Tx port direction register
TXNPORT     EQU     RXNPORT     ; Tx port data register
TXNBIT      EQU     RXNBIT      ; Tx output bit

; Previous controller link serial interface ports
RXPTRIS     EQU     TRISB       ; Rx port direction register
RXPPORT     EQU     PORTB       ; Rx port data register
RXPBIT      EQU     4           ; Rx input bit
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
INHBIT      EQU     5           ; Inhibit, forced stop aspect (active low)
APRBIT      EQU     6           ; Approach clear local signal (active low)
TOTBIT      EQU     7           ; Train on track, set block occupied if clear
INPMSK      EQU     B'11101111' ; Mask for input bits

; Detector I/O constants
EMTPORT     EQU     PORTA       ; Emitter drive port
EMTBIT      EQU     7           ; Emmitter drive bit (active low)
EMTMSK      EQU     1 << EMTBIT ; Emmitter drive bit mask (active low)
SNSPORT     EQU     PORTA       ; Sensor input port
SNSBIT      EQU     5           ; Sensor input bit (active high)
SNSMSK      EQU     1 << SNSBIT ; Sensor input bit mask
DETPORT     EQU     PORTA       ; Detection indicator port
DETBIT      EQU     4           ; Detection indicator bit (active low)
DETACTV     EQU     7           ; Correspondance accumulator >= high water bit

OCCPORT     EQU     PORTA       ; Occupation indicator port
OCCBIT      EQU     6           ; Occupation indicator bit (active low)

; Local controller status flags
STRFLG      EQU     3           ; Exit detection stretched bit in status byte
STRMSK      EQU     1 << STRFLG ; Exit detection stretched state bit mask
EXTFLG      EQU     4           ; Exit detection bit in status byte
EXTMSK      EQU     1 << EXTFLG ; Exit detection state bit mask
ENTFLG      EQU     5           ; Entrance detection bit in status byte
ENTMSK      EQU     1 << ENTFLG ; Entrance detection state bit mask
BRVFLG      EQU     6           ; Block reversed bit in status byte
BRVMSK      EQU     1 << BRVFLG ; Block reversed state bit mask
APRFLG      EQU     7           ; Train approaching bit in status byte
APRMSK      EQU     1 << APRFLG ; Train approaching state bit mask

; Status flags sent to previous controller, received from next controller
SPDFLG      EQU     4           ; Normal speed bit in status byte
SPDMSK      EQU     1 << SPDFLG ; Normal speed state bit mask
LRVFLG      EQU     5           ; Line reversed bit in status byte
LRVMSK      EQU     1 << LRVFLG ; Line reversed state bit mask
