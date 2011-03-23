# This should work on Linux.  Modify as needed for other platforms.

# Change the following to match your installation
BOINC_DIR = /usr/include/boinc
BOINC_API_DIR = $(BOINC_DIR)/api
BOINC_LIB_DIR = $(BOINC_DIR)/lib

CXXFLAGS = -g \
    -I../.. \
    -I../../api \
    -I../../lib \
    -L../../lib \
    -I$(BOINC_DIR) \
    -I$(BOINC_LIB_DIR) \
    -I$(BOINC_API_DIR) \
    -L$(BOINC_API_DIR) \
    -L$(BOINC_LIB_DIR) \

PROGS= wrapper
EXTRA_SOURCES=

all: $(PROGS)

LIBS=libstdc++.a libboinc_api.a libboinc.a

$(LIBS):
	if [ -r "../../lib/$@" ]; then \
		ln -sf "../../lib/$@" . ; \
	elif [ -r "../../api/$@" ]; then \
		ln -sf "../../api/$@" . ; \
	else \
		ln -sf `g++ -print-file-name=$@` ; \
	fi

clean:
	rm -f $(PROGS) *.o *.a

distclean:
	/bin/rm -f $(PROGS) *.o $(LIBS)

$(PROGS):  $(LIBS)
	g++ $(CXXFLAGS) -Wl,--as-needed -o $@ $(@).cpp $(EXTRA_SOURCES) -L . $(LIBS) -pthread

