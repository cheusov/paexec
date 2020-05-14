#!/usr/bin/env sh

export LC_ALL=C

: ${PAEXEC_FIND:=find}
FIND_CMD=$PAEXEC_FIND

#EXEPREFIX='valgrind -q'
#EXEPREFIX='env EF_PROTECT_BELOW=1 ef'

export PATH=`pwd`/fakeflac:$PATH

DIFF_PROG=${DIFF_PROG-diff -U10}

OBJDIR=${OBJDIR-..}
SRCDIR=${SRCDIR-..}

runpaexec (){
    $EXEPREFIX paexec "$@" 2>&1
}

runpaexec_resort (){
    $EXEPREFIX paexec "$@" 2>&1 | resort
}

runpaargs (){
    $EXEPREFIX paargs "$@" 2>&1
}

cut_version (){
    awk '$1 == "paexec" || $1 == "paargs" {$2 = "x.y.x"} {print}'
}

cut_help (){
    awk '/^OPTIONS/ {exit 0} {print}'
}

cut_full_path_closed_stdin (){
    sed 's|^\(.*\) [^ ]*\(transport_closed_stdin.*\)$|\1 \2|'
}

spc2semicolon (){
    tr ' ' ';'
}

resort (){
    awk '{print $1, NR, $0}' |
    sort -k1,1n -k2,2n |
    awk '{sub(/^[^ ]+ [^ ]+ /, ""); print $0}'
}

gln (){
    awk -v task=$1 '
    $2 == task { ln = NR; tn = $1}
    $1 == tn && $2 == "success" {print ln; exit}
    $1 == tn && $2 == "failure" {print "f"; exit}
    $2 == "failure" {tf [$1] = 1}
    $1 in tf {
        for (i=3; i <= NF; ++i) {fail [$i] = 1}
    }
    END {
        if (task in fail) {print "rf"}
    }
    ' $OBJDIR/_test.tmp
}

filter_succeded_tasks (){
    awk '
    $3 == "success" {
        print $2, hash [$1, $2]
        next
    }
    $3 != "fatal" {
        hash [$1, $2] = $3
    }'
}

test_tasks1 (){
    cat<<EOF
pcc
weight: gcc 10
weight: tcl 8
glibc
weight: glibc 9
weight: python 7
python
weight: pcc 4
weight: dictd 3
weight: mplayer 11
weight: pike 6
weight: ruby 5
weight: gnome 12
gnome
weight: kde 13
weight: runawk 2
weight: mk-configure 1
weight: qt4 14
EOF
}

test_tasks2 (){
    cat <<EOF
pipestatus pkg_status
pkg_summary-utils pkg_status
dict pkg_online-client
pkg_summary-utils pkg_online-client
pipestatus pkg_online-client
netcat pkg_online-client
dictd pkg_online-server
pkg_summary-utils pkg_online-server
pipestatus pkg_online-server
judyhash
runawk
libmaa paexec
libmaa dictd
libmaa dict
pipestatus pkg_summary-utils
pipestatus pkg_summary-utils
pipestatus pkg_summary-utils
pipestatus pkg_summary-utils
pipestatus pkg_summary-utils
paexec pkg_summary-utils
runawk pkg_summary-utils
weight: judyhash 12
weight: dictd 20
weight: dict 15
weight: pkg_summary-utils 2
weight: runawk 2
weight: paexec 4
weight: libmaa 5
weight: pkg_online-server 1
weight: pkg_online-client 1
EOF
}

test_tasks3 (){
    cat <<'EOF'
1 2
2 3
3 4
4 5
5 0
0 7
5 0
0 7
7 8
8 9
9 10
10 11
11 12
EOF
}

test_tasks4 (){
    cat <<'EOF'
1 2
2 3
3 4
4 5
5 0
0 7
7 8
8 9
5 10
10 11
11 12
5 10
10 11
11 12
EOF
}

paexec_reorder_input1 (){
    cat <<'EOF'
2 TABLE1
2 TABLE2
1 APPLE1
2 TABLE3
2 TABLE4
2 
1 APPLE2
3 GREEN1
3 GREEN2
3 GREEN3
1 APPLE3
1 APPLE4
3 GREEN4
3 
EOF
}

paexec_reorder_input2 (){
    cat <<'EOF'
2 fatal 1
2 fatal 2
1 APPLE1
2 fatal 3
2 fatal 4
2 
1 APPLE2
3 GREEN1
3 GREEN2
3 GREEN3
1 APPLE3
1 APPLE4
1 
3 GREEN4
3 
EOF
}

paexec_reorder_input3 (){
    cat <<'EOF'
2  TABLE1
2  TABLE2
1  APPLE1
2  TABLE3
2  TABLE4
2 success
2 
1  APPLE2
3  GREEN1
3  GREEN2
3  GREEN3
1  APPLE3
1  APPLE4
1 success
1 
3  GREEN???
3 failure
3 4 5
3 
EOF
}

paexec_reorder_input4 (){
    cat <<'EOF'
3  foo
3  bar
1  blablabla
1 fatal
1 
2  TABLE1
2  TABLE2
1  APPLE1
2  TABLE3
2  TABLE4
2 success
2 
1  APPLE2
3 fatal
3 
3  GREEN1
3  GREEN2
3  GREEN3
1  APPLE3
1  APPLE4
1 success
1 
3  GREEN???
3 failure
3 4 5
3 
EOF
}

paexec_reorder_input5 (){
    cat <<'EOF'
2 TABLE1
2 TABLE2
1 APPLE1
2 TABLE3
2 TABLE4
2 success
1 APPLE2
3 GREEN1
3 GREEN2
3 GREEN3
1 APPLE3
1 APPLE4
1 success
3 GREEN???
3 failure
3 4 5
EOF
}

nonstandard_msgs (){
    sed -e 's/^\([0-9][0-9]* \)$/\1Konec!/' \
	-e 's/^\([0-9][0-9]* \)success/\1Ura!/' \
	-e 's/^\([0-9][0-9]* \)failure/\1Zhopa!/' \
	-e 's/^\([0-9][0-9]* \)fatal/\1PolnayaZhopa!/' \
	"$@"
}

# on exit
on_exit (){
    rm -rf $tmpdir
}

sig_handler (){
    on_exit
    trap - "$1"
    kill -"$1" $$
}

trap "sig_handler INT"  INT
trap "sig_handler QUIT" QUIT
trap "sig_handler TERM" TERM
trap "on_exit" 0

#
tmpdir="/tmp/paexec-test.$$"
mkdir -m 0700 "$tmpdir" || exit 60

tmpfn1="$tmpdir/1"
tmpfn2="$tmpdir/2"
tmpfn3="$tmpdir/3"
tmpfn4="$tmpdir/4"
tmpex="$tmpdir/5"

# 
echo > $tmpex

cmp (){
    # $1 - progress message
    # $2 - expected text
    printf '    %s... ' "$1" 1>&2

    cat > "$tmpfn2"
    printf '%s' "$2" > "$tmpfn1"

    if $DIFF_PROG "$tmpfn1" "$tmpfn2" > "$tmpfn3"; then
	echo ok
    else
	echo FAILED
	awk '{print "   " $0}' "$tmpfn3"
	rm -f $tmpex
    fi
}

