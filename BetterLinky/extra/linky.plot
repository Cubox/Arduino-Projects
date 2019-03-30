set style line 1 lc rgb '#A0A0A0' pt 1 ps 0.5 lt 1 lw 0.2
set style line 2 lc rgb '#FF0000' pt 1 ps 0.5 lt 1 lw 1
set style line 3 lc rgb '#0000FF' pt 1 ps 0.5 lt 1 lw 1
set style line 11 lc rgb '#808080' lt 0 lw 1
set border 3 back ls 11
set tics nomirror

set autoscale xfix
set ytics 100
set xtics 7200

set datafile separator " "
set terminal png size 2200,800
set title "Watts over time"
set ylabel "Watts"
set xlabel "Date"
set xdata time
set timefmt "%s"
set format x "(%d) %Hh"
set key left top
set grid

plot \
    "< awk -v date=`date +'%s'` '{if ($1 > date - 86400) print $0; }' log | go run go/thing.go" \
    using 1:3 w linespoints ls 1 notitle, \
    '' using 1:4 w lines ls 2 title "5m average", \
    '' using 1:5 w lines ls 3 title "30m average" \
