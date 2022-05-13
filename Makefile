all: compile upload

compile:
	esphome compile example.yaml

upload:
	esphome upload example.yaml

logs:
	esphome logs example.yaml

copy:
	cp .esphome/build/alarm-clock/.pioenvs/alarm-clock/firmware.* /mnt/c/Users/mauri/Downloads/
