;**********************************************************************
; Configuration directives
;**********************************************************************

    list      p=16F84

#include "p16f84.inc"
; Configuration word
;  - Code Protection Off
;  - Watchdog timer enabled
;  - Power up timer enabled
;  - Crystal (resonator) oscillator

    __config   _CP_OFF & _PWRTE_ON & _XT_OSC

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

; Next controller link serial interface ports
RXNTRIS     EQU     TRISB       ; Rx port direction register
RXNPORT     EQU     PORTB       ; Rx port data register
RXNBIT      EQU     7           ; Rx input bit
;  Half duplex so Rx and Tx use same port
TXNTRIS     EQU     RXNPORT     ; Tx port direction register
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

; Previous controller (received)status flags
ENTFLG      EQU     1           ; Entrance detection bit in status byte
ENTMSK      EQU     B'00000010' ; Entrance detection state bit mask

; Next and Previous controllers common (received and sent) status flags
REVFLG      EQU     3           ; Line reversed bit in status byte
REVMSK      EQU     B'00001000' ; Line reversed state bit mask
SPDFLG      EQU     4           ; Special speed bit in status byte
SPDMSK      EQU     B'00010000' ; Special speed state bit mask

; Detector I/O constants
EMTPORT     EQU     PORTB       ; Emitter drive port
EMTBIT      EQU     5           ; Emmitter drive bit (active low)
SNSPORT     EQU     PORTB       ; Sensor input port
SNSBIT      EQU     4           ; Sensor input bit (active high)
DETPORT     EQU     PORTA       ; Detection indicator port
DETBIT      EQU     4           ; Detection indicator bit (active low)
INPACTV     EQU     7           ; Correspondance accumulator >= high water bit

; Input bits, PORTB
LCHBIT      EQU     0           ; Latch stop aspect (active low)
REVBIT      EQU     1           ; Line reversed, force stop aspect (active low)
BIDBIT      EQU     2           ; Running line is bidirectional
SPDBIT      EQU     3           ; Display special speed aspects (active low)
INHBIT      EQU     7           ; Inhibit, forced stop aspect (active low)

; Aspect output constants
ASPPORT     EQU     PORTA       ; Aspect output port
STPMSK      EQU     B'00000001' ; Mask for stop aspect output bit
CL1MSK      EQU     B'00000010' ; Mask for clear1 aspect output bit
CL2MSK      EQU     B'00000100' ; Mask for clear2 yellow aspect output bit
CLRMSK      EQU     B'00001000' ; Mask for clear aspect output bit
ASPOUTMSK   EQU     B'00001111' ; Mask for aspect output bits
