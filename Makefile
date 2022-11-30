overlay: overlay.cpp
	g++ overlay overlay.cpp

all: overlay.cpp
	g++ -o overlay overlay.cpp
	# -std=c++11 maybe?

clean:
	$(RM) overlay