overlay: overlay.cpp
	g++ overlay overlay.cpp

all: overlay.cpp
	g++ -o overlay overlay.cpp

clean:
	$(RM) overlay