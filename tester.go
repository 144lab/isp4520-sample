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
	dump     bool
	raw      bool
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
	flag.BoolVar(&dump, "dump", false, "dump")
	flag.BoolVar(&raw, "raw", false, "raw mode")
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
	case dump:
		send("G")
	}
	buff := make([]byte, 1024)
	msg := make([]byte, 0)
	bindata := []byte{}
	reading := false
	for {
		n, err := port.Read(buff)
		if err != nil {
			log.Fatal(err)
		}
		b := buff[:n]
		if raw {
			log.Printf("raw: %X", b)
			continue
		}
		for _, c := range b {
			switch c {
			case 0x02:
				if len(bindata) > 0 {
					log.Printf("bin: %X", bindata)
					bindata = bindata[0:0]
				}
				reading = true
			default:
				if reading {
					msg = append(msg, c)
				} else {
					bindata = append(bindata, c)
				}
			case 0x03:
				if reading {
					log.Printf("%02X:%s", msg[0], string(msg[1:]))
					msg = msg[0:0]
					reading = false
				}
			}
		}
	}
}
