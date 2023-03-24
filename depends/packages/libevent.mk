package=libevent
$(package)_version=c6e8f17541b99e8c3a089a1c6f70119d6f95db9d
$(package)_download_path=https://github.com/libevent/libevent/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=bf71f60b46aa2facfde16a4413af01e2f2ece119428e1edb29633acafdf01379

# When building for Windows, we set _WIN32_WINNT to target the same Windows
# version as we do in configure. Due to quirks in libevents build system, this
# is also required to enable support for ipv6. See #19375.
define $(package)_set_vars
  $(package)_config_opts=--disable-shared --disable-openssl --disable-libevent-regress --disable-samples
  $(package)_config_opts += --disable-dependency-tracking --enable-option-checking
  $(package)_config_opts_release=--disable-debug-mode
  $(package)_cppflags_mingw32=-D_WIN32_WINNT=0x0601

  ifeq ($(NO_HARDEN),)
  $(package)_cppflags+=-D_FORTIFY_SOURCE=3
  endif
endef

define $(package)_config_cmds
  ./autogen.sh && \
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

define $(package)_postprocess_cmds
  rm lib/*.la && \
  rm include/ev*.h && \
  rm include/event2/*_compat.h
endef
