NAME := sirc_logger
FQBN := 144lab:nrf52:isp1507spiflash
PORT := /dev/tty.usbserial-AD0JSD43
#CFLAGS:=-DCFG_DEBUG=0 -DRELEASE
CFLAGS:=-DCFG_DEBUG=0
OPTS:=--build-properties="build.debug_flags=$(CFLAGS)"
OUTPUT:=$(NAME).$(subst :,.,$(FQBN))

.PHONY: build upload setup mon

build:
	arduino-cli compile -b $(FQBN) -o $(OUTPUT).hex $(OPTS) $(NAME)
	adafruit-nrfutil dfu genpkg --dev-type 0x0052 --application $(OUTPUT).hex $(OUTPUT).zip

upload:
	arduino-cli upload -b $(FQBN) -p $(PORT) -i $(OUTPUT).hex

setup:
	pip3 install --user adafruit-nrfutil

mon:
	go run tester.go -port $(PORT)

clean:
	rm *.elf *.zip *.hex

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
