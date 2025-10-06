# Create the execcutables. The main code is mpi, we don't want
# tamulauncher to be dependent on any external toolchain, so
# have to make sure tamulauncher is fully self contained.
# To do this, we create executables with RPATH set to the 
# correct mpi and GCCLIB libraries
#
# To make tamulauncher completely independent we want to 
# remove the "--enable-new-dtags" flag. In this case 
# LD_LIBRARY_PATH will not have higher precedence. 
#
# We use intel compilers. If you want to use a different compiler,
# the flags and env variables below will need to be adjusted
#

# define MPICXX GCCLIB OMPLIBS CXXFLAGS OPT 
#
# need to hardcode the GCC library path (if not might conflict with system GCC)
grace faster aces launch: GCCLIBS=-Xlinker --disable-new-dtags -Xlinker -rpath -Xlinker $(EBROOTGCCCORE)/lib64 

# need to hardcode the intel omp5 library path since it's not in the default path
grace faster aces launch: OMPLIBS=-Xlinker -rpath -Xlinker $(EBROOTIMKL)/compiler/$(EBVERSIONIMKL)/linux/compiler/lib/intel64_lin
grace faster aces launch: CXXFLAGS=-std=c++0x -qopenmp
grace faster aces launch: OPT=-O3  -g
grace faster aces launch: COMPILER=icpc

SRC=run_command_type.cpp commands_type.cpp tamulauncher-loadbalanced.cpp logger_type.cpp 

default: message

message:
	@echo "... Not building, please specify target: ada / terra / curie";

tamulauncher-loadbalanced.x: $(SRC) 
	@echo "$(COMPILER) $(GCCLIBS) $(OMPLIBS) $(CXXFLAGS) -Iinclude $(OPT)  -o $@ $^"
	@ $(COMPILER) $(GCCLIBS) $(OMPLIBS) $(CXXFLAGS) -Iinclude $(OPT)  -o $@ $^



# GRACE
grace: tamulauncher-loadbalanced.x dograce scripts
dograce:
	cp system.grace.sh system.sh
	cp release_script.slurm.sh release_script.sh


# FASTER
faster: tamulauncher-loadbalanced.x dofaster scripts
dofaster:
	cp system.faster.sh system.sh
	cp release_script.slurm.sh release_script.sh


# ACES
aces: tamulauncher-loadbalanced.x doaces scripts
doaces:
	cp system.aces.sh system.sh
	cp release_script.slurm.sh release_script.sh


#LAUNCH
launch: tamulauncher-loadbalanced.x dolaunch scripts
dolaunch:
	cp system.launch.sh system.sh
	cp release_script.slurm.sh release_script.sh



scripts:
	sed -i "s|<MPIRUN>|`which mpirun`|" system.sh;
	sed -i "s|<TAMULAUNCHERBASE>|`dirname ${PWD}`|" system.sh;
	sed -i "s|<GCCCOREMODULE>|`echo ${EBROOTGCCCORE} | sed 's#/software/easybuild/software/##'`|" system.sh
	sed -i "s|<MPIMODULE>|`echo ${EBROOTIMPI} | sed 's#/software/easybuild/software/##'`|" system.sh
	cp tamulauncher.template tamulauncher
	sed -i "s|<INCLUDE>|`dirname ${PWD}`/src/system.sh|" tamulauncher;
	sed -i "s|<VERSION>|`cat version_string`|" tamulauncher;
	sed -i "s|<TAMULAUNCHERBASE>|`dirname ${PWD}`|" tamulauncher

versionmessage:
	@echo
	@echo
	@echo "WARNING: make sure Version string has been updated correctly. If not, update version string and do make clean <TARGET> install."

setup:
	@mkdir -p ../bin
	@mkdir -p ../log
	@touch ../log/tamulauncher.log
	@chmod uog+w ../log/tamulauncher.log

install: setup 
	@echo "WARNING: make sure target has been built before executing install; do make clean <TARGET> install."
	@cp system.sh ../bin/
	@cp tamulauncher ../bin/
	@cp tamulauncher-loadbalanced.x ../bin
	@cp release_script.sh ../bin
	@sed -i "s|src|bin|" ../bin/system.sh;
	@sed -i "s|src|bin|" ../bin/tamulauncher


clean:
	rm -f tamulauncher release_cores.sh system.sh tamulauncher-loadbalanced.x  release_script.sh *~

purge: clean
	rm ../bin/tamulauncher ../bin/system.sh ../bin/tamulauncher-loadbalanced.x 
