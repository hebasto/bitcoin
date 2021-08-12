package=dummy
$(package)_version=3320100
$(package)_download_path=https://sqlite.org/2020/
$(package)_file_name=sqlite-autoconf-$($(package)_version).tar.gz
$(package)_sha256_hash=486748abfb16abd8af664e3a5f03b228e5f124682b0c942e157644bf6fff7d10

define $(package)_set_vars
  $(package)_config_env+=FOO=BAR
endef

define $(package)_config_cmds
  true && echo 'Attempting to print FOO env var' && printenv FOO && echo "FOO was just printed"
endef
