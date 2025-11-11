qt_details_version := 6.9.3
qt_details_download_path := https://download.qt.io/archive/qt/6.9/$(qt_details_version)/submodules
qt_details_suffix := everywhere-src-$(qt_details_version).tar.xz

qt_details_qtbase_file_name := qtbase-$(qt_details_suffix)
qt_details_qtbase_sha256_hash := c5a1a2f660356ec081febfa782998ae5ddbc5925117e64f50e4be9cd45b8dc6e

qt_details_qttranslations_file_name := qttranslations-$(qt_details_suffix)
qt_details_qttranslations_sha256_hash := f36d545e6681b146fd79b3ebb74ef275e88694cf81eae8323327cae3bfc490a1

qt_details_qttools_file_name := qttools-$(qt_details_suffix)
qt_details_qttools_sha256_hash := 0cf7ab0e975fc57f5ce1375576a0a76e9ede25e6b01db3cf2339cd4d9750b4e9

qt_details_patches_path := $(PATCHES_PATH)/qt

qt_details_top_download_path := https://code.qt.io/cgit/qt/qt5.git/plain
qt_details_top_cmakelists_file_name := CMakeLists.txt
qt_details_top_cmakelists_download_file := $(qt_details_top_cmakelists_file_name)?h=$(qt_details_version)
qt_details_top_cmakelists_sha256_hash := e841d25050cfc7cac691755de225ded5f4ae0e2478ab2f597d1848b161d9dfe5
qt_details_top_cmake_download_path := $(qt_details_top_download_path)/cmake
qt_details_top_cmake_ecmoptionaladdsubdirectory_file_name := ECMOptionalAddSubdirectory.cmake
qt_details_top_cmake_ecmoptionaladdsubdirectory_download_file := $(qt_details_top_cmake_ecmoptionaladdsubdirectory_file_name)?h=$(qt_details_version)
qt_details_top_cmake_ecmoptionaladdsubdirectory_sha256_hash := 97ee8bbfcb0a4bdcc6c1af77e467a1da0c5b386c42be2aa97d840247af5f6f70
qt_details_top_cmake_qttoplevelhelpers_file_name := QtTopLevelHelpers.cmake
qt_details_top_cmake_qttoplevelhelpers_download_file := $(qt_details_top_cmake_qttoplevelhelpers_file_name)?h=$(qt_details_version)
qt_details_top_cmake_qttoplevelhelpers_sha256_hash := e11581b2101a6836ca991817d43d49e1f6016e4e672bbc3523eaa8b3eb3b64c2
