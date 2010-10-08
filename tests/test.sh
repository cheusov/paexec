#!/bin/sh

#EXEPREFIX='valgrind -q'
#EXEPREFIX='env EF_PROTECT_BELOW=1 ef'

DIFF_PROG=${DIFF_PROG-diff -U10}

print_header (){
    printf '=================================================================\n'
    printf '======= args: %s\n' "$*" | cut_full_path_closed_stdin
}

runtest (){
    $EXEPREFIX $OBJDIR/paexec "$@" 2>&1
}

runtest_resort (){
    $EXEPREFIX $OBJDIR/paexec "$@" 2>&1 | resort
}

cut_version (){
    awk '$1 == "paexec" {$2 = "x.y.x"} {print}'
}

cut_help (){
    awk 'NR <= 3'
}

cut_full_path_closed_stdin (){
    sed 's|^\(.*\) [^ ]*\(transport_closed_stdin.*\)$|\1 \2|'
}

resort (){
    awk '{print $1, NR, $0}' |
    sort -k1,1n -k2,2n |
    awk '{$1 = $2 = ""; print substr($0, 3)}'
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
7 8
8 9
9 10
10 11
11 12
EOF
}

test_tasks4 (){
    cat <<'EOF'
task-2 task-1
task-1 task0
task0 task10
task10 task20
task20 task30
task30 task10
EOF
}

tmpdir="/tmp/paexec-test.$$"
mkdir -m 0700 "$tmpdir" || exit 60

tmpfn1="$tmpdir/1"
tmpfn2="$tmpdir/2"
tmpfn3="$tmpdir/3"
tmpex="$tmpdir/4"

trap "rm -rf $tmpdir" 0 INT QUIT TERM HUP

echo > $tmpex

