CXX=mpiicpc

# cannot use mpiicpc, contains --enable-new-dtags so all rpath values also added to runpath, in that case LD_LIBRARY_PATH takes precedence 
CXX=icpc -I/software/easybuild/software/impi/5.0.3.048-iccifort-2015.2.164-GCC-4.9.2/intel64/include -L/software/easybuild/software/impi/5.0.3.048-iccifort-2015.2.164-GCC-4.9.2/intel64/lib/release_mt -L/software/easybuild/software/impi/5.0.3.048-iccifort-2015.2.164-GCC-4.9.2/intel64/lib -Xlinker -rpath -Xlinker /software/easybuild/software/impi/5.0.3.048-iccifort-2015.2.164-GCC-4.9.2/intel64/lib/release_mt -Xlinker -rpath -Xlinker /software/easybuild/software/impi/5.0.3.048-iccifort-2015.2.164-GCC-4.9.2/intel64/lib -Xlinker -rpath -Xlinker /opt/intel/mpi-rt/5.0/intel64/lib/release_mt -Xlinker -rpath -Xlinker /opt/intel/mpi-rt/5.0/intel64/lib -lmpicxx -lmpifort -lmpi -lmpigi -ldl -lrt -lpthread


# for now build using module intel/2015.02
# need to update Makefile to automatically pick directories from LD_LIBRARY_PATH and add them rpath.
# setting RUNPATH does not work since mpiwrapper uses rpath on command line which overrides RUNPATH
# for now just manually set the rpath

LDFLAGS1="-Wl,-rpath=/software/easybuild/software/imkl/11.2.2.164-iimpi-7.2.5-GCC-4.9.2/mkl/lib/intel64"
LDFLAGS2="-Wl,-rpath=/software/easybuild/software/imkl/11.2.2.164-iimpi-7.2.5-GCC-4.9.2/lib/intel64"
LDFLAGS3="-Wl,-rpath=/software/easybuild/software/impi/5.0.3.048-iccifort-2015.2.164-GCC-4.9.2/lib64"
LDFLAGS4="-Wl,-rpath=/software/easybuild/software/ifort/2015.2.164-GCC-4.9.2/lib/intel64"
LDFLAGS5="-Wl,-rpath=/software/easybuild/software/ifort/2015.2.164-GCC-4.9.2/lib"
LDFLAGS6="-Wl,-rpath=/software/easybuild/software/icc/2015.2.164-GCC-4.9.2/lib/intel64"
LDFLAGS7="-Wl,-rpath=/software/easybuild/software/icc/2015.2.164-GCC-4.9.2/lib"
LDFLAGS8="-Wl,-rpath=/software/easybuild/software/GCC/4.9.2/lib/gcc/x86_64-unknown-linux-gnu/4.9.2"
LDFLAGS9="-Wl,-rpath=/software/easybuild/software/GCC/4.9.2/lib64"
LDFLAGS10="-Wl,-rpath=/software/easybuild/software/GCC/4.9.2/lib"

RUNPATH=${LD_LIBRARY_PATH}

CXXFLAGS=-std=c++0x

run-many-serial.x: run-many-serial.cpp
	${CXX} ${CXXFLAGS} -Xlinker --disable-new-dtags ${LDFLAGS1} ${LDFLAGS2} ${LDFLAGS3} ${LDFLAGS4} ${LDFLAGS5} ${LDFLAGS6} ${LDFLAGS7} ${LDFLAGS8} ${LDFLAGS9} ${LDFLAGS10} -o $@ $^

install: run-many-serial.x
	cp run-many-serial.x ../bin/
	cp tamulauncher ../bin/

clean:
	rm -f run-many-serial.x *~

 