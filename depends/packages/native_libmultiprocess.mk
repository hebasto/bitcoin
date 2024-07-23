package=native_libmultiprocess
$(package)_version=c373a94d5b4b3f9c3ee582d15db61d17525bb9bf
$(package)_download_path=https://github.com/hebasto/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=7588e8e729886de43753088b103b5deaed2e57d39e3936ad4af54b12e08b42a1
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
