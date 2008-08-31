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
    awk -v task=$1 '
    $2 == task { ln = NR; tn = $1}
    $1 == tn && $2 == "success" {print ln; exit}
    $1 == tn && $2 == "failure" {print "f"; exit}
    ' _test.tmp
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
    runtest -e -s -l -c ../examples/1_div_X/1_div_X_cmd -n +10 <<EOF
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

    # -s: all are successful
    ../paexec -l -s -c ../examples/make_package/make_package_cmd -n +4 \
	> _test.tmp < ../examples/make_package/make_package_tasks

    cat <<EOF
=================================================================
======= args: -l -s -c ../examples/make_package/make_package_cmd -n +4
======= make package test!!!
EOF

    test "`gln devel/glib2`" -gt 0 && echo ok1 || true
    test "`gln textproc/dictem`" -gt 0 && echo ok2 || true
    test "`gln devel/libjudy`" -gt 0 && echo ok3 || true
    test "`gln devel/autoconf`" -lt "`gln wip/libmaa`" && echo ok4 || true
    test "`gln devel/gmake`" -lt "`gln wip/libmaa`" && echo ok5 || true
    test "`gln wip/libmaa`" -lt "`gln wip/dict-server`" && echo ok6 || true
    test "`gln wip/libmaa`" -lt "`gln wip/dict-client`" && echo ok7 || true
    test "`gln devel/m4`" -lt "`gln wip/dict-server`" && echo ok8 || true
    test "`gln devel/byacc`" -lt "`gln wip/dict-client`" && echo ok9 || true
    test "`gln devel/byacc`" -lt "`gln wip/dict-server`" && echo ok10 || true
    test "`gln devel/flex`" -lt "`gln wip/dict-client`" && echo ok11 || true
    test "`gln devel/flex`" -lt "`gln wip/dict-server`" && echo ok12 || true

    test "`awk 'END {print NR}' _test.tmp`" -eq 22 && echo ok100 || true

    # -s: byacc fails
    ../paexec -l -s -c ../examples/make_package/make_package_cmd__byacc \
	-n +4 > _test.tmp < ../examples/make_package/make_package_tasks

    cat <<EOF
=================================================================
======= args: -l -s -c ../examples/make_package/make_package_cmd__byacc -n +4
======= make package test!!!
EOF

    test "`gln devel/glib2`" -gt 0 && echo ok1 || true
    test "`gln textproc/dictem`" -gt 0 && echo ok2 || true
    test "`gln devel/libjudy`" -gt 0 && echo ok3 || true
    test "`gln devel/autoconf`" -lt "`gln wip/libmaa`" && echo ok4 || true
    test "`gln devel/gmake`" -lt "`gln wip/libmaa`" && echo ok5 || true
    test "`gln wip/libmaa`" -gt 0 && echo ok6 || true
    test "`gln devel/m4`" -gt 0 && echo ok7 || true
    test "`gln devel/flex`" -gt 0 && echo ok8 || true
    test "`gln devel/byacc`" = f && echo ok9 || true

    test "`awk 'END {print NR}' _test.tmp`" -eq 19 && echo ok100 || true

    # -s: flex fails
    ../paexec -l -s -c ../examples/make_package/make_package_cmd__flex \
	-n +4 > _test.tmp < ../examples/make_package/make_package_tasks

    cat <<EOF
=================================================================
======= args: -l -s -c ../examples/make_package/make_package_cmd__flex -n +4
======= make package test!!!
EOF

    test "`gln devel/glib2`" -gt 0 && echo ok1 || true
    test "`gln textproc/dictem`" -gt 0 && echo ok2 || true
    test "`gln devel/libjudy`" -gt 0 && echo ok3 || true
    test "`gln devel/autoconf`" -lt "`gln wip/libmaa`" && echo ok4 || true
    test "`gln devel/gmake`" -lt "`gln wip/libmaa`" && echo ok5 || true
    test "`gln wip/libmaa`" -gt 0 && echo ok6 || true
    test "`gln devel/m4`" -gt 0 && echo ok7 || true
    test "`gln devel/byacc`" -gt 0 && echo ok8 || true
    test "`gln devel/flex`" = f && echo ok9 || true

    test "`awk 'END {print NR}' _test.tmp`" -eq 19 && echo ok100 || true

    # -s: libmaa fails
    ../paexec -l -s -c ../examples/make_package/make_package_cmd__libmaa \
	-n +4 > _test.tmp < ../examples/make_package/make_package_tasks

    cat <<EOF
=================================================================
======= args: -l -s -c ../examples/make_package/make_package_cmd__libmaa -n +4
======= make package test!!!
EOF

    test "`gln devel/glib2`" -gt 0 && echo ok1 || true
    test "`gln textproc/dictem`" -gt 0 && echo ok2 || true
    test "`gln devel/libjudy`" -gt 0 && echo ok3 || true
    test "`gln devel/autoconf`" -gt 0 && echo ok4 || true
    test "`gln devel/gmake`" -gt 0 && echo ok5 || true
    test "`gln devel/flex`" -gt 0 && echo ok66 || true
    test "`gln devel/m4`" -gt 0 && echo ok7 || true
    test "`gln devel/byacc`" -gt 0 && echo ok8 || true
    test "`gln wip/libmaa`" = f && echo ok9 || true

    test "`awk 'END {print NR}' _test.tmp`" -eq 19 && echo ok100 || true

    # -s: m4 fails
    ../paexec -l -s -c ../examples/make_package/make_package_cmd__m4 \
	-n +4 > _test.tmp < ../examples/make_package/make_package_tasks

    cat <<EOF
