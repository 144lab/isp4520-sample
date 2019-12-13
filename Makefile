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
	./tester -port $(PORT)

clean:
	rm *.elf *.zip *.hex

./tester:
	go build tester.go

erase: ./tester
	# 約30秒
	./tester -port $(PORT) -erase

check: ./tester
	./tester -port $(PORT) -check

start: ./tester
	./tester -port $(PORT) -start 3

stop: ./tester
	./tester -port $(PORT) -stop

get: ./tester
	./tester -port $(PORT) -all

next: ./tester
	./tester -port $(PORT) -raw -next

dump: ./tester
	./tester -port $(PORT) -raw -dump
