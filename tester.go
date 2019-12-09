// +build ignore

package main

import (
	"flag"
	"fmt"
	"log"
	"strconv"
	"time"

	"github.com/tarm/serial"
)

var (
	port     *serial.Port
	sn       int = -1
	erase    bool
	check    bool
	get      bool
	getAll   bool
	getNext  bool
	portName string
)

func send(line string) error {
	fmt.Println("send:", line)
	_, err := fmt.Fprintf(port, "\x02%s\x03", line)
	if err != nil {
		return err
	}
	return nil
}

func main() {
	flag.IntVar(&sn, "start", -1, "start serial number")
	flag.BoolVar(&erase, "erase", false, "erace")
	flag.BoolVar(&check, "check", false, "check data status")
	flag.BoolVar(&get, "get", false, "get data 100")
	flag.BoolVar(&getAll, "all", false, "get all data")
	flag.BoolVar(&getNext, "next", false, "get next data 100")
	flag.StringVar(&portName, "port", "", "serial port name")
	flag.Parse()
	c := &serial.Config{Name: portName, Baud: 115200}
	s, err := serial.OpenPort(c)
	if err != nil {
		log.Fatal(err)
	}
	port = s
	switch {
	case sn >= 0:
		send("A" + strconv.Itoa(int(time.Now().Unix())) + "," + fmt.Sprintf("%03d", sn))
	case erase:
		send("B")
	case check:
		send("C")
	case get:
		send("D")
	case getAll:
		send("E")
	case getNext:
		send("F")
	}
	buff := make([]byte, 1024)
	msg := make([]byte, 0)
	discardCount := 0
	reading := false
	for {
		n, err := port.Read(buff)
		if err != nil {
			log.Fatal(err)
		}
		b := buff[:n]
		for _, c := range b {
			switch c {
			case 0x02:
				if discardCount > 0 {
					log.Println("discarded:", discardCount)
					discardCount = 0
				}
				reading = true
			default:
				if reading {
					msg = append(msg, c)
				} else {
					discardCount++
				}
			case 0x03:
				log.Println(msg[0], string(msg[1:]))
				msg = msg[0:0]
				reading = false
			}
		}
	}
}
