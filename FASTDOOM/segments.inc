
; predefined segments for NASM - 32 bit obj format


%ifdef watcom
segment _TEXT public align=256 class=CODE use32
segment _DATA public align=256 class=DATA use32
segment _BSS  public align=256 class=BSS  use32
segment STACK stack align=256 class=STACK use32
segment _BSS

%elifdef djgpp
bits 32

%else
segment _TEXT public align=256 class=CODE use32
segment _DATA public align=256 class=DATA use32
segment _BSS  public align=256 class=BSS  use32
segment STACK stack align=256 class=STACK use32
segment _BSS

%endif