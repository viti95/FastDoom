; use djgpp or (Open) Watcom compatible segments and symbols?
; default is Watcom

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
