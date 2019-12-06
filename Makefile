NAME := sample
FQBN := adafruit:nrf52:feather52832
PORT := /dev/tty.usbserial-DN05INJ6
#CFLAGS:=-DCFG_DEBUG=0 -DRELEASE
CFLAGS:=-DCFG_DEBUG=0
OPTS:=--build-properties="build.debug_flags=$(CFLAGS)"

.PHONY: build upload setup mon

build:
	mkdir -p ./build
	arduino-cli compile -b $(FQBN) -o ./build/$(NAME).$(subst :,.,$(FQBN)).hex $(OPTS) $(NAME)
	adafruit-nrfutil dfu genpkg --dev-type 0x0052 \
	--application ./build/$(NAME).$(subst :,.,$(FQBN)).hex \
	./build/$(NAME).$(subst :,.,$(FQBN)).zip

upload:
	arduino-cli upload -b $(FQBN) -p $(PORT) -i ./build/$(NAME).$(subst :,.,$(FQBN)).hex

setup:
	pip3 install --user adafruit-nrfutil

mon:
	go run . $(PORT)
