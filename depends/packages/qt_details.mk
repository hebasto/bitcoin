qt_details_version := 6.8.0
qt_details_download_path := https://download.qt.io/official_releases/qt/6.8/$(qt_details_version)/submodules
qt_details_suffix := everywhere-src-$(qt_details_version).tar.xz

qt_details_qtbase_file_name := qtbase-$(qt_details_suffix)
qt_details_qtbase_sha256_hash := 1bad481710aa27f872de6c9f72651f89a6107f0077003d0ebfcc9fd15cba3c75

qt_details_qttranslations_file_name := qttranslations-$(qt_details_suffix)
qt_details_qttranslations_sha256_hash := 84bf2b67c243cd0c50a08acd7bfa9df2b1965028511815c1b6b65a0687437cb6

qt_details_qttools_file_name := qttools-$(qt_details_suffix)
qt_details_qttools_sha256_hash := 403115d8268503c6cc6e43310c8ae28eb9e605072a5d04e4a2de8b6af39981f7

qt_details_patches_path := $(PATCHES_PATH)/qt

qt_details_top_download_path := https://code.qt.io/cgit/qt/qt5.git/plain
qt_details_top_cmakelists_file_name := CMakeLists.txt
qt_details_top_cmakelists_download_file := $(qt_details_top_cmakelists_file_name)?h=$(qt_details_version)
qt_details_top_cmakelists_sha256_hash := 54e9a4e554da37792446dda4f52bc308407b01a34bcc3afbad58e4e0f71fac9b
qt_details_top_cmake_download_path := $(qt_details_top_download_path)/cmake
qt_details_top_cmake_ecmoptionaladdsubdirectory_file_name := ECMOptionalAddSubdirectory.cmake
qt_details_top_cmake_ecmoptionaladdsubdirectory_download_file := $(qt_details_top_cmake_ecmoptionaladdsubdirectory_file_name)?h=$(qt_details_version)
qt_details_top_cmake_ecmoptionaladdsubdirectory_sha256_hash := 97ee8bbfcb0a4bdcc6c1af77e467a1da0c5b386c42be2aa97d840247af5f6f70
qt_details_top_cmake_qttoplevelhelpers_file_name := QtTopLevelHelpers.cmake
qt_details_top_cmake_qttoplevelhelpers_download_file := $(qt_details_top_cmake_qttoplevelhelpers_file_name)?h=$(qt_details_version)
qt_details_top_cmake_qttoplevelhelpers_sha256_hash := bf90ef349f39f285ba761f1c9f5d6511f8c14ede9654ce51fcdea3a937770541
