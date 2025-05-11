
%macro Mul320Start 2
    lea %1, [%2+%2*4]
%endmacro

%macro Mul320End 1
    shl %1, 6
%endmacro

%macro Mul640Start 2
    lea %1, [%2+%2*4]
%endmacro

%macro Mul640End 1
    shl %1, 7
%endmacro

%macro Mul1600Start 2
    lea %1, [%2+%2*4]
    lea %1, [%1+%1*4]
%endmacro

%macro Mul1600End 1
    shl %1, 6
%endmacro

%macro Mul1280Start 2
    lea %1, [%2+%2*4]
%endmacro

%macro Mul1280End 1
    shl %1, 8
%endmacro

%macro Mul256Start 2
    mov %1, %2
%endmacro

%macro Mul256End 1
    shl %1, 8
%endmacro

%macro Mul512Start 2
    mov %1, %2
%endmacro

%macro Mul512End 1
    shl %1, 9
%endmacro

%macro Mul1024Start 2
    mov %1, %2
%endmacro

%macro Mul1024End 1
    shl %1, 10
%endmacro

%macro Mul400Start 2
    lea %1, [%2+%2*4]
    lea %1, [%1+%1*4]
%endmacro

%macro Mul400End 1
    shl %1, 4
%endmacro

%macro Mul800Start 2
    lea %1, [%2+%2*4]
    lea %1, [%1+%1*4]
%endmacro

%macro Mul800End 1
    shl %1, 5
%endmacro

%macro MulStart 3
    Mul%1Start %2, %3
%endmacro

%macro MulEnd 2
    Mul%1End %2
%endmacro
