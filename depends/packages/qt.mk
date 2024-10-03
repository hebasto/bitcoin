package=qt
$(package)_version=6.7.3
$(package)_download_path=https://download.qt.io/official_releases/qt/6.7/$($(package)_version)/submodules
$(package)_suffix=everywhere-src-$($(package)_version).tar.xz
$(package)_file_name=qtbase-$($(package)_suffix)
$(package)_sha256_hash=8ccbb9ab055205ac76632c9eeddd1ed6fc66936fc56afc2ed0fd5d9e23da3097
$(package)_linux_dependencies=freetype fontconfig libxcb libxkbcommon libxcb_util libxcb_util_cursor libxcb_util_render libxcb_util_keysyms libxcb_util_image libxcb_util_wm
$(package)_patches := dont_hardcode_pwd.patch
$(package)_patches += qtbase-moc-ignore-gcc-macro.patch
$(package)_patches += rcc_hardcode_timestamp.patch
$(package)_patches += guix_cross_lib_path.patch
$(package)_patches += utc_from_string_no_optimize.patch
$(package)_patches += windows_lto.patch
$(package)_patches += macos_skip_version_checks.patch
$(package)_patches += guard_headers_properly.patch
$(package)_patches += qttools_skip_dependencies.patch

$(package)_qttranslations_file_name=qttranslations-$($(package)_suffix)
$(package)_qttranslations_sha256_hash=dcc762acac043b9bb5e4d369b6d6f53e0ecfcf76a408fe0db5f7ef071c9d6dc8

$(package)_qttools_file_name=qttools-$($(package)_suffix)
$(package)_qttools_sha256_hash=f03bb7df619cd9ac9dba110e30b7bcab5dd88eb8bdc9cc752563b4367233203f

$(package)_extra_sources  = $($(package)_qttranslations_file_name)
$(package)_extra_sources += $($(package)_qttools_file_name)

$(package)_top_download_path=https://code.qt.io/cgit/qt/qt5.git/plain
$(package)_top_cmakelists_file_name=CMakeLists.txt
$(package)_top_cmakelists_download_file=$($(package)_top_cmakelists_file_name)?h=$($(package)_version)
$(package)_top_cmakelists_sha256_hash=9fb720a633c0c0a21c31fe62a34bf617726fed72480d4064f29ca5d6973d513f
$(package)_top_cmake_download_path=$($(package)_top_download_path)/cmake
$(package)_top_cmake_ecmoptionaladdsubdirectory_file_name=ECMOptionalAddSubdirectory.cmake
$(package)_top_cmake_ecmoptionaladdsubdirectory_download_file=$($(package)_top_cmake_ecmoptionaladdsubdirectory_file_name)?h=$($(package)_version)
$(package)_top_cmake_ecmoptionaladdsubdirectory_sha256_hash=97ee8bbfcb0a4bdcc6c1af77e467a1da0c5b386c42be2aa97d840247af5f6f70
$(package)_top_cmake_qttoplevelhelpers_file_name=QtTopLevelHelpers.cmake
$(package)_top_cmake_qttoplevelhelpers_download_file=$($(package)_top_cmake_qttoplevelhelpers_file_name)?h=$($(package)_version)
$(package)_top_cmake_qttoplevelhelpers_sha256_hash=5ac2a7159ee27b5b86d26ecff44922e7b8f319aa847b7b5766dc17932fd4a294

$(package)_extra_sources += $($(package)_top_cmakelists_file_name)
$(package)_extra_sources += $($(package)_top_cmake_ecmoptionaladdsubdirectory_file_name)
$(package)_extra_sources += $($(package)_top_cmake_qttoplevelhelpers_file_name)

