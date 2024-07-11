package=boost
$(package)_version=1.82.0
$(package)_download_path=https://archives.boost.io/release/$($(package)_version)/source/
$(package)_file_name=boost_$(subst .,_,$($(package)_version)).tar.gz
$(package)_sha256_hash=66a469b6e608a51f8347236f4912e27dc5c60c60d7d53ae9bfe4683316c6f04c

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir)/include && \
  cp -r boost $($(package)_staging_prefix_dir)/include && \
  cp -r tools/cmake $($(package)_staging_prefix_dir)
endef
