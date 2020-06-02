#!/usr/bin/env zsh

set -e

while true; do
    ./separate.zsh log
    cd log-output/
    for i in *.txt; do
        cat $i | ../minilinky > $(basename $i .txt).processed.txt
        gnuplot -e "filename='$(basename $i .txt).processed.txt'" ../linky.plot > $(basename $i .txt).png
        scp ./$(basename $i .txt).png cubox@cubox.dev:/www/cubox/graphs/linky/
    done
    cd ../
    rm -rf log-output/

    ./separatesolar.zsh solarlog
    cd solarlog-output/
    for i in *.txt; do
        # We need to pass the offset to gnuplot later. This is to fix dst
        firstTimeStamp=$(date -r $(head -n 1 $i | cut -d " " -f 1) +%s)
        if [[ $(date -r $firstTimeStamp -v +6H +%z) == "+0100" ]];then
            offset="3600"
        elif [[ $(date -r $firstTimeStamp -v +6H +%z) == "+0200" ]];then
            offset="7200"
        fi
        gnuplot -e "filename='$i'" -e "offset=$offset" ../solar.plot > $(basename $i .txt).png
        scp ./$(basename $i .txt).png cubox@cubox.dev:/www/cubox/graphs/solar/
    done
    cd ../
    rm -rf solarlog-output/
    sleep 600
done