define $(package)_set_vars
$(package)_config_opts_release = -release
$(package)_config_opts_debug = -debug
$(package)_config_opts_debug += -optimized-tools
$(package)_config_opts += -no-egl
$(package)_config_opts += -no-eglfs
$(package)_config_opts += -no-evdev
$(package)_config_opts += -no-gif
$(package)_config_opts += -no-glib
$(package)_config_opts += -no-icu
$(package)_config_opts += -no-ico
$(package)_config_opts += -no-kms
$(package)_config_opts += -no-linuxfb
$(package)_config_opts += -no-libjpeg
$(package)_config_opts += -no-libproxy
$(package)_config_opts += -no-libudev
$(package)_config_opts += -no-mtdev
$(package)_config_opts += -no-openssl
$(package)_config_opts += -no-openvg
$(package)_config_opts += -no-reduce-relocations
$(package)_config_opts += -no-schannel
$(package)_config_opts += -no-sctp
$(package)_config_opts += -no-securetransport
$(package)_config_opts += -no-system-proxies
$(package)_config_opts += -no-use-gold-linker
$(package)_config_opts += -no-zstd
$(package)_config_opts += -nomake examples
$(package)_config_opts += -nomake tests
$(package)_config_opts += -pkg-config
$(package)_config_opts += -prefix $(host_prefix)
$(package)_config_opts += -qt-doubleconversion
$(package)_config_opts += -qt-libpng
$(package)_config_opts += -qt-pcre
$(package)_config_opts += -qt-harfbuzz
$(package)_config_opts += -qt-zlib
$(package)_config_opts += -static
$(package)_config_opts += -no-feature-backtrace
$(package)_config_opts += -no-feature-colordialog
$(package)_config_opts += -no-feature-concurrent
$(package)_config_opts += -no-feature-dial
$(package)_config_opts += -no-feature-gssapi
$(package)_config_opts += -no-feature-http
$(package)_config_opts += -no-feature-image_heuristic_mask
$(package)_config_opts += -no-feature-keysequenceedit
$(package)_config_opts += -no-feature-lcdnumber
$(package)_config_opts += -no-feature-libresolv
$(package)_config_opts += -no-feature-networkdiskcache
$(package)_config_opts += -no-feature-networkproxy
$(package)_config_opts += -no-feature-printsupport
$(package)_config_opts += -no-feature-sessionmanager
$(package)_config_opts += -no-feature-socks5
$(package)_config_opts += -no-feature-sql
$(package)_config_opts += -no-feature-textmarkdownreader
$(package)_config_opts += -no-feature-textmarkdownwriter
$(package)_config_opts += -no-feature-textodfwriter
$(package)_config_opts += -no-feature-topleveldomain
$(package)_config_opts += -no-feature-udpsocket
$(package)_config_opts += -no-feature-undocommand
$(package)_config_opts += -no-feature-undogroup
$(package)_config_opts += -no-feature-undostack
$(package)_config_opts += -no-feature-undoview
$(package)_config_opts += -no-feature-vnc
# A workaround for https://bugreports.qt.io/browse/QTBUG-99957.
$(package)_config_opts += -no-pch

# Core tools.
$(package)_config_opts += -no-feature-androiddeployqt
$(package)_config_opts += -no-feature-macdeployqt
$(package)_config_opts += -no-feature-windeployqt
$(package)_config_opts += -no-feature-qmake

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

$(package)_config_opts_darwin = -no-dbus
$(package)_config_opts_darwin += -no-opengl
$(package)_config_opts_darwin += -no-freetype
$(package)_config_opts_darwin += QMAKE_MACOSX_DEPLOYMENT_TARGET=$(OSX_MIN_VERSION)

ifneq ($(build_os),darwin)
$(package)_config_opts_darwin += -xplatform macx-clang-linux
$(package)_config_opts_darwin += -device-option MAC_SDK_PATH=$(OSX_SDK)
$(package)_config_opts_darwin += -device-option MAC_SDK_VERSION=$(OSX_SDK_VERSION)
$(package)_config_opts_darwin += -device-option CROSS_COMPILE="llvm-"
$(package)_config_opts_darwin += -device-option MAC_TARGET=$(host)
$(package)_config_opts_darwin += -device-option XCODE_VERSION=$(XCODE_VERSION)
endif

ifneq ($(build_arch),$(host_arch))
$(package)_config_opts_aarch64_darwin += -device-option QMAKE_APPLE_DEVICE_ARCHS=arm64
$(package)_config_opts_x86_64_darwin += -device-option QMAKE_APPLE_DEVICE_ARCHS=x86_64
endif

$(package)_config_opts_linux = -xcb
$(package)_config_opts_linux += -no-xcb-xlib
$(package)_config_opts_linux += -no-feature-xlib
$(package)_config_opts_linux += -no-feature-process
$(package)_config_opts_linux += -system-freetype
$(package)_config_opts_linux += -fontconfig
$(package)_config_opts_linux += -no-opengl
$(package)_config_opts_linux += -no-feature-vulkan
$(package)_config_opts_linux += -dbus-runtime
ifneq ($(LTO),)
$(package)_config_opts_linux += -ltcg
endif

ifneq (,$(findstring clang,$($(package)_cxx)))
  ifneq (,$(findstring -stdlib=libc++,$($(package)_cxx)))
    $(package)_config_opts_linux += -platform linux-clang-libc++ -xplatform linux-clang-libc++
  else
    $(package)_config_opts_linux += -platform linux-clang -xplatform linux-clang
  endif
endif

$(package)_config_opts_mingw32 = -no-opengl
$(package)_config_opts_mingw32 += -no-dbus
$(package)_config_opts_mingw32 += -no-freetype
$(package)_config_opts_mingw32 += -xplatform win32-g++
$(package)_config_opts_mingw32 += -device-option CROSS_COMPILE="$(host)-"
ifneq ($(LTO),)
$(package)_config_opts_mingw32 += -ltcg
endif