cmp (){
    # $1 - progress message
    # $2 - expected text
    printf '%s... ' "$1" 1>&2

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
    runtest -V | cut_version |
cmp 'paexec -V' 'paexec x.y.x written by Aleksey Cheusov
'

#    runtest --version | cut_version
    runtest --version | cut_version |
cmp 'paexec --version' 'paexec x.y.x written by Aleksey Cheusov
'

    runtest -h 2>&1 | cut_help |
cmp 'paexec -h' 'paexec - parallel executor
         that distributes tasks over CPUs or machines in a network.
usage: paexec [OPTIONS] [files...]
'

    runtest --help 2>&1 | cut_help |
cmp 'paexec --help' 'paexec - parallel executor
         that distributes tasks over CPUs or machines in a network.
usage: paexec [OPTIONS] [files...]
'

    # toupper
    printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
    runtest -l -t ./paexec_notransport -c ../examples/toupper/toupper_cmd \
	-n '1 2 3 4 5 6 7 8 9' | resort |
    cmp 'paexec toupper #1' \
'1 A
2 BB
3 CCC
4 DDDD
5 EEEEE
6 FFFFFF
'

    # toupper
    printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
    runtest -l -t '' -c ../examples/toupper/toupper_cmd \
	-n '1 2 3 4 5 6 7 8 9' | resort |
    cmp 'paexec toupper #2' \
'1 A
2 BB
3 CCC
4 DDDD
5 EEEEE
6 FFFFFF
'

    printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
    runtest -l -c ../examples/toupper/toupper_cmd \
	-n '1 2 3 4 5 6 7 8 9' | resort |
    cmp 'paexec toupper #3' \
'1 A
2 BB
3 CCC
4 DDDD
5 EEEEE
6 FFFFFF
'

    printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
    runtest -l -p -t ./paexec_notransport \
	-c ../examples/toupper/toupper_cmd -n '+2' |
    resort | awk '$1 ~ /^[0-9]/ {$2 = "pid"; print; next} {print}' |
    cmp 'paexec toupper #4' \
'1 pid A
2 pid BB
3 pid CCC
4 pid DDDD
5 pid EEEEE
6 pid FFFFFF
'

    printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
    runtest -l -p -t '' \
	-c ../examples/toupper/toupper_cmd -n '+2' |
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
    runtest -l -p \
	-c ../examples/toupper/toupper_cmd -n '+2' |
    resort | awk '$1 ~ /^[0-9]/ {$2 = "pid"; print; next} {print}' |
    cmp 'paexec toupper #6' \
'1 pid A
2 pid BB
3 pid CCC
4 pid DDDD
5 pid EEEEE
6 pid FFFFFF
'

    # all_substr
    printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
    runtest -l -c ../examples/all_substr/all_substr_cmd \
	-n '1 2 3 4 5 6 7 8 9' | resort |
    cmp 'paexec all_substr #1' \
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
    runtest -l -c ../examples/all_substr/all_substr_cmd -n '+9' |
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
    runtest -c ../examples/all_substr/all_substr_cmd -n +3 < /dev/null |
    cmp 'paexec all_substr #3' ''

    # bad command + no input
    runtest -l -c /path/to/bad/prog -n +3 < /dev/null |
    cmp 'paexec bad_command' ''

    # bi-i-i-i-i-i-ig result
    awk '
    BEGIN {
	for (i=0; i < 10; ++i) {
	    print "1234567890-=qwertyuiop[]asdfghjkl;zxcvbnm,./zaqwsxcderfvbgtyhnmjuik,.lo";
	}
    }' | runtest -c ../tests/big_result_cmd -n '+9' |
    uniq -c |
    head -n 100 |
    awk '{$1 = $1; print $0}' |
    cmp 'paexec big_result_cmd' \
'100000 1234567890-=QWERTYUIOP[]ASDFGHJKL;ZXCVBNM,./ZAQWSXCDERFVBGTYHNMJUIK,.LO
'

    # tests for partially ordered set of tasks (-s option)
    test_tasks3 |
    runtest -e -s -l -c ../examples/1_div_X/1_div_X_cmd -n +10 |
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

    # -s and no input
    runtest -s -l -c ../examples/1_div_X/1_div_X_cmd -n +10 < /dev/null |
    cmp 'paexec 1/X #2' ''

    # -s all failed
    runtest -s -l -c '../examples/make_package/make_package_cmd__xxx_failed .' \
	-n +10 < /dev/null |
    cmp 'paexec all fails #1' ''

    # -s all failed
    runtest -s -l -c '../examples/make_package/make_package_cmd__xxx_failed .' \
	-n +5 < /dev/null |
    cmp 'paexec all fails #2' ''

    # -s all failed
    runtest -s -l -c '../examples/make_package/make_package_cmd__xxx_failed .' \
	-n +1 < /dev/null |
    cmp 'paexec all fails #3' ''

    # -s: all succeeded
    runtest -l -s -c ../examples/make_package/make_package_cmd -n +2 \
	> $OBJDIR/_test.tmp < ../examples/make_package/make_package_tasks

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
    runtest -l -s -c '../examples/make_package/make_package_cmd__xxx_failed byacc' \
	-n +3 > $OBJDIR/_test.tmp < ../examples/make_package/make_package_tasks

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
    runtest -l -s -c '../examples/make_package/make_package_cmd__xxx_failed flex' \
	-n +5 > $OBJDIR/_test.tmp < ../examples/make_package/make_package_tasks

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
    runtest -l -s -c '../examples/make_package/make_package_cmd__xxx_failed libmaa' \
	-n +6 > $OBJDIR/_test.tmp < ../examples/make_package/make_package_tasks

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
    runtest -l -s -c '../examples/make_package/make_package_cmd__xxx_failed m4' \
	-n +4 > $OBJDIR/_test.tmp < ../examples/make_package/make_package_tasks

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
    runtest -l -s -c '../examples/make_package/make_package_cmd__xxx_failed libjudy' \
	-n +2 > $OBJDIR/_test.tmp < ../examples/make_package/make_package_tasks

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
    runtest -l -s -c '../examples/make_package/make_package_cmd__xxx_failed dictem' \
	-n +3 > $OBJDIR/_test.tmp < ../examples/make_package/make_package_tasks

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
    runtest -l -s -c '../examples/make_package/make_package_cmd__xxx_failed glib2' \
	-n +6 > $OBJDIR/_test.tmp < ../examples/make_package/make_package_tasks

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
    runtest -l -s -c '../examples/make_package/make_package_cmd__xxx_failed gmake' -n +5 \
	> $OBJDIR/_test.tmp < ../examples/make_package/make_package_tasks

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
    runtest -l -s -c '../examples/make_package/make_package_cmd__xxx_failed autoconf' \
	-n +4 > $OBJDIR/_test.tmp < ../examples/make_package/make_package_tasks

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
    runtest -l -s -c '../examples/make_package/make_package_cmd__xxx_failed dict-server' \
	-n +4 > $OBJDIR/_test.tmp < ../examples/make_package/make_package_tasks

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
    runtest -l -s -c '../examples/make_package/make_package_cmd__xxx_failed "flex|byacc"' \
	-n +4 > $OBJDIR/_test.tmp < ../examples/make_package/make_package_tasks

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
    runtest -l -s \
	-c '../examples/make_package/make_package_cmd__xxx_failed "gmake|autoconf"' \
	-n +4 > $OBJDIR/_test.tmp < ../examples/make_package/make_package_tasks

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
    runtest -l -s \
	-c '../examples/make_package/make_package_cmd__xxx_failed flex' \
	-n +5 > $OBJDIR/_test.tmp < ../examples/make_package/make_package_tasks2

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
    runtest -l -s \
	-c ../examples/make_package/make_package_cmd \
	-n +5 < ../examples/make_package/make_package_tasks_cycle |
    cmp 'paexec cyclic deps #1' \
'Cyclic dependancy detected:
  devel/gettext-lib -> devel/gmake
  devel/gmake -> lang/gcc
  lang/gcc -> devel/gettext-lib
'

    # cycle detection2
    test_tasks4 | runtest -l -s \
	-c ../examples/make_package/make_package_cmd \
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
' | runtest -l -s \
	-c ../examples/make_package/make_package_cmd \
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
' | runtest -l -s \
	-c ../examples/make_package/make_package_cmd \
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
' | runtest -s -E \
	-t ../examples/broken_echo/transport_broken_echo -c ':' \
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
' | runtest_resort -el -z \
	-t ../examples/broken_echo/transport_broken_toupper \
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
    runtest_resort -Z1 -w \
	-t ../tests/transport_broken_rnd \
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
' | runtest -z -g -E \
	-t ../examples/broken_echo/transport_broken_echo -c ':' \
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
' | runtest -z -r -g -E \
	-t ../examples/broken_echo/transport_broken_echo -c ':' \
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
' | runtest -g -z -lre -t $OBJDIR/transport_closed_stdin -c : \
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
    runtest -z -r -g -E \
	-t ../examples/broken_echo/transport_broken_echo -c ':' \
	-n '4' |
    cmp 'paexec broken transport #7' \
'4 fatal
4 
all nodes failed
'

    # resistance to transport failure
    echo mama | runtest -z -r -g -i -E \
	-t ../examples/broken_echo/transport_broken_echo -c ':' \
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

    runtest -s -z -lr -t ../tests/transport_broken_rnd -c : \
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

    # resistance to transport failure
    awk 'BEGIN {for (i=1; i <= 1000; ++i) {print "dat" i}}' |
    runtest -s -z -lr -t ../tests/transport_broken_rnd -c : \
	-n '0.01-ns 0.03-ns 0.09-ns 0.09-ns 0.03-ns 0-ns' |
    filter_succeded_tasks | sort -n | cksum |
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

    runtest -Z1 -s -n '1 2' -c: \
	-t "../examples/broken_echo/transport_broken_echo2 $test_file" \
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

    # tests for weighted nodes of graph (-W option)
    test_tasks1 | runtest -We -c ../examples/make_package/make_package_cmd -n +1 |
    cmp 'paexec -W #1' \
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

    # tests for sum_weight calculation (-W option)
#    test_tasks2 | runtest -We -c ../examples/make_package/make_package_cmd -n +1
    # tests for sum_weight calculation (-W option)
    test_tasks1 |
    runtest -We -d -c ../examples/make_package/make_package_cmd \
	-n +1 2>&1 | grep '^sum_weight' |
    cmp 'paexec -W #2' \
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

    # tests for sum_weight calculation (-W option)
    test_tasks2 |
    runtest -We -d -c ../examples/make_package/make_package_cmd \
	-n +1 2>&1 | grep '^sum_weight' |
    cmp 'paexec -W #3' \
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

    test -f $tmpex
    return $?
}

for PAEXEC_BUFSIZE in 5; do # 1 2 3 4 5 6 7 8 9 10 11 12 13 14 1000 10000; do
    printf "PAEXEC_BUFSIZE=%d:\n" $PAEXEC_BUFSIZE
    export PAEXEC_BUFSIZE

    if do_test; then
	:
    else
	exit 1
    fi
done
