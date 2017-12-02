;**********************************************************************
; Include and configuration directives
;**********************************************************************

    list      p=16F84

#include "p16f84.inc"
; Configuration word
;  - Code Protection Off
;  - Watchdog timer enabled
;  - Power up timer enabled
;  - Crystal (resonator) oscillator

    __CONFIG   _CP_OFF & _PWRTE_ON & _XT_OSC

; '__CONFIG' directive is used to embed configuration data within .asm file.
; The lables following the directive are located in the respective .inc file.
; See respective data sheet for additional information on configuration word.

; Include serial link interface macros
;  - Serial link bit timing is performed by link service routines
#define CLKD_SERIAL
#include "utility/asyn_srl.inc"
#include "utility/link_hd.inc"


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
EEpromStart EQU     0x2100
IntVector   EQU     0x004
RamStart    EQU     0x0C
RamEnd      EQU     0x2F

; I/O port direction masks
PORTASTATUS EQU     B'00001000'
PORTBSTATUS EQU     B'00001011'

; Interrupt & timing constants
RTCCINT     EQU     160         ; 10KHz = ((4MHz / 4) / 100) (0.1 mSec tick)

; Timing constants
INTSCLNG    EQU     80 + 1      ; Interrupts scaling to 125Hz (8 mSec tick)
SECSCLNG    EQU     125         ; Scaled interrupts per second
HALFSEC     EQU     B'11000000' ; Roughly half second scaled interrupts mask

INTSERINI   EQU     6           ; Interrupts per initial Rx serial bit @ 2K5
INTSERBIT   EQU     4           ; Interrupts per serial bit @ 2K5 baud
INTLNKDLYRX EQU     0           ; Interrupts for link Rx turnaround delay
INTLNKDLYTX EQU     4           ; Interrupts for link Tx turnaround delay

INTLINKTMOP EQU     5           ; Scaled interrupts, previous link timeout
INTLINKTMON EQU     25          ; Scaled interrupts, next link timeout

; Next controller serial interface constants (see 'asyn_srl.inc')
RXNFLG      EQU     0           ; Receive byte buffer 'loaded' status bit
RXNERR      EQU     1           ; Receive error status bit
RXNBREAK    EQU     2           ; Received 'break' status bit
RXNSTOP     EQU     3           ; Seeking stop bit status bit
RXNTRIS     EQU     TRISB       ; Rx port direction register
RXNPORT     EQU     PORTB       ; Rx port data register
RXNBIT      EQU     1           ; Rx input bit

TXNFLG      EQU     RXNFLG      ; Transmit byte buffer 'clear' status bit
TXNBREAK    EQU     RXNBREAK    ; Send 'break' status bit
TXNTRIS     EQU     RXNPORT     ; Tx port direction register
TXNPORT     EQU     RXNPORT     ; Tx port data register
TXNBIT      EQU     RXNBIT      ; Tx output bit

; Previous controller serial interface constants (see 'asyn_srl.inc')
RXPFLG      EQU     4           ; Receive byte buffer 'loaded' status bit
RXPERR      EQU     5           ; Receive error status bit
RXPBREAK    EQU     6           ; Received 'break' status bit
RXPSTOP     EQU     7           ; Seeking stop bit
RXPTRIS     EQU     TRISB       ; Rx port direction register
RXPPORT     EQU     PORTB       ; Rx port data register
RXPBIT      EQU     2           ; Rx input bit

TXPFLG      EQU     RXPFLG      ; Transmit byte buffer 'clear' status bit
TXPBREAK    EQU     RXPBREAK    ; Send 'break' status bit
TXPTRIS     EQU     RXPTRIS     ; Tx port direction register
TXPPORT     EQU     RXPPORT     ; Tx port data register
TXPBIT      EQU     RXPBIT      ; Tx output bit

; Detector I/O constants
EMTPORT     EQU     PORTA       ; Emitter drive port
EMTBIT      EQU     2           ; Emmitter drive bit (active low)
SNSPORT     EQU     PORTA       ; Sensor input port
SNSBIT      EQU     3           ; Sensor input bit (active high)
DETPORT     EQU     PORTA       ; Detection indicator port
DETBIT      EQU     4           ; Detection indicator bit (active low)

; Latch signal 'on' input constants
LCHPORT     EQU     PORTB       ; Latch signal input port
LCHBIT      EQU     0           ; Latch signal input bit (active low)

; Inhibit (force display of red aspect) input constants
INHPORT     EQU     PORTB       ; Inhibit input port
INHBIT      EQU     1           ; Inhibit input bit (active low)

; Special speed input constants
SPDPORT     EQU     PORTB       ; Special speed input port
SPDBIT      EQU     3           ; Special speed input bit (active low)

; Block state values
BLOCKCLEAR     EQU  0           ; Block clear
TRAINENTERINGF EQU  1           ; Train entering forward
BLOCKOCCUPIED  EQU  2           ; Train in block
TRAINLEAVINGF  EQU  3           ; Train leaving forward
BLOCKSPANNED   EQU  4           ; Train spanning block
TRAINENTERINGR EQU  5           ; Train entering reverse
TRAINLEAVINGR  EQU  6           ; Train leaving reverse
BLKSTATE       EQU  B'00000111' ; Mask to isolate block

; Aspect values
ASPSTP      EQU     B'00000000' ; Stop aspect value
ASPCL1      EQU     B'01000000'
ASPCL2      EQU     B'10000000'
ASPCLR      EQU     B'11000000' ; Clear aspect value
ASPCLFLG    EQU     7           ; Clear aspect value flag bit
ASPINCR     EQU     B'01000000' ; Aspect value increment
ASPSTATE    EQU     B'11000000' ; Aspect value mask

; Local controller status flags
STRFLG      EQU     3           ; Exit detection stretched bit in status byte
STRMSK      EQU     B'00001000' ; Exit detection stretched state bit mask

INHFLG      EQU     4           ; Signal inhibited bit in status byte
INHMSK      EQU     B'00010000' ; Signal inhibited state bit mask

EXTFLG      EQU     5           ; Exit detection bit in status byte
EXTMSK      EQU     B'00100000' ; Exit detection state bit mask

; Previous controller status flags
ENTFLG      EQU     1           ; Entrance detection bit in status byte
ENTMSK      EQU     B'00000010' ; Entrance detection state bit mask

; Next and Previous controllers status flags
REVFLG      EQU     3           ; Line reversed bit in status byte
REVMSK      EQU     B'00001000' ; Line reversed state bit mask

SPDFLG      EQU     4           ; Special speed bit in status byte
SPDMSK      EQU     B'00010000' ; Special speed state bit mask

; Aspect output constants
ASPPORT     EQU     PORTB       ; Aspect output port
STPMSK      EQU     B'10000000' ; Mask for red aspect output bit
CL1MSK      EQU     B'01000000' ; Mask for yellow aspect output bit
CL2MSK      EQU     B'00100000' ; Mask for double yellow aspect output bit
CLRMSK      EQU     B'00010000' ; Mask for green aspect output bit
ASPOUTMSK   EQU     B'11110000' ; Mask for aspect output bits

INPACTV     EQU     7           ; Indicates debounce accumulator > high water