=================================================================
======= args: -l -s -c ../examples/make_package/make_package_cmd__m4 -n +4
======= make package test!!!
EOF

    test "`gln devel/glib2`" -gt 0 && echo ok1 || true
    test "`gln textproc/dictem`" -gt 0 && echo ok2 || true
    test "`gln devel/libjudy`" -gt 0 && echo ok3 || true
    test "`gln devel/autoconf`" -lt "`gln wip/libmaa`" && echo ok4 || true
    test "`gln devel/gmake`" -lt "`gln wip/libmaa`" && echo ok5 || true
    test "`gln devel/flex`" -lt "`gln wip/dict-client`" && echo ok6 || true
    test "`gln devel/byacc`" -lt "`gln wip/dict-client`" && echo ok7 || true
    test "`gln wip/libmaa`" -lt "`gln wip/dict-client`" && echo ok8 || true
    test "`gln devel/m4`" = f && echo ok9 || true

    test "`awk 'END {print NR}' _test.tmp`" -eq 21 && echo ok100 || true

    # -s: libjudy fails
    ../paexec -l -s -c ../examples/make_package/make_package_cmd__libjudy \
	-n +4 > _test.tmp < ../examples/make_package/make_package_tasks

    cat <<EOF
=================================================================
======= args: -l -s -c ../examples/make_package/make_package_cmd__libjudy -n +4
======= make package test!!!
EOF

    test "`gln devel/glib2`" -gt 0 && echo ok1 || true
    test "`gln textproc/dictem`" -gt 0 && echo ok2 || true
#    test "`gln devel/libjudy`" -gt 0 && echo ok3 || true
    test "`gln devel/autoconf`" -lt "`gln wip/libmaa`" && echo ok4 || true
    test "`gln devel/gmake`" -lt "`gln wip/libmaa`" && echo ok5 || true
    test "`gln wip/libmaa`" -lt "`gln wip/dict-server`" && echo ok6 || true
    test "`gln wip/libmaa`" -lt "`gln wip/dict-client`" && echo ok7 || true
    test "`gln devel/m4`" -lt "`gln wip/dict-server`" && echo ok8 || true
    test "`gln devel/byacc`" -lt "`gln wip/dict-client`" && echo ok9 || true
    test "`gln devel/byacc`" -lt "`gln wip/dict-server`" && echo ok10 || true
    test "`gln devel/flex`" -lt "`gln wip/dict-client`" && echo ok11 || true
    test "`gln devel/flex`" -lt "`gln wip/dict-server`" && echo ok12 || true

    test "`awk 'END {print NR}' _test.tmp`" -eq 23 && echo ok100 || true

    # -s: dictem fails
    ../paexec -l -s -c ../examples/make_package/make_package_cmd__dictem \
	-n +4 > _test.tmp < ../examples/make_package/make_package_tasks

    cat <<EOF
=================================================================
======= args: -l -s -c ../examples/make_package/make_package_cmd__dictem -n +4
======= make package test!!!
EOF

    test "`gln devel/glib2`" -gt 0 && echo ok1 || true
#    test "`gln textproc/dictem`" -gt 0 && echo ok2 || true
    test "`gln devel/libjudy`" -gt 0 && echo ok3 || true
    test "`gln devel/autoconf`" -lt "`gln wip/libmaa`" && echo ok4 || true
    test "`gln devel/gmake`" -lt "`gln wip/libmaa`" && echo ok5 || true
    test "`gln wip/libmaa`" -lt "`gln wip/dict-server`" && echo ok6 || true
    test "`gln wip/libmaa`" -lt "`gln wip/dict-client`" && echo ok7 || true
    test "`gln devel/m4`" -lt "`gln wip/dict-server`" && echo ok8 || true
    test "`gln devel/byacc`" -lt "`gln wip/dict-client`" && echo ok9 || true
    test "`gln devel/byacc`" -lt "`gln wip/dict-server`" && echo ok10 || true
    test "`gln devel/flex`" -lt "`gln wip/dict-client`" && echo ok11 || true
    test "`gln devel/flex`" -lt "`gln wip/dict-server`" && echo ok12 || true

    test "`awk 'END {print NR}' _test.tmp`" -eq 23 && echo ok100 || true

    # -s: glib2 fails
    ../paexec -l -s -c ../examples/make_package/make_package_cmd__glib2 \
	-n +4 > _test.tmp < ../examples/make_package/make_package_tasks

    cat <<EOF
=================================================================
======= args: -l -s -c ../examples/make_package/make_package_cmd__glib2 -n +4
======= make package test!!!
EOF

