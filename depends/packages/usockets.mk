package=usockets
$(package)_version=0.8.6
$(package)_download_path=https://github.com/uNetworking/uSockets/archive/refs/tags/
$(package)_file_name=v$($(package)_version).tar.gz
$(package)_sha256_hash=16eba133dd33eade2f5f8dd87612c04b5dd711066e0471c60d641a2f6a988f16

define $(package)_build_cmds
  $(MAKE) default && \
  mkdir -p $($(package)_staging_prefix_dir)/lib/
endef

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir)/include/ && \
  cp src/libusockets.h $($(package)_staging_prefix_dir)/include/ && \
  install uSockets.a $($(package)_staging_prefix_dir)/lib
endef