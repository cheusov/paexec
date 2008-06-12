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
    runtest -l -p -t ./paexec_notransport \
	-c ../examples/toupper/toupper_cmd -n '+2' |
    resort | gawk '$1 ~ /^[0-9]/ {$2 = "pid"; print; next} {print}'

    # all_substr
    printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
    runtest -l -c ../examples/all_substr/all_substr_cmd \
	-n '1 2 3 4 5 6 7 8 9' | resort

    # all_substr
    printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
    runtest -l -c ../examples/all_substr/all_substr_cmd -n '+9' |
    resort

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
}

for PAEXEC_BUFSIZE in 1 2 3 4 5 6 7 8 9 10 11 12 13 14; do
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
