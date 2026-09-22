cmake_minimum_required (VERSION 3.12)
project (H5114EX C CXX)

#-----------------------------------------------------------------------------
# Basic H5114EX stuff here
#-----------------------------------------------------------------------------
if (NOT H5EXAMPLES_RESOURCES_DIR)
  include (${H5114EX_SOURCE_DIR}/config/cmake/HDFMacros.cmake)
  include (${H5114EX_SOURCE_DIR}/config/cmake/HDFExampleMacros.cmake)
  set (CMAKE_JAVA_INCLUDE_PATH "")

  SET_HDF_BUILD_TYPE()
endif ()

BASIC_SETTINGS (114EX)

#-----------------------------------------------------------------------------
# Define some CMake variables for use later in the project
#-----------------------------------------------------------------------------
set (HDF5EX_C_SRC_DIR          ${H5114EX_SOURCE_DIR}/C)
set (HDF5EX_F90_SRC_DIR        ${H5114EX_SOURCE_DIR}/FORTRAN)
set (HDF5EX_JAVA_DIR           ${H5114EX_SOURCE_DIR}/JAVA)

#-----------------------------------------------------------------------------
# HDF5 support
#-----------------------------------------------------------------------------
HDF5_SUPPORT (TRUE)
message (STATUS "H5112EX link libs: ${LINK_LIBS}")

if (WIN32)
  set(CMAKE_TEST_LIB_DIRECTORY "${HDF5_TOOLS_DIR}")
else ()
  set(CMAKE_TEST_LIB_DIRECTORY "${HDF5_LIBRARY_PATH}")
endif ()

#-----------------------------------------------------------------------------
# Option to use 1.6.x API
#-----------------------------------------------------------------------------
option (${EXAMPLE_VARNAME}_USE_16_API "Use the HDF5 1.6.x API" OFF)

#-----------------------------------------------------------------------------
# Option to use 1.8.x API
#-----------------------------------------------------------------------------
option (${EXAMPLE_VARNAME}_USE_18_API "Use the HDF5 1.8.x API" OFF)

#-----------------------------------------------------------------------------
# Option to use 1.10.x API
#-----------------------------------------------------------------------------
option (${EXAMPLE_VARNAME}_USE_110_API "Use the HDF5 1.10.x API" OFF)

#-----------------------------------------------------------------------------
# Option to use 1.12.x API
#-----------------------------------------------------------------------------
option (${EXAMPLE_VARNAME}_USE_112_API "Use the HDF5 1.12.x API" OFF)

#-----------------------------------------------------------------------------
# Option to use 1.14.x API
#-----------------------------------------------------------------------------
option (${EXAMPLE_VARNAME}_USE_114_API "Use the HDF5 1.14.x API" ON)

#-----------------------------------------------------------------------------
# Option to Enable MPI Parallel
#-----------------------------------------------------------------------------
set (CMAKE_MODULE_PATH ${H5114EX_RESOURCES_DIR} ${CMAKE_MODULE_PATH})
option (HDF_ENABLE_PARALLEL "Enable parallel build (requires MPI)" OFF)
if (HDF_ENABLE_PARALLEL)
  find_package(MPI REQUIRED)
  if (MPI_C_FOUND)
    set (H5_HAVE_PARALLEL 1)
    # MPI checks, only do these if MPI_C_FOUND is true, otherwise they always fail
    # and once set, they are cached as false and not regenerated
    set (CMAKE_REQUIRED_LIBRARIES "${MPI_C_LIBRARIES}" )
    # Used by Fortran + MPI
    include (${CMAKE_ROOT}/Modules/CheckSymbolExists.cmake)
    CHECK_SYMBOL_EXISTS (MPI_Comm_c2f "${MPI_C_INCLUDE_DIRS}/mpi.h"  H5_HAVE_MPI_MULTI_LANG_Comm)
    CHECK_SYMBOL_EXISTS (MPI_Info_c2f "${MPI_C_INCLUDE_DIRS}/mpi.h"  H5_HAVE_MPI_MULTI_LANG_Info)
  else ()
    message (STATUS "Parallel libraries not found")
  endif ()
endif ()

# Parallel IO usage requires MPI to be Linked and Included
if (H5_HAVE_PARALLEL)
  set (LINK_LIBS ${LINK_LIBS} ${MPI_C_LIBRARIES})
  if (MPI_C_LINK_FLAGS)
    set (CMAKE_EXE_LINKER_FLAGS "${MPI_C_LINK_FLAGS} ${CMAKE_EXE_LINKER_FLAGS}")
  endif ()
  INCLUDE_DIRECTORIES (${MPI_C_INCLUDE_DIRS})
