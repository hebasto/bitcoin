package=native_libmultiprocess
$(package)_version=264eabc9541fd08598b33b8aa2086df0400115f7
$(package)_download_path=https://github.com/hebasto/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=389c80ff9cd26aad002e1cc0b5b98887a2073818ca8d36e991a30d2baba5c050
$(package)_dependencies=native_capnp

define $(package)_config_cmds
  $($(package)_cmake) .
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install-bin
endef
