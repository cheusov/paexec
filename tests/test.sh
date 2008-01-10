#!/bin/sh

set -e

runtest (){
    printf '=================================================================\n'
    printf '======= args: %s\n' "$*"
    ../paexec "$@"
}

cut_version (){
    awk 'NR <= 2 {print $0} NR == 3 {print "xxx"}'
}

cut_help (){
    awk 'NR <= 3'
}

runtest -V        | cut_version
runtest --version | cut_version

runtest -h        | cut_help
runtest --help    | cut_help

resort (){
    awk '{print $1, NR, $0}' |
    sort -k1,1n -k2,2n |
    awk '{$1 = $2 = ""; print substr($0, 3)}'
}

printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
runtest -l -t ./paexec_notransport -c ../examples/toupper/toupper_cmd \
    -n '1 2 3 4 5 6 7 8 9' | resort

printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
runtest -l -p -t ./paexec_notransport -c ../examples/toupper/toupper_cmd -n '+2' |
resort | gawk '$1 ~ /^[0-9]/ {$2 = "pid"; print; next} {print}'

printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
runtest -l -c ../examples/all_substr/all_substr_cmd \
    -n '1 2 3 4 5 6 7 8 9' | resort

printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
runtest -l -c ../examples/all_substr/all_substr_cmd -n '+9' |
resort
