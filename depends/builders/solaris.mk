build_solaris_SHA256SUM = shasum
build_solaris_DOWNLOAD = curl --location --fail --connect-timeout $(DOWNLOAD_CONNECT_TIMEOUT) --retry $(DOWNLOAD_RETRIES) -o

build_solaris_TAR = gtar
# On illumos-based distros, touch doesn't understand -h
build_solaris_TOUCH = touch -m -t 200001011200
