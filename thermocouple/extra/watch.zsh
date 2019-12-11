#!/usr/bin/env zsh

set -e

while true; do
    ./separate.zsh log1
    #./separate.zsh log2
    ./separate.zsh log3
    ./separate.zsh log4
    ./separate.zsh log5
    ./separate.zsh log6
    rm -rf png
    mkdir png
    for i in log1-output/*.txt ;do
        gnuplot -e "log1='log1-output/$(basename $i)'" -e "log3='log3-output/$(basename $i)'" -e "log4='log4-output/$(basename $i)'" -e "log5='log5-output/$(basename $i)'" -e "log6='log6-output/$(basename $i)'" therm.plot > png/$(basename $i .txt).png
        scp ./png/$(basename $i .txt).png cubox@cubox.dev:/www/cubox/graphs/therm/
    done
    rm -rf png
    rm -rf ./*-output
    sleep 3600
done
