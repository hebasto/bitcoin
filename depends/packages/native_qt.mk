package := native_qt
$(package)_version := 6.7.2
$(package)_download_path := https://download.qt.io/official_releases/qt/6.7/$($(package)_version)/submodules
$(package)_suffix := everywhere-src-$($(package)_version).tar.xz
$(package)_file_name := qtbase-$($(package)_suffix)
$(package)_sha256_hash := c5f22a5e10fb162895ded7de0963328e7307611c688487b5d152c9ee64767599
$(package)_patches := top_level_CMakeLists.txt
$(package)_patches += top_level_ECMOptionalAddSubdirectory.cmake
$(package)_patches += top_level_QtTopLevelHelpers.cmake
$(package)_patches += mac-qmake.conf
$(package)_patches += dont_hardcode_pwd.patch
$(package)_patches += qtbase-moc-ignore-gcc-macro.patch
$(package)_patches += no_warnings_for_symbols.patch
$(package)_patches += rcc_hardcode_timestamp.patch
$(package)_patches += guix_cross_lib_path.patch
$(package)_patches += utc_from_string_no_optimize.patch
$(package)_patches += windows_lto.patch

$(package)_qttools_file_name := qttools-$($(package)_suffix)
$(package)_qttools_sha256_hash := 58e855ad1b2533094726c8a425766b63a04a0eede2ed85086860e54593aa4b2a
$(package)_extra_sources := $($(package)_qttools_file_name)

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
$(package)_config_opts += -no-feature-testlib

# Network is required for Qt Tools
$(package)_config_opts += -feature-network
$(package)_config_opts += -no-libproxy
$(package)_config_opts += -no-schannel
$(package)_config_opts += -no-sctp
$(package)_config_opts += -no-securetransport
$(package)_config_opts += -no-system-proxies
$(package)_config_opts += -no-feature-gssapi
$(package)_config_opts += -no-feature-http
$(package)_config_opts += -no-feature-libresolv
$(package)_config_opts += -no-feature-networkdiskcache
$(package)_config_opts += -no-feature-networkproxy
$(package)_config_opts += -no-feature-socks5
$(package)_config_opts += -no-feature-topleveldomain
$(package)_config_opts += -no-feature-udpsocket

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
  echo "$($(package)_qttools_sha256_hash)  $($(package)_source_dir)/$($(package)_qttools_file_name)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  $(build_SHA256SUM) -c $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  mkdir qtbase && \
  $(build_TAR) --no-same-owner --strip-components=1 -xf $($(package)_source) -C qtbase && \
  mkdir qttools && \
  $(build_TAR) --no-same-owner --strip-components=1 -xf $($(package)_source_dir)/$($(package)_qttools_file_name) -C qttools
endef

# Preprocessing steps work as follows:
#
# 1. Apply our patches to the extracted source. See each patch for more info.
#
# 2. Create a macOS-Clang-Linux mkspec using our mac-qmake.conf.
#
# 3. After making a copy of the mkspec for the linux-arm-gnueabi host, named
#    bitcoin-linux-g++, replace tool names with $($($(package)_type)_TOOL).
#
# 4. Put our C, CXX and LD FLAGS into gcc-base.conf. Only used for non-host builds.
#
# 5. In clang.conf, swap out clang & clang++, for our compiler + flags. See #17466.
define $(package)_preprocess_cmds
  cp $($(package)_patch_dir)/top_level_CMakeLists.txt CMakeLists.txt && \
  mkdir -p cmake && \
  cp $($(package)_patch_dir)/top_level_ECMOptionalAddSubdirectory.cmake cmake/ECMOptionalAddSubdirectory.cmake && \
  cp $($(package)_patch_dir)/top_level_QtTopLevelHelpers.cmake cmake/QtTopLevelHelpers.cmake && \
  patch -p1 -i $($(package)_patch_dir)/dont_hardcode_pwd.patch && \
  patch -p1 -i $($(package)_patch_dir)/qtbase-moc-ignore-gcc-macro.patch && \
  patch -p1 -i $($(package)_patch_dir)/no_warnings_for_symbols.patch && \
  patch -p1 -i $($(package)_patch_dir)/rcc_hardcode_timestamp.patch && \
  patch -p1 -i $($(package)_patch_dir)/utc_from_string_no_optimize.patch && \
  patch -p1 -i $($(package)_patch_dir)/guix_cross_lib_path.patch && \
  patch -p1 -i $($(package)_patch_dir)/windows_lto.patch && \
  mkdir -p qtbase/mkspecs/macx-clang-linux &&\
  cp -f qtbase/mkspecs/macx-clang/qplatformdefs.h qtbase/mkspecs/macx-clang-linux/ &&\
  cp -f $($(package)_patch_dir)/mac-qmake.conf qtbase/mkspecs/macx-clang-linux/qmake.conf && \
  cp -r qtbase/mkspecs/linux-arm-gnueabi-g++ qtbase/mkspecs/bitcoin-linux-g++ && \
  sed -i.old "s|arm-linux-gnueabi-gcc|$($($(package)_type)_CC)|" qtbase/mkspecs/bitcoin-linux-g++/qmake.conf && \
  sed -i.old "s|arm-linux-gnueabi-g++|$($($(package)_type)_CXX)|" qtbase/mkspecs/bitcoin-linux-g++/qmake.conf && \
  sed -i.old "s|arm-linux-gnueabi-ar|$($($(package)_type)_AR)|" qtbase/mkspecs/bitcoin-linux-g++/qmake.conf && \
  sed -i.old "s|arm-linux-gnueabi-objcopy|$($($(package)_type)_OBJCOPY)|" qtbase/mkspecs/bitcoin-linux-g++/qmake.conf && \
  sed -i.old "s|arm-linux-gnueabi-nm|$($($(package)_type)_NM)|" qtbase/mkspecs/bitcoin-linux-g++/qmake.conf && \
  sed -i.old "s|arm-linux-gnueabi-strip|$($($(package)_type)_STRIP)|" qtbase/mkspecs/bitcoin-linux-g++/qmake.conf && \
  echo "!host_build: QMAKE_CFLAGS     += $($(package)_cflags) $($(package)_cppflags)" >> qtbase/mkspecs/common/gcc-base.conf && \
  echo "!host_build: QMAKE_CXXFLAGS   += $($(package)_cxxflags) $($(package)_cppflags)" >> qtbase/mkspecs/common/gcc-base.conf && \
  echo "!host_build: QMAKE_LFLAGS     += $($(package)_ldflags)" >> qtbase/mkspecs/common/gcc-base.conf && \
  sed -i.old "s|QMAKE_CC                = \$$$$\$$$${CROSS_COMPILE}clang|QMAKE_CC                = $($(package)_cc)|" qtbase/mkspecs/common/clang.conf && \
  sed -i.old "s|QMAKE_CXX               = \$$$$\$$$${CROSS_COMPILE}clang++|QMAKE_CXX               = $($(package)_cxx)|" qtbase/mkspecs/common/clang.conf
endef

define $(package)_config_cmds
  cd qtbase && \
  ./configure -top-level $($(package)_config_opts) -- -DCMAKE_CXX_STANDARD=20 --log-level=STATUS
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
