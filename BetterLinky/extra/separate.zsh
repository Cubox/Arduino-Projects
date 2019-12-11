#!/usr/bin/env zsh

set -e

if [ $# -eq 0 ]; then
    echo "We need an argument mate"
    return 1
fi

i=$((0))
beginning="$(date -r $(head -n 1 $1 | cut -d " " -f 1) "+%Y-%m-%d")T00:00:00"
end=$(tail -n 1 $1 | cut -d " " -f 1)
rm -rf "$1"-output
mkdir "$1"-output

while true; do
    outfile="$1"-output/$(date -v +"$i"d -jf "%Y-%m-%dT%H:%M:%S" $beginning "+%Y-%m-%dT%H:%M:%S").txt
    awk -v date=$(date -v +"$i"d -jf "%Y-%m-%dT%H:%M:%S" $beginning "+%s") '{if ($1 >= date && $1 < date + 86400) print $0;}' $1 > $outfile
    if [ ! -s ${outfile} ]; then
        rm $outfile
    fi
    i=$((i = i+1))
    if [[ $(date -v +"$i"d -jf "%Y-%m-%dT%H:%M:%S" $beginning "+%s") -gt $end ]]; then
        break
    fi
done