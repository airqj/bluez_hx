# bluez_hx
假设使用的步骤:
1.模拟hciconfig noleadv 关闭蓝牙广播
2.模拟hcitool cmd 发送自定义数据
3.模拟hciconfig leadv 开始蓝牙广播
4.广播一段时间后，关闭广播
