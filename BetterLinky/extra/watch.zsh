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

    ./separate.zsh solarlog
    cd solarlog-output/
    for i in *.txt; do
        gnuplot -e "filename='$i'" ../solar.plot > $(basename $i .txt).png
        scp ./$(basename $i .txt).png cubox@cubox.dev:/www/cubox/graphs/solar/
    done
    cd ../
    rm -rf solarlog-output/
    sleep 3600
done
