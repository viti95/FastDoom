Dim filename As String
Dim count As UINTEGER

Dim benchmark_selection as String
Dim benchmark_demo as String
Dim benchmark_files() As String
Dim benchmark_executables() as String
Dim benchmark_executable as String
Dim benchmark_wads() as String
Dim benchmark_wad as String
Dim benchmark_options as String

Dim inputVal As String

sub cleanScreen()
	cls

    print
    print "   ##########################################################################"
    print "   #                                                                        #"
    print "   #                           FASTDOOM BENCHMARK                           #"
    print "   #                                                                        #"
    print "   ##########################################################################"
    print
    
end sub

'Flush keyboard
WHILE INKEY$ <> ""
WEND

cleanScreen()

print "     Choose an IWAD"
print

count = 0

filename = Dir(".\*.WAD")
Do While Len( filename ) > 0

    if InStr(filename, "MODE") = 0 then
        count += 1
        ReDim preserve benchmark_wads(1 To count) As String
        benchmark_wads(count) = filename
    endif

    filename = Dir( )
Loop

count = 0

For position As Integer = LBound(benchmark_wads) To UBound(benchmark_wads)
    count += 1
    Print "     " + Str(count) + ") " + benchmark_wads(position)
Next

print 
Input "     Please enter option: ", inputVal

benchmark_wad = benchmark_wads(ValInt(inputVal))

cleanScreen()

print "     Choose a benchmark"
print

count = 0

filename = Dir(".\BENCH\*.BNC")
Do While Len( filename ) > 0
    count += 1

    ReDim preserve benchmark_files(1 To count) As String
    benchmark_files(count) = filename

    filename = Dir( )
Loop

count = 0

For position As Integer = LBound(benchmark_files) To UBound(benchmark_files)
    count += 1
    Print "     " + Str(count) + ") " + benchmark_files(position)
Next

print 
Input "     Please enter option: ", inputVal

benchmark_selection = "BENCH\" + benchmark_files(ValInt(inputVal))

cleanScreen()

print "     Choose a demo file (or type any demo you want)"
print
print "     1) DEMO1"
print "     2) DEMO2"
print "     3) DEMO3"
print "     4) DEMO4"
print 
Input "     Please enter option: ", inputVal

if inputVal = "1" then
    benchmark_demo = "demo1"
elseif inputVal = "2" then
    benchmark_demo = "demo2"
elseif inputVal = "3" then
    benchmark_demo = "demo3"
elseif inputVal = "4" then
    benchmark_demo = "demo4"
else
    benchmark_demo = inputVal
endif

cleanScreen()

print "     Choose a FastDoom executable"
print

count = 0

filename = Dir(".\FDOOM*.EXE")
Do While Len( filename ) > 0
    count += 1
    ReDim preserve benchmark_executables(1 To count) As String
    benchmark_executables(count) = filename

    filename = Dir( )
Loop

count = 0

For position As Integer = LBound(benchmark_executables) To UBound(benchmark_executables)
    count += 1
    Print "     " + Str(count) + ") " + benchmark_executables(position)
Next

print 
Input "     Please enter option: ", inputVal

benchmark_executable = benchmark_executables(ValInt(inputVal))

cleanScreen()

print "     Choose additional options"
print
print "     A) Advanced benchmark (frametimes)"
print

input "     Please enter option: ", inputVal

if inputVal = "A" or inputVal = "a" then
    benchmark_options = "-advanced"
else
    benchmark_options = ""
endif

Dim arguments as String
arguments = "-iwad " + benchmark_wad + " -benchmark file " + benchmark_demo + " " + benchmark_selection + " " + benchmark_options

'print benchmark_executable + " " + arguments

'Run FDOOM.EXE crashes with a page fault (¿?)
'run(benchmark_executable, arguments)

Dim f As Integer

f = FreeFile

Open "bench2.bat" For Output As #f
Put #f, , lcase(benchmark_executable + " " + arguments)
Put #f, , !"\n"
Close

cls
