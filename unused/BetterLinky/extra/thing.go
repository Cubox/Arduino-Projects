package main

import (
	"os"
	"bufio"
	"time"
	"strings"
	"strconv"
	"fmt"
)

type reading struct {
	when time.Time
	amps float64
	watts float64
	minavg5 float64
	minavg30 float64
}

func checkIncof5(t time.Time) bool {
	if t.Second() < 59 {
		return false
	} else if t.Minute() % 5 == 4 {
		return true
	} else {
		return false
	}
}

func checkIncof30(t time.Time) bool {
	if t.Second() < 59 {
		return false
	} else if t.Minute() % 30 == 29 {
		return true
	} else {
		return false
	}
}

func main() {
	reader := bufio.NewReader(os.Stdin)
	var values = make([]reading, 0)

	for {
		str, readerr := reader.ReadString('\n')
		if readerr != nil {
			break;
		}
		str = str[:len(str)-1]
		var splitted = strings.Split(str, " ")
		when, err := strconv.Atoi(splitted[0])
		if err != nil {
			panic(err)
		}
		amps, err := strconv.ParseFloat(splitted[1], 64)
		if err != nil {
			panic(err)
		}
		watts, err := strconv.ParseFloat(splitted[2], 64)
		if err != nil {
			panic(err)
		}
		toAdd := reading {
			when: time.Unix(int64(when), 0),
			amps: amps,
			watts: watts,
		}
		values = append(values, toAdd)
	}

	index := 0
	var sum float64
	for i := range values {
		sum += values[i].watts
		index++

		if i+1 == len(values) || checkIncof5(values[i].when) {
			rangeStart := i - (index - 1)
			for k := range values[rangeStart:i+1] {
				elementIndex := rangeStart+k
				values[elementIndex].minavg5 = sum / float64(index)
			}
			sum = 0
			index = 0
		}
	}

	index = 0
	sum = 0
	for i := range values {
		sum += values[i].watts
		index++

		if i+1 == len(values) || checkIncof30(values[i].when) {
			rangeStart := i - (index - 1)
			for k := range values[rangeStart:i+1] {
				elementIndex := rangeStart+k
				values[elementIndex].minavg30 = sum / float64(index)
			}
			sum = 0
			index = 0
		}
	}

	for _, e := range values {
		fmt.Printf("%d %.6f %.6f %.6f %.6f\n", e.when.Unix(), e.amps, e.watts, e.minavg5, e.minavg30)
	}
}