package=native_libmultiprocess
$(package)_version=cfadeb6bc068e167ae7539e14548f6d3bed94678
$(package)_download_path=https://github.com/hebasto/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=d258eeec00de9b8b62895ff7409dce462dadd1d5b14c00d716c761a680dd5bb4
$(package)_dependencies=native_capnp

define $(package)_config_cmds
  $($(package)_cmake) -B build
endef

define $(package)_build_cmds
  $(MAKE) -C build mpgen
endef

define $(package)_stage_cmds
  cmake --install build --prefix $($(package)_staging_prefix_dir) --component bin
endef
