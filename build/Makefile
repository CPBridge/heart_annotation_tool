SOURCE_DIR:=../src

CPP:=g++
CPPFLAGS:=-Wall -Wextra -O2 -std=c++11 -I$(SOURCE_DIR)
LDFLAGS:=`pkg-config --libs opencv4` -lboost_program_options -lboost_system -lboost_filesystem

VPATH:=$(SOURCE_DIR)

all: heart_annotations substructure_annotations

heart_annotations: heart_annotations.o thesisUtilities.o
	$(CPP) $(LDFLAGS) $^ -o $@ $(LDFLAGS)
	
substructure_annotations: substructure_annotations.o thesisUtilities.o thesisUtilities.o
	$(CPP) $(LDFLAGS) $^ -o $@ $(LDFLAGS)
	
%.o: %.cpp %.h
	$(CPP) -c $(CPPFLAGS) $< -o $@
	
clean:
	rm *.o heart_annotations substructure_annotations
