# Revamped IoT System Simulator
This project is based on an Operating System's class assignment but better thought out, better structured and more efficient.

## Original Diagram
![Original Iot Sim Diagram](assets/original-diagram.png)

## Revamped Diagram
![Revised Iot Sim Diagram](assets/new_diagram.svg)

## Build
### Normal Build
Default compilation option, the program will run as normal.
```sh
mkdir build
cd build
cmake ..
make
```

### Debug Build
When setting the DEBUG flag, the simulator will give all kinds of information about what's going on under the hood (ie. insertions in the Bin Max Heap), thanks to conditional compilation. 
```sh
mkdir build
cd build
cmake -DDEBUG=ON ..
make
```

## Usage
### iot-system-sim
```sh
./iot-system-sim *config_file*
```
### sensor
```sh
./sensor *sensorID* *interval (seconds)* *key* *min value* *max value* *config file*
```