#!/bin/sh

set -e

runtest (){
    printf '=================================================================\n'
    printf '======= args: %s\n' "$*"
    ../paexec "$@"
}

cut_version (){
    awk '$1 == "paexec" {$2 = "x.y.x"} {print}'
}

cut_help (){
    awk 'NR <= 3'
}

resort (){
    awk '{print $1, NR, $0}' |
    sort -k1,1n -k2,2n |
    awk '{$1 = $2 = ""; print substr($0, 3)}'
}

gln (){
    awk "\$1 == \"$1\" {print NR; exit}" _test.tmp
}

do_test (){
    runtest -V        | cut_version
    runtest --version | cut_version

    runtest -h 2>&1     | cut_help
    runtest --help 2>&1 | cut_help

    # toupper
    printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
    runtest -l -t ./paexec_notransport -c ../examples/toupper/toupper_cmd \
	-n '1 2 3 4 5 6 7 8 9' | resort

    # toupper
    printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
    runtest -l -t '' -c ../examples/toupper/toupper_cmd \
	-n '1 2 3 4 5 6 7 8 9' | resort

    printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
    runtest -l -c ../examples/toupper/toupper_cmd \
	-n '1 2 3 4 5 6 7 8 9' | resort


    printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
    runtest -l -p -t ./paexec_notransport \
	-c ../examples/toupper/toupper_cmd -n '+2' |
    resort | gawk '$1 ~ /^[0-9]/ {$2 = "pid"; print; next} {print}'

    # all_substr
    printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
    runtest -l -p -t '' \
	-c ../examples/toupper/toupper_cmd -n '+2' |
    resort | gawk '$1 ~ /^[0-9]/ {$2 = "pid"; print; next} {print}'

    printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
    runtest -l -p \
	-c ../examples/toupper/toupper_cmd -n '+2' |
    resort | gawk '$1 ~ /^[0-9]/ {$2 = "pid"; print; next} {print}'



    printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
    runtest -l -c ../examples/all_substr/all_substr_cmd \
	-n '1 2 3 4 5 6 7 8 9' | resort

    # all_substr
    printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
    runtest -l -c ../examples/all_substr/all_substr_cmd -n '+9' |
    resort

    # no input
    runtest -c ../examples/all_substr/all_substr_cmd -n +3 < /dev/null

    # bad command + no input
    runtest -l -c /path/to/bad/prog -n +3 < /dev/null

    # bi-i-i-i-i-i-ig result
    awk '
    BEGIN {
	for (i=0; i < 10; ++i) {
	    print "1234567890-=qwertyuiop[]asdfghjkl;zxcvbnm,./zaqwsxcderfvbgtyhnmjuik,.lo";
	}
    }' | runtest -c ../tests/big_result_cmd -n '+9' |
    uniq -c |
    head -n 100 |
    awk '{$1 = $1; print $0}'

    # tests for partially ordered set of tasks (-s option)
    runtest -s -l -c ../examples/1_div_X/1_div_X_cmd -n +10 <<EOF
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
    # -s and no input
    runtest -s -l -c ../examples/1_div_X/1_div_X_cmd -n +10 < /dev/null

    # -s
    runtest -s -c ../examples/make_package/make_package_cmd -n +4 \
	> _test.tmp <<EOF
textproc/dictem
devel/autoconf wip/libmaa
devel/gmake wip/libmaa
wip/libmaa wip/dict-server
wip/libmaa wip/dict-client
devel/m4 wip/dict-server
devel/byacc wip/dict-server
devel/byacc wip/dict-client
devel/flex wip/dict-server
devel/flex wip/dict-client
devel/glib2
devel/libjudy
EOF

    cat <<EOF
=================================================================
======= make package test!!!
EOF

    test "`gln devel/glib2`" -gt 0 && echo ok
    test "`gln textproc/dictem`" -gt 0 && echo ok
    test "`gln devel/libjudy`" -gt 0 && echo ok
    test "`gln devel/autoconf`" -lt "`gln wip/libmaa`" && echo ok
    test "`gln devel/gmake`" -lt "`gln wip/libmaa`" && echo ok
    test "`gln wip/libmaa`" -lt "`gln wip/dict-server`" && echo ok
    test "`gln wip/libmaa`" -lt "`gln wip/dict-client`" && echo ok
    test "`gln devel/m4`" -lt "`gln wip/dict-server`" && echo ok
    test "`gln wip/byacc`" -lt "`gln wip/dict-client`" && echo ok
    test "`gln wip/byacc`" -lt "`gln wip/dict-server`" && echo ok
    test "`gln wip/flex`" -lt "`gln wip/dict-client`" && echo ok
    test "`gln wip/flex`" -lt "`gln wip/dict-server`" && echo ok
}

for PAEXEC_BUFSIZE in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 1000 10000; do
    printf "PAEXEC_BUFSIZE=%d:\n" $PAEXEC_BUFSIZE
    export PAEXEC_BUFSIZE

    if do_test > _test.res && diff -u test.out _test.res; then
	true
    else
	echo "paexec fails (PAEXEC_BUFSIZE=$PAEXEC_BUFSIZE)" 1>&2
	exit 1
    fi
    printf "done\n\n"
done