$(package)_cmake_opts := -DCMAKE_PREFIX_PATH=$(host_prefix)
$(package)_cmake_opts += -DQT_FEATURE_cxx20=ON
$(package)_cmake_opts += -DQT_ENABLE_CXX_EXTENSIONS=OFF
$(package)_cmake_opts += --log-level=STATUS

$(package)_config_env := CC="$$($(package)_cc)"
$(package)_config_env += CFLAGS="$$($(package)_cppflags) $$($(package)_cflags)"
$(package)_config_env += CXX="$$($(package)_cxx)"
$(package)_config_env += CXXFLAGS="$$($(package)_cppflags) $$($(package)_cxxflags)"
$(package)_config_env += LDFLAGS="$$($(package)_ldflags)"
endef

define $(package)_fetch_cmds
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_download_file),$($(package)_file_name),$($(package)_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_qttranslations_file_name),$($(package)_qttranslations_file_name),$($(package)_qttranslations_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_qttools_file_name),$($(package)_qttools_file_name),$($(package)_qttools_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_top_download_path),$($(package)_top_cmakelists_download_file),$($(package)_top_cmakelists_file_name),$($(package)_top_cmakelists_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_top_cmake_download_path),$($(package)_top_cmake_ecmoptionaladdsubdirectory_download_file),$($(package)_top_cmake_ecmoptionaladdsubdirectory_file_name),$($(package)_top_cmake_ecmoptionaladdsubdirectory_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_top_cmake_download_path),$($(package)_top_cmake_qttoplevelhelpers_download_file),$($(package)_top_cmake_qttoplevelhelpers_file_name),$($(package)_top_cmake_qttoplevelhelpers_sha256_hash))
endef

define $(package)_extract_cmds
  mkdir -p $($(package)_extract_dir) && \
  echo "$($(package)_sha256_hash)  $($(package)_source)" > $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_qttranslations_sha256_hash)  $($(package)_source_dir)/$($(package)_qttranslations_file_name)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_qttools_sha256_hash)  $($(package)_source_dir)/$($(package)_qttools_file_name)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_top_cmakelists_sha256_hash)  $($(package)_source_dir)/$($(package)_top_cmakelists_file_name)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_top_cmake_ecmoptionaladdsubdirectory_sha256_hash)  $($(package)_source_dir)/$($(package)_top_cmake_ecmoptionaladdsubdirectory_file_name)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_top_cmake_qttoplevelhelpers_sha256_hash)  $($(package)_source_dir)/$($(package)_top_cmake_qttoplevelhelpers_file_name)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  $(build_SHA256SUM) -c $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  mkdir qtbase && \
  $(build_TAR) --no-same-owner --strip-components=1 -xf $($(package)_source) -C qtbase && \
  mkdir qttranslations && \
  $(build_TAR) --no-same-owner --strip-components=1 -xf $($(package)_source_dir)/$($(package)_qttranslations_file_name) -C qttranslations && \
  mkdir qttools && \
  $(build_TAR) --no-same-owner --strip-components=1 -xf $($(package)_source_dir)/$($(package)_qttools_file_name) -C qttools && \
  cp --target-directory=. $($(package)_source_dir)/$($(package)_top_cmakelists_file_name) && \
  mkdir cmake && \
  cp --target-directory=./cmake $($(package)_source_dir)/$($(package)_top_cmake_ecmoptionaladdsubdirectory_file_name) && \
  cp --target-directory=./cmake $($(package)_source_dir)/$($(package)_top_cmake_qttoplevelhelpers_file_name)
endef

define $(package)_preprocess_cmds
  patch -p1 -i $($(package)_patch_dir)/dont_hardcode_pwd.patch && \
  patch -p1 -i $($(package)_patch_dir)/qtbase-moc-ignore-gcc-macro.patch && \
  patch -p1 -i $($(package)_patch_dir)/rcc_hardcode_timestamp.patch && \
  patch -p1 -i $($(package)_patch_dir)/utc_from_string_no_optimize.patch && \
  patch -p1 -i $($(package)_patch_dir)/guix_cross_lib_path.patch && \
  patch -p1 -i $($(package)_patch_dir)/windows_lto.patch && \
  patch -p1 -i $($(package)_patch_dir)/macos_skip_version_checks.patch && \
  patch -p1 -i $($(package)_patch_dir)/guard_headers_properly.patch && \
  patch -p1 -i $($(package)_patch_dir)/qttools_skip_dependencies.patch
endef

define $(package)_config_cmds
  cd qtbase && \
  ./configure -top-level $($(package)_config_opts) -- $($(package)_cmake_opts)
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
