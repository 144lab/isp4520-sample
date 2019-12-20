NAME := LoRaTransmitter
FQBN := 144lab:nrf52:isp1507spiflash
PORT := $(shell ls -1 /dev/tty.usbserial-A*)
REGION:=REGION_AS923
RX_CHANNEL:=1
CFLAGS:=-DCFG_DEBUG=0 -D$(REGION) -DRX_CHANNEL=$(RX_CHANNEL)
OPTS:=--build-properties="build.debug_flags=$(CFLAGS)"
OUTPUT:=$(NAME).$(subst :,.,$(FQBN))
SERIAL:=1

.PHONY: build upload setup mon

build:
	arduino-cli compile -t -b $(FQBN) -o $(OUTPUT).hex $(OPTS) $(NAME)
	adafruit-nrfutil dfu genpkg --dev-type 0x0052 --application $(OUTPUT).hex $(OUTPUT).zip

receiver:
	$(MAKE) PORT=$(shell ls -1 /dev/tty.usbserial-D*) NAME=LoRaReceiver build

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

start:
	go run tester.go -port $(PORT) -start $(SERIAL)

erase:
	# 約30秒
	go run tester.go -port $(PORT) -erase

check:
	go run tester.go -port $(PORT) -check

stop:
	go run tester.go -port $(PORT) -stop

get:
	go run tester.go -port $(PORT) -all

next:
	go run tester.go -port $(PORT) -raw -next

dump:
	go run tester.go -port $(PORT) -raw -dump
