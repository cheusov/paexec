#!/bin/sh

print_header (){
    printf '=================================================================\n'
    printf '======= args: %s\n' "$*" | cut_full_path_closed_stdin
}

runtest (){
    print_header "$@"
    $OBJDIR/paexec "$@" 2>&1
}

runtest_resort (){
    print_header "$@"
    $OBJDIR/paexec "$@" 2>&1 | resort
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

do_test (){
    runtest -V        | cut_version
#    runtest --version | cut_version
    runtest -V | cut_version

    runtest -h 2>&1     | cut_help
#    runtest --help 2>&1 | cut_help
    runtest -h 2>&1 | cut_help

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
    resort | awk '$1 ~ /^[0-9]/ {$2 = "pid"; print; next} {print}'

    # all_substr
    printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
    runtest -l -p -t '' \
	-c ../examples/toupper/toupper_cmd -n '+2' |
    resort | awk '$1 ~ /^[0-9]/ {$2 = "pid"; print; next} {print}'

    printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
    runtest -l -p \
	-c ../examples/toupper/toupper_cmd -n '+2' |
    resort | awk '$1 ~ /^[0-9]/ {$2 = "pid"; print; next} {print}'



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

    # -s all failed
    runtest -s -l -c '../examples/make_package/make_package_cmd__xxx_failed .' \
	-n +10 < /dev/null

    # -s all failed
    runtest -s -l -c '../examples/make_package/make_package_cmd__xxx_failed .' \
	-n +5 < /dev/null

    # -s all failed
    runtest -s -l -c '../examples/make_package/make_package_cmd__xxx_failed .' \
	-n +1 < /dev/null

    # -s: all succeeded
    cat <<EOF
=================================================================
======= args: -l -s -c ../examples/make_package/make_package_cmd -n +2
======= make package test!!!
EOF

    $OBJDIR/paexec -l -s -c ../examples/make_package/make_package_cmd -n +2 \
	> $OBJDIR/_test.tmp < ../examples/make_package/make_package_tasks

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

    # -s: byacc fails
    cat <<EOF
=================================================================
======= args: -l -s -c '../examples/make_package/make_package_cmd__xxx_failed byacc' -n +3
======= make package test!!!
EOF

    $OBJDIR/paexec -l -s -c '../examples/make_package/make_package_cmd__xxx_failed byacc' \
	-n +3 > $OBJDIR/_test.tmp < ../examples/make_package/make_package_tasks

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

    # -s: flex fails
    cat <<EOF
=================================================================
======= args: -l -s -c '../examples/make_package/make_package_cmd__xxx_failed flex' -n +5
======= make package test!!!
EOF

    $OBJDIR/paexec -l -s -c '../examples/make_package/make_package_cmd__xxx_failed flex' \
	-n +5 > $OBJDIR/_test.tmp < ../examples/make_package/make_package_tasks

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

    # -s: libmaa fails
    cat <<EOF
=================================================================
======= args: -l -s -c '../examples/make_package/make_package_cmd__xxx_failed libmaa' -n +6
======= make package test!!!
EOF

    $OBJDIR/paexec -l -s -c '../examples/make_package/make_package_cmd__xxx_failed libmaa' \
	-n +6 > $OBJDIR/_test.tmp < ../examples/make_package/make_package_tasks

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

    # -s: m4 fails
    cat <<EOF
=================================================================
======= args: -l -s -c '../examples/make_package/make_package_cmd__xxx_failed m4' -n +4
======= make package test!!!
EOF

    $OBJDIR/paexec -l -s -c '../examples/make_package/make_package_cmd__xxx_failed m4' \
	-n +4 > $OBJDIR/_test.tmp < ../examples/make_package/make_package_tasks

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

    # -s: libjudy fails
    cat <<EOF
=================================================================
======= args: -l -s -c '../examples/make_package/make_package_cmd__xxx_failed libjudy' -n +2
======= make package test!!!
EOF

    $OBJDIR/paexec -l -s -c '../examples/make_package/make_package_cmd__xxx_failed libjudy' \
	-n +2 > $OBJDIR/_test.tmp < ../examples/make_package/make_package_tasks

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

    # -s: dictem fails
    cat <<EOF
=================================================================
======= args: -l -s -c '../examples/make_package/make_package_cmd__xxx_failed dictem' -n +3
======= make package test!!!
EOF

    $OBJDIR/paexec -l -s -c '../examples/make_package/make_package_cmd__xxx_failed dictem' \
	-n +3 > $OBJDIR/_test.tmp < ../examples/make_package/make_package_tasks

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

    # -s: glib2 fails
    cat <<EOF
=================================================================
======= args: -l -s -c '../examples/make_package/make_package_cmd__xxx_failed glib2' -n +6
======= make package test!!!
EOF

    $OBJDIR/paexec -l -s -c '../examples/make_package/make_package_cmd__xxx_failed glib2' \
	-n +6 > $OBJDIR/_test.tmp < ../examples/make_package/make_package_tasks

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

    # -s: gmake fails
    cat <<EOF
=================================================================
======= args: -l -s -c '../examples/make_package/make_package_cmd__xxx_failed gmake' -n +5
======= make package test!!!
EOF

    $OBJDIR/paexec -l -s -c '../examples/make_package/make_package_cmd__xxx_failed gmake' -n +5 \
	> $OBJDIR/_test.tmp < ../examples/make_package/make_package_tasks

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

    # -s: autoconf fails
    cat <<EOF
=================================================================
======= args: -l -s -c '../examples/make_package/make_package_cmd__xxx_failed autoconf' -n +4
======= make package test!!!
EOF

    $OBJDIR/paexec -l -s -c '../examples/make_package/make_package_cmd__xxx_failed autoconf' \
	-n +4 > $OBJDIR/_test.tmp < ../examples/make_package/make_package_tasks

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

    # -s: dict-server fails
    cat <<EOF
=================================================================
======= args: -l -s -c '../examples/make_package/make_package_cmd__xxx_failed dict-server' -n +4
======= make package test!!!
EOF

    $OBJDIR/paexec -l -s -c '../examples/make_package/make_package_cmd__xxx_failed dict-server' \
	-n +4 > $OBJDIR/_test.tmp < ../examples/make_package/make_package_tasks

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

    # -s: flex and byacc fail
    cat <<EOF
=================================================================
======= args: -l -s -c '../examples/make_package/make_package_cmd__xxx_failed "flex|byacc"' -n +4
======= make package test!!!
EOF

    $OBJDIR/paexec -l -s -c '../examples/make_package/make_package_cmd__xxx_failed "flex|byacc"' \
	-n +4 > $OBJDIR/_test.tmp < ../examples/make_package/make_package_tasks

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

    # -s: gmake and autoconf fail
    cat <<EOF
=================================================================
======= args: -l -s -c '../examples/make_package/make_package_cmd__xxx_failed "gmake|autoconf"' -n +5
======= make package test!!!
EOF

    $OBJDIR/paexec -l -s \
	-c '../examples/make_package/make_package_cmd__xxx_failed "gmake|autoconf"' \
	-n +4 > $OBJDIR/_test.tmp < ../examples/make_package/make_package_tasks

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

    # diamond-like dependancy and failure
    cat <<EOF
=================================================================
======= args: -l -s -c '../examples/make_package/make_package_cmd__xxx_failed flex' -n +5
======= make package test!!!
EOF

    $OBJDIR/paexec -l -s \
	-c '../examples/make_package/make_package_cmd__xxx_failed flex' \
	-n +5 > $OBJDIR/_test.tmp < ../examples/make_package/make_package_tasks2

    test "`gln devel/flex`" = f && echo ok1

    awk 'NF > 2 && $2 == "devel/flex" &&
         /wip\/dict-client/ && /wip\/dict-server/ &&
         /wip\/pkg_online/ {print "ok2"}' $OBJDIR/_test.tmp

    if grep 'wip/pkg_online.*wip/pkg_online' $OBJDIR/_test.tmp > /dev/null; then
	echo 'needs to be fixed!!!'
    else
	echo ok
    fi

    # cycle detection1
    runtest -l -s \
	-c ../examples/make_package/make_package_cmd \
	-n +5 < ../examples/make_package/make_package_tasks_cycle

    # cycle detection2
    runtest -l -s \
	-c ../examples/make_package/make_package_cmd \
	-n +5 <<EOF
task-2 task-1
task-1 task0
task0 task10
task10 task20
task20 task30
task30 task10
EOF

    # cycle detection2
    runtest -l -s \
	-c ../examples/make_package/make_package_cmd \
	-n +5 <<EOF
task0 task10
task10 task20
task20 task30
task50 task50
task30 task40
EOF

    # cycle detection2
    runtest -l -s \
	-c ../examples/make_package/make_package_cmd \
	-n +5 <<EOF
task0
task100
task0 task200
task0 task100
task10 task100
task20 task100
task50 task100
task100 task110
task200 task300
task300 task0
EOF

    # transport failure
    runtest -s -E \
	-t ../examples/broken_echo/transport_broken_echo -c ':' \
	-n '1 2 3 4 5 6' <<EOF
-1 0
0 1
1 2
2 3
3 4
4 5
EOF

    # resistance/-z without -s
    runtest_resort -el -z \
	-t ../examples/broken_echo/transport_broken_toupper \
	-c : -n '4 5 6 7' <<'EOF'
bilberry
gooseberry
apple
pear
plum
cherry
EOF

    # resistance to transport failure
    runtest -z -g -E \
	-t ../examples/broken_echo/transport_broken_echo -c ':' \
	-n '1' <<EOF
-1 0
0 1
EOF

    # resistance to transport failure
    runtest -z -r -g -E \
	-t ../examples/broken_echo/transport_broken_echo -c ':' \
	-n '1 2 3 4 5 6 7' <<EOF
-1 0
0 1
1 2
2 3
3 4
4 5
5 6
EOF

    # resistance to transport failure2 (write(2) errors)
    runtest -g -z -lre -t $OBJDIR/transport_closed_stdin -c : \
	-n '0 1 2 3 4 5 6 7 8' <<EOF
0 1
1 2
2 3
3 4
4 5
5 6
EOF

    # resistance to transport failure
    awk '
    BEGIN {
        for (i=1; i < 300; ++i){
            print i
        }
    }' |
    runtest -z -r -g -E \
	-t ../examples/broken_echo/transport_broken_echo -c ':' \
	-n '4'

    # resistance to transport failure
    runtest -z -r -g -i -E \
	-t ../examples/broken_echo/transport_broken_echo -c ':' \
	-n '4' <<EOF
mama
EOF

    # resistance to transport failure
    cat <<EOF
=================================================================
======= paexec -s -z -lr -t ../tests/transport_broken_rnd -c : -n '0.1 0.15 0.2 0.25 0.3 0' | filter_succeded_tasks
======= -z test!!!
EOF

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

    ${OBJDIR}/paexec -s -z -lr -t ../tests/transport_broken_rnd -c : \
	-n '0.1 0.15 0.2 0.25 0.3 0' < $OBJDIR/_test.in |
    filter_succeded_tasks | sort -n

    # resistance to transport failure
    cat <<EOF
=================================================================
======= <echo 1-1000 number> |
======= paexec -s -z -lr -t ../tests/transport_broken_rnd -c :
=======     -n '0.01 0.01 0.01 0.01 0.01 0' | filter_succeded_tasks
======= -z test!!!
EOF

    awk 'BEGIN {for (i=1; i <= 1000; ++i) {print "dat" i}}' |
    ${OBJDIR}/paexec -s -z -lr -t ../tests/transport_broken_rnd -c : \
	-n '0.01-ns 0.03-ns 0.09-ns 0.09-ns 0.03-ns 0-ns' |
    filter_succeded_tasks | sort -n | cksum

    # resistance with timeout to rerun failed command
    cat <<EOF
=================================================================
======= paexec -Z1 -s -r -n '1 2' -c:
=======   -t "../examples/broken_echo/transport_broken_echo2 \$test_file"
======= < _tasks.tmp | grep output | sort
EOF

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

    ${OBJDIR}/paexec -Z1 -s -n '1 2' -c: \
	-t "../examples/broken_echo/transport_broken_echo2 $test_file" \
	< $OBJDIR/_tasks.tmp | grep output | sort

    rm -f "$test_file"

    return 0
}

for PAEXEC_BUFSIZE in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 1000 10000; do
    printf "PAEXEC_BUFSIZE=%d:\n" $PAEXEC_BUFSIZE
    export PAEXEC_BUFSIZE

    if do_test > $OBJDIR/_test.res 2>&1 &&
	diff -C10 test.out $OBJDIR/_test.res
    then
	true
    else
	echo "paexec fails (PAEXEC_BUFSIZE=$PAEXEC_BUFSIZE)" 1>&2
	exit 1
    fi
    printf "done\n\n"
done
