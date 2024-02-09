# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

find_package(PkgConfig REQUIRED)

function(remove_isystem_from_include_directories_internal target)
  #[=[
  A workaround for https://gitlab.kitware.com/cmake/cmake/-/issues/20652.

  When the pkg-config provides CFLAGS with -isystem options, for instance:

    $ pkg-config --cflags-only-I libzmq
    -isystem /usr/include/mit-krb5 -I/usr/include/pgm-5.3 -I/usr/include/libxml2

  an old CMake fails to parse them properly and the INTERFACE_INCLUDE_DIRECTORIES
  property contains "-isystem" as a separated element:

    -isystem;/usr/include/mit-krb5;/usr/include/pgm-5.3;/usr/include/libxml2

  which ends with an error "Imported target includes non-existent path".

  Fixing by removing the "-isystem" element from the INTERFACE_INCLUDE_DIRECTORIES.
  ]=]

  get_target_property(include_directories ${target} INTERFACE_INCLUDE_DIRECTORIES)
  if(include_directories)
    list(REMOVE_ITEM include_directories -isystem)
    set_target_properties(${target} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${include_directories}")
  endif()
endfunction()

macro(cross_pkg_check_modules prefix)
  if(CMAKE_CROSSCOMPILING)
    set(pkg_config_path_saved "$ENV{PKG_CONFIG_PATH}")
    set(pkg_config_libdir_saved "$ENV{PKG_CONFIG_LIBDIR}")
    set(ENV{PKG_CONFIG_PATH} ${PKG_CONFIG_PATH})
    set(ENV{PKG_CONFIG_LIBDIR} ${PKG_CONFIG_LIBDIR})
    pkg_check_modules(${prefix} ${ARGN})
    set(ENV{PKG_CONFIG_PATH} ${pkg_config_path_saved})
    set(ENV{PKG_CONFIG_LIBDIR} ${pkg_config_libdir_saved})
    unset(pkg_config_path_saved)
    unset(pkg_config_libdir_saved)

    if(TARGET PkgConfig::${prefix})
      # TODO: Consider to rework this code after addressing this CMake
      # issue https://gitlab.kitware.com/cmake/cmake/-/issues/21714
      set(${prefix}_private_libraries ${${prefix}_STATIC_LIBRARIES})
      list(REMOVE_ITEM ${prefix}_private_libraries ${${prefix}_LIBRARIES})
      target_link_libraries(PkgConfig::${prefix} INTERFACE ${${prefix}_private_libraries})
      unset(${prefix}_private_libraries)
    endif()
  else()
    pkg_check_modules(${prefix} ${ARGN})
  endif()

  if(CMAKE_VERSION VERSION_LESS 3.17.3 AND TARGET PkgConfig::${prefix})
    remove_isystem_from_include_directories_internal(PkgConfig::${prefix})
  endif()
endmacro()
