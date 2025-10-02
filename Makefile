# Will use the currently loaded intel mpi c++ wrapper.
# To make tamulauncher completely independent we want to 
# remove the "--enable-new-dtags" flag. In this case 
# LD_LIBRARY_PATH will not have higher precedence. 


# define MPICXX GCCLIB OMPLIBS CXXFLAGS OPT for ada and terra 
#ada terra: MPICXX=`mpiicpc -show | sed -r 's/-Xlinker --enable-new-dtags//'`
# need to hardcode the GCC library path (if not might conflict with system GCC)
ada terra grace aces: GCCLIBS=-Xlinker --disable-new-dtags -Xlinker -rpath -Xlinker $(EBROOTGCCCORE)/lib64 
# need to hardcode the intel omp5 library path since it's not in the default path
grace faster aces: OMPLIBS=-Xlinker -rpath -Xlinker $(EBROOTIMKL)/compiler/2023.1.0/linux/compiler/lib/intel64_lin
#ada terra grace: OMPLIBS=-Xlinker -rpath -Xlinker $(EBROOTIMKL)/lib/intel64 
ada terra grace aces: CXXFLAGS=-std=c++0x -qopenmp
ada terra grace aces: OPT=-O3  -g
ada terra grace aces: COMPILER=icpc

#DEFINE MPICXX GCCLIB OMPLIBS CXXFLAGS OPT for curie
curie: MPICXX=`mpic++ --show | sed -r 's/-Wl,--enable-new-dtags//'`
curie: GCCLIBS=-Wl,--disable-new-dtags -Wl,-rpath -Wl,$(EBROOTGCCCORE)/lib64
curie: OMPLIBS=
curie: CXXFLAGS=-std=c++14 -fopenmp
curie: OPT=-O2 -g
curie: COMPILER=g++

SRC=run_command_type.cpp commands_type.cpp tamulauncher-loadbalanced.cpp logger_type.cpp 


default: message

message:
	@echo "... Not building, please specify target: ada / terra / curie";

tamulauncher-loadbalanced.x: $(SRC) 
	@echo "$(COMPILER) $(GCCLIBS) $(OMPLIBS) $(CXXFLAGS) -Iinclude $(OPT)  -o $@ $^"
	@ $(COMPILER) $(GCCLIBS) $(OMPLIBS) $(CXXFLAGS) -Iinclude $(OPT)  -o $@ $^

doada:
	cp system.ada.sh system.sh;
	cp release_script.lsf.sh  release_script.sh;

ada: tamulauncher-loadbalanced.x doada scripts

doterra:
	cp system.terra.sh system.sh
	cp release_script.slurm.sh release_script.sh

terra: tamulauncher-loadbalanced.x doterra scripts

dograce:
	cp system.grace.sh system.sh
	cp release_script.slurm.sh release_script.sh

grace: tamulauncher-loadbalanced.x dograce scripts

doaces:
	cp system.aces.sh system.sh
	cp release_script.slurm.sh release_script.sh

aces: tamulauncher-loadbalanced.x doaces scripts

docurie:
	cp system.curie.sh system.sh
	cp release_script.lsf.sh release_script.sh

curie: tamulauncher-loadbalanced.x docurie scripts

scripts:
	sed -i "s|<MPIRUN>|`which mpirun`|" system.sh;
	sed -i "s|<TAMULAUNCHERBASE>|`dirname ${PWD}`|" system.sh;
	sed -i "s|<GCCCOREMODULE>|`echo ${EBROOTGCCCORE} | sed 's#/software/easybuild/software/##'`|" system.sh
	sed -i "s|<MPIMODULE>|`echo ${EBROOTIMPI} | sed 's#/software/easybuild/software/##'`|" system.sh
	cp tamulauncher.template tamulauncher
	sed -i "s|<INCLUDE>|`dirname ${PWD}`/tamulauncher-src/system.sh|" tamulauncher;
	sed -i "s|<CHECKOLD>|`dirname ${PWD}`/tamulauncher-src/check_classic.sh|" tamulauncher;
	sed -i "s|<VERSION>|`cat version_string`|" tamulauncher;
	sed -i "s|<TAMULAUNCHERBASE>|`dirname ${PWD}`|" tamulauncher
versionmessage:
	@echo
	@echo
	@echo "WARNING: make sure Version string has been updated correctly. If not, update version string and do make clean <TARGET> install."

install: versionmessage
	@echo "WARNING: make sure target has been built before executing install; do make clean <TARGET> install."
	@cp system.sh ../bin/
	@cp check_classic.sh ../bin/
	@cp tamulauncher ../bin/
	@cp tamulauncher-loadbalanced.x ../bin
	@cp release_script.sh ../bin
	@sed -i "s|tamulauncher-src|bin|" ../bin/system.sh;
	@sed -i "s|tamulauncher-src|bin|" ../bin/tamulauncher


clean:
	rm -f tamulauncher release_cores.sh system.sh tamulauncher-loadbalanced.x  release_script.sh *~

purge: clean
	rm ../bin/tamulauncher ../bin/system.sh ../bin/tamulauncher-loadbalanced.x 
