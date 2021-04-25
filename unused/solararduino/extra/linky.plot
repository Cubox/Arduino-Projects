if (!exists("filename")) quit

set style line 1 lc rgb '#A0A0A0' pt 1 ps 0.5 lt 1 lw 0.2
set style line 2 lc rgb '#FF0000' pt 1 ps 0.5 lt 1 lw 1
set style line 3 lc rgb '#0000FF' pt 1 ps 0.5 lt 1 lw 1
set style line 11 lc rgb '#808080' lt 0 lw 1
set border back ls 11
set tics nomirror

set autoscale xfix
set ytics 100 format "% gW"
set y2tics 100 format "% gW"
set xtics 3600

set datafile separator " "
set terminal png size 66000,1600
set title "Watts over time"
set ylabel "Watts"
set xlabel "Date"
set xdata time
set timefmt "%s"
set format x "(%d) %Hh"
set key right top
set grid

plot \
    filename \
    using 1:3 w linespoints ls 1 notitle, \
    '' using 1:4 w lines ls 2 title "5m average", \
    '' using 1:5 w lines ls 3 title "30m average" \
