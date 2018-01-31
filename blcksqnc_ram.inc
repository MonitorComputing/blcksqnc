;**********************************************************************
; Variable registers
;**********************************************************************

            cblock  RAM_Start

; Status and accumulator storage during interrupt
w_isr           ; 'w' register, accumulator, store during ISR
pclath_isr      ; PCLATH register store during ISR
status_isr      ; status register store during ISR


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
                ;   bits 2,3 - Unused
                ;   bit  4 - Tx enabled
                ;   bit  5 - Rx enabled
                ;   bit  6 - Synchronise, Tx or Rx a break
                ;   bit  7 - Required direction, set = Tx, clear = Rx

telemPrv        ; Data previously received from previous controller


; Timing
intScCount      ; Interrupt scaling counter for second timing
secCount        ; Scaled interrupts counter for second timing

aspectTime      ; Aspect interval for simulating next signal
nxtTimer        ; Second counter for simulating next signal


; Inputs
debnce          ; Previous read values used to debounce inputs
inputs          ; Input values after debouncing:
                ;   bit 0 - Latch signal on (active low)
                ;   bit 1 - Line reversed (active low)
                ;   bit 2 - Line is bidirectional, propogate line reversed
                ;   bit 3 - Show special speed aspects (active low)
                ;   bit 4 - Detecting (active low)
                ;   bit 7 - Inhibit, display stop aspect (active low)

snsAcc          ; Detector sensor match (emitter state) accumulator


; Outputs
aspOut          ; Value indicating aspects to display (not actual ouptut)


; Block and signal control
nxtCntlr        ; Status received from next (in advance) controller:
                ;   bits 0 - 3 - Reserved for bits 4 to 7 ones complement
                ;   bit 4      - Special speed
                ;   bit 5      - Line reversed
                ;   bits 6,7   - Aspect value (bits as for this controller)

lclCntlr        ; Status of this controller:
                ;   bits 0 - 2 - Occupation block state
                ;     0 - Block clear
                ;     1 - Train entering forward
                ;     2 - Train in block
                ;     3 - Train leaving forward
                ;     4 - Train spanning block
                ;     5 - Train entering reverse
                ;     6 - Train leaving reverse
                ;   bit 3      - Exit detection stretched
                ;   bit 4      - Signal inhibit (display red aspect)
                ;   bit 5      - Exit detection
                ;   bits 6,7   - Aspect value
                ;     0 - Stop
                ;     1 - Clear1
                ;     2 - Clear2
                ;     3 - Clear

prvCntlr        ; Status received from previous (in rear) controller:
                ;   bit 0    - Ignored
                ;   bit 1    - Entrance detection
                ;   bits 2,3 - Ignored
                ; Status sent to previous controller:
                ;   bit 4    - Special speed
                ;   bit 5    - Line reversed, only sent if line is bidirectional
                ;   bits 6,7 - Aspect value (bits as for this controller)
