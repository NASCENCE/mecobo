#all:
#	g++ -std=c++11 -lthrift -I /usr/include/libusb-1.0/ emEvolvableMotherboard.cpp shared_constants.cpp emDataApi.cpp  NascenseAPI_v01d_types.cpp NascenseAPI_v01d_constants.cpp emMotherboard.cpp  ga.cpp usb.cpp -lusb-1.0 -lgomp -o evo

#client:
#	g++ -std=c++11 -lthrift emEvolvableMotherboard.cpp shared_constants.cpp emDataApi.cpp  NascenseAPI_v01d_types.cpp NascenseAPI_v01d_constants.cpp client.cpp -o cli


GEN_SRC := emEvolvableMotherboard.cpp shared_constants.cpp emDataApi.cpp  NascenseAPI_v01e_types.cpp NascenseAPI_v01e_constants.cpp 
#ga.cpp
GEN_OBJ := $(patsubst %.cpp,%.o, $(GEN_SRC))

THRIFT_DIR := /usr/local/include/thrift
BOOST_DIR := /usr/local/include

INC := -I$(THRIFT_DIR) -I$(BOOST_DIR) -I /usr/include/libusb-1.0/

.PHONY: all clean

all: evo_srv

%.o: %.cpp
	$(CXX) -std=c++11 -Wall -DHAVE_INTTYPES_H -DHAVE_NETINET_IN_H $(INC) -c $< -o $@

evo_srv: channelmap.o usb.o emMotherboard.o emMotherboardImp.o $(GEN_OBJ)
	$(CXX) -std=c++11 $(INC) -L/usr/local/lib/ $^ -o $@ -lthrift -lusb-1.0 

evo_cli: evo_cli.o $(GEN_OBJ)
	$(CXX) $^ -std=c++11 -o $@ -L/usr/local/lib -lthrift

evoGates: evoGates.o $(GEN_OBJ)
	$(CXX) $^ -std=c++11 -o $@ -L/usr/local/lib -lthrift


clean:
	$(RM) *.o evo_srv evo_cli evoGates

