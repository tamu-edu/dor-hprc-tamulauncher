MPICXX=mpiicpc

CXXFLAGS=-std=c++0x
OPT=-O2

default: scripts tamulauncher-loadbalanced.x

tamulauncher-loadbalanced.x: tamulauncher-loadbalanced.cpp
	$(MPICXX) $(CXXFLAGS) $(OPT)  -o $@ $<

run-many-serial.x: run-many-serial.cpp
	$(MPICXX) $(CXXFLAGS) $(OPT)  -o $@ $<

scripts:
	cp system_template.sh system.sh;
	sed -i "s|<MPIRUN>|`which mpiexec.hydra`|" system.sh;
	sed -i "s|<TAMULAUNCHERBASE>|`dirname ${PWD}`|" system.sh;
	cp tamulauncher_template tamulauncher
	sed -i "s|<INCLUDE>|`dirname ${PWD}`/src-git/system.sh|" tamulauncher;


install: scripts tamulauncher-loadbalanced.x run-many-serial.x
	cp system.sh ../bin/
	cp tamulauncher ../bin/tamulauncher
	cp tamulauncher-loadbalanced.x ../bin
	sed -i "s|src-git|bin|" ../bin/system.sh;
	sed -i "s|src-git|bin|" ../bin/tamulauncher
	cp tamulauncher-classic ../bin/
	cp run-many-serial.x ../bin/



clean:
	rm tamulauncher system.sh tamulauncher-loadbalanced.x run-many-serial.x

purge: clean
	rm ../bin/tamulauncher ../bin/system.sh ../bin/tamulauncher-loadbalanced.x 
