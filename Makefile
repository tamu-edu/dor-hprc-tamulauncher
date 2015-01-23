CXX=mpiicpc


ICCLIB=/software/easybuild/software/icc/2013_sp1.3.174/lib/intel64

CXXFLAGS=-std=c++0x
LDFLAGS=-Wl,-rpath ${ICCLIB}

run-many-serial.x: run-many-serial.cpp
	${CXX} ${CXXFLAGS} ${LDFLAGS} -o $@ $^

install: run-many-serial.x
	cp run-many-serial.x ../bin/
	cp tamulauncher ../bin/
	cp tamulauncher-local ../bin/

clean:
	rm -f run-many-serial.x *~

 