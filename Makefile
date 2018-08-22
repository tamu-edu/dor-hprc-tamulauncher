# Will use the currently loaded intel mpi c++ wrapper.
# To make tamulauncher completely independent we want to 
# remove the "--enable-new-dtags" flag. In this case 
# LD_LIBRARY_PATH will not have higher precedence. 


# define MPICXX GCCLIB OMPLIBS CXXFLAGS OPT for ada and terra 
#ada terra: MPICXX=`mpiicpc -show | sed -r 's/-Xlinker --enable-new-dtags//'`
# need to hardcode the GCC library path (if not might conflict with system GCC)
ada terra: GCCLIBS=-Xlinker --disable-new-dtags -Xlinker -rpath -Xlinker $(EBROOTGCCCORE)/lib64 
# need to hardcode the intel omp5 library path since it's not in the default path
ada terra: OMPLIBS=-Xlinker -rpath -Xlinker $(EBROOTIMKL)/lib/intel64 
ada terra: CXXFLAGS=-std=c++0x -qopenmp
ada terra: OPT=-O3  -g
ada terra: PERNODE=-perhost
ada terra: COMPILER=icpc

#DEFINE MPICXX GCCLIB OMPLIBS CXXFLAGS OPT for curie
curie: MPICXX=`mpic++ --show | sed -r 's/-Wl,--enable-new-dtags//'`
curie: GCCLIBS=-Wl,--disable-new-dtags -Wl,-rpath -Wl,$(EBROOTGCCCORE)/lib64
curie: OMPLIBS=
curie: CXXFLAGS=-std=c++14 -fopenmp
curie: OPT=-O2 -g
curie: PERNODE=-npernode
curie: COMPILER=g++

SRC=run_command_type.cpp commands_type.cpp tamulauncher-loadbalanced.cpp logger_type.cpp 


default: message

message:
	@echo "... Not building, please specify target: ada / terra / curie";

tamulauncher-loadbalanced.x: $(SRC) 
	@echo "$(COMPILER) $(GCCLIBS) $(OMPLIBS) $(CXXFLAGS) -Iinclude $(OPT)  -o $@ $^"
	@ $(COMPILER) $(GCCLIBS) $(OMPLIBS) $(CXXFLAGS) -Iinclude $(OPT)  -o $@ $^

doada:
	cp system.ada system.sh;

ada: tamulauncher-loadbalanced.x doada scripts

doterra:
	cp system.terra system.sh

terra: tamulauncher-loadbalanced.x doterra scripts


docurie:
	cp system.curie system.sh

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
	sed -i "s|<PERNODE>|${PERNODE}|" tamulauncher;
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
	@sed -i "s|tamulauncher-src|bin|" ../bin/system.sh;
	@sed -i "s|tamulauncher-src|bin|" ../bin/tamulauncher


clean:
	rm -f tamulauncher system.sh tamulauncher-loadbalanced.x *~

purge: clean
	rm ../bin/tamulauncher ../bin/system.sh ../bin/tamulauncher-loadbalanced.x 
