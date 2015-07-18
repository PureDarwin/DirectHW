#
# DirectHW - Kernel extension to pass through IO commands to user space
#
# Copyright © 2008-2010 coresystems GmbH <info@coresystems.de>
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
# 
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

all: dmg

prepare:
	if [ ! -r .patched ]; then \
		test `uname -r  | cut -f1 -d\.` -lt 10 && \
			patch -p0 < DirectHW-i386-only.diff || echo "Not patching."; \
	fi
	touch .patched

build: prepare
	xcodebuild -alltargets

install: build
	sudo xcodebuild -alltargets DSTROOT=`pwd`/build/DirectHW install

package: install
	/Developer/usr/bin/packagemaker -v --doc DirectHW.pmdoc \
		--id com.coresystems.DirectHW --out build/DirectHW.pkg

dmg: package
	rm -rf DirectHW.dmg
	rm -rf out
	mkdir out
	cp -r build/DirectHW.pkg out/Install\ DirectHW.pkg
	cp -r ReadMe.rtf out/Read\ Me.rtf
	/Developer/Tools/SetFile -a E out/Install\ DirectHW.pkg
	/Developer/Tools/SetFile -a E out/Read\ Me.rtf
	../create-dmg/create-dmg --window-size 447 337 \
		--background background.png --icon-size 80 \
		--volname "Install DirectHW" \
		--icon "Install DirectHW.pkg" 142 64 \
		--icon "Read Me.rtf" 310 64 \
		DirectHW.dmg out

load: install
	cd build/DirectHW/System/Library/Extensions; sudo kextunload -v DirectHW.kext; sudo kextload -v DirectHW.kext

installer: package
	rm -rf ~/Desktop/DirectHW.pkg
	cp -r build/DirectHW.pkg ~/Desktop

clean:
	sudo rm -rf build/Release build/DirectHW.build build/DirectHW build/DirectHW.pkg
	rm -rf out

distclean:
	rm DirectHW.dmg

.PHONY: prepare build install package dmg load copy clean distclean

