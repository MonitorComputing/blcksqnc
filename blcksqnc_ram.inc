;**********************************************************************
; Variable registers
;**********************************************************************

            CBLOCK  RamStart

; Status and accumulator storage during interrupt
w_isr           ; 'w' register, accumulator, store during ISR
pclath_isr      ; PCLATH register store during ISR
status_isr      ; status register store during ISR

; Serial interface
srlIfStat       ; Serial I/F status flags (see 'asyn_srl.inc')
                ;
                ;  Next link (half duplex so Rx and Tx flags share bits)
                ;  =====================================================
                ;
                ;   bit 0 - Rx buffer full, Tx buffer clear
                ;   bit 1 - Rx error
                ;   bit 2 - Received or sending break
                ;   bit 3 - Seeking stop bit
                ;
                ;  Previous link (half duplex so Rx and Tx flags share bits)
                ;  =========================================================
                ;
                ;   bit 4 - Rx buffer full, Tx buffer clear
                ;   bit 5 - Rx error
                ;   bit 6 - Received or sending break
                ;   bit 7 - Seeking stop bit

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
                ;   bits 2,3 - Unused
                ;   bit  4 - Tx enabled
                ;   bit  5 - Rx enabled
                ;   bit  6 - Synchronise, Tx or Rx a break
                ;   bit  7 - Required direction, set = Tx, clear = Rx

intScCount      ; Interrupt scaling counter for second timing
secCount        ; Scaled interrupts counter for second timing

snsAcc          ; Detector sensor match (emitter state) accumulator

debnce
inputs
aspOut

lclCntlr        ; Status of this controller
                ;   bits 0,2 - Occupation block state
                ;     0 - Block clear
                ;     1 - Train entering forward
                ;     2 - Train in block
                ;     3 - Train leaving forward
                ;     4 - Train spanning block
                ;     5 - Train entering reverse
                ;     6 - Train leaving reverse
                ;   bit 3    - Exit detection stretched
                ;   bit 4    - Signal inhibit (display red aspect)
                ;   bit 5    - Exit detection
                ;   bits 6,7 - Aspect value
                ;     0 - Stop
                ;     1 - Clear1
                ;     2 - Clear2
                ;     3 - Clear

nxtCntlr        ; Status received from next controller
                ;   bits 0,3 - Ignored (ones complement of bits 4 to 7)
                ;   bit 4    - Special speed
                ;   bit 5    - Line reversed
                ;   bits 6,7 - Aspect value (bits as for this controller)

prvCntlr        ; Status received from previous controller
                ;   bit 0    - Ignored
                ;   bit 1    - Entrance detection
                ;   bits 2,3 - Ignored
                ; Status sent to previous controller
                ;   bit 4    - Special speed
                ;   bit 5    - Line reversed
                ;   bits 6,7 - Aspect value (bits as for this controller)

aspectTime      ; Aspect interval for simulating next signal
nxtTimer        ; Second counter for simulating next signal
telemPrv        ; Data last received from previous controller
telemNxt        ; Data last received from next controller

            ENDC
