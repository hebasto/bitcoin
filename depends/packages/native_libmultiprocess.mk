package=native_libmultiprocess
$(package)_version=4e70ad47191f85188da869400690c22f6f81e1e0
$(package)_download_path=https://github.com/hebasto/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=1984697d6937a6e5aeede421fb35e5711ba7063b622ab9e4afe4e0558d1fe5f8
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
