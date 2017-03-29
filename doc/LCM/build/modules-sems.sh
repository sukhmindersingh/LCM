# see:
# Trilinos/cmake/load_ci_sems_dev_env.sh
# Trilinos/cmake/load_sems_dev_env.sh
# Trilinos/cmake/std/sems/get_default_modules.sh
module purge
module load sems-env
module load sems-python/2.7.9
module load sems-cmake/3.5.2
module load sems-git/2.10.1
module load sems-gcc/6.1.0
module load sems-openmpi/1.8.7
module load sems-boost/1.63.0/base
module load sems-zlib/1.2.8/base
module load sems-hdf5/1.8.12/parallel
module load sems-netcdf/4.3.2/parallel
module load sems-parmetis/4.0.3/parallel
module load sems-superlu/4.3/base
module load sems-yaml_cpp/0.5.3/base
