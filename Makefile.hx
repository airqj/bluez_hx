all:main_central

main_central:
	mips-openwrt-linux-gcc main_central.c hx_ble.c ./lib/hci.c ./lib/bluetooth.c sha1.c -o main_central -I ./lib/bluetooth -I ./lib/ -I ./src/ -I ./tools/
clean:
	rm main_central
