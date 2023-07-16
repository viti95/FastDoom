DECLARE SUB cleanScreen ()
DIM filename AS STRING
DIM count AS INTEGER

DIM benchmarkSelection AS STRING
DIM benchmarkDemo AS STRING
DIM benchmarkFiles(1 TO 255) AS STRING
DIM benchmarkFilesTotal AS INTEGER
DIM benchmarkExecutables(1 TO 255) AS STRING
DIM benchmarkExecutablesTotal AS INTEGER
DIM benchmarkExecutable AS STRING
DIM benchmarkWads(1 TO 255) AS STRING
DIM benchmarkWadsTotal AS INTEGER
DIM benchmarkWad AS STRING
DIM benchmarkOptions AS STRING

DIM inputVal AS STRING
Dim route as String

'Flush keyboard
WHILE INKEY$ <> ""
WEND

CALL cleanScreen

PRINT "     Choose an IWAD"
PRINT

count = 0

route = "\*.WAD"

filename = DIR$(route)
DO WHILE LEN(filename) > 0

    IF INSTR(filename, "MODE") = 0 THEN
	count = count + 1
	benchmarkWads(count) = filename
    END IF

    filename = DIR$
LOOP

benchmarkWadsTotal = count

FOR position = 1 TO benchmarkWadsTotal
    PRINT "     " + STR$(position) + ") " + benchmarkWads(position)
NEXT

PRINT
INPUT "     Please enter option: ", inputVal

benchmarkWad = benchmarkWads(VAL(inputVal))

CALL cleanScreen

PRINT "     Choose a benchmark"
PRINT

count = 0


route = "\BENCH\*.BNC"

filename = DIR$(route)
DO WHILE LEN(filename) > 0
    count = count + 1
    benchmarkFiles(count) = filename
    filename = DIR$
LOOP

benchmarkFilesTotal = count

FOR position = 1 TO benchmarkFilesTotal
    PRINT "     " + STR$(position) + ") " + benchmarkFiles(position)
NEXT

PRINT
INPUT "     Please enter option: ", inputVal

benchmarkSelection = "BENCH\" + benchmarkFiles(VAL(inputVal))

CALL cleanScreen

PRINT "     Choose a demo file (or type any demo you want)"
PRINT
PRINT "      1) DEMO1"
PRINT "      2) DEMO2"
PRINT "      3) DEMO3"
PRINT "      4) DEMO4"
PRINT
INPUT "     Please enter option: ", inputVal

IF inputVal = "1" THEN
    benchmarkDemo = "demo1"
ELSEIF inputVal = "2" THEN
    benchmarkDemo = "demo2"
ELSEIF inputVal = "3" THEN
    benchmarkDemo = "demo3"
ELSEIF inputVal = "4" THEN
    benchmarkDemo = "demo4"
ELSE
    benchmarkDemo = inputVal
END IF

CALL cleanScreen

PRINT "     Choose a FastDoom executable"
PRINT

count = 0

route = "\FDOOM*.EXE"

filename = DIR$(route)
DO WHILE LEN(filename) > 0
    count = count + 1
    benchmarkExecutables(count) = filename
    filename = DIR$
LOOP

benchmarkExecutablesTotal = count

FOR position = 1 TO benchmarkExecutablesTotal
    PRINT "     " + STR$(position) + ") " + benchmarkExecutables(position)
NEXT

PRINT
INPUT "     Please enter option: ", inputVal

benchmarkExecutable = benchmarkExecutables(VAL(inputVal))

CALL cleanScreen

PRINT "     Choose additional options"
PRINT
PRINT "      A) Advanced benchmark (frametimes)"
PRINT

INPUT "     Please enter option: ", inputVal

IF inputVal = "A" OR inputVal = "a" THEN
    benchmarkOptions = "-advanced"
ELSE
    benchmarkOptions = ""
END IF

DIM arguments AS STRING
arguments = "-iwad " + benchmarkWad + " -benchmark file " + benchmarkDemo + " " + benchmarkSelection + " " + benchmarkOptions

DIM text AS STRING

text = LCASE$(benchmarkExecutable + " " + arguments)

OPEN "BENCH2.BAT" FOR OUTPUT AS #1
PRINT #1, text
CLOSE

CLS

SUB cleanScreen
	CLS

    PRINT
    PRINT "   ##########################################################################"
    PRINT "   #                                                                        #"
    PRINT "   #                           FASTDOOM BENCHMARK                           #"
    PRINT "   #                                                                        #"
    PRINT "   ##########################################################################"
    PRINT  
END SUB
