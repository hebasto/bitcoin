package=uwebsockets
$(package)_version=20.46.0
$(package)_download_path=https://github.com/uNetworking/uWebSockets/archive/refs/tags/
$(package)_file_name=v$($(package)_version).tar.gz
$(package)_sha256_hash=18fa7dfcc33dcc583ad494255591bef53862136a4fde36e06ef07e0887a2a857
$(package)_dependencies=usockets
$(package)_cppflags += -l:uSockets.a

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir)/include/$(package) && \
  cp src/* $($(package)_staging_prefix_dir)/include/$(package)/
endef