endif ()

#-----------------------------------------------------------------------------
# Option to use threadsafe
# Note: Currently CMake only allows configuring of threadsafe on WINDOWS.
#-----------------------------------------------------------------------------
option (HDF_ENABLE_THREADSAFE "Enable Threadsafety" OFF)
if (HDF_ENABLE_THREADSAFE)
  # check for unsupported options
  if (HDF_ENABLE_PARALLEL)
    message (FATAL " **** Parallel and Threadsafe options are mutually exclusive **** ")
  endif ()
  if (WIN32)
    set (H5_HAVE_WIN_THREADS 1)
    set (H5_HAVE_THREADSAFE 1)
  endif ()
  set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
  set(THREADS_PREFER_PTHREAD_FLAG TRUE)
  find_package(Threads REQUIRED)
  if (NOT Threads_FOUND)
    message (STATUS " **** thread-safe package not found - threads still might work **** ")
  endif ()
  set (LINK_LIBS ${LINK_LIBS} Threads::Threads)
endif ()

#-----------------------------------------------------------------------------
# Dashboard and Testing Settings
#-----------------------------------------------------------------------------
option (BUILD_TESTING "Build H5114EX Example Testing" OFF)
if (BUILD_TESTING)
  set (DART_TESTING_TIMEOUT 1200 CACHE STRING
       "Timeout in seconds for each test (default 1200=20minutes)")
  enable_testing ()
  include (CTest)
  include (${H5114EX_SOURCE_DIR}/CTestConfig.cmake)
  configure_file (${${EXAMPLE_PACKAGE_NAME}_RESOURCES_DIR}/CTestCustom.cmake ${PROJECT_BINARY_DIR}/CTestCustom.ctest @ONLY)
endif ()

#-----------------------------------------------------------------------------
# Build examples
#-----------------------------------------------------------------------------
add_subdirectory (C)

#-----------------------------------------------------------------------------
# Option to build Fortran examples
# Make sure this appears before the CONFIGURE_FILE step
# so that fortran name mangling is detected before writing H5pubconf.h
#-----------------------------------------------------------------------------
# Set default name mangling : overridden by Fortran detection in fortran dir
set (H5_FC_FUNC  "H5_FC_FUNC(name,NAME) name ## _")
set (H5_FC_FUNC_ "H5_FC_FUNC_(name,NAME) name ## _")
if (EXISTS "${H5114EX_SOURCE_DIR}/FORTRAN" AND IS_DIRECTORY "${H5114EX_SOURCE_DIR}/FORTRAN")
  option (HDF_BUILD_FORTRAN "Build FORTRAN support" OFF)
  if (HDF_BUILD_FORTRAN AND HDF5_BUILD_FORTRAN)
    include (${${EXAMPLE_PACKAGE_NAME}_RESOURCES_DIR}/HDFUseFortran.cmake)
    include (${${EXAMPLE_PACKAGE_NAME}_RESOURCES_DIR}/HDF5UseFortran.cmake)
    set (LINK_Fortran_LIBS ${LINK_LIBS})

    # Parallel IO usage requires MPI to be Linked and Included
    if (H5_HAVE_PARALLEL)
      set (LINK_Fortran_LIBS ${LINK_Fortran_LIBS} ${MPI_Fortran_LIBRARIES})
      if (MPI_Fortran_LINK_FLAGS)
        set (CMAKE_Fortran_EXE_LINKER_FLAGS "${MPI_Fortran_LINK_FLAGS} ${CMAKE_EXE_LINKER_FLAGS}")
      endif ()
    endif ()

    add_subdirectory (FORTRAN)
  endif ()
endif ()

#-----------------------------------------------------------------------------
# Option to build JAVA examples
#-----------------------------------------------------------------------------
if (EXISTS "${H5114EX_SOURCE_DIR}/JAVA" AND IS_DIRECTORY "${H5114EX_SOURCE_DIR}/JAVA")
  option (HDF_BUILD_JAVA "Build JAVA support" OFF)
  if (HDF_BUILD_JAVA AND HDF5_BUILD_JAVA)
    add_subdirectory (JAVA)
  endif ()
endif ()

#-----------------------------------------------------------------------------
# Option to build filter examples
#-----------------------------------------------------------------------------
if (EXISTS "${H5114EX_SOURCE_DIR}/C/H5Filters" AND IS_DIRECTORY "${H5114EX_SOURCE_DIR}/C/H5Filters")
  option (HDF_BUILD_FILTERS "Test filter support" OFF)
endif ()
