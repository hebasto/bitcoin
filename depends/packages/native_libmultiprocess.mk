package=native_libmultiprocess
$(package)_version=e4540729c9d4c2cc31db98571ec700f6fbdb4a27
$(package)_download_path=https://github.com/chaincodelabs/libmultiprocess/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=1f72fe9834b7e26b3f2b1073de9e2e7429e18cb881899c2e8fac012623019bf8
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
