package main

import (
	"errors"
	"flag"
	"io"
	"log"
	"os"

	"github.com/tarm/serial"
)

func main() {
	flag.Parse()
	c := &serial.Config{Name: flag.Arg(0), Baud: 115200}
	s, err := serial.OpenPort(c)
	if err != nil {
		log.Fatal(err)
	}
	if _, err := io.Copy(os.Stdout, s); err != nil {
		if errors.Is(err, io.EOF) {
			return
		}
		log.Fatal(err)
	}
}
