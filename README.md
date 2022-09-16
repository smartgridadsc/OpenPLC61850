
# 1. OpenPLC61850

OpenPLC61850 is an extension of OpenPLC (https://github.com/thiagoralves/OpenPLC_v3) by Thiago Alves. 

OpenPLC61850 is compatible with the IEC61850-MMS protocol and runs an IEC61850-MMS server and also a client for communicating with other IEC61850-MMS devices.

OpenPLC61850 supports the IEC61850-MMS protocol using libIEC61850 (https://libiec61850.com/libiec61850/) and pugixml (https://pugixml.org/).

# 2. Compatibility

OpenPLC61850 is only compatible with Linux. We have only tested it on Ubuntu 20 and Debian 10. However, it should work on most Linux distributions.

# 3. Getting OpenPLC61850

To get the OpenPLC61850 files, you can either use git, or download the ZIP file.

### 3.1 For git:
If you have not installed git, you can do so with the following command:
```
sudo apt install git
```
Once git has been installed, you can download the OpenPLC61850 files using git:
```
git clone https://github.com/smartgridadsc/OpenPLC61850.git
```

### 3.2 For ZIP file:
You can download the ZIP file from this page. Once it is downloaded, you can extract the OpenPLC61850 files like this:
```
unzip OpenPLC61850-main.zip
```

# 4. Installation

To install OpenPLC61850, you just need to run the install script:
```
cd OpenPLC61850
./install.sh linux
```
The installation could take a while, depending on your system. Once the installation is complete, you can reboot your system and OpenPLC61850 will be started on boot. 

You can check if OpenPLC61850 is running by accessing the web interface in your web browser, at `localhost:8080`. To access the web interface on another device, you can replace `localhost` with the IP address of the device running OpenPLC61850.

# 5. Usage

The default username and password for the web interface is `openplc` and `openplc`.

## 5.1 Configuration

### 5.1.1 Adding IEC 61131-3 (ST) PLC programs to OpenPLC61850
You can add your IEC 61131-3 PLC programs using the web interface. Since the steps are the same as the original OpenPLC, you can refer to same guide [here](https://www.openplcproject.com/reference/basics/upload) (follow OpenPLC v3 instructions).

Once the program is added to OpenPLC61850, you can find it in `OpenPLC68150/webserver/st_files`. This directory holds all the `*.st` files that you have added to OpenPLC61850. The program that will be used during operation of OpenPLC61850 is listed in `OpenPLC61850/webserver/active_program`.

### 5.1.2 Adding SCL files to OpenPLC61850

Before adding SCL files to OpenPLC61850, you will need to modify them.

You need to add <Private> child elements to the data attributes. This is done as described [here](https://www.sciencedirect.com/science/article/pii/S235271102100162X) (section 3.2) and in the following image. This step is needed so that it can create a mapping between your PLC program variables and the IEC61850 data attributes. 

![image](https://user-images.githubusercontent.com/42404058/169481915-48e1e432-2c28-4ff0-a429-030a0d41333b.png)\

#### 5.1.2.1 Server SCL file
Since the web interface was not modified from the original OpenPLC, you have to manually add the server SCL file to OpenPLC61850 directory.

To add the SCL file for the OpenPLC61850 server, you can copy the SCL file to `OpenPLC61850/webserver/SCL_server_files`. Since this directory can contain multiple SCL files, you should also edit the `OpenPLC61850/webserver/active_scl` text file to reflect the name of the SCL file you would like to use.

#### 5.1.2.2 IED SCL files
For the same reason above, you also have to manually add the IED SCL files to OpenPLC61850 directory. You can add them to `OpenPLC61850/webserver/SCL_client_files`. Unlike the server SCL file, every SCL file in `OpenPLC61850/webserver/SCL_client_files` is processed for configuration, hence unused IED SCL files should be removed.

#### 5.1.2.3 Warning for SCL files
After adding SCL files for server and/or IED, you have to recompile the PLC runtime, or the new configuration will not be set. Unfortunately, this is not done automatically. 

To recompile the PLC runtime, you can just reload the IEC 61131-3 PLC program in the web interface. You can do so by selecting the same PLC program again in the `Programs` tab and pressing `Launch Program` button.

### 5.1.3 Final checks
Before starting OpenPLC61850, you should perform some checks to ensure that the configuration is properly set. 

There are two files you should check: `OpenPLC61850/webserver/core/iecserver.map` and `OpenPLC61850/webserver/core/iecclient.map`. These files are generated after compilation and reflect the configuration taken from the server and IED SCL files. They should not be empty, and should be similar to these:

*iecserver.map* <br/>
![iecserver.map](https://raw.githubusercontent.com/smartgridadsc/OpenPLC61850/main/documentation/images/iecserver_map.png)

*iecclient.map* <br/>
![iecclient.map](https://raw.githubusercontent.com/smartgridadsc/OpenPLC61850/main/documentation/images/iecclient_map.png)

## 5.2 Starting OpenPLC61850

Once OpenPLC61850 is configured to your preference, you can start it by pressing the `Start PLC` button on the web interface.

# 6. Acknowledgement

This work is supported by the National Research Foundation, Singapore, Singapore University of Technology and Design under its National Satellite of Excellence in Design Science and Technology for Secure Critical Infrastructure Grant (NSoE_DeST-SCI2019-0005).

# 7. Publications
- Muhammad M. Roomi, Wen Shei Ong, S.M. Suhail Hussain and Daisuke Mashima, "IEC 61850 Compatible OpenPLC for Cyber Attack Case Studies on Smart Substation Systems" On IEEE Xplore, 2022. <br/> (https://ieeexplore.ieee.org/document/9684382)
-	Muhammad M. Roomi, Wen Shei Ong, Daisuke Mashima, and S.M. Suhail Hussain, “OpenPLC61850: An IEC 61850 Compatible Open-source PLC for Smart Grid Research.” On Elsevier SoftwareX Journal, 2022. <br/> (https://www.sciencedirect.com/science/article/pii/S235271102100162X)
-	Muhammad M. Roomi, Wen Shei Ong, Daisuke Mashima and S. M. Suhail Hussain, “OpenPLC61850: An IEC 61850 Compatible OpenPLC for Smart Grid Research.” On TechRxiv, 2021. <br/> (https://www.techrxiv.org/articles/preprint/OpenPLC61850_An_IEC_61850_compatible_OpenPLC_for_Smart_Grid_Research/14845062).


