#pugixml
PUGI = pugixml_src/src/

#mapper
MAPPERFILES = mapper.cpp $(PUGI)pugixml.cpp
MAPPERHEADERS = $(PUGI)pugixml.hpp

all: mapper

mapper: $(MAPPERFILES) $(MAPPERHEADER)
	$(CXX) -o $@ $(MAPPERFILES) -I $(PUGI)