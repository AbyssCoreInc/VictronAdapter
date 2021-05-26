# Victron VE.Direct - Fiware Adapter

Purpose of this piece of software is to capture Victron VE.Direct traffic using serial adapter and
inject that Fiware using MQTT Ultra Light protocoll or REST API. Datamodels for devices are somethign that are crucial and 
currently under work. VE.Direct protocoll is released by Victron as open source which elevates Victron above all the other device manufacturers. 
C-code for serial communication lend heavily from https://github.com/ullman/velog -project

## End Goal
End goal is to build adapter to OpenCPN that can use Fiware as data source for ship data. To this end data need to be 
structured in more logical way than simple devices and data they produce. This service is a component to that offers 
access to legacy devices that use NMEA0183 protocoll. Other components are build for NMEA2000 devices and other 
generic or proprietary data sources like Victron power systems that use VE.Direct or CanBUS bus. 

## Dependencies
Following dependencies need to be installed to compile the software
```
sudo apt-get install -y nlohmann-json-dev

# libusbp
sudo apt-get install cmake libudev-dev
git clone https://github.com/pololu/libusbp.git
cd libusbp
mkdir build
cd build
cmake ..
make
sudo make install

sudo apt-get install libmosquitto-dev libmosquittopp-dev libssl-dev
```
Obviously you need to have MQTT broker such as Mosquitto installed on the machine where messages are published
## Installation
Installation follows basic route except is much simpler
```
git clone https://github.com/AbyssCoreInc/VictronAdapter.git
cd VictronAdapter
make
sudo make install
```
thats it now adapter should be running and passing all NMEA sentences to MQTT broker
you can verify that with mosquitto_sub tool with command:
```
mosquitto_sub -t "#" -h 127.0.0.1 -p 1883 -v
```
As a result you should see messages such as:
```
/1234/VHW/attrs {"speed_knots":"0.0"}
```
## Next steps
...
