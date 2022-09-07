package=native_libmultiprocess
$(package)_version=fa130db398ef12752e976468eb12ad36dfd49b99
$(package)_download_path=https://github.com/hebasto/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=c40b6a6202667e48a92d8b400a9e7652e52e70a48ee6aa0231c2838275736546
$(package)_dependencies=native_capnp

define $(package)_config_cmds
  $($(package)_cmake) .
endef

define $(package)_build_cmds
  $(MAKE) mpgen
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install-bin
endef
