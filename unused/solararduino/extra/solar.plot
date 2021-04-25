if (!exists("filename")) quit
if (!exists("offset")) quit

set style line 1 lc rgb '#FF0000' pt 5 ps 1 lt 1 lw 1
set style line 2 lc rgb '#00FF00' pt 5 ps 1 lt 1 lw 1
set style line 11 lc rgb '#808080' lt 1
set border back ls 11
set tics nomirror

set style line 11 lc rgb '#808080' lt 0 lw 1
set grid back ls 12

set auto fix
set ytics format "% gW"
set y2tics format "% gV"
set xtics 3600

set datafile separator " "
set terminal png size 20480,1600
set title "Production solaire"
set ylabel "Watts"
set y2label "Volts"
set xlabel "Date"
set xdata time
set timefmt "%s"
set format x "(%d) %Hh"
set key right top
set grid

previous=1
current=1
shift(x) = (previous=current, current=x)
yornothing(x,y) = (shift(x), abs(x-previous)<600?y:sqrt(0/0))

plot \
    filename \
    using ($1 + offset):(yornothing($1,$4)) w dots ls 2 title "Watts",\
    '' using ($1 + offset):(yornothing($1,$2)) w dots ls 1 axes x1y2 title "Volts" \
