package=native_libmultiprocess
$(package)_version=d795e969dd7c913c937a4f97d78115072feb6cb4
$(package)_download_path=https://github.com/hebasto/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=280534ef1a4bd82855513ae74afeb251bbdaa1e46b351ae3262c0a5771711248
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
