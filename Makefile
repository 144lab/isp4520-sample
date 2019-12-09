NAME := sample
FQBN := adafruit:nrf52:feather52832
PORT := /dev/tty.usbserial-AD0JSEUI
#CFLAGS:=-DCFG_DEBUG=0 -DRELEASE
CFLAGS:=-DCFG_DEBUG=0
OPTS:=--build-properties="build.debug_flags=$(CFLAGS)"
OUTPUT:=.\\build\\$(NAME).$(subst :,.,$(FQBN))

.PHONY: build upload setup mon

build:
	arduino-cli compile -b $(FQBN) -o $(OUTPUT).hex $(OPTS) $(NAME)
	adafruit-nrfutil dfu genpkg --dev-type 0x0052 --application $(OUTPUT).hex $(OUTPUT).zip

upload:
	arduino-cli upload -b $(FQBN) -p $(PORT) -i $(OUTPUT).hex

setup:
	pip3 install --user adafruit-nrfutil

mon:
	go run . $(PORT)
