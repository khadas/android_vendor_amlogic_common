to run the bridge, simply at linux prompt 

#./mfgbridge

the sample installation script is provided for installing driver with bridge, user should be able to customize based on their setup

1. for 8797 chip, go to the bridge_linux_0.1.0.31 folder and start the bridge by
	./install_drv usb_usb if the usb wifi and usb BT interface is used
	
	or

	./install_drv sd_sd if the sd wifi and sd BT interface is used

	
2. for 8777 and 8887 chip, go to the bridge_linux_0.1.0.31 folder and start the bridge by
	./install_drv xxxx ( xxxx could be 8777 or 8887 )

3. driver load following below convention: for parallel download 8887:
	drvfw/fc18_bin_sd8xxx
		insmod mlan.ko
		insmod sd8xxx.ko mfg_mode=1 fw_name=mrvl/xxx.bin
	drvfw/fc18_bin_sd8xxx_bt
		insmod bt8xxx.ko fw_name=mrvl/xxx.bin
	
