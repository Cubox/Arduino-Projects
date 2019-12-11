if (!exists("log1")) quit
#if (!exists("log2")) quit
if (!exists("log3")) quit
if (!exists("log4")) quit
if (!exists("log5")) quit
if (!exists("log6")) quit

set style line 1 lc rgb '#cb584d' pt 1 ps 0.5 lt 1 lw 1
set style line 2 lc rgb '#49adaa' pt 1 ps 0.5 lt 1 lw 1
set style line 3 lc rgb '#c45ca2' pt 1 ps 0.5 lt 1 lw 1
set style line 4 lc rgb '#6ca74d' pt 1 ps 0.5 lt 1 lw 1
set style line 5 lc rgb '#7a75cd' pt 1 ps 0.5 lt 1 lw 1
set style line 6 lc rgb '#b78f40' pt 1 ps 0.5 lt 1 lw 1
set style line 11 lc rgb '#808080' lt 0 lw 1
set border back ls 11
set tics nomirror

set autoscale xfix
set ytics 0.5
set y2tics 0.5
set xtics 3600

set datafile separator " "
set terminal png size 11000,800
set title "Temperature"
set ylabel "Celsius"
set xlabel "Time"
set xdata time
set timefmt "%s"
set format x "(%d) %Hh"
set key outside right top
set grid

previous=1
current=1
shift(x) = (previous=current, current=x)
yornothing(x,y) = (shift(x), abs(x-previous)<600?y:sqrt(0/0))

plot \
    log1 \
    using ($1 + 7200):(yornothing($1,$2)) w lines ls 1 title "Chambre Andy", \
    log3 \
    using ($1 + 7200):(yornothing($1,$2)) w lines ls 3 title "Salon", \
    log4 \
    using ($1 + 7200):(yornothing($1,$2)) w lines ls 4 title "Chambre Mina", \
    log5 \
    using ($1 + 7200):(yornothing($1,$2)) w lines ls 5 title "Couloir", \
    log6 \
    using ($1 + 7200):(yornothing($1,$2)) w lines ls 6 title "Salle de bain" \
