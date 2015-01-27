
CPPFLAGS += -std=c++1y -g -ggdb -O2 -I./common/RMQ -Icommon
LIBS += -ldivsufsort

all: trivial phi kasai no_fvc

common/RMQ/libRMQ_improved.a:
	$(MAKE) -C $(@D)

trivial: common/driver.cxx common/esa.cxx src/trivial.cxx common/RMQ/libRMQ_improved.a
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^ $(LIBS)

phi: common/driver.cxx common/esa.cxx src/phi.cxx common/RMQ/libRMQ_improved.a
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^ $(LIBS)

kasai: common/driver.cxx common/esa.cxx src/kasai.cxx common/RMQ/libRMQ_improved.a
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^ $(LIBS)

no_fvc: common/driver.cxx common/no_fvc.cxx common/RMQ/libRMQ_improved.a
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^ $(LIBS)

clean:
	-rm -rf trivial phi
