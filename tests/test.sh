#!/bin/sh

set -e

runtest (){
    echo '-----------------------------------------------------------------'
    echo "------- args: $@"
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
    awk '{print $1, NR, $2}' |
    sort -k1,1n -k2,2n |
    awk '{print $1, $3}'
}

printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
runtest -l -t ./paexec_notransport  -c ./task_toupper \
    -n '1 2 3 4 5 6 7 8 9' | resort

printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
runtest -l -c ./task_all_substrings \
    -n '1 2 3 4 5 6 7 8 9' | resort

printf 'a\nbb\nccc\ndddd\neeeee\nffffff\n' |
runtest -l -c ./task_all_substrings \
    -n '+9' | resort
