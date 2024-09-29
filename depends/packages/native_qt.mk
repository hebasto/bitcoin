package=native_qt
include packages/qt_details.mk
$(package)_version=$(qt_details_version)
$(package)_download_path=$(qt_details_download_path)
$(package)_file_name=$(qt_details_qtbase_file_name)
$(package)_sha256_hash=$(qt_details_qtbase_sha256_hash)
$(package)_patches := top_level_CMakeLists.txt
$(package)_patches += top_level_ECMOptionalAddSubdirectory.cmake
$(package)_patches += top_level_QtTopLevelHelpers.cmake
$(package)_patches += dont_hardcode_pwd.patch
$(package)_patches += qtbase-moc-ignore-gcc-macro.patch
$(package)_patches += rcc_hardcode_timestamp.patch
$(package)_patches += qttools_skip_dependencies.patch

$(package)_qttranslations_file_name=$(qt_details_qttranslations_file_name)
$(package)_qttranslations_sha256_hash=$(qt_details_qttranslations_sha256_hash)

$(package)_qttools_file_name=$(qt_details_qttools_file_name)
$(package)_qttools_sha256_hash=$(qt_details_qttools_sha256_hash)

$(package)_extra_sources  = $($(package)_qttranslations_file_name)
$(package)_extra_sources += $($(package)_qttools_file_name)

define $(package)_set_vars
# Build options:
$(package)_config_opts += -release
$(package)_config_opts += -make tools
$(package)_config_opts += -static
$(package)_config_opts += -prefix $(host_prefix)
$(package)_config_opts += -no-reduce-relocations
$(package)_config_opts += -no-use-gold-linker
$(package)_config_opts += -pkg-config
# Modules:
$(package)_config_opts += -no-feature-concurrent
$(package)_config_opts += -no-gui
$(package)_config_opts += -no-feature-network
$(package)_config_opts += -no-feature-sql
$(package)_config_opts += -no-feature-testlib

$(package)_config_opts += -no-glib
$(package)_config_opts += -no-icu
$(package)_config_opts += -no-libudev
$(package)_config_opts += -no-mimetype-database
$(package)_config_opts += -no-openssl
$(package)_config_opts += -no-zstd
$(package)_config_opts += -qt-pcre
$(package)_config_opts += -qt-zlib
$(package)_config_opts += -no-feature-backtrace
$(package)_config_opts += -no-feature-permissions
$(package)_config_opts += -no-feature-process
$(package)_config_opts += -no-feature-settings

# Qt Tools module.
$(package)_config_opts += -feature-linguist
$(package)_config_opts += -no-feature-assistant
$(package)_config_opts += -no-feature-clang
$(package)_config_opts += -no-feature-qdoc
$(package)_config_opts += -no-feature-clangcpp
$(package)_config_opts += -no-feature-designer
$(package)_config_opts += -no-feature-pixeltool
$(package)_config_opts += -no-feature-qtattributionsscanner
$(package)_config_opts += -no-feature-qtdiag
$(package)_config_opts += -no-feature-qtplugininfo

endef

define $(package)_fetch_cmds
  $(call fetch_file,$(package),$($(package)_download_path),$($(package)_download_file),$($(package)_file_name),$($(package)_sha256_hash)) && \
  $(call fetch_file,$(package),$($(package)_download_path),$($(package)_qttools_file_name),$($(package)_qttools_file_name),$($(package)_qttools_sha256_hash))
endef


define $(package)_extract_cmds
  mkdir -p $($(package)_extract_dir) && \
  echo "$($(package)_sha256_hash)  $($(package)_source)" > $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_qttranslations_sha256_hash)  $($(package)_source_dir)/$($(package)_qttranslations_file_name)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_qttools_sha256_hash)  $($(package)_source_dir)/$($(package)_qttools_file_name)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  $(build_SHA256SUM) -c $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  mkdir qtbase && \
  $(build_TAR) --no-same-owner --strip-components=1 -xf $($(package)_source) -C qtbase && \
  mkdir qttranslations && \
  $(build_TAR) --no-same-owner --strip-components=1 -xf $($(package)_source_dir)/$($(package)_qttranslations_file_name) -C qttranslations && \
  mkdir qttools && \
  $(build_TAR) --no-same-owner --strip-components=1 -xf $($(package)_source_dir)/$($(package)_qttools_file_name) -C qttools
endef

define $(package)_preprocess_cmds
  cp $($(package)_patch_dir)/top_level_CMakeLists.txt CMakeLists.txt && \
  mkdir -p cmake && \
  cp $($(package)_patch_dir)/top_level_ECMOptionalAddSubdirectory.cmake cmake/ECMOptionalAddSubdirectory.cmake && \
  cp $($(package)_patch_dir)/top_level_QtTopLevelHelpers.cmake cmake/QtTopLevelHelpers.cmake && \
  patch -p1 -i $($(package)_patch_dir)/dont_hardcode_pwd.patch && \
  patch -p1 -i $($(package)_patch_dir)/qtbase-moc-ignore-gcc-macro.patch && \
  patch -p1 -i $($(package)_patch_dir)/rcc_hardcode_timestamp.patch && \
  patch -p1 -i $($(package)_patch_dir)/qttools_skip_dependencies.patch
endef

define $(package)_config_cmds
  cd qtbase && \
  ./configure -top-level $($(package)_config_opts) -- --log-level=STATUS
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
