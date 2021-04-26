package=native_llvm
$(package)_version=14.0.0
$(package)_download_path=https://github.com/llvm/llvm-project/releases/download/llvmorg-$($(package)_version)
ifneq (,$(findstring aarch64,$(BUILD)))
$(package)_file_name=clang+llvm-$($(package)_version)-aarch64-linux-gnu.tar.xz
$(package)_sha256_hash=1792badcd44066c79148ffeb1746058422cc9d838462be07e3cb19a4b724a1ee
else
$(package)_file_name=clang+llvm-$($(package)_version)-x86_64-linux-gnu-ubuntu-18.04.tar.xz
$(package)_sha256_hash=61582215dafafb7b576ea30cc136be92c877ba1f1c31ddbbd372d6d65622fef5
endif

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir)/lib/clang/$($(package)_version)/include && \
  cp -r lib/clang/$($(package)_version)/include/* $($(package)_staging_prefix_dir)/lib/clang/$($(package)_version)/include/ && \
  mkdir -p $($(package)_staging_prefix_dir)/bin && \
  cp bin/clang $($(package)_staging_prefix_dir)/bin/ && \
  cp -P bin/clang++ $($(package)_staging_prefix_dir)/bin/ && \
  cp bin/dsymutil $($(package)_staging_prefix_dir)/bin/$(host)-dsymutil && \
  cp bin/lld $($(package)_staging_prefix_dir)/bin/$(host)-ld && \
  cp bin/llvm-ar $($(package)_staging_prefix_dir)/bin/$(host)-ar && \
  cp bin/llvm-config $($(package)_staging_prefix_dir)/bin/llvm-config && \
  cp bin/llvm-install-name-tool $($(package)_staging_prefix_dir)/bin/$(host)-install_name_tool && \
  cp bin/llvm-libtool-darwin $($(package)_staging_prefix_dir)/bin/$(host)-libtool && \
  cp bin/llvm-nm $($(package)_staging_prefix_dir)/bin/$(host)-nm && \
  cp bin/llvm-otool $($(package)_staging_prefix_dir)/bin/$(host)-otool && \
  cp bin/llvm-ranlib $($(package)_staging_prefix_dir)/bin/$(host)-ranlib && \
  cp bin/llvm-strip $($(package)_staging_prefix_dir)/bin/$(host)-strip && \
  cp lib/libLTO.so $($(package)_staging_prefix_dir)/lib/
endef
