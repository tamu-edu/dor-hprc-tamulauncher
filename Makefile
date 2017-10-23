# Will use the currently loaded intel mpi c++ wrapper.
# To make tamulauncher completely independent we want to 
# remove the "--enable-new-dtags" flag. In this case 
# LD_LIBRARY_PATH will not have higher precedence. 
#MPICXX=`mpiicpc -show | sed -r 's/-Xlinker --enable-new-dtags//'`
MPICXX=mpiicpc

# need to hardcode the GCC library path (if not might conflict with system GCC)
GCCLIBS=-Xlinker --disable-new-dtags -Xlinker -rpath -Xlinker $(EBROOTGCCCORE)/lib64 -Xlinker --enable-new-dtags

# need to hardcode the intel omp5 library path since it's not in the default path
OMPLIBS=-Xlinker --disable-new-dtags -Xlinker -rpath -Xlinker $(EBROOTIMKL)/lib/intel64 -Xlinker --enable-new-dtags

CXXFLAGS=-std=c++0x -qopenmp
OPT=-O2 -g

SRC=run_command_type.cpp commands_type.cpp tamulauncher-loadbalanced.cpp logger_type.cpp 


default: scripts tamulauncher-loadbalanced.x

tamulauncher-loadbalanced.x: $(SRC) 
	$(MPICXX) $(GCCLIBS) $(OMPLIBS) $(CXXFLAGS) -Iinclude $(OPT)  -o $@ $^

run-many-serial.x: run-many-serial.cpp
	$(MPICXX)  $(CXXFLAGS) $(OPT)  -o $@ $<

scripts:
	cp system.template system.sh;
	sed -i "s|<MPIRUN>|`which mpiexec.hydra`|" system.sh;
	sed -i "s|<TAMULAUNCHERBASE>|`dirname ${PWD}`|" system.sh;
	sed -i "s|<GCCCOREMODULE>|`echo ${EBROOTGCCCORE} | sed 's#/software/easybuild/software/##'`|" system.sh
	sed -i "s|<MPIMODULE>|`echo ${EBROOTIMPI} | sed 's#/software/easybuild/software/##'`|" system.sh
	cp tamulauncher.template tamulauncher
	sed -i "s|<INCLUDE>|`dirname ${PWD}`/src-git/system.sh|" tamulauncher;

scripts-classic:
	cp tamulauncher-classic.template tamulauncher-classic;
	sed -i "s|<MPIRUN>|`which mpiexec.hydra`|" tamulauncher-classic
	sed -i "s|<TAMULAUNCHERBASE>|`dirname ${PWD}`|" tamulauncher-classic

install: clean scripts scripts-classic tamulauncher-loadbalanced.x run-many-serial.x
	cp system.sh ../bin/
	cp tamulauncher ../
	cp tamulauncher-loadbalanced.x ../bin
	sed -i "s|src-git|bin|" ../bin/system.sh;
	sed -i "s|src-git|bin|" ../bin/tamulauncher
	cp tamulauncher-classic ../bin/
	cp run-many-serial.x ../bin/
	sed -i "s|src-git|bin|" ../bin/tamulauncher-classic


clean:
	rm -f tamulauncher tamulauncher-classic system.sh tamulauncher-loadbalanced.x run-many-serial.x

purge: clean
	rm ../bin/tamulauncher ../bin/system.sh ../bin/tamulauncher-loadbalanced.x 
