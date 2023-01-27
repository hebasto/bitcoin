package=native_libmultiprocess
$(package)_version=df4914aeffca85a18121f0e3ddf4aa4b098bdf04
$(package)_download_path=https://github.com/hebasto/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=2cee4ab73186fca6a3dcbe3b00c108591d8a42bfd304af307c6c6db24896f1fb
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