#    test "`gln devel/glib2`" -gt 0 && echo ok1 || true
    test "`gln textproc/dictem`" -gt 0 && echo ok2 || true
    test "`gln devel/libjudy`" -gt 0 && echo ok3 || true
    test "`gln devel/autoconf`" -lt "`gln wip/libmaa`" && echo ok4 || true
    test "`gln devel/gmake`" -lt "`gln wip/libmaa`" && echo ok5 || true
    test "`gln wip/libmaa`" -lt "`gln wip/dict-server`" && echo ok6 || true
    test "`gln wip/libmaa`" -lt "`gln wip/dict-client`" && echo ok7 || true
    test "`gln devel/m4`" -lt "`gln wip/dict-server`" && echo ok8 || true
    test "`gln devel/byacc`" -lt "`gln wip/dict-client`" && echo ok9 || true
    test "`gln devel/byacc`" -lt "`gln wip/dict-server`" && echo ok10 || true
    test "`gln devel/flex`" -lt "`gln wip/dict-client`" && echo ok11 || true
    test "`gln devel/flex`" -lt "`gln wip/dict-server`" && echo ok12 || true

    test "`awk 'END {print NR}' _test.tmp`" -eq 23 && echo ok100 || true

    # -s: gmake fails
    ../paexec -l -s -c ../examples/make_package/make_package_cmd__gmake -n +4 \
	> _test.tmp < ../examples/make_package/make_package_tasks

    cat <<EOF
=================================================================
======= args: -l -s -c ../examples/make_package/make_package_cmd__gmake -n +4
======= make package test!!!
EOF

    test "`gln devel/glib2`" -gt 0 && echo ok1 || true
    test "`gln textproc/dictem`" -gt 0 && echo ok2 || true
    test "`gln devel/libjudy`" -gt 0 && echo ok3 || true
    test "`gln devel/autoconf`" -gt 0 && echo ok4 || true
    test "`gln devel/gmake`" = f && echo ok5 || true
    test "`gln devel/m4`" -gt 0 && echo ok8 || true
    test "`gln devel/byacc`" -gt 0 && echo ok9 || true
    test "`gln devel/flex`" -gt 0 && echo ok12 || true

    test "`awk 'END {print NR}' _test.tmp`" -eq 17 && echo ok100 || true

    # -s: autoconf fails
    ../paexec -l -s -c ../examples/make_package/make_package_cmd__autoconf \
	-n +4 > _test.tmp < ../examples/make_package/make_package_tasks

    cat <<EOF
=================================================================
======= args: -l -s -c ../examples/make_package/make_package_cmd__autoconf -n +4
======= make package test!!!
EOF

    test "`gln devel/glib2`" -gt 0 && echo ok1 || true
    test "`gln textproc/dictem`" -gt 0 && echo ok2 || true
    test "`gln devel/libjudy`" -gt 0 && echo ok3 || true
    test "`gln devel/gmake`" -gt 0 && echo ok4 || true
    test "`gln devel/autoconf`" = f && echo ok5 || true
    test "`gln devel/m4`" -gt 0 && echo ok8 || true
    test "`gln devel/byacc`" -gt 0 && echo ok9 || true
    test "`gln devel/flex`" -gt 0 && echo ok12 || true

    test "`awk 'END {print NR}' _test.tmp`" -eq 17 && echo ok100 || true

    # -s: dict-server fails
    ../paexec -l -s -c ../examples/make_package/make_package_cmd__dict-server \
	-n +4 > _test.tmp < ../examples/make_package/make_package_tasks

    cat <<EOF
=================================================================
======= args: -l -s -c ../examples/make_package/make_package_cmd__dict-server -n +4
======= make package test!!!
EOF

    test "`gln devel/glib2`" -gt 0 && echo ok1 || true
    test "`gln textproc/dictem`" -gt 0 && echo ok2 || true
    test "`gln devel/libjudy`" -gt 0 && echo ok3 || true
    test "`gln devel/autoconf`" -lt "`gln wip/libmaa`" && echo ok4 || true
    test "`gln devel/gmake`" -lt "`gln wip/libmaa`" && echo ok5 || true
    test "`gln wip/dict-server`" = f && echo ok6 || true
    test "`gln wip/libmaa`" -lt "`gln wip/dict-client`" && echo ok7 || true
    test "`gln devel/m4`" -gt 0 && echo ok8 || true
    test "`gln devel/byacc`" -lt "`gln wip/dict-client`" && echo ok9 || true
    test "`gln devel/flex`" -lt "`gln wip/dict-client`" && echo ok11 || true

    test "`awk 'END {print NR}' _test.tmp`" -eq 23 && echo ok100 || true

}

for PAEXEC_BUFSIZE in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 1000 10000; do
    printf "PAEXEC_BUFSIZE=%d:\n" $PAEXEC_BUFSIZE
    export PAEXEC_BUFSIZE

    if do_test > _test.res && diff -H20 -u test.out _test.res; then
	true
    else
	echo "paexec fails (PAEXEC_BUFSIZE=$PAEXEC_BUFSIZE)" 1>&2
	exit 1
    fi
    printf "done\n\n"
done
