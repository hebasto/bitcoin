#include <util/fs.h>
#include <util/fs_helpers.h>
#include <util/translation.h>

#include <iostream>

const TranslateFn G_TRANSLATION_FUN{nullptr};

int main() {
    for (const auto& entry : fs::directory_iterator(fs::current_path())) {
        std::cerr << fs::PathToString(entry.path()) << (IsSymlink(entry.path()) ? " is a SYMLINK" : " is NOT SYMLINK") << "\n";
    }

    return 0;
}
