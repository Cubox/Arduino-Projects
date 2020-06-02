#!/usr/bin/env zsh

set -e

if [ $# -eq 0 ]; then
    echo "We need an argument mate"
    return 1
fi

i=$((0))
## Get the first value, which is a timestamp.
## Turn it into a date entering only the day/month/year, and set it to midnight that day.
#beginningText="$(date -r $(head -n 1 $1 | cut -d " " -f 1) "+%Y-%m-%d")T00:00:00"

# Well now we're not doing that. We just take today and remove a day and let's go baby
beginningText="$(date -v -1d "+%Y-%m-%d")T00:00:00"
beginning="$(date -jf "%Y-%m-%dT%H:%M:%S" $beginningText "+%s")"
# Get the last timestamp of the file. We'll use it later to stop the loop
end=$(tail -n 1 $1 | cut -d " " -f 1)
rm -rf "$1"-output
mkdir "$1"-output

while true; do
    # Use $beginning and add i days to it. Use that for the filename
    outfile="$1"-output/$(date -r $beginning -v +"$i"d "+%Y-%m-%dT%H:%M:%S").txt
    # Now, we pass the timestamp of that day (midnight) as date. 
    date=$(date -r $beginning -v +"$i"d "+%s")
    # enddate is the same with a day added
    enddate=$(date -r $beginning -v +"$i"d -v +1d "+%s")
    # Use awk to filter out any lines where the timestamp is between date and enddate
    awk -v date=$date -v enddate=$enddate '{if ($1 >= date && $1 < enddate) print $0;}' $1 > $outfile
    if [ ! -s ${outfile} ]; then
        rm $outfile
    fi
    i=$((i = i+1))
    # We are at the end of the file
    if [[ $(date -r $beginning -v +"$i"d "+%s") -gt $end ]]; then
        break
    fi
done
