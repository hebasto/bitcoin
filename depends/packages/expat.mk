package := expat
$(package)_version := $(native_$(package)_version)
$(package)_download_path := $(native_$(package)_download_path)
$(package)_file_name := $(native_$(package)_file_name)
$(package)_sha256_hash := $(native_$(package)_sha256_hash)

define $(package)_set_vars
  $(package)_config_opts := --disable-shared --without-docbook --without-tests --without-examples
  $(package)_config_opts += --disable-dependency-tracking --enable-option-checking
  $(package)_config_opts_linux := --with-pic
endef

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmds
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef

define $(package)_postprocess_cmds
  rm -rf share lib/*.la
endef
