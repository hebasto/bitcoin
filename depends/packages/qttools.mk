package=qttools
$(package)_version=$(qt_version)
$(package)_download_path=$(qt_download_path)
$(package)_file_name=$(package)-everywhere-src-$($(package)_version).tar.xz
$(package)_sha256_hash=58e855ad1b2533094726c8a425766b63a04a0eede2ed85086860e54593aa4b2a
$(package)_dependencies=qt
$(package)_build_subdir=build

define $(package)_set_vars
$(package)_config_opts += -qt-host-path $(build_prefix)
$(package)_config_opts += -no-feature-assistant
$(package)_config_opts += -no-feature-clang
$(package)_config_opts += -no-feature-qdoc
$(package)_config_opts += -no-feature-clangcpp
$(package)_config_opts += -no-feature-designer
$(package)_config_opts += -no-feature-pixeltool
$(package)_config_opts += -no-feature-qdbus
$(package)_config_opts += -no-feature-qtattributionsscanner
$(package)_config_opts += -no-feature-qtdiag
$(package)_config_opts += -no-feature-qtplugininfo
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

define $(package)_postprocess_cmds
  rm -rf doc/
endef
