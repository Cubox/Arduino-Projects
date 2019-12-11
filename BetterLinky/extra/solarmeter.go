package main

import (
	"github.com/pkg/term"
	"encoding/binary"
	"fmt"
	"time"
)

var serial *term.Term

type packetStruct struct {
	Voltage uint16
	Current int16
	Power int32
	Amphour uint32
	Watthour uint32
}

var average struct {
	voltage, current, power float64
	count uint16
	beginning time.Time
}

var correctHeader = [...]byte{170, 28, 1, 42}
var buffer [4]byte

func main() {
	var err error
	serial, err := term.Open("/dev/cuaU0", term.Speed(57600), term.RawMode)
	
	if err != nil {
		serial, err = term.Open("/dev/cuaU1", term.Speed(57600), term.RawMode)
	}
	if err != nil {
		panic(err)
	}
	serial.SetReadTimeout(time.Duration(time.Minute))
	average.beginning = time.Now()

	var toRead byte
	for {
		err := binary.Read(serial, binary.BigEndian, &toRead)
		if err != nil {
			panic(err)
		}

		buffer[3] = toRead
		if buffer == correctHeader {
			var packet packetStruct
			err := binary.Read(serial, binary.BigEndian, &packet)
			if err != nil {
				panic(err)
			}
			printPacket(&packet)
			buffer = [...]byte{0, 0, 0, 0}

		} else {
			buffer[0] = buffer[1]
			buffer[1] = buffer[2]
			buffer[2] = buffer[3]
		}
	}
}

func printPacket(packet *packetStruct) {
	if time.Now().Sub(average.beginning) <= time.Second { // Add to average
		average.voltage += float64(packet.Voltage) / 100
		average.current += float64(packet.Current) / 100
		average.power += float64(packet.Power) / 1000
		average.count++
	} else {
		fmt.Printf("%d %.3f %.2f %.2f\n", time.Now().Unix(), average.voltage / float64(average.count), average.current / float64(average.count), average.power / float64(average.count))
		average.voltage = 0
		average.current = 0
		average.power = 0
		average.count = 0
		average.beginning = time.Now()
	}
}