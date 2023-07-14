Dim filename As String
Dim benchmark_files() As String
Dim count As UINTEGER
Dim benchmark_selection as String
Dim benchmark_demo as String
Dim benchmark_executables() as String
Dim benchmark_executable as String
Dim inputVal As String
Dim inputValInt As Integer

cls

print
print "   ##########################################################################"
print "   #                                                                        #"
print "   #                           FASTDOOM BENCHMARK                           #"
print "   #                                                                        #"
print "   ##########################################################################"
print
print "     Available benchmarks"
print

count = 0

filename = Dir(".\BENCH\*.BNC")
Do While Len( filename ) > 0
    count += 1
    'Print filename

    ReDim preserve benchmark_files(1 To count) As String
    benchmark_files(count) = filename

    filename = Dir( )
Loop

count = 0

For position As Integer = LBound(benchmark_files) To UBound(benchmark_files)
    count += 1
    Print "     " + Str(count) + ": " + benchmark_files(position)
Next

print 
Input "     Please select one: ", inputVal

benchmark_selection = ".\BENCH\" + benchmark_files(ValInt(inputVal))
'print " Benchmark selection: " + benchmark_selection

print "     Now select a demo file:"
print
print "     1. DEMO1"
print "     2. DEMO2"
print "     3. DEMO3"
print "     4. DEMO4"
print "     or type any demo you want"
print 
input "     Selection: ", inputVal

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

count = 0

filename = Dir(".\FDOOM*.EXE")
Do While Len( filename ) > 0
    count += 1
    'Print filename

    ReDim preserve benchmark_executables(1 To count) As String
    benchmark_executables(count) = filename

    filename = Dir( )
Loop

count = 0

For position As Integer = LBound(benchmark_executables) To UBound(benchmark_executables)
    count += 1
    Print "     " + Str(count) + ": " + benchmark_executables(position)
Next

print "Resultado: " + benchmark_demo