EXECUTABLE=bgt
SRC=$(wildcard src/*.cpp)
OBJS=$(patsubst src/%.cpp, build/%.o, $(SRC))
ASMS=$(patsubst src/%.cpp, asm/%.S, $(SRC))
CXX=g++
CXXLIBS=-lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_video -lv4l2 -lmraa
CXXFLAGS=-I./include -Wall

all: $(EXECUTABLE)

$(OBJS): build/%.o : src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@	

$(EXECUTABLE): $(OBJS)
	$(CXX) -o build/$(EXECUTABLE).out $(OBJS) $(CXXLIBS)	

$(ASMS): asm/%.S : src/%.cpp
	$(CXX) $(CXXFLAGS) -S $< -o $@
	
asm: $(ASMS) 
	$(CXX) -o build/$(EXECUTABLE).out $(ASMS) $(CXXLIBS)	
	
run: build/$(EXECUTABLE).out
	./build/$(EXECUTABLE).out
	
install: build/$(EXECUTABLE).out startup/bgtd
	cp build/$(EXECUTABLE).out /usr/bin/$(EXECUTABLE)
	mkdir -p /etc/init.d
	cp startup/bgtd /etc/init.d/bgtd
	chmod 775 /etc/init.d/bgtd
	update-rc.d bgtd defaults
	mkdir -p /opt/bgt
	mkdir -p /opt/bgt/pix
	cp pix/* /opt/bgt/pix/
	mkdir -p /etc/bgt
	cp MFL_settings.ini /etc/bgt
	
uninstall: /usr/bin/$(EXECUTABLE) /etc/init.d/bgtd
	rm -f /usr/bin/$(EXECUTABLE)
	rm -f /etc/init.d/bgtd
	update-rc.d bgtd remove
	rm -f /var/log/bgt.log

clean:
	rm -f asm/*.S
	rm -f build/*.o
	rm -f build/$(EXECUTABLE).out
