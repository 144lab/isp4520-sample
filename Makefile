NAME := LoRaTransmitter
FQBN := 144lab:nrf52:isp1507spiflash
PORT := $(shell ls -1 /dev/tty.usbserial-A*)
#CFLAGS:=-DCFG_DEBUG=0
CFLAGS:=-DCFG_DEBUG=0 -DREGION_AS923
OPTS:=--build-properties="build.debug_flags=$(CFLAGS)"
OUTPUT:=$(NAME).$(subst :,.,$(FQBN))

.PHONY: build upload setup mon

build:
	arduino-cli compile -t -b $(FQBN) -o $(OUTPUT).hex $(OPTS) $(NAME)
	adafruit-nrfutil dfu genpkg --dev-type 0x0052 --application $(OUTPUT).hex $(OUTPUT).zip

transmitter:
	$(MAKE) PORT=$(shell ls -1 /dev/tty.usbserial-A*) NAME=LoRaTransmitter build upload

transmitter-mon:
	$(MAKE) PORT=$(shell ls -1 /dev/tty.usbserial-A*)  mon

receiver:
	$(MAKE) PORT=$(shell ls -1 /dev/tty.usbserial-D*) NAME=LoRaReceiver build upload

receiver-mon:
	$(MAKE) PORT=$(shell ls -1 /dev/tty.usbserial-D*)  mon

upload:
	arduino-cli upload -b $(FQBN) -p $(PORT) -i $(OUTPUT).hex

setup:
	pip3 install --user adafruit-nrfutil

mon:
	go run monitor.go $(PORT)

clean:
	rm *.elf *.zip *.hex

props:
	arduino-cli compile -b $(FQBN) $(OPTS) --show-properties $(NAME)

port:
	@echo $(PORT)

erase:
	# 約30秒
	go run tester.go -port $(PORT) -erase

check:
	go run tester.go -port $(PORT) -check

start:
	go run tester.go -port $(PORT) -start 3

stop:
	go run tester.go -port $(PORT) -stop

get:
	go run tester.go -port $(PORT) -all

next:
	go run tester.go -port $(PORT) -raw -next

dump:
	go run tester.go -port $(PORT) -raw -dump