do_test (){
    runpaargs -V | cut_version |
cmp 'paargs -V' 'paargs x.y.x written by Aleksey Cheusov
'

    runpaargs -h 2>&1 | cut_help |
cmp 'paargs -h' 'paargs -- wrapper for paexec
usage: paargs [OPTIONS]
'

    runpaargs -t ssh -P localhost < /dev/null 2>&1 |
    cmp 'paargs missing free arguments' \
'paargs: missing arguments. Run paargs -h for details
'

    printf 'AAA\nBBB\nTRTRTR'\''BRBRBR\nCCC\nDDD\nEEE\nFFF\n"Y;X\nGGG\nZ Y X\n' |
	runpaargs -P +3 -I '{}' echo xxx '{}' yyy |
	paexec_reorder -lgy -Ms |
    cmp 'paargs -P #1.1' \
'1 xxx AAA yyy
1 success
2 xxx BBB yyy
2 success
3 xxx TRTRTR'"'"'BRBRBR yyy
3 success
4 xxx CCC yyy
4 success
5 xxx DDD yyy
5 success
6 xxx EEE yyy
6 success
7 xxx FFF yyy
7 success
8 xxx "Y;X yyy
8 success
9 xxx GGG yyy
9 success
10 xxx Z Y X yyy
10 success
'

        # -J
    printf 'aaa\nbbb\nz y x\ntrtrtrbrbrbr\nccc\nddd\neee\nfff\ny;x\nggg\n' |
	runpaargs -P+2 -I '$$' -c 'awk "BEGIN {print toupper(\"$$\")}"' |
	paexec_reorder -lgy -Ms |
    cmp 'paargs -P #1.2' \
'1 AAA
1 success
2 BBB
2 success
3 Z Y X
3 success
4 TRTRTRBRBRBR
4 success
5 CCC
5 success
6 DDD
6 success
7 EEE
7 success
8 FFF
8 success
9 Y;X
9 success
10 GGG
10 success
'

    printf 'AAA\nBBB\nZ Y X\nTRTRTR'\''BRBRBR\nCCC\nDDD\nEEE\nFFF\n"Y;X\nGGG\n' |
	runpaargs -P +3 -I '{}' -c 'echo "{} is equal to {}"' |
	paexec_reorder -lgy -Ms |
    cmp 'paargs -J #1.3' \
'1 AAA is equal to AAA
1 success
2 BBB is equal to BBB
2 success
3 Z Y X is equal to Z Y X
3 success
4 TRTRTR'"'"'BRBRBR is equal to TRTRTR'"'"'BRBRBR
4 success
5 CCC is equal to CCC
5 success
6 DDD is equal to DDD
6 success
7 EEE is equal to EEE
7 success
8 FFF is equal to FFF
8 success
9 "Y;X is equal to "Y;X
9 success
10 GGG is equal to GGG
10 success
'

    # -J
    printf 'AAA\nBBB\nZ Y X\nTRTRTR'\''BRBRBR\nCCC\nDDD\nEEE\nFFF\n"Y;X\nGGG\n' |
	runpaargs -I '{}' -c "echo '{} is equal to {}'" -P +3 |
	paexec_reorder -lgy -Ms |
    cmp 'paargs -J #1.4' \
'1 $1 is equal to $1
1 success
2 $1 is equal to $1
2 success
3 $1 is equal to $1
3 success
4 $1 is equal to $1
4 success
5 $1 is equal to $1
5 success
6 $1 is equal to $1
6 success
7 $1 is equal to $1
7 success
8 $1 is equal to $1
8 success
9 $1 is equal to $1
9 success
10 $1 is equal to $1
10 success
'

    # -J
    printf 'AAA\nBBB\nZ Y X\nTRTRTR'\''BRBRBR\nCCC\nDDD\nEEE\nFFF\n"Y;X\nGGG\n' |
	runpaargs -P+3 -I '{}' echo '{}' is equal to '{}' |
	paexec_reorder -lgy -Ms |
    cmp 'paargs -J #1.5' \
'1 AAA is equal to AAA
1 success
2 BBB is equal to BBB
2 success
3 Z Y X is equal to Z Y X
3 success
4 TRTRTR'"'"'BRBRBR is equal to TRTRTR'"'"'BRBRBR
4 success
5 CCC is equal to CCC
5 success
6 DDD is equal to DDD
6 success
7 EEE is equal to EEE
7 success
8 FFF is equal to FFF
8 success
9 "Y;X is equal to "Y;X
9 success
10 GGG is equal to GGG
10 success
'

    printf 'aaa\nbbb\nz y x\ntrtrtr'\''brbrbr\nccc\nddd\neee\nfff\n"y;x\nggg\n' |
	runpaargs -P+3 awk 'BEGIN {print toupper(ARGV [1])}' |
	paexec_reorder -lgy -Ms |
    cmp 'paargs #1.1' \
'1 AAA
1 success
2 BBB
2 success
3 Z Y X
3 success
4 TRTRTR'"'"'BRBRBR
4 success
5 CCC
5 success
6 DDD
6 success
7 EEE
7 success
8 FFF
8 success
9 "Y;X
9 success
10 GGG
10 success
'

    # -x + -t
    export PAEXEC_ENV='   ZZZZ, , ,YYYY,CCCC LALALA    '
    export ZZZZ=zz1234zz
    export YYYY=yy1234yy
    export CCCC=cc1234cc
    printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
    runpaargs -t paexec_notransport \
	-P '1 2 3 4 5 6 7 8 9' \
	awk 'BEGIN {print ENVIRON["ZZZZ"], ENVIRON["YYYY"], ENVIRON["CCCC"], ENVIRON["LALALA"], toupper(ARGV[1])}' |
    paexec_reorder -lgy -Ms | grep -v success | resort |
    cmp 'paargs -t #1 (PAEXEC_ENV)' \
'1 zz1234zz yy1234yy cc1234cc  A
2 zz1234zz yy1234yy cc1234cc  BB
3 zz1234zz yy1234yy cc1234cc  CCC
4 zz1234zz yy1234yy cc1234cc  DDDD
5 zz1234zz yy1234yy cc1234cc  EEEEE
6 zz1234zz yy1234yy cc1234cc  FFFFFF
'
    unset ZZZZ YYYY CCCC PAEXEC_ENV

    #
    runpaargs -t ssh -P localhost -c echo file1 file2 < /dev/null 2>&1 |
    cmp 'paargs bad files' \
'paargs: extra arguments. Run paargs -h for details
'

############################################################    
    runpaexec -V | cut_version |
cmp 'paexec -V' 'paexec x.y.x written by Aleksey Cheusov
'

    runpaexec -h 2>&1 | cut_help |
cmp 'paexec -h' 'paexec -- parallel executor
         that distributes tasks over CPUs or machines in a network.
usage: paexec    [OPTIONS]
       paexec -C [OPTIONS] cmd [args...]
'

    # bad -md= arg
    runpaexec -cfake -n+3 -md=aa 2>&1 |
    cmp 'paexec -md= #1' 'paexec: bad argument for -md=. At most one character is allowed
'

    # bad -ms=/-mf/-mF arg
    runpaexec -ms='lalala"trtrtr' 2>&1 |
    cmp 'paexec -ms= bad #1.1' 'paexec: symbols '\'' and " are not allowed in -m argument
'

    runpaexec -ms="lalala'trtrtr" 2>&1 |
    cmp 'paexec -ms= bad #1.2' 'paexec: symbols '\'' and " are not allowed in -m argument
'


    runpaexec -ms='lalala"trtrtr' 2>&1 |
    cmp 'paexec -mf= bad #1.1' 'paexec: symbols '\'' and " are not allowed in -m argument
'

    runpaexec -ms="lalala'trtrtr" 2>&1 |
    cmp 'paexec -mf= bad #1.2' 'paexec: symbols '\'' and " are not allowed in -m argument
'


    runpaexec -ms='lalala"trtrtr' 2>&1 |
    cmp 'paexec -mF= bad #1.1' 'paexec: symbols '\'' and " are not allowed in -m argument
'

    runpaexec -ms="lalala'trtrtr" 2>&1 |
    cmp 'paexec -mF= bad #1.2' 'paexec: symbols '\'' and " are not allowed in -m argument
'

    # bad use
    runpaexec -l -t dummy -c dummy -n ',bad arg' < /dev/null 2>&1 |
    cmp 'paexec bad -n #1' \
'paexec: invalid argument for option -n
'

    runpaexec -t ssh -n +32 < /dev/null 2>&1 |
    cmp 'paexec bad -c' \
'paexec: -c option is mandatory!
'

    runpaexec -t ssh -n localhost -x -c echo file1 file2 < /dev/null 2>&1 |
    cmp 'paexec bad files' \
'paexec: extra arguments. Run paexec -h for details
'

    runpaexec -t ssh -n localhost -Cx  < /dev/null 2>&1 |
    cmp 'paexec bad -C' \
'paexec: missing arguments. Run paexec -h for details
'

    # -J
    printf 'AAA\nBBB\nZ Y X\nTRTRTR'\''BRBRBR\nCCC\nDDD\nEEE\nFFF\n"Y;X\nGGG\n' |
    runpaexec -J '{}' -c 'echo "xxx{}yyy"' -n +3 | sort |
    cmp 'paexec -J #1.1' \
'xxx"Y;Xyyy
xxxAAAyyy
xxxBBByyy
xxxCCCyyy
xxxDDDyyy
xxxEEEyyy
xxxFFFyyy
xxxGGGyyy
xxxTRTRTR'"'"'BRBRBRyyy
xxxZ Y Xyyy
'

    # -J
    printf 'aaa\nbbb\nz y x\ntrtrtrbrbrbr\nccc\nddd\neee\nfff\ny;x\nggg\n' |
    runpaexec -n+2 -J '$$' -c 'awk "BEGIN {print toupper(\"$$\")}"' | sort |
    cmp 'paexec -J #1.2' \
'AAA
BBB
CCC
DDD
EEE
FFF
GGG
TRTRTRBRBRBR
Y;X
Z Y X
'

    # -J
    printf 'AAA\nBBB\nZ Y X\nTRTRTR'\''BRBRBR\nCCC\nDDD\nEEE\nFFF\n"Y;X\nGGG\n' |
    runpaexec -J '{}' -c 'echo "{} is equal to {}"' -n +3 | sort |
    cmp 'paexec -J #1.3' \
'"Y;X is equal to "Y;X
AAA is equal to AAA
BBB is equal to BBB
CCC is equal to CCC
DDD is equal to DDD
EEE is equal to EEE
FFF is equal to FFF
GGG is equal to GGG
TRTRTR'"'"'BRBRBR is equal to TRTRTR'"'"'BRBRBR
Z Y X is equal to Z Y X
'

    # -J
    printf 'AAA\nBBB\nZ Y X\nTRTRTR'\''BRBRBR\nCCC\nDDD\nEEE\nFFF\n"Y;X\nGGG\n' |
    runpaexec -J '{}' -c "echo '{} is equal to {}'" -n +3 | sort |
    cmp 'paexec -J #1.4' \
'$1 is equal to $1
$1 is equal to $1
$1 is equal to $1
$1 is equal to $1
$1 is equal to $1
$1 is equal to $1
$1 is equal to $1
$1 is equal to $1
$1 is equal to $1
$1 is equal to $1
'

    # -J
    printf 'AAA\nBBB\nZ Y X\nTRTRTR'\''BRBRBR\nCCC\nDDD\nEEE\nFFF\n"Y;X\nGGG\n' |
    runpaexec  -n +3 -J '{}' -C echo '{}' 'is equal to' '{}' | sort |
    cmp 'paexec -J #1.5' \
'"Y;X is equal to "Y;X
AAA is equal to AAA
BBB is equal to BBB
CCC is equal to CCC
DDD is equal to DDD
EEE is equal to EEE
FFF is equal to FFF
GGG is equal to GGG
TRTRTR'"'"'BRBRBR is equal to TRTRTR'"'"'BRBRBR
Z Y X is equal to Z Y X
'

    # -J
    printf 'AAA\nBBB\nZ Y X\nTRTRTR'\''BRBRBR\nCCC\nDDD\nEEE\nFFF\n"Y;X\nGGG\n' |
    runpaexec  -n +3 -J '{}' -C echo '{}' is not equal to ' {}' | sort |
    cmp 'paexec -J #1.6' \
'"Y;X is not equal to  $1
AAA is not equal to  $1
BBB is not equal to  $1
CCC is not equal to  $1
DDD is not equal to  $1
EEE is not equal to  $1
FFF is not equal to  $1
GGG is not equal to  $1
TRTRTR'"'"'BRBRBR is not equal to  $1
Z Y X is not equal to  $1
'

    # -J
    printf 'AAA\nBBB\nZ Y X\nTRTRTR'\''BRBRBR\nCCC\nDDD\nEEE\nFFF\n"Y;X\nGGG\n' |
    runpaexec  -n +3 -J '{}' -C echo '{}/' is not equal to '{}' | sort |
    cmp 'paexec -J #1.7' \
'$1/ is not equal to "Y;X
$1/ is not equal to AAA
$1/ is not equal to BBB
$1/ is not equal to CCC
$1/ is not equal to DDD
$1/ is not equal to EEE
$1/ is not equal to FFF
$1/ is not equal to GGG
$1/ is not equal to TRTRTR'"'"'BRBRBR
$1/ is not equal to Z Y X
'

    # x
    printf 'aaa\nbbb\nz y x\ntrtrtr'\''brbrbr\nccc\nddd\neee\nfff\n"y;x\nggg\n' |
    runpaexec -x -c "awk 'BEGIN {print toupper(ARGV [1])}'" -n +3 | sort |
    cmp 'paexec -x #1.1' \
'"Y;X
AAA
BBB
CCC
DDD
EEE
FFF
GGG
TRTRTR'"'"'BRBRBR
Z Y X
'

    # x
    export PAEXEC_NODES=+3
    printf 'aaa\nbbb\nz y x\ntrtrtr'\''brbrbr\nccc\nddd\neee\nfff\n"y;x\nggg\n' |
    runpaexec -x -c "awk 'BEGIN {print toupper(ARGV [1])}'" | sort |
    cmp 'paexec -x #1.1.1 (PAEXEC_NODES)' \
'"Y;X
AAA
BBB
CCC
DDD
EEE
FFF
GGG
TRTRTR'"'"'BRBRBR
Z Y X
'
    export PAEXEC_NODES

    printf 'aaa\nbbb\nz y x\ntrtrtr'\''brbrbr\nccc\nddd\neee\nfff\n"y;x\nggg\n' |
    runpaexec -xCn+3 -- awk 'BEGIN {print toupper(ARGV [1])}' | sort |
    cmp 'paexec -x #1.2' \
'"Y;X
AAA
BBB
CCC
DDD
EEE
FFF
GGG
TRTRTR'"'"'BRBRBR
Z Y X
'

    printf 'aaa\nbbb\nz y x\ntrtrtr'\''brbrbr\nccc\nddd\neee\nfff\n"y;x\nggg\n' |
    runpaexec -el -x -c 'awk "BEGIN {print toupper(ARGV[1])}"' -n +4 |
    paexec_reorder -x -Ms |
    cmp 'paexec -x #2' \
'AAA
BBB
Z Y X
TRTRTR'"'"'BRBRBR
CCC
DDD
EEE
FFF
"Y;X
GGG
'

    # x
    rm -f fakeflac/*.flac

    ls -1 fakeflac/*.wav |
    runpaexec -x -c 'flac --silent' -n +3 -p |
    sed 's/[0-9][0-9]*/NNN/' |
    cmp 'paexec -x (flac) #2.1' \
'NNN 
NNN 
NNN 
NNN 
NNN 
';

    ls -1 fakeflac/*.flac 2>/dev/null | sed 's,.*/,,' | sort |
    cmp 'paexec -x (flac) #2.2' \
'fake1.flac
fake2.flac
fake3.flac
fake4.flac
fake5.flac
'
    rm -f fakeflac/*.flac;

    # x
    run_wav2flac ./fakeflac 3 |
    cmp 'paexec -x (flac) #3.1' \
'




'

    # -X
    rm -f fakeflac/*.flac

    ls -1 fakeflac/*.wav |
    runpaexec -X -c 'flac --silent' -n +3 -p |
    cmp 'paexec -X (flac) #3.1.1' \
''

    ls -1 fakeflac/*.flac | sort |
    cmp 'paexec -X (flac) #3.1.2' \
'fakeflac/fake1.flac
fakeflac/fake2.flac
fakeflac/fake3.flac
fakeflac/fake4.flac
fakeflac/fake5.flac
'

    # -X
    rm -f fakeflac/*.flac

    ls -1 fakeflac/*.wav |
    runpaexec -Xg -c 'flac --silent' -n +3 |
    cmp 'paexec -X (flac) #3.1.3' \
'success
success
success
success
success
'

    ls -1 fakeflac/*.flac | sort |
    cmp 'paexec -X (flac) #3.1.4' \
'fakeflac/fake1.flac
fakeflac/fake2.flac
fakeflac/fake3.flac
fakeflac/fake4.flac
fakeflac/fake5.flac
'

    # x
    ls -1 fakeflac/*.flac 2>/dev/null | sed 's,.*/,,' | sort |
    cmp 'paexec -x (flac) #3.2' \
'fake1.flac
fake2.flac
fake3.flac
fake4.flac
fake5.flac
'
    rm -f fakeflac/*.flac;

    # x
    ( cd ..; run_dirtest < examples/dirtest/tasks ) |
    paexec_reorder -l -Mm |
    cmp 'paexec -x (dirtest) #4' \
'1 failure
1 /nonexistant;/nonexistant/subdir;/nonexistant/subdir/subsubdir
4 success
5 success
6 success
7 success
8 success
9 success
10 success
11 success
12 success
13 success
14 success
15 success
16 success
17 success
18 failure
18 /etc/dir with spaces;/etc/dir with spaces/subdir
'

    $FIND_CMD . -type f -name '*.wav' -print0 |
	runpaexec -n+6 -0x -C echo |
    sort | sed 's|^[.]/||' |
    cmp 'paexec -0' \
'fakeflac/fake1.wav
fakeflac/fake2.wav
fakeflac/fake3.wav
fakeflac/fake4.wav
fakeflac/fake5.wav
'

    # toupper
    printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
    runpaexec_resort -l -t paexec_notransport -c cmd_toupper \
	-n '1 2 3 4 5 6 7 8 9' |
    cmp 'paexec toupper #1' \
'1  A
2  BB
3  CCC
4  DDDD
5  EEEEE
6  FFFFFF
'

    export PAEXEC_TRANSPORT=paexec_notransport
    printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
    runpaexec_resort -l -c cmd_toupper \
	-n '1 2 3 4 5 6 7 8 9' |
    cmp 'paexec toupper #1.0.1 (PAEXEC_TRANSPORT)' \
'1  A
2  BB
3  CCC
4  DDDD
5  EEEEE
6  FFFFFF
'
    unset PAEXEC_TRANSPORT

    # toupper
    printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
    ( export PAEXEC_EOT=foobarbaz;
	runpaexec_resort -l -t paexec_notransport -c cmd_toupper \
	   -n '1 2 3 4 5 6 7 8 9'; ) |
    cmp 'paexec toupper #1.1 (PAEXEC_EOT)' \
'1  A
2  BB
3  CCC
4  DDDD
5  EEEEE
6  FFFFFF
'

    # toupper
    printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
    runpaexec_resort -l -t '		    ' -c cmd_toupper \
	-n '1 2 3 4 5 6 7 8 9' |
    cmp 'paexec toupper #2' \
'1  A
2  BB
3  CCC
4  DDDD
5  EEEEE
6  FFFFFF
'

    printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
    runpaexec_resort -l -c cmd_toupper \
	-n '1 2 3 4 5 6 7 8 9' |
    cmp 'paexec toupper #3' \
'1  A
2  BB
3  CCC
4  DDDD
5  EEEEE
6  FFFFFF
'

    printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
    runpaexec_resort -l -p -t paexec_notransport \
	-c cmd_toupper -n '+2' |
    awk '$1 ~ /^[0-9]/ {$2 = "pid"; print; next} {print}' |
    cmp 'paexec toupper #4' \
'1 pid A
2 pid BB
3 pid CCC
4 pid DDDD
5 pid EEEEE
6 pid FFFFFF
'

    export PAEXEC_NODES=+2
    printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
    runpaexec -l -p -t paexec_notransport \
	-c cmd_toupper |
    resort | awk '$1 ~ /^[0-9]/ {$2 = "pid"; print; next} {print}' |
    cmp 'paexec toupper #4.0.1 (PAEXEC_NODES)' \
'1 pid A
2 pid BB
3 pid CCC
4 pid DDDD
5 pid EEEEE
6 pid FFFFFF
'
    unset PAEXEC_NODES

    printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
    runpaexec -l -p -t '' \
	-c cmd_toupper -n '+2' |
    resort | awk '$1 ~ /^[0-9]/ {$2 = "pid"; print; next} {print}' |
    cmp 'paexec toupper #5' \
'1 pid A
2 pid BB
3 pid CCC
4 pid DDDD
5 pid EEEEE
6 pid FFFFFF
'

    printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
    runpaexec -l -p \
	-c cmd_toupper -n '+2' |
    resort | awk '$1 ~ /^[0-9]/ {$2 = "pid"; print; next} {print}' |
    cmp 'paexec toupper #6' \
'1 pid A
2 pid BB
3 pid CCC
4 pid DDDD
5 pid EEEEE
6 pid FFFFFF
'

    printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
    runpaexec -l -p \
	-c cmd_toupper -n '+2' |
    resort | awk '$1 ~ /^[0-9]/ {$2 = "pid"; print; next} {print}' |
    cmp 'paexec toupper #6' \
'1 pid A
2 pid BB
3 pid CCC
4 pid DDDD
5 pid EEEEE
6 pid FFFFFF
'

    printf 'a\nbb\n   ccc  with \\backslashes and  spaces   \\\ndddd\neeeee\nffffff\n' |
    runpaexec -lpx -n+7 \
	    -C awk 'BEGIN {print toupper(ARGV[1])}' |
    resort | awk '{ sub(/^[0-9]+ [0-9]+ /, "num pid "); print }' |
    cmp 'paexec toupper #6.5 (-x, spaces and backslashes)' \
'num pid A
num pid BB
num pid    CCC  WITH \BACKSLASHES AND  SPACES   \
num pid DDDD
num pid EEEEE
num pid FFFFFF
'

    # all_substr
    printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
    runpaexec -l -c cmd_all_substr \
	-n '1 2 3 4 5 6 7 8 9' | resort |
    cmp 'paexec all_substr #1.1' \
'1 substr[1,1]=a
2 substr[1,1]=b
2 substr[1,2]=bb
2 substr[2,1]=b
3 substr[1,1]=c
3 substr[1,2]=cc
3 substr[1,3]=ccc
3 substr[2,1]=c
3 substr[2,2]=cc
3 substr[3,1]=c
4 substr[1,1]=d
4 substr[1,2]=dd
4 substr[1,3]=ddd
4 substr[1,4]=dddd
4 substr[2,1]=d
4 substr[2,2]=dd
4 substr[2,3]=ddd
4 substr[3,1]=d
4 substr[3,2]=dd
4 substr[4,1]=d
5 substr[1,1]=e
5 substr[1,2]=ee
5 substr[1,3]=eee
5 substr[1,4]=eeee
5 substr[1,5]=eeeee
5 substr[2,1]=e
5 substr[2,2]=ee
5 substr[2,3]=eee
5 substr[2,4]=eeee
5 substr[3,1]=e
5 substr[3,2]=ee
5 substr[3,3]=eee
5 substr[4,1]=e
5 substr[4,2]=ee
5 substr[5,1]=e
6 substr[1,1]=f
6 substr[1,2]=ff
6 substr[1,3]=fff
6 substr[1,4]=ffff
6 substr[1,5]=fffff
6 substr[1,6]=ffffff
6 substr[2,1]=f
6 substr[2,2]=ff
6 substr[2,3]=fff
6 substr[2,4]=ffff
6 substr[2,5]=fffff
6 substr[3,1]=f
6 substr[3,2]=ff
6 substr[3,3]=fff
6 substr[3,4]=ffff
6 substr[4,1]=f
6 substr[4,2]=ff
6 substr[4,3]=fff
6 substr[5,1]=f
6 substr[5,2]=ff
6 substr[6,1]=f
'

    # all_substr
    printf '1\n2\n3\n4\n5\n6\n7\n8\n9\n' > "$tmpfn4"
    printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
    runpaexec -l -c cmd_all_substr \
	-n ":$tmpfn4" | resort |
    cmp 'paexec all_substr #1.2' \
'1 substr[1,1]=a
2 substr[1,1]=b
2 substr[1,2]=bb
2 substr[2,1]=b
3 substr[1,1]=c
3 substr[1,2]=cc
3 substr[1,3]=ccc
3 substr[2,1]=c
3 substr[2,2]=cc
3 substr[3,1]=c
4 substr[1,1]=d
4 substr[1,2]=dd
4 substr[1,3]=ddd
4 substr[1,4]=dddd
4 substr[2,1]=d
4 substr[2,2]=dd
4 substr[2,3]=ddd
4 substr[3,1]=d
4 substr[3,2]=dd
4 substr[4,1]=d
5 substr[1,1]=e
5 substr[1,2]=ee
5 substr[1,3]=eee
5 substr[1,4]=eeee
5 substr[1,5]=eeeee
5 substr[2,1]=e
5 substr[2,2]=ee
5 substr[2,3]=eee
5 substr[2,4]=eeee
5 substr[3,1]=e
5 substr[3,2]=ee
5 substr[3,3]=eee
5 substr[4,1]=e
5 substr[4,2]=ee
5 substr[5,1]=e
6 substr[1,1]=f
6 substr[1,2]=ff
6 substr[1,3]=fff
6 substr[1,4]=ffff
6 substr[1,5]=fffff
6 substr[1,6]=ffffff
6 substr[2,1]=f
6 substr[2,2]=ff
6 substr[2,3]=fff
6 substr[2,4]=ffff
6 substr[2,5]=fffff
6 substr[3,1]=f
6 substr[3,2]=ff
6 substr[3,3]=fff
6 substr[3,4]=ffff
6 substr[4,1]=f
6 substr[4,2]=ff
6 substr[4,3]=fff
6 substr[5,1]=f
6 substr[5,2]=ff
6 substr[6,1]=f
'

    # all_substr
    printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
    runpaexec -l -c cmd_all_substr -n '+9' |
    resort |
    cmp 'paexec all_substr #2' \
'1 substr[1,1]=a
2 substr[1,1]=b
2 substr[1,2]=bb
2 substr[2,1]=b
3 substr[1,1]=c
3 substr[1,2]=cc
3 substr[1,3]=ccc
3 substr[2,1]=c
3 substr[2,2]=cc
3 substr[3,1]=c
4 substr[1,1]=d
4 substr[1,2]=dd
4 substr[1,3]=ddd
4 substr[1,4]=dddd
4 substr[2,1]=d
4 substr[2,2]=dd
4 substr[2,3]=ddd
4 substr[3,1]=d
4 substr[3,2]=dd
4 substr[4,1]=d
5 substr[1,1]=e
5 substr[1,2]=ee
5 substr[1,3]=eee
5 substr[1,4]=eeee
5 substr[1,5]=eeeee
5 substr[2,1]=e
5 substr[2,2]=ee
5 substr[2,3]=eee
5 substr[2,4]=eeee
5 substr[3,1]=e
5 substr[3,2]=ee
5 substr[3,3]=eee
5 substr[4,1]=e
5 substr[4,2]=ee
5 substr[5,1]=e
6 substr[1,1]=f
6 substr[1,2]=ff
6 substr[1,3]=fff
6 substr[1,4]=ffff
6 substr[1,5]=fffff
6 substr[1,6]=ffffff
6 substr[2,1]=f
6 substr[2,2]=ff
6 substr[2,3]=fff
6 substr[2,4]=ffff
6 substr[2,5]=fffff
6 substr[3,1]=f
6 substr[3,2]=ff
6 substr[3,3]=fff
6 substr[3,4]=ffff
6 substr[4,1]=f
6 substr[4,2]=ff
6 substr[4,3]=fff
6 substr[5,1]=f
6 substr[5,2]=ff
6 substr[6,1]=f
'

    # no input
    runpaexec -c cmd_all_substr -n +3 < /dev/null |
    cmp 'paexec all_substr #3' ''

    # bad command + no input
    runpaexec -l -c /path/to/bad/prog -n +3 < /dev/null 2>/dev/null 1>&2|
    cmp 'paexec bad_command' ''

    # bi-i-i-i-i-i-ig result
    for i in 0 1 2 3 4 5 6 7 8 9; do
	awk '
	BEGIN {
	    for (i=0; i < 10; ++i) {
		print "1234567890-=qwertyuiop[]asdfghjkl;zxcvbnm,./zaqwsxcderfvbgtyhnmjuik,.lo";
	    }
	}' | runpaexec -c big_result_cmd -n '+9' |
	uniq -c |
	head -n 100 |
	awk '{$1 = $1; print $0}' |
	cmp 'paexec big_result_cmd' \
'100000 1234567890-=QWERTYUIOP[]ASDFGHJKL;ZXCVBNM,./ZAQWSXCDERFVBGTYHNMJUIK,.LO
'

	if test -f $tmpex; then
	    :
	else
	    break
	fi
    done

    # tests for partially ordered set of tasks (-s option)
    test_tasks3 |
    runpaexec -e -s -l -c cmd_divide -n +10 |
    cmp 'paexec 1/X #1' \
'1 1/1=1
1 success
1 
2 1/2=0.5
2 success
2 
3 1/3=0.333333
3 success
3 
4 1/4=0.25
4 success
4 
5 1/5=0.2
5 success
5 
6 Cannot calculate 1/0
6 failure
6 0 7 8 9 10 11 12
6 
'

    test_tasks3 | spc2semicolon |
    runpaexec -eslmd=";" -c cmd_divide -n +10 |
    cmp 'paexec 1/X -md=";" #1.1' \
'1 1/1=1
1 success
1 
2 1/2=0.5
2 success
2 
3 1/3=0.333333
3 success
3 
4 1/4=0.25
4 success
4 
5 1/5=0.2
5 success
5 
6 Cannot calculate 1/0
6 failure
6 0;7;8;9;10;11;12
6 
'

    test_tasks3 |
    runpaexec -ms='Ura!' -mf='Zhopa!' -mt='Konec!' \
	-e -s -l -c cmd_divide2 -n +10 |
    cmp 'paexec 1/X nonstandard #1' \
'1 1/1=1
1 Ura!
1 Konec!
2 1/2=0.5
2 Ura!
2 Konec!
3 1/3=0.333333
3 Ura!
3 Konec!
4 1/4=0.25
4 Ura!
4 Konec!
5 1/5=0.2
5 Ura!
5 Konec!
6 Cannot calculate 1/0
6 Zhopa!
6 0 7 8 9 10 11 12
6 Konec!
'

    test_tasks3 |
    ( export PAEXEC_EOT='Konec!'; runpaexec -ms='Ura!' -mf='Zhopa!' \
	-e -s -l -c cmd_divide2 -n +10; ) |
    cmp 'paexec 1/X nonstandard #1.1' \
'1 1/1=1
1 Ura!
1 Konec!
2 1/2=0.5
2 Ura!
2 Konec!
3 1/3=0.333333
3 Ura!
3 Konec!
4 1/4=0.25
4 Ura!
4 Konec!
5 1/5=0.2
5 Ura!
5 Konec!
6 Cannot calculate 1/0
6 Zhopa!
6 0 7 8 9 10 11 12
6 Konec!
'

    test_tasks3 |
    ( export PAEXEC_EOT='Konec!'; runpaexec -ms='Ura!' -mf='Zhopa!' \
	-esly -c cmd_divide2 -n +10; ) |
    cmp 'paexec 1/X nonstandard -y #1.1' \
'1 1/1=1
1 Ura!
1 HG>&OSO@#;L8N;!&.U4ZC_9X:0AF,2Y>SRXAD_7U&QZ5S>N^?Y,I=W?@5
2 1/2=0.5
2 Ura!
2 HG>&OSO@#;L8N;!&.U4ZC_9X:0AF,2Y>SRXAD_7U&QZ5S>N^?Y,I=W?@5
3 1/3=0.333333
3 Ura!
3 HG>&OSO@#;L8N;!&.U4ZC_9X:0AF,2Y>SRXAD_7U&QZ5S>N^?Y,I=W?@5
4 1/4=0.25
4 Ura!
4 HG>&OSO@#;L8N;!&.U4ZC_9X:0AF,2Y>SRXAD_7U&QZ5S>N^?Y,I=W?@5
5 1/5=0.2
5 Ura!
5 HG>&OSO@#;L8N;!&.U4ZC_9X:0AF,2Y>SRXAD_7U&QZ5S>N^?Y,I=W?@5
6 Cannot calculate 1/0
6 Zhopa!
6 0 7 8 9 10 11 12
6 HG>&OSO@#;L8N;!&.U4ZC_9X:0AF,2Y>SRXAD_7U&QZ5S>N^?Y,I=W?@5
'

    test_tasks3 |
    ( export PAEXEC_EOT='Konec!'; runpaexec -ms='Ura!' -mf='Zhopa!' \
	-esly -c cmd_divide2 -n +10; ) | paexec_reorder -y |
    cmp 'paexec 1/X nonstandard -y #1.2' \
'1/1=1
Ura!
1/2=0.5
Ura!
1/3=0.333333
Ura!
1/4=0.25
Ura!
1/5=0.2
Ura!
Cannot calculate 1/0
Zhopa!
0 7 8 9 10 11 12
'

    test_tasks3 |
    ( export PAEXEC_EOT='Konec!'; export PAEXEC_TRANSPORT=lalala;
	runpaexec -ms='Ura!' -mf='Zhopa!' \
	-esly -c cmd_divide2 -n +10; ) | paexec_reorder -y |
    cmp 'paexec 1/X nonstandard -y #1.2.1 (PAEXEC_TRANSPORT)' \
'1/1=1
Ura!
1/2=0.5
Ura!
1/3=0.333333
Ura!
1/4=0.25
Ura!
1/5=0.2
Ura!
Cannot calculate 1/0
Zhopa!
0 7 8 9 10 11 12
'

    # -s and no input
    runpaexec -s -l -c cmd_divide -n +10 < /dev/null |
    cmp 'paexec 1/X #2' ''

    runpaexec -ms='Ura!' -mf='Zhopa!' -mt='Konec!' \
	-s -l -c cmd_divide -n +10 < /dev/null |
    cmp 'paexec 1/X nonstandard #2' ''

    ( export PAEXEC_EOT='Konec!'; runpaexec -ms='Ura!' -mf='Zhopa!' \
	-s -l -c cmd_divide -n +10 < /dev/null; ) |
    cmp 'paexec 1/X nonstandard #2.1' ''

    # paexec_reorder + failure
    test_tasks4 |
    runpaexec -se -l -c cmd_divide -n +1 |
    paexec_reorder -gl |
    cmp 'paexec 1/X #3' \
'1 1/1=1
1 success
2 1/2=0.5
2 success
3 1/3=0.333333
3 success
4 1/4=0.25
4 success
5 1/5=0.2
5 success
6 Cannot calculate 1/0
6 failure
6 0 7 8 9
10 1/10=0.1
10 success
11 1/11=0.0909091
11 success
12 1/12=0.0833333
12 success
'

    test_tasks4 |
    runpaexec -ms='Ura!' -mf='Zhopa!' -mt='Konec!' \
	-se -l -c cmd_divide2 -n +1 |
    paexec_reorder -gl -ms='Ura!' -mf='Zhopa!' -mt='Konec!' |
    cmp 'paexec 1/X nonstandard #3' \
'1 1/1=1
1 Ura!
2 1/2=0.5
2 Ura!
3 1/3=0.333333
3 Ura!
4 1/4=0.25
4 Ura!
5 1/5=0.2
5 Ura!
6 Cannot calculate 1/0
6 Zhopa!
6 0 7 8 9
10 1/10=0.1
10 Ura!
11 1/11=0.0909091
11 Ura!
12 1/12=0.0833333
12 Ura!
'

    test_tasks4 |
    ( export PAEXEC_EOT='Konec!'; runpaexec -ms='Ura!' -mf='Zhopa!' \
	-se -l -c cmd_divide2 -n +1 |
    paexec_reorder -gl -ms='Ura!' -mf='Zhopa!'; ) |
    cmp 'paexec 1/X nonstandard #3.1' \
'1 1/1=1
1 Ura!
2 1/2=0.5
2 Ura!
3 1/3=0.333333
3 Ura!
4 1/4=0.25
4 Ura!
5 1/5=0.2
5 Ura!
6 Cannot calculate 1/0
6 Zhopa!
6 0 7 8 9
10 1/10=0.1
10 Ura!
11 1/11=0.0909091
11 Ura!
12 1/12=0.0833333
12 Ura!
'

    # paexec_reorder + failure
    ( cd ../examples/divide; ./run_divide; ) | sed 's/^[^ ]* //' | sort |
    cmp 'paexec 1/X #4' \
'0 7 8 9
1/10=0.1
1/11=0.0909091
1/12=0.0833333
1/1=1
1/2=0.5
1/3=0.333333
1/4=0.25
1/5=0.2
Cannot calculate 1/0
failure
success
success
success
success
success
success
success
success
'

    ( cd ../examples/divide; ./run_divide2; ) | sed 's/^[^ ]* //' | sort |
    cmp 'paexec 1/X #4 nonstandard' \
'0 7 8 9
1/10=0.1
1/11=0.0909091
1/12=0.0833333
1/1=1
1/2=0.5
1/3=0.333333
1/4=0.25
1/5=0.2
Cannot calculate 1/0
Ura!
Ura!
Ura!
Ura!
Ura!
Ura!
Ura!
Ura!
Zhopa!
'

    # -s all failed
    runpaexec -s -l -c 'cmd_xxx_failed_make_package .' -n +10 < /dev/null |
    cmp 'paexec all fails #1.1' ''

    runpaexec -n +10 -sCl cmd_xxx_failed_make_package . < /dev/null |
    cmp 'paexec all fails #1.2' ''

    # -s all failed
    runpaexec -s -l -c 'cmd_xxx_failed_make_package .' -n +5 < /dev/null |
    cmp 'paexec all fails #2' ''

    # -s all failed
    runpaexec -s -l -c 'cmd_xxx_failed_make_package .' -n +1 < /dev/null |
    cmp 'paexec all fails #3' ''

    # -s: all succeeded
    runpaexec -l -s -c cmd_make_package -n +2 \
	> $OBJDIR/_test.tmp < ../examples/make_package/tasks

    {
	test "`gln devel/glib2`" -gt 0 && echo ok1
	test "`gln textproc/dictem`" -gt 0 && echo ok2
	test "`gln devel/libjudy`" -gt 0 && echo ok3
	test "`gln devel/autoconf`" -lt "`gln wip/libmaa`" && echo ok4
	test "`gln devel/gmake`" -lt "`gln wip/libmaa`" && echo ok5
	test "`gln wip/libmaa`" -lt "`gln wip/dict-server`" && echo ok6
	test "`gln wip/libmaa`" -lt "`gln wip/dict-client`" && echo ok7
	test "`gln devel/m4`" -lt "`gln wip/dict-server`" && echo ok8
	test "`gln devel/byacc`" -lt "`gln wip/dict-client`" && echo ok9
	test "`gln devel/byacc`" -lt "`gln wip/dict-server`" && echo ok10
	test "`gln devel/flex`" -lt "`gln wip/dict-client`" && echo ok11
	test "`gln devel/flex`" -lt "`gln wip/dict-server`" && echo ok12
	
	test "`awk 'END {print NR}' $OBJDIR/_test.tmp`" -eq 22 && echo ok100
    } | cmp 'paexec packages #1' \
'ok1
ok2
ok3
ok4
ok5
ok6
ok7
ok8
ok9
ok10
ok11
ok12
ok100
'

    # -s: byacc fails
    runpaexec -l -s -c 'cmd_xxx_failed_make_package byacc' \
	-n +3 > $OBJDIR/_test.tmp < ../examples/make_package/tasks

    {
	test "`gln devel/glib2`" -gt 0 && echo ok1
	test "`gln textproc/dictem`" -gt 0 && echo ok2
	test "`gln devel/libjudy`" -gt 0 && echo ok3
	test "`gln devel/autoconf`" -lt "`gln wip/libmaa`" && echo ok4
	test "`gln devel/gmake`" -lt "`gln wip/libmaa`" && echo ok5
	test "`gln wip/libmaa`" -gt 0 && echo ok6
	test "`gln devel/m4`" -gt 0 && echo ok7
	test "`gln devel/flex`" -gt 0 && echo ok8
	test "`gln devel/byacc`" = f && echo ok9

	test "`gln wip/dict-server`" = "rf" && echo ok10
	test "`gln wip/dict-client`" = "rf" && echo ok11

	test "`awk 'END {print NR}' $OBJDIR/_test.tmp`" -eq 19 && echo ok100
    } | cmp 'paexec packages #2' \
'ok1
ok2
ok3
ok4
ok5
ok6
ok7
ok8
ok9
ok10
ok11
ok100
'

    # -s: flex fails
    runpaexec -l -s -c 'cmd_xxx_failed_make_package flex' \
	-n +5 > $OBJDIR/_test.tmp < ../examples/make_package/tasks

    {
	test "`gln devel/glib2`" -gt 0 && echo ok1
	test "`gln textproc/dictem`" -gt 0 && echo ok2
	test "`gln devel/libjudy`" -gt 0 && echo ok3
	test "`gln devel/autoconf`" -lt "`gln wip/libmaa`" && echo ok4
	test "`gln devel/gmake`" -lt "`gln wip/libmaa`" && echo ok5
	test "`gln wip/libmaa`" -gt 0 && echo ok6
	test "`gln devel/m4`" -gt 0 && echo ok7
	test "`gln devel/byacc`" -gt 0 && echo ok8
	test "`gln devel/flex`" = f && echo ok9

	test "`gln wip/dict-server`" = "rf" && echo ok10
	test "`gln wip/dict-client`" = "rf" && echo ok11

	test "`awk 'END {print NR}' $OBJDIR/_test.tmp`" -eq 19 && echo ok100
    } | cmp 'paexec packages #3' \
'ok1
ok2
ok3
ok4
ok5
ok6
ok7
ok8
ok9
ok10
ok11
ok100
'

    # -s: libmaa fails
    runpaexec -l -s -c 'cmd_xxx_failed_make_package libmaa' \
	-n +6 > $OBJDIR/_test.tmp < ../examples/make_package/tasks

    {
	test "`gln devel/glib2`" -gt 0 && echo ok1
	test "`gln textproc/dictem`" -gt 0 && echo ok2
	test "`gln devel/libjudy`" -gt 0 && echo ok3
	test "`gln devel/autoconf`" -gt 0 && echo ok4
	test "`gln devel/gmake`" -gt 0 && echo ok5
	test "`gln devel/flex`" -gt 0 && echo ok66
	test "`gln devel/m4`" -gt 0 && echo ok7
	test "`gln devel/byacc`" -gt 0 && echo ok8
	test "`gln wip/libmaa`" = f && echo ok9

	test "`gln wip/dict-server`" = "rf" && echo ok10
	test "`gln wip/dict-client`" = "rf" && echo ok11

	test "`awk 'END {print NR}' $OBJDIR/_test.tmp`" -eq 19 && echo ok100
    } | cmp 'paexec packages #4' \
'ok1
ok2
ok3
ok4
ok5
ok66
ok7
ok8
ok9
ok10
ok11
ok100
'

    # -s: m4 fails
    runpaexec -l -s -c 'cmd_xxx_failed_make_package m4' \
	-n +4 > $OBJDIR/_test.tmp < ../examples/make_package/tasks

    {
	test "`gln devel/glib2`" -gt 0 && echo ok1
	test "`gln textproc/dictem`" -gt 0 && echo ok2
	test "`gln devel/libjudy`" -gt 0 && echo ok3
	test "`gln devel/autoconf`" -lt "`gln wip/libmaa`" && echo ok4
	test "`gln devel/gmake`" -lt "`gln wip/libmaa`" && echo ok5
	test "`gln devel/flex`" -lt "`gln wip/dict-client`" && echo ok6
	test "`gln devel/byacc`" -lt "`gln wip/dict-client`" && echo ok7
	test "`gln wip/libmaa`" -lt "`gln wip/dict-client`" && echo ok8
	test "`gln devel/m4`" = f && echo ok9

	test "`gln wip/dict-server`" = "rf" && echo ok10

	test "`awk 'END {print NR}' $OBJDIR/_test.tmp`" -eq 21 && echo ok100
    } | cmp 'paexec packages #5' \
'ok1
ok2
ok3
ok4
ok5
ok6
ok7
ok8
ok9
ok10
ok100
'

    # -s: libjudy fails
    runpaexec -l -s -c 'cmd_xxx_failed_make_package libjudy' \
	-n +2 > $OBJDIR/_test.tmp < ../examples/make_package/tasks

    {
	test "`gln devel/glib2`" -gt 0 && echo ok1
	test "`gln textproc/dictem`" -gt 0 && echo ok2
	test "`gln devel/autoconf`" -lt "`gln wip/libmaa`" && echo ok4
	test "`gln devel/gmake`" -lt "`gln wip/libmaa`" && echo ok5
	test "`gln wip/libmaa`" -lt "`gln wip/dict-server`" && echo ok6
	test "`gln wip/libmaa`" -lt "`gln wip/dict-client`" && echo ok7
	test "`gln devel/m4`" -lt "`gln wip/dict-server`" && echo ok8
	test "`gln devel/byacc`" -lt "`gln wip/dict-client`" && echo ok9
	test "`gln devel/byacc`" -lt "`gln wip/dict-server`" && echo ok10
	test "`gln devel/flex`" -lt "`gln wip/dict-client`" && echo ok11
	test "`gln devel/flex`" -lt "`gln wip/dict-server`" && echo ok12

	test "`awk 'END {print NR}' $OBJDIR/_test.tmp`" -eq 23 && echo ok100
    } | cmp 'paexec packages #6' \
'ok1
ok2
ok4
ok5
ok6
ok7
ok8
ok9
ok10
ok11
ok12
ok100
'

    # -s: dictem fails
    runpaexec -l -s -c 'cmd_xxx_failed_make_package dictem' \
	-n +3 > $OBJDIR/_test.tmp < ../examples/make_package/tasks

    {
	test "`gln devel/glib2`" -gt 0 && echo ok1
	test "`gln devel/libjudy`" -gt 0 && echo ok3
	test "`gln devel/autoconf`" -lt "`gln wip/libmaa`" && echo ok4
	test "`gln devel/gmake`" -lt "`gln wip/libmaa`" && echo ok5
	test "`gln wip/libmaa`" -lt "`gln wip/dict-server`" && echo ok6
	test "`gln wip/libmaa`" -lt "`gln wip/dict-client`" && echo ok7
	test "`gln devel/m4`" -lt "`gln wip/dict-server`" && echo ok8
	test "`gln devel/byacc`" -lt "`gln wip/dict-client`" && echo ok9
	test "`gln devel/byacc`" -lt "`gln wip/dict-server`" && echo ok10
	test "`gln devel/flex`" -lt "`gln wip/dict-client`" && echo ok11
	test "`gln devel/flex`" -lt "`gln wip/dict-server`" && echo ok12

	test "`awk 'END {print NR}' $OBJDIR/_test.tmp`" -eq 23 && echo ok100
    } | cmp 'paexec packages #7' \
'ok1
ok3
ok4
ok5
ok6
ok7
ok8
ok9
ok10
ok11
ok12
ok100
'

    # -s: glib2 fails
    runpaexec -l -s -c 'cmd_xxx_failed_make_package glib2' \
	-n +6 > $OBJDIR/_test.tmp < ../examples/make_package/tasks

    {
	test "`gln textproc/dictem`" -gt 0 && echo ok2
	test "`gln devel/libjudy`" -gt 0 && echo ok3
	test "`gln devel/autoconf`" -lt "`gln wip/libmaa`" && echo ok4
	test "`gln devel/gmake`" -lt "`gln wip/libmaa`" && echo ok5
	test "`gln wip/libmaa`" -lt "`gln wip/dict-server`" && echo ok6
	test "`gln wip/libmaa`" -lt "`gln wip/dict-client`" && echo ok7
	test "`gln devel/m4`" -lt "`gln wip/dict-server`" && echo ok8
	test "`gln devel/byacc`" -lt "`gln wip/dict-client`" && echo ok9
	test "`gln devel/byacc`" -lt "`gln wip/dict-server`" && echo ok10
	test "`gln devel/flex`" -lt "`gln wip/dict-client`" && echo ok11
	test "`gln devel/flex`" -lt "`gln wip/dict-server`" && echo ok12

	test "`awk 'END {print NR}' $OBJDIR/_test.tmp`" -eq 23 && echo ok100
    } | cmp 'paexec packages #8' \
'ok2
ok3
ok4
ok5
ok6
ok7
ok8
ok9
ok10
ok11
ok12
ok100
'

    # -s: gmake fails
    runpaexec -l -s -c 'cmd_xxx_failed_make_package gmake' -n +5 \
	> $OBJDIR/_test.tmp < ../examples/make_package/tasks

    {
	test "`gln devel/glib2`" -gt 0 && echo ok1
	test "`gln textproc/dictem`" -gt 0 && echo ok2
	test "`gln devel/libjudy`" -gt 0 && echo ok3
	test "`gln devel/autoconf`" -gt 0 && echo ok4
	test "`gln devel/gmake`" = f && echo ok5
	test "`gln devel/m4`" -gt 0 && echo ok8
	test "`gln devel/byacc`" -gt 0 && echo ok9
	test "`gln devel/flex`" -gt 0 && echo ok10
	test "`gln wip/libmaa`"      = "rf" && echo ok11
	test "`gln wip/dict-server`" = "rf" && echo ok12
	test "`gln wip/dict-client`" = "rf" && echo ok13

	test "`awk 'END {print NR}' $OBJDIR/_test.tmp`" -eq 17 && echo ok100
    } | cmp 'paexec packages #9' \
'ok1
ok2
ok3
ok4
ok5
ok8
ok9
ok10
ok11
ok12
ok13
ok100
'

    # -s: autoconf fails
    runpaexec -l -s -c 'cmd_xxx_failed_make_package autoconf' \
	-n +4 > $OBJDIR/_test.tmp < ../examples/make_package/tasks

    {
	test "`gln devel/glib2`" -gt 0 && echo ok1
	test "`gln textproc/dictem`" -gt 0 && echo ok2
	test "`gln devel/libjudy`" -gt 0 && echo ok3
	test "`gln devel/gmake`" -gt 0 && echo ok4
	test "`gln devel/autoconf`" = f && echo ok5
	test "`gln devel/m4`" -gt 0 && echo ok8
	test "`gln devel/byacc`" -gt 0 && echo ok9
	test "`gln devel/flex`" -gt 0 && echo ok10

	test "`gln wip/libmaa`"      = "rf" && echo ok11
	test "`gln wip/dict-server`" = "rf" && echo ok12
	test "`gln wip/dict-client`" = "rf" && echo ok13

	test "`awk 'END {print NR}' $OBJDIR/_test.tmp`" -eq 17 && echo ok100
    } | cmp 'paexec packages #9' \
'ok1
ok2
ok3
ok4
ok5
ok8
ok9
ok10
ok11
ok12
ok13
ok100
'

    # -s: dict-server fails
    runpaexec -l -s -c 'cmd_xxx_failed_make_package dict-server' \
	-n +4 > $OBJDIR/_test.tmp < ../examples/make_package/tasks

    {
	test "`gln devel/glib2`" -gt 0 && echo ok1
	test "`gln textproc/dictem`" -gt 0 && echo ok2
	test "`gln devel/libjudy`" -gt 0 && echo ok3
	test "`gln devel/autoconf`" -lt "`gln wip/libmaa`" && echo ok4
	test "`gln devel/gmake`" -lt "`gln wip/libmaa`" && echo ok5
	test "`gln wip/dict-server`" = f && echo ok6
	test "`gln wip/libmaa`" -lt "`gln wip/dict-client`" && echo ok7
	test "`gln devel/m4`" -gt 0 && echo ok8
	test "`gln devel/byacc`" -lt "`gln wip/dict-client`" && echo ok9
	test "`gln devel/flex`" -lt "`gln wip/dict-client`" && echo ok11

	test "`awk 'END {print NR}' $OBJDIR/_test.tmp`" -eq 23 && echo ok100
    } | cmp 'paexec packages #10' \
'ok1
ok2
ok3
ok4
ok5
ok6
ok7
ok8
ok9
ok11
ok100
'

    # -s: flex and byacc fail
    runpaexec -n +4 -Cls cmd_xxx_failed_make_package 'flex|byacc' \
	 > $OBJDIR/_test.tmp < ../examples/make_package/tasks

    {
	test "`gln devel/glib2`" -gt 0 && echo ok1
	test "`gln textproc/dictem`" -gt 0 && echo ok2
	test "`gln devel/libjudy`" -gt 0 && echo ok3
	test "`gln devel/autoconf`" -lt "`gln wip/libmaa`" && echo ok4
	test "`gln devel/gmake`" -lt "`gln wip/libmaa`" && echo ok5
	test "`gln devel/m4`" -gt 0 && echo ok8
	test "`gln devel/byacc`" = f && echo ok9
	test "`gln devel/flex`" = f && echo ok10

	test "`gln wip/dict-server`" = "rf" && echo ok11
	test "`gln wip/dict-client`" = "rf" && echo ok12

	awk 'NF > 2 && $2 == "devel/flex" &&
	    /wip\/dict-server/ &&
	    /wip\/dict-client/ {print "ok20"}' $OBJDIR/_test.tmp
	awk 'NF > 2 && $2 == "devel/byacc" &&
	    /wip\/dict-server/ &&
	    /wip\/dict-client/ {print "ok21"}' $OBJDIR/_test.tmp

	test "`awk 'END {print NR}' $OBJDIR/_test.tmp`" -eq 20 && echo ok100
    } | cmp 'paexec packages #11' \
'ok1
ok2
ok3
ok4
ok5
ok8
ok9
ok10
ok11
ok12
ok20
ok21
ok100
'

    # -s: gmake and autoconf fail
    runpaexec -l -s \
	-c 'cmd_xxx_failed_make_package "gmake|autoconf"' \
	-n +4 > $OBJDIR/_test.tmp < ../examples/make_package/tasks

    {
	test "`gln devel/glib2`" -gt 0 && echo ok1
	test "`gln textproc/dictem`" -gt 0 && echo ok2
	test "`gln devel/libjudy`" -gt 0 && echo ok3
	test "`gln devel/autoconf`" = f && echo ok4
	test "`gln devel/gmake`" = f && echo ok5
	test "`gln devel/m4`" -gt 0 && echo ok8
	test "`gln devel/byacc`" -gt 0 && echo ok9
	test "`gln devel/flex`" -gt 0 && echo ok10

	test "`gln wip/libmaa`"      = "rf" && echo ok11
	test "`gln wip/dict-server`" = "rf" && echo ok12
	test "`gln wip/dict-client`" = "rf" && echo ok13

	awk 'NF > 2 && $2 == "devel/autoconf" &&
	    /wip\/libmaa/ && /wip\/dict-server/ &&
	    /wip\/dict-client/ {print "ok20"}' $OBJDIR/_test.tmp
	awk 'NF > 2 && $2 == "devel/gmake" &&
	    /wip\/libmaa/ && /wip\/dict-server/ &&
	    /wip\/dict-client/ {print "ok21"}' $OBJDIR/_test.tmp

	test "`awk 'END {print NR}' $OBJDIR/_test.tmp`" -eq 18 && echo ok100
    } | cmp 'paexec packages #12' \
'ok1
ok2
ok3
ok4
ok5
ok8
ok9
ok10
ok11
ok12
ok13
ok20
ok21
ok100
'

    # diamond-like dependancy and failure
    runpaexec -l -s -C -n +5 cmd_xxx_failed_make_package flex \
	> $OBJDIR/_test.tmp < ../examples/make_package/tasks2

    {
	test "`gln devel/flex`" = f && echo ok1

	awk 'NF > 2 && $2 == "devel/flex" &&
	    /wip\/dict-client/ && /wip\/dict-server/ &&
	    /wip\/pkg_online/ {print "ok2"}' $OBJDIR/_test.tmp

	if grep 'wip/pkg_online.*wip/pkg_online' $OBJDIR/_test.tmp > /dev/null
	then
	    echo 'needs to be fixed!!!'
	else
	    echo ok
	fi
    } | cmp 'paexec packages #13' \
'ok1
ok2
ok
'

    # cycle detection1
    runpaexec -l -s \
	-c cmd_make_package \
	-n +5 < ../examples/make_package/tasks_cycle |
    cmp 'paexec cyclic deps #1' \
'Cyclic dependancy detected:
  devel/gettext-lib -> devel/gmake
  devel/gmake -> lang/gcc
  lang/gcc -> devel/gettext-lib
'

    # cycle detection2
    printf 'task-2 task-1
task-1 task0
task0 task10
task10 task20
task20 task30
task30 task10
' | runpaexec -l -s \
	-c cmd_make_package \
	-n +5 |
    cmp 'paexec cyclic deps #2' \
'Cyclic dependancy detected:
  task10 -> task20
  task20 -> task30
  task30 -> task10
'

    # cycle detection2
    printf 'task0 task10
task10 task20
task20 task30
task50 task50
task30 task40
' | runpaexec -l -s \
	-c cmd_make_package \
	-n +5 |
    cmp 'paexec cyclic deps #3' \
'Cyclic dependancy detected:
  task50 -> task50
'

    # cycle detection2
    printf 'task0
task100
task0 task200
task0 task100
task10 task100
task20 task100
task50 task100
task100 task110
task200 task300
task300 task0
' | runpaexec -l -s \
	-c cmd_make_package \
	-n +5 |
    cmp 'paexec cyclic deps #4' \
'Cyclic dependancy detected:
  task0 -> task200
  task200 -> task300
  task300 -> task0
'
    

    # transport failure
    printf '%s' '-1 0
0 1
1 2
2 3
3 4
4 5
' | runpaexec -s -E \
	-t transport_broken_echo -c ':' \
	-n '1 2 3 4 5 6' |
    cmp 'paexec broken transport #1' \
"I'll output -1
-1
success

I'll output 0
0
success

I'll output 1
Node 1 exited unexpectedly
"

    # resistance/-z without -s
    printf 'bilberry
gooseberry
apple
pear
plum
cherry
' | runpaexec_resort -el -z \
	-t transport_broken_toupper \
	-c : -n '4 5 6 7' |
    cmp 'paexec broken transport #2' \
'1 fatal
1 
1 BILBERRY
1 
2 GOOSEBERRY
2 
3 APPLE
3 
4 PEAR
4 
5 PLUM
5 
6 CHERRY
6 
'

    # -Z + -w without -s
    printf '1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14\n15\n16\n' |
    runpaexec_resort -Z1 -w \
	-t transport_broken_rnd \
	-m F= -c: -n '0.5ns-nopostfail 0.5ns-nopostfail 0.5ns-nopostfail' |
    cmp 'paexec broken transport #3' \
'success
success
success
success
success
success
success
success
success
success
success
success
success
success
success
success
 1
 2
 3
 4
 5
 6
 7
 8
 9
 10
 11
 12
 13
 14
 15
 16
'

    # resistance to transport failure
    printf '%s' '-1 0
0 1
' | runpaexec -z -g -E \
	-t transport_broken_echo -c ':' \
	-n '1' |
    cmp 'paexec broken transport #4' \
"I'll output -1
-1
success

I'll output 0
0
success

I'll output 1
fatal

all nodes failed
"

    # resistance to transport failure
    printf '%s' '-1 0
0 1
1 2
2 3
3 4
4 5
5 6
' | runpaexec -z -r -g -E \
	-t transport_broken_echo -c ':' \
	-n '1 2 3 4 5 6 7' |
    cmp 'paexec broken transport #5' \
"1 I'll output -1
1 -1
1 success
1 
1 I'll output 0
1 0
1 success
1 
1 I'll output 1
1 fatal
1 
2 I'll output 1
2 1
2 success
2 
2 I'll output 2
2 fatal
2 
3 I'll output 2
3 2
3 success
3 
3 I'll output 3
3 fatal
3 
4 fatal
4 
5 I'll output 3
5 3
5 success
5 
5 I'll output 4
5 4
5 success
5 
5 I'll output 5
5 fatal
5 
6 I'll output 5
6 5
6 success
6 
6 I'll output 6
6 fatal
6 
7 I'll output 6
7 6
7 success
7 
"

    # resistance to transport failure2 (write(2) errors)
    printf '0 1
1 2
2 3
3 4
4 5
5 6
' | runpaexec -g -z -lre -t transp_closed_stdin -c : \
	-n '0 1 2 3 4 5 6 7 8' |
    cmp 'paexec broken transport #6' \
"0 1 I'll output 0
0 1 0
0 1 success
0 1 
0 2 fatal
0 2 
1 2 I'll output 1
1 2 1
1 2 success
1 2 
1 3 fatal
1 3 
2 3 I'll output 2
2 3 2
2 3 success
2 3 
2 4 fatal
2 4 
3 4 I'll output 3
3 4 3
3 4 success
3 4 
3 5 fatal
3 5 
4 5 I'll output 4
4 5 4
4 5 success
4 5 
4 6 fatal
4 6 
5 6 I'll output 5
5 6 5
5 6 success
5 6 
5 7 fatal
5 7 
6 7 I'll output 6
6 7 6
6 7 success
6 7 
"

    # resistance to transport failure
    awk '
    BEGIN {
        for (i=1; i < 300; ++i){
            print i
        }
    }' |
    runpaexec -z -r -g -E \
	-t transport_broken_echo -c ':' \
	-n '4' |
    cmp 'paexec broken transport #7' \
'4 fatal
4 
all nodes failed
'

    # resistance to transport failure
    export PAEXEC_TRANSPORT=transport_broken_echo
    awk '
    BEGIN {
        for (i=1; i < 300; ++i){
            print i
        }
    }' |
    runpaexec -z -r -g -E -c ':' -n '4' |
    cmp 'paexec broken transport #7.0.1 (PAEXEC_TRANSPORT)' \
'4 fatal
4 
all nodes failed
'
    unset PAEXEC_TRANSPORT

    # resistance to transport failure
    echo mama | runpaexec -z -r -g -i -E \
	-t transport_broken_echo -c ':' \
	-n '4' | 
    cmp 'paexec broken transport #8' \
'4 mama
4 fatal
4 
all nodes failed
'

    # resistance to transport failure
    cat > $OBJDIR/_test.in <<EOF
libmaa paexec
libmaa dict-client
libmaa dict-server
dat1
dat2
dat3
dat4
dat5
dat6
EOF

    if test "$SLEEP_FRACT"; then
    runpaexec -s -z -lr -t transport_broken_rnd -c : \
	-n '0.1 0.15 0.2 0.25 0.3 0' < $OBJDIR/_test.in |
    filter_succeded_tasks | sort -n |
    cmp 'paexec broken transport #9' \
'1 libmaa
2 paexec
3 dict-client
4 dict-server
5 dat1
6 dat2
7 dat3
8 dat4
9 dat5
10 dat6
'
    fi

    # resistance to transport failure
    awk 'BEGIN {for (i=1; i <= 1000; ++i) {print "dat" i}}' |
    runpaexec -s -z -lr -t transport_broken_rnd -c : \
	-n '0.01-ns 0.03-ns 0.09-ns 0.09-ns 0.03-ns 0-ns' |
    filter_succeded_tasks | sort -n | cksum | awk '{$1=$1; print $0}' |
    cmp 'paexec broken transport #10' \
'3878868009 10786
'

    # resistance with timeout to rerun failed command
    cat > $OBJDIR/_tasks.tmp <<EOF
1 2
2 3
3 4
4 5
5 6
10
EOF

    test_file=/tmp/paexec_test.$$

    rm -f "$test_file"

    runpaexec -Z1 -s -n '1 2' -c: \
	-t "transport_broken_echo2 $test_file" \
	< $OBJDIR/_tasks.tmp | grep output | sort |
    cmp 'paexec broken transport #11' \
'output 1
output 10
output 2
output 3
output 4
output 5
output 6
'

    rm -f "$test_file"

    # tests for weighted nodes of graph (-W0 option)
    test_tasks1 | runpaexec -W0 -e -c cmd_make_package -n +1 |
    cmp 'paexec -W0 #1' \
'qt4
success

kde
success

gnome
success

mplayer
success

gcc
success

glibc
success

tcl
success

python
success

pike
success

ruby
success

pcc
success

dictd
success

runawk
success

mk-configure
success

'

    # tests for max_weight calculation (-W0 option)
    test_tasks1 |
    runpaexec -W0 -e -d -c cmd_make_package \
	-n +1 2>&1 | grep '^sum_weight' |
    cmp 'paexec -W0 #2.1' \
'sum_weight [pcc]=4
sum_weight [gcc]=10
sum_weight [tcl]=8
sum_weight [glibc]=9
sum_weight [python]=7
sum_weight [dictd]=3
sum_weight [mplayer]=11
sum_weight [pike]=6
sum_weight [ruby]=5
sum_weight [gnome]=12
sum_weight [kde]=13
sum_weight [runawk]=2
sum_weight [mk-configure]=1
sum_weight [qt4]=14
'

    test_tasks1 | sed 's/^weight:/W:/' |
    runpaexec -W0 -e -d -mw=W: -c cmd_make_package \
	-n +1 2>&1 | grep '^sum_weight' |
    cmp 'paexec -W0 -mw=W: #2.2' \
'sum_weight [pcc]=4
sum_weight [gcc]=10
sum_weight [tcl]=8
sum_weight [glibc]=9
sum_weight [python]=7
sum_weight [dictd]=3
sum_weight [mplayer]=11
sum_weight [pike]=6
sum_weight [ruby]=5
sum_weight [gnome]=12
sum_weight [kde]=13
sum_weight [runawk]=2
sum_weight [mk-configure]=1
sum_weight [qt4]=14
'

    # tests for max_weight calculation (-W0 option)
    test_tasks2 |
    runpaexec -edW0 -c cmd_make_package \
	-n +1 2>&1 | grep '^sum_weight' |
    cmp 'paexec -W0 #3' \
'sum_weight [pipestatus]=1
sum_weight [pkg_status]=1
sum_weight [pkg_summary-utils]=2
sum_weight [dict]=15
sum_weight [pkg_online-client]=1
sum_weight [netcat]=1
sum_weight [dictd]=20
sum_weight [pkg_online-server]=1
sum_weight [judyhash]=12
sum_weight [runawk]=2
sum_weight [libmaa]=5
sum_weight [paexec]=4
'

    test_tasks2 | spc2semicolon |
    runpaexec -edW0 -md=';' -c cmd_make_package \
	-n +1 2>&1 | grep '^sum_weight' |
    cmp 'paexec -W0 -md=";" #3.1' \
'sum_weight [pipestatus]=1
sum_weight [pkg_status]=1
sum_weight [pkg_summary-utils]=2
sum_weight [dict]=15
sum_weight [pkg_online-client]=1
sum_weight [netcat]=1
sum_weight [dictd]=20
sum_weight [pkg_online-server]=1
sum_weight [judyhash]=12
sum_weight [runawk]=2
sum_weight [libmaa]=5
sum_weight [paexec]=4
'

    # tests for sum_weight calculation (-W0 option)
    test_tasks2 |
    runpaexec -eW0 -c cmd_make_package -n +1 2>&1 |
    cmp 'paexec -W0 #4' \
'judyhash
success

libmaa
success

dictd
success

dict
success

paexec
success

runawk
success

pipestatus
success

pkg_summary-utils
success

pkg_status
success

netcat
success

pkg_online-client
success

pkg_online-server
success

'

    test_tasks2 | spc2semicolon |
    runpaexec -eW0 -md=';' -c cmd_make_package -n +1 2>&1 |
    cmp 'paexec -W0 -md=";" #4.1' \
'judyhash
success

libmaa
success

dictd
success

dict
success

paexec
success

runawk
success

pipestatus
success

pkg_summary-utils
success

pkg_status
success

netcat
success

pkg_online-client
success

pkg_online-server
success

'

    # tests for weighted nodes of graph (-W1 option)
    test_tasks1 | runpaexec -W1 -e -c cmd_make_package -n +1 |
    cmp 'paexec -W1 #1' \
'qt4
success

kde
success

gnome
success

mplayer
success

gcc
success

glibc
success

tcl
success

python
success

pike
success

ruby
success

pcc
success

dictd
success

runawk
success

mk-configure
success

'

    # tests for sum_weight calculation (-W1 option)
    test_tasks1 |
    runpaexec -W1 -e -d -c cmd_make_package \
	-n +1 2>&1 | grep '^sum_weight' |
    cmp 'paexec -W1 #2' \
'sum_weight [pcc]=4
sum_weight [gcc]=10
sum_weight [tcl]=8
sum_weight [glibc]=9
sum_weight [python]=7
sum_weight [dictd]=3
sum_weight [mplayer]=11
sum_weight [pike]=6
sum_weight [ruby]=5
sum_weight [gnome]=12
sum_weight [kde]=13
sum_weight [runawk]=2
sum_weight [mk-configure]=1
sum_weight [qt4]=14
'

    # tests for sum_weight calculation (-W1 option)
    test_tasks2 |
    runpaexec -edW1 -c cmd_make_package \
	-n +1 2>&1 | grep '^sum_weight' |
    cmp 'paexec -W1 #3' \
'sum_weight [pipestatus]=6
sum_weight [pkg_status]=1
sum_weight [pkg_summary-utils]=5
sum_weight [dict]=16
sum_weight [pkg_online-client]=1
sum_weight [netcat]=2
sum_weight [dictd]=21
sum_weight [pkg_online-server]=1
sum_weight [judyhash]=12
sum_weight [runawk]=7
sum_weight [libmaa]=49
sum_weight [paexec]=9
'

    # tests for sum_weight calculation (-W1 option)
    test_tasks2 |
    runpaexec -eW1 -c cmd_make_package -n +1 2>&1 |
    cmp 'paexec -W1 #4' \
'libmaa
success

dictd
success

dict
success

judyhash
success

paexec
success

runawk
success

pipestatus
success

pkg_summary-utils
success

netcat
success

pkg_status
success

pkg_online-client
success

pkg_online-server
success

'

    # tests for sum_weight calculation (-W1 option)
    printf 'task1 task2\ntask1 task2\nweight: task1 7\nweight: task2 9\n' |
    runpaexec -edW1 -c cmd_make_package -n +1 2>&1 | grep '^sum_weight' |
    cmp 'paexec -W1 #5' \
'sum_weight [task1]=16
sum_weight [task2]=9
'

    # tests for weighted nodes of graph (-W2 option)
    test_tasks1 | runpaexec -W2 -e -c cmd_make_package -n +1 |
    cmp 'paexec -W2 #1' \
'qt4
success

kde
success

gnome
success

mplayer
success

gcc
success

glibc
success

tcl
success

python
success

pike
success

ruby
success

pcc
success

dictd
success

runawk
success

mk-configure
success

'

    # tests for max_weight calculation (-W2 option)
    test_tasks1 |
    runpaexec -W2 -e -d -c cmd_make_package \
	-n +1 2>&1 | grep '^sum_weight' |
    cmp 'paexec -W2 #2' \
'sum_weight [pcc]=4
sum_weight [gcc]=10
sum_weight [tcl]=8
sum_weight [glibc]=9
sum_weight [python]=7
sum_weight [dictd]=3
sum_weight [mplayer]=11
sum_weight [pike]=6
sum_weight [ruby]=5
sum_weight [gnome]=12
sum_weight [kde]=13
sum_weight [runawk]=2
sum_weight [mk-configure]=1
sum_weight [qt4]=14
'

    # tests for max_weight calculation (-W2 option)
    test_tasks2 |
    runpaexec -edW2 -c cmd_make_package \
	-n +1 2>&1 | grep '^sum_weight' |
    cmp 'paexec -W2 #3' \
'sum_weight [pipestatus]=2
sum_weight [pkg_status]=1
sum_weight [pkg_summary-utils]=2
sum_weight [dict]=15
sum_weight [pkg_online-client]=1
sum_weight [netcat]=1
sum_weight [dictd]=20
sum_weight [pkg_online-server]=1
sum_weight [judyhash]=12
sum_weight [runawk]=2
sum_weight [libmaa]=20
sum_weight [paexec]=4
'

    # tests for sum_weight calculation (-W2 option)
    test_tasks2 |
    runpaexec -eW2 -c cmd_make_package -n +1 2>&1 |
    cmp 'paexec -W2 #4' \
'libmaa
success

dictd
success

dict
success

judyhash
success

paexec
success

pipestatus
success

runawk
success

pkg_summary-utils
success

pkg_status
success

netcat
success

pkg_online-client
success

pkg_online-server
success

'

    # tests for sum_weight calculation (-W0 option)
    test_tasks2 |
    runpaexec -lgX -n+1 -C \
	awk 'BEGIN {
	    if (ARGV[1] == "libmaa") { print "libmaa is broken"; exit 1} 
	    { print ARGV[1] " is fine"} 
	}' |
    cmp 'paexec -X #1' \
'9 success
11 failure
11 libmaa dict pkg_online-client dictd pkg_online-server paexec pkg_summary-utils pkg_status
10 success
1 success
6 success
'

    # the first line on input is empty
    printf '\n\n' |
    runpaexec -xc echo -n +2 -l |
    sort |
    cmp 'paexec # empty line1' \
'1 
2 
'

    # empty lines anywhere
    printf 'aaa\n\nbbb\n' |
    runpaexec -xc echo -n +2 -l |
    sort |
    cmp 'paexec # empty line2' \
'1 aaa
2 
3 bbb
'

    # -g + the first line on input is empty
    printf '\n\n' |
    runpaexec -gxc 'echo task' -n +2 -l |
    paexec_reorder -gl |
    cmp 'paexec # empty line1' \
'1 task 
1 success
'

    # -g + empty lines anywhere
    printf 'aaa\n\nbbb\n' |
    runpaexec -gxc 'echo task' -n +2 -l |
    paexec_reorder -gl |
    cmp 'paexec # -g + empty line2' \
'1 task aaa
1 success
2 task 
2 success
3 task bbb
3 success
'

    # -g + empty task
    printf ' ccc\nbbb \naaa bbb\nccc ddd\n' |
    runpaexec -gxc 'printf '"'"'task "%s"\n'"'" -n +2 -l |
    cmp 'paexec # -g + empty task1' \
'4 task "aaa"
4 success
3 task "bbb"
3 success
1 task ""
1 success
2 task "ccc"
2 success
5 task "ddd"
5 success
'

    # -x + -t
    export PAEXEC_ENV='   ZZZZ, , ,YYYY,CCCC LALALA    '
    export ZZZZ=zz1234zz
    export YYYY=yy1234yy
    export CCCC=cc1234cc
    printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
    runpaexec -xlC -t paexec_notransport \
	-n '1 2 3 4 5 6 7 8 9' \
	awk 'BEGIN {print ENVIRON["ZZZZ"], ENVIRON["YYYY"], ENVIRON["CCCC"], ENVIRON["LALALA"], toupper(ARGV[1])}' |
    resort |
    cmp 'paexec -g + -t #1 (PAEXEC_ENV)' \
'1 zz1234zz yy1234yy cc1234cc  A
2 zz1234zz yy1234yy cc1234cc  BB
3 zz1234zz yy1234yy cc1234cc  CCC
4 zz1234zz yy1234yy cc1234cc  DDDD
5 zz1234zz yy1234yy cc1234cc  EEEEE
6 zz1234zz yy1234yy cc1234cc  FFFFFF
'
    unset ZZZZ YYYY CCCC PAEXEC_ENV

    # -t + shquote(3)
    printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
    runpaexec -t paexec_notransport \
	-n '1 2 3 4 5 6 7 8 9' \
	-C sh -c 'while read f; do echo $f; echo; done' |
    sort |
    cmp 'paexec -t + shquote(3) #1' \
'a
bb
ccc
dddd
eeeee
ffffff
'

    # -t + shquote(3)
    printf 'a\nbb\n' |
    runpaexec -x -t /bad/transport -n +1 -c echo |
    sort |
    cmp 'paexec -n +1 -t /bad/transport' \
'a
bb
'

    # tests for paexec_reorder
    paexec_reorder_input1 |
    paexec_reorder |
    cmp 'paexec_reorder' \
'TABLE1
TABLE2
TABLE3
TABLE4
GREEN1
GREEN2
GREEN3
GREEN4
APPLE1
APPLE2
APPLE3
APPLE4
'

    paexec_reorder_input1 |
    nonstandard_msgs |
    paexec_reorder -m t='Konec!' |
    cmp 'paexec_reorder nonstandard' \
'TABLE1
TABLE2
TABLE3
TABLE4
GREEN1
GREEN2
GREEN3
GREEN4
APPLE1
APPLE2
APPLE3
APPLE4
'

    # tests for paexec_reorder -l
    paexec_reorder_input2 | paexec_reorder -l |
    cmp 'paexec_reorder -l' \
'2 fatal 1
2 fatal 2
2 fatal 3
2 fatal 4
1 APPLE1
1 APPLE2
1 APPLE3
1 APPLE4
3 GREEN1
3 GREEN2
3 GREEN3
3 GREEN4
'

    paexec_reorder_input2 |
    nonstandard_msgs |
    paexec_reorder -l \
	-m t='Konec!' -m f='Zhopa!' -mF='PolnayaZhopa!' -m s='Ura!' |
    cmp 'paexec_reorder -l nonstandard' \
'2 PolnayaZhopa! 1
2 PolnayaZhopa! 2
2 PolnayaZhopa! 3
2 PolnayaZhopa! 4
1 APPLE1
1 APPLE2
1 APPLE3
1 APPLE4
3 GREEN1
3 GREEN2
3 GREEN3
3 GREEN4
'

    # tests for paexec_reorder -g
    paexec_reorder_input3 |
    paexec_reorder -gS |
    cmp 'paexec_reorder -gS' \
'TABLE1
TABLE2
TABLE3
TABLE4
success
APPLE1
APPLE2
APPLE3
APPLE4
success
GREEN1
GREEN2
GREEN3
GREEN???
failure
4 5
'

    paexec_reorder_input3 |
    nonstandard_msgs |
    paexec_reorder -gS \
	-mt='Konec!' -mf='Zhopa!' -mF='PolnayaZhopa!' -ms='Ura!' |
    cmp 'paexec_reorder -gS nonstandard' \
'TABLE1
TABLE2
TABLE3
TABLE4
Ura!
APPLE1
APPLE2
APPLE3
APPLE4
Ura!
GREEN1
GREEN2
GREEN3
GREEN???
Zhopa!
4 5
'

    # tests for paexec_reorder -glS
    paexec_reorder_input4 | paexec_reorder -glS |
    cmp 'paexec_reorder -glS' \
'2 TABLE1
2 TABLE2
2 TABLE3
2 TABLE4
2 success
1 APPLE1
1 APPLE2
1 APPLE3
1 APPLE4
1 success
3 GREEN1
3 GREEN2
3 GREEN3
3 GREEN???
3 failure
3 4 5
'

    paexec_reorder_input4 |
    nonstandard_msgs |
    paexec_reorder -glS \
	-m t='Konec!' -m f='Zhopa!' -mF='PolnayaZhopa!' -m s='Ura!' |
    cmp 'paexec_reorder -glS nonstandard' \
'2 TABLE1
2 TABLE2
2 TABLE3
2 TABLE4
2 Ura!
1 APPLE1
1 APPLE2
1 APPLE3
1 APPLE4
1 Ura!
3 GREEN1
3 GREEN2
3 GREEN3
3 GREEN???
3 Zhopa!
3 4 5
'

    # tests for paexec_reorder -gl
    paexec_reorder_input5 |
    paexec_reorder -gl |
    cmp 'paexec_reorder -gl' \
'1 APPLE1
1 APPLE2
1 APPLE3
1 APPLE4
1 success
2 TABLE1
2 TABLE2
2 TABLE3
2 TABLE4
2 success
3 GREEN1
3 GREEN2
3 GREEN3
3 GREEN???
3 failure
3 4 5
'

    paexec_reorder_input5 |
    nonstandard_msgs |
    paexec_reorder -gl \
	-m t='Konec!' -m f='Zhopa!' -mF='PolnayaZhopa!' -m s='Ura!' |
    cmp 'paexec_reorder -gl nonstandard' \
'1 APPLE1
1 APPLE2
1 APPLE3
1 APPLE4
1 Ura!
2 TABLE1
2 TABLE2
2 TABLE3
2 TABLE4
2 Ura!
3 GREEN1
3 GREEN2
3 GREEN3
3 GREEN???
3 Zhopa!
3 4 5
'

    # tests for paexec_reorder -Ms
    paexec_reorder_input1 |
    paexec_reorder -Ms |
    cmp 'paexec_reorder -Ms' \
'APPLE1
APPLE2
APPLE3
APPLE4
TABLE1
TABLE2
TABLE3
TABLE4
GREEN1
GREEN2
GREEN3
GREEN4
'

    paexec_reorder_input1 |
    nonstandard_msgs |
    paexec_reorder -Ms \
	-m t='Konec!' -m f='Zhopa!' -mF='PolnayaZhopa!' -m s='Ura!' |
    cmp 'paexec_reorder -Ms nonstandard' \
'APPLE1
APPLE2
APPLE3
APPLE4
TABLE1
TABLE2
TABLE3
TABLE4
GREEN1
GREEN2
GREEN3
GREEN4
'

    # tests for paexec_reorder -l -Ms
    paexec_reorder_input2 |
    paexec_reorder -l -Ms |
    cmp 'paexec_reorder -l -Ms' \
'1 APPLE1
1 APPLE2
1 APPLE3
1 APPLE4
2 fatal 1
2 fatal 2
2 fatal 3
2 fatal 4
3 GREEN1
3 GREEN2
3 GREEN3
3 GREEN4
'

    paexec_reorder_input2 |
    nonstandard_msgs |
    paexec_reorder -l -Ms \
	-m t='Konec!' -m f='Zhopa!' -mF='PolnayaZhopa!' -m s='Ura!' |
    cmp 'paexec_reorder -l -Ms nonstandard ' \
'1 APPLE1
1 APPLE2
1 APPLE3
1 APPLE4
2 PolnayaZhopa! 1
2 PolnayaZhopa! 2
2 PolnayaZhopa! 3
2 PolnayaZhopa! 4
3 GREEN1
3 GREEN2
3 GREEN3
3 GREEN4
'

    # tests for paexec_reorder -gS -Ms
    paexec_reorder_input3 |
    paexec_reorder -gS -Ms |
    cmp 'paexec_reorder -gS -Ms' \
'APPLE1
APPLE2
APPLE3
APPLE4
success
TABLE1
TABLE2
TABLE3
TABLE4
success
GREEN1
GREEN2
GREEN3
GREEN???
failure
4 5
'

    paexec_reorder_input3 |
    nonstandard_msgs |
    paexec_reorder -gS -Ms \
	-m t='Konec!' -m f='Zhopa!' -mF='PolnayaZhopa!' -m s='Ura!' |
    cmp 'paexec_reorder -gS -Ms nonstandard' \
'APPLE1
APPLE2
APPLE3
APPLE4
Ura!
TABLE1
TABLE2
TABLE3
TABLE4
Ura!
GREEN1
GREEN2
GREEN3
GREEN???
Zhopa!
4 5
'

    # tests for paexec_reorder -gl -Ms
    paexec_reorder_input4 |
    paexec_reorder -gl -Ms |
    cmp 'paexec_reorder -gl -Ms' \
'1  blablabla
1 fatal
1  APPLE1
1  APPLE2
1  APPLE3
1  APPLE4
1 success
2  TABLE1
2  TABLE2
2  TABLE3
2  TABLE4
2 success
3  foo
3  bar
3 fatal
3  GREEN1
3  GREEN2
3  GREEN3
3  GREEN???
3 failure
3 4 5
'

    paexec_reorder_input4 |
    nonstandard_msgs |
    paexec_reorder -gl -Ms \
	-m t='Konec!' -m f='Zhopa!' -mF='PolnayaZhopa!' -m s='Ura!' |
    cmp 'paexec_reorder -gl -Ms nonstandard' \
'1  blablabla
1 PolnayaZhopa!
1  APPLE1
1  APPLE2
1  APPLE3
1  APPLE4
1 Ura!
2  TABLE1
2  TABLE2
2  TABLE3
2  TABLE4
2 Ura!
3  foo
3  bar
3 PolnayaZhopa!
3  GREEN1
3  GREEN2
3  GREEN3
3  GREEN???
3 Zhopa!
3 4 5
'

    # tests for paexec_reorder -gl -Ms
    paexec_reorder_input5 |
    paexec_reorder -gl -Ms |
    cmp 'paexec_reorder -gl -Ms' \
'1 APPLE1
1 APPLE2
1 APPLE3
1 APPLE4
1 success
2 TABLE1
2 TABLE2
2 TABLE3
2 TABLE4
2 success
3 GREEN1
3 GREEN2
3 GREEN3
3 GREEN???
3 failure
3 4 5
'

    paexec_reorder_input5 |
    nonstandard_msgs |
    paexec_reorder -gl -Ms \
	-m t='Konec!' -m f='Zhopa!' -mF='PolnayaZhopa!' -m s='Ura!' |
    cmp 'paexec_reorder -gl -Ms nonstandard' \
'1 APPLE1
1 APPLE2
1 APPLE3
1 APPLE4
1 Ura!
2 TABLE1
2 TABLE2
2 TABLE3
2 TABLE4
2 Ura!
3 GREEN1
3 GREEN2
3 GREEN3
3 GREEN???
3 Zhopa!
3 4 5
'

    # tests for paexec_reorder -Mf
    paexec_reorder_input1 |
    paexec_reorder -Mf |
    cmp 'paexec_reorder -Mf' \
'TABLE1
TABLE2
TABLE3
TABLE4
GREEN1
GREEN2
GREEN3
GREEN4
APPLE1
APPLE2
APPLE3
APPLE4
'

    paexec_reorder_input1 |
    nonstandard_msgs |
    paexec_reorder -Mf \
	-m t='Konec!' -m f='Zhopa!' -mF='PolnayaZhopa!' -m s='Ura!' |
    cmp 'paexec_reorder -Mf nonstandard ' \
'TABLE1
TABLE2
TABLE3
TABLE4
GREEN1
GREEN2
GREEN3
GREEN4
APPLE1
APPLE2
APPLE3
APPLE4
'

    # tests for paexec_reorder -l -Mf
    paexec_reorder_input2 |
    paexec_reorder -l -Mf |
    cmp 'paexec_reorder -l -Mf' \
'2 fatal 1
2 fatal 2
2 fatal 3
2 fatal 4
1 APPLE1
1 APPLE2
1 APPLE3
1 APPLE4
3 GREEN1
3 GREEN2
3 GREEN3
3 GREEN4
'

    paexec_reorder_input2 |
    nonstandard_msgs |
    paexec_reorder -l -Mf \
	-m t='Konec!' -m f='Zhopa!' -mF='PolnayaZhopa!' -m s='Ura!' |
    cmp 'paexec_reorder -l -Mf nonstandard' \
'2 PolnayaZhopa! 1
2 PolnayaZhopa! 2
2 PolnayaZhopa! 3
2 PolnayaZhopa! 4
1 APPLE1
1 APPLE2
1 APPLE3
1 APPLE4
3 GREEN1
3 GREEN2
3 GREEN3
3 GREEN4
'

    # tests for paexec_reorder -gS -Mf
    paexec_reorder_input3 |
    paexec_reorder -gS -Mf |
    cmp 'paexec_reorder -gS -Mf' \
'TABLE1
TABLE2
TABLE3
TABLE4
success
APPLE1
APPLE2
APPLE3
APPLE4
success
GREEN1
GREEN2
GREEN3
GREEN???
failure
4 5
'

    paexec_reorder_input3 |
    nonstandard_msgs |
    paexec_reorder -gS -Mf \
	-m t='Konec!' -m f='Zhopa!' -mF='PolnayaZhopa!' -m s='Ura!' |
    cmp 'paexec_reorder -gS -Mf nonstandard' \
'TABLE1
TABLE2
TABLE3
TABLE4
Ura!
APPLE1
APPLE2
APPLE3
APPLE4
Ura!
GREEN1
GREEN2
GREEN3
GREEN???
Zhopa!
4 5
'

    # tests for paexec_reorder -gl -Mf
    paexec_reorder_input4 |
    paexec_reorder -gl -Mf |
    cmp 'paexec_reorder -gl -Mf' \
'2  TABLE1
2  TABLE2
2  TABLE3
2  TABLE4
2 success
1  APPLE1
1  APPLE2
1  APPLE3
1  APPLE4
1 success
3  GREEN1
3  GREEN2
3  GREEN3
3  GREEN???
3 failure
3 4 5
'

    paexec_reorder_input4 |
    nonstandard_msgs |
    paexec_reorder -gl -Mf \
	-m t='Konec!' -m f='Zhopa!' -mF='PolnayaZhopa!' -m s='Ura!' |
    cmp 'paexec_reorder -gl -Mf nonstandard' \
'2  TABLE1
2  TABLE2
2  TABLE3
2  TABLE4
2 Ura!
1  APPLE1
1  APPLE2
1  APPLE3
1  APPLE4
1 Ura!
3  GREEN1
3  GREEN2
3  GREEN3
3  GREEN???
3 Zhopa!
3 4 5
'

    # tests for paexec_reorder -gl -Mf
    paexec_reorder_input5 |
    paexec_reorder -gl -Mf |
    cmp 'paexec_reorder -gl -Mf' \
'1 APPLE1
1 APPLE2
1 APPLE3
1 APPLE4
1 success
2 TABLE1
2 TABLE2
2 TABLE3
2 TABLE4
2 success
3 GREEN1
3 GREEN2
3 GREEN3
3 GREEN???
3 failure
3 4 5
'

    paexec_reorder_input5 |
    nonstandard_msgs |
    paexec_reorder -gl -Mf \
	-m t='Konec!' -m f='Zhopa!' -mF='PolnayaZhopa!' -m s='Ura!' |
    cmp 'paexec_reorder -gl -Mf nonstandard' \
'1 APPLE1
1 APPLE2
1 APPLE3
1 APPLE4
1 Ura!
2 TABLE1
2 TABLE2
2 TABLE3
2 TABLE4
2 Ura!
3 GREEN1
3 GREEN2
3 GREEN3
3 GREEN???
3 Zhopa!
3 4 5
'

    #
    test -f $tmpex
    return $?
}

for PAEXEC_BUFSIZE in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 1000 10000; do
    printf "PAEXEC_BUFSIZE=%d:\n" $PAEXEC_BUFSIZE
    export PAEXEC_BUFSIZE

    do_test || exit
done
