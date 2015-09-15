all:main_central main_peripheral

main_central:
	gcc main_central.c hx_ble.c ./lib/hci.c ./lib/bluetooth.c sha1.c -o main_central -I ./lib/bluetooth -I ./lib/ -I ./src/ -I ./tools/
main_peripheral:
	mips-openwrt-linux-gcc main_peripheral.c hx_ble.c ./lib/hci.c ./lib/bluetooth.c sha1.c -o main_peripheral -I ./lib/bluetooth -I ./lib/ -I ./src/ -I ./tools/
clean:
	rm main_central main_peripheral
