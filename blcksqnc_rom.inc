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
; EEPROM initialisation
;**********************************************************************

            ORG EEpromStart ; EEPROM data area

EEaspectMax DE  21  ; Maximum seconds allowed for train to transit next block
EEaspectMin DE   7  ; Minimum seconds allowed for train to transit next block
