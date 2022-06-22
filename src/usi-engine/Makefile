include ../../Makefile.config

THE_OS := $(shell uname -s)
PROGRAM = aobak

# isnan() does not work with -ffast-math


default:
	@echo "Detected OS: ${THE_OS}"
	$(MAKE) CC=gcc CXX=g++ \
		CXXFLAGS='$(CXXFLAGS) -Wall -Wextra -Wno-implicit-fallthrough -pipe -O3 -g -march=native -std=c++14 -DNDEBUG'  \
		LDFLAGS='$(LDFLAGS) -g' \
		$(PROGRAM)

debug:
	@echo "Detected OS: ${THE_OS}"
	$(MAKE) CC