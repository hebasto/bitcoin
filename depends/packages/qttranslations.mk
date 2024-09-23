package=qttranslations
$(package)_version=$(qt_version)
$(package)_download_path=$(qt_download_path)
$(package)_file_name=$(package)-everywhere-src-$($(package)_version).tar.xz
$(package)_sha256_hash=9845780b5dc1b7279d57836db51aeaf2e4a1160c42be09750616f39157582ca9
$(package)_dependencies=qt qttools
$(package)_build_subdir=$(package)/build

define $(package)_set_vars
$(package)_config_opts += -qt-host-path $(build_prefix)
endef

define $(package)_extract_cmds
  mkdir -p $($(package)_extract_dir) && \
  echo "$($(package)_sha256_hash)  $($(package)_source)" > $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  $(build_SHA256SUM) -c $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  mkdir -p $(package) && \
  $(build_TAR) --no-same-owner --strip-components=1 -xf $($(package)_source) -C $(package) && \
  mkdir -p qtbase && \
  $(build_TAR) --no-same-owner --strip-components=1 -xf $(qt_source) -C qtbase
endef

define $(package)_config_cmds
  $(host_prefix)/bin/qt-configure-module .. $($(package)_config_opts) -- -DCMAKE_CXX_STANDARD=20 --log-level=STATUS
endef

define $(package)_build_cmds
  cmake --build . --parallel
endef

define $(package)_stage_cmds
  cmake --install . --prefix $($(package)_staging_prefix_dir)
endef
