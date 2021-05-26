# NMEA0183 - Fiware Adapter

Purpose of this piece of software is to capture NMEA0183 traffic using serial adapter like that by Brookhouse and
inject that Fiware using MQTT Ultra Light protocoll. Datamodel for NMEA0183 devices is somethign that is crucial and 
somewhat use case specific. Simple approach is to have each talker identifier or sentence identifier as its own entity 
and all datafields as attributes to it. Datemodel I have not even started yet, so that is to-be-done.
NMEA0183 data formats and so one are found from: https://www.tronico.fi/OH6NT/docs/NMEA0183.pdf

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
git clone https://github.com/AbyssCoreInc/NMEA_Adapter.git
cd NMEA_Adapter
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
Propably I am going to ditch the MQTT and push data directly to IoT Agent, that may me more energy efficient and C++ MQTT library seems to be a little unstable. Or more likely that I am using it wrong.
