MPICXX=mpiicpc

CXXFLAGS=-std=c++0x
OPT=-O2 -g

default: scripts tamulauncher-loadbalanced.x

tamulauncher-loadbalanced.x: run_command_type.cpp commands_type.cpp tamulauncher-loadbalanced.cpp 
	$(MPICXX) $(CXXFLAGS) -Iinclude $(OPT)  -o $@ $^

run-many-serial.x: run-many-serial.cpp
	$(MPICXX) $(CXXFLAGS) $(OPT)  -o $@ $<

scripts:
	cp system.template system.sh;
	sed -i "s|<MPIRUN>|`which mpiexec.hydra`|" system.sh;
	sed -i "s|<TAMULAUNCHERBASE>|`dirname ${PWD}`|" system.sh;
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
