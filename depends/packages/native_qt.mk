package=native_qt
$(package)_version=6.7.2
$(package)_download_path=https://download.qt.io/official_releases/qt/6.7/$($(package)_version)/submodules
$(package)_file_name=qtbase-everywhere-src-$($(package)_version).tar.xz
$(package)_sha256_hash=c5f22a5e10fb162895ded7de0963328e7307611c688487b5d152c9ee64767599
$(package)_patches += dont_hardcode_pwd.patch
$(package)_patches += qtbase-moc-ignore-gcc-macro.patch
$(package)_build_subdir=build

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
$(package)_config_opts += -no-feature-sql
$(package)_config_opts += -no-feature-xml

$(package)_config_opts += -no-feature-permissions
$(package)_config_opts += -no-feature-process
$(package)_config_opts += -no-feature-settings

ifneq (,$(findstring clang,$($(package)_cxx)))
  ifneq (,$(findstring -stdlib=libc++,$($(package)_cxx)))
    $(package)_config_opts += -platform linux-clang-libc++
  else
    $(package)_config_opts += -platform linux-clang
  endif
else
  $(package)_config_opts += -platform linux-g++
endif

endef

# Preprocessing steps work as follows:
#
# 1. Apply our patches to the extracted source. See each patch for more info.
#
# 4. Put our C, CXX and LD FLAGS into gcc-base.conf. Only used for non-host builds.
#
# 5. In clang.conf, swap out clang & clang++, for our compiler + flags. See #17466.
define $(package)_preprocess_cmds
  patch -p1 -i $($(package)_patch_dir)/dont_hardcode_pwd.patch && \
  patch -p1 -i $($(package)_patch_dir)/qtbase-moc-ignore-gcc-macro.patch
endef

define $(package)_config_cmds
  ../configure $($(package)_config_opts)
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
