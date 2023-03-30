; use djgpp or (Open) Watcom compatible segments and symbols?
; default is Watcom

CPU 386

%ifdef DJGPP_ASM
  %define BEGIN_CODE_SECTION segment .text public class=CODE USE32
  %define BEGIN_DATA_SECTION segment .data public class=DATA USE32
  %define CONTINUE_CODE_SECTION segment .text
  %define CONTINUE_DATA_SECTION segment .data
  %macro CODE_SYM_DEF 1
    global _%1
    _%1:
  %endmacro
%else ; Watcom
  %define BEGIN_CODE_SECTION segment _TEXT public align=4 class=CODE USE32
  %define BEGIN_DATA_SECTION segment _DATA public align=4 class=DATA USE32
  %define CONTINUE_CODE_SECTION segment _TEXT
  %define CONTINUE_DATA_SECTION segment _DATA
  %macro CODE_SYM_DEF 1
    global %1_
    %1_:
  %endmacro
%endif

%ifdef MODE_13H
%define USE_BACKBUFFER
%endif

%ifdef MODE_ATI640
%define USE_BACKBUFFER
%endif

%ifdef MODE_CGA_BW
%define USE_BACKBUFFER
%endif

%ifdef MODE_CGA16
%define USE_BACKBUFFER
%endif

%ifdef MODE_CGA
%define USE_BACKBUFFER
%endif

%ifdef MODE_CVB
%define USE_BACKBUFFER
%endif

%ifdef MODE_EGA640
%define USE_BACKBUFFER
%endif

%ifdef MODE_HERC
%define USE_BACKBUFFER
%endif

%ifdef MODE_PCP
%define USE_BACKBUFFER
%endif

%ifdef MODE_SIGMA
%define USE_BACKBUFFER
%endif

%ifdef MODE_V2
%define USE_BACKBUFFER
%endif

%ifdef MODE_VGA16
%define USE_BACKBUFFER
%endif

%ifdef MODE_VBE2
%define USE_BACKBUFFER
%endif

%ifdef MODE_EGA
%define USE_BACKBUFFER
%endif

%ifdef MODE_CGA_AFH
%define USE_BACKBUFFER
%endif

%ifdef MODE_CGA512
%define USE_BACKBUFFER
%endif
