GEN_SRC := emEvolvableMotherboard.cpp emLogServer.cpp shared_constants.cpp emDataApi.cpp  NascenseAPI_v01e_types.cpp NascenseAPI_v01e_constants.cpp USB.cpp Mecobo.cpp channelmap.cpp emMotherboard.cpp
GEN_OBJ := $(patsubst %.cpp,%.o, $(GEN_SRC))

THRIFT_DIR := /usr/local/include/thrift
BOOST_DIR := /usr/local/include

INC := -I$(THRIFT_DIR) -I$(BOOST_DIR) -I /usr/include/libusb-1.0/

.PHONY: all clean

all: evo_srv

%.o: %.cpp
	$(CXX) -std=c++11 -Wall -DHAVE_INTTYPES_H -DHAVE_NETINET_IN_H $(INC) -c $< -o $@

evo_srv: $(GEN_OBJ)
	$(CXX) -std=c++11 $(INC) -L/usr/local/lib/ $^ -o $@ -lthrift -lusb-1.0 -L/usr/local/lib/ -lpthread

clean:
	$(RM) *.o evo_srv evo_cli evoGates

