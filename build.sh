# only intended to run on linux
# options (combine in any order):
# pc - compile for linux (default)
# win - compile for windows
# android - compile for android
# psp - compile for psp
# switch - compile for switch
# wiiu - compile for wii u
# ps4 - compile for ps4
# buildsoloud - force rebuild soloud even if it's up to date
# buildlua - force rebuild lua even if it's up to date
# debug - compile for debugging (has its own build directory)
# clean - remove object files in the selected build directory
# link - link even if output seems to be up to date
# test - run after building (for windows, uses wine, for android, connects to an android device with ADB)
# verbose - print every compiler command run

args=$@
echoprefix=""

stdoutwithprefix () {
	while read -r line; do
		if [[ $echoprefix != "" ]]; then
			echo -ne "\033[0m$echoprefix | "
		fi
		echo $line
	done
}

stderrwithprefix () {
	while read -r line; do
		if [[ $echoprefix != "" ]]; then
			>&2 echo -ne "\033[0m$echoprefix | "
		fi
		>&2 echo $line
	done
}

#( clang++ src/fs.cpp | stdoutwithprefix ) 2>&1 >/dev/null | stderrwithprefix
#exit

echowithprefix () {
	if [[ $echoprefix != "" ]]; then
		echo -n "$echoprefix | "
	fi
	echo $@
}

has_arg () {
	for arg in $args; do
		if [[ $arg == "--" ]]; then
			return $(false)
		elif [[ $arg == $1 ]]; then
			return $(true)
		fi
	done
	return $(false)
}

runargs=
capturing=0
for arg in $args; do
	if [[ $capturing == 1 ]]; then
		runargs+=" $arg"
	elif [[ $arg == "--" ]]; then
		capturing=1
	fi
done
if [[ -n "$runargs" ]]; then
	echo passing args: $runargs
fi

buildffmpeg () {
	if has_arg debug; then
		if [[ "$builddir" != *debug ]]; then
			builddir+="debug"
			mkdir -p $builddir
		fi
	fi
	if [[ $ccompiler == "placeholder" ]]; then
		ccompiler=$compiler
	fi
	if [[ $linker == "placeholder" ]]; then
		linker=$compiler
	fi
	cd ext/src
	if [[ ! -e FFmpeg ]]; then
		git clone https://github.com/FFmpeg/FFmpeg || exit 1
	fi
	cd FFmpeg
	if has_arg buildffmpeg; then
		ffmpeg_dirty=1
	else
		if [[ ! -f ../../../$builddir/libavcodec.a ]]; then
			ffmpeg_dirty=1
		else
			ffmpeg_dirty=
		fi
	fi
	if [[ $ffmpeg_dirty ]]; then
		echo building ffmpeg for $libdir
		if has_arg debug; then
			CFLAGS="-Og -g -s" CXXFLAGS="-Og -g -s" ./configure --disable-network --disable-postproc --disable-faan --disable-fft --disable-rdft --disable-mdct --disable-lsp --disable-dwt --disable-dct --disable-programs --disable-debug \
			--disable-safe-bitstream-reader --enable-hardcoded-tables --enable-small --disable-lto \
			--disable-decoders --disable-encoders --disable-muxers --disable-bsfs --disable-devices --disable-filters \
			--enable-decoder=wmv3 --enable-decoder=wmav2 \
			--extra-ldflags="-static -s" $@ --cc="$ccompiler" --cxx="$compiler" --ld="$linker" || exit 1
		else
			#./configure --disable-everything --enable-decoder=h264_decoder --enable-decoder=aac_decoder --enable-decoder=wmv3 --enable-decoder=wmav2 --enable-lto --disable-programs --extra-ldflags=-static $@ --cc="$ccompiler" --cxx="$compiler" --ld="$linker" || exit 1
			CFLAGS="-Os -G0 -s" CXXFLAGS="-Os -G0 -s" ./configure --disable-network --disable-postproc --disable-faan --disable-fft --disable-rdft --disable-mdct --disable-lsp --disable-dwt --disable-dct --disable-programs --disable-debug \
			--disable-safe-bitstream-reader --enable-hardcoded-tables --enable-small --disable-lto \
			--disable-decoders --disable-encoders --disable-muxers --disable-bsfs --disable-devices --disable-filters \
			--enable-decoder=wmv3 --enable-decoder=wmav2 \
			--extra-ldflags="-static -s" $@ --cc="$ccompiler" --cxx="$compiler" --ld="$linker" || exit 1
		fi
		make clean
		make -j$jobs || exit 1
		cp libavcodec/libavcodec.a ../../../$builddir/
		mkdir -p ../../../$builddir/libavcodec && cp libavcodec/*.h ../../../$builddir/libavcodec
		cp libavformat/libavformat.a ../../../$builddir/
		mkdir -p ../../../$builddir/libavformat && cp libavformat/*.h ../../../$builddir/libavformat
		cp libavfilter/libavfilter.a ../../../$builddir/
		mkdir -p ../../../$builddir/libavfilter && cp libavfilter/*.h ../../../$builddir/libavfilter
		cp libavdevice/libavdevice.a ../../../$builddir/
		mkdir -p ../../../$builddir/libavdevice && cp libavdevice/*.h ../../../$builddir/libavdevice
		cp libswresample/libswresample.a ../../../$builddir/
		mkdir -p ../../../$builddir/libswresample && cp libswresample/*.h ../../../$builddir/libswresample
		cp libswscale/libswscale.a ../../../$builddir/
		mkdir -p ../../../$builddir/libswscale && cp libswscale/*.h ../../../$builddir/libswscale
		cp libavutil/libavutil.a ../../../$builddir/
		mkdir -p ../../../$builddir/libavutil && cp libavutil/*.h ../../../$builddir/libavutil
	fi
	cd ../../..
}

buildflac () {
	if has_arg debug; then
		if [[ "$builddir" != *debug ]]; then
			builddir+="debug"
			mkdir -p $builddir
		fi
	fi
	if [[ $ccompiler == "placeholder" ]]; then
		ccompiler=$compiler
	fi
	cd ext/src
	if [[ ! -e flac ]]; then
		git clone https://github.com/xiph/flac || exit 1
	fi
	cd flac
	flacdst=../../../$builddir/libFLAC.a
	if has_arg buildflac; then
		flac_dirty=1
	else
		flac_dirty=
		for src in src/*; do
			if [[ $src -nt $flacdst || ! -f $flacdst ]]; then
				flac_dirty=1
				break
			fi
		done
	fi
	if [[ $flac_dirty ]]; then
		echo building flac for $libdir
		rm -rf build
		mkdir -p build
		cd build
		cmake -DCMAKE_BUILD_TYPE=RELEASE -DENABLE_STATIC_RUNTIME=ON -DBUILD_TESTING=OFF -DBUILD_PROGRAMS=OFF -DBUILD_EXAMPLES=OFF -DINSTALL_MANPAGES=OFF $@ .. || exit 1
		make -j$jobs || exit 1
		mv src/libFLAC/libFLAC.a ../$flacdst || exit 1
		cp -r ../include/libFLAC ../../../../$builddir/
		cd ..
	fi
	cd ../../..
}

buildopus () {
	if has_arg debug; then
		if [[ "$builddir" != *debug ]]; then
			builddir+="debug"
			mkdir -p $builddir
		fi
	fi
	if [[ $ccompiler == "placeholder" ]]; then
		ccompiler=$compiler
	fi
	cd ext/src
	if [[ ! -e opus ]]; then
		git clone https://github.com/xiph/opus || exit 1
	fi
	cd opus
	opusdst=../../../$builddir/libopus.a
	if has_arg buildflac; then
		opus_dirty=1
	else
		opus_dirty=
		for src in src/*; do
			if [[ $src -nt $opusdst || ! -f $opusdst ]]; then
				opus_dirty=1
				break
			fi
		done
	fi
	if [[ $opus_dirty ]]; then
		echo building opus for $libdir
		rm -rf build
		mkdir -p build
		cd build
		cmake -DCMAKE_BUILD_TYPE=RELEASE -DENABLE_STATIC_RUNTIME=ON -DBUILD_TESTING=OFF -DBUILD_PROGRAMS=OFF -DBUILD_EXAMPLES=OFF -DINSTALL_MANPAGES=OFF $@ .. || exit 1
		make -j$jobs || exit 1
		mv libopus.a ../$opusdst || exit 1
		cp ../include/*.h ../../../../$builddir/
		cd ..
	fi
	cd ../../..
}

buildlibsndfile () {
	if has_arg debug; then
		if [[ "$builddir" != *debug ]]; then
			builddir+="debug"
			mkdir -p $builddir
		fi
	fi
	if [[ $ccompiler == "placeholder" ]]; then
		ccompiler=$compiler
	fi
	cd ext/src
	if [[ ! -e libsndfile ]]; then
		git clone https://github.com/libsndfile/libsndfile || exit 1
	fi
	cd libsndfile
	libsndfiledst=../../../$builddir/libsndfile.a
	if has_arg buildlibsndfile; then
		libsndfile_dirty=1
	else
		libsndfile_dirty=
		for src in src/*; do
			if [[ $src -nt $libsndfiledst || ! -f $libsndfiledst ]]; then
				libsndfile_dirty=1
				break
			fi
		done
	fi
	if [[ $libsndfile_dirty ]]; then
		echo building libsndfile for $libdir
		rm -rf build
		mkdir -p build
		cd build
		cmake -DCMAKE_BUILD_TYPE=RELEASE -DENABLE_STATIC_RUNTIME=ON -DBUILD_TESTING=OFF -DBUILD_PROGRAMS=OFF -DBUILD_EXAMPLES=OFF $@ .. || exit 1
		make -j$jobs || exit 1
		mv libsndfile.a ../$libsndfiledst || exit 1
		cp ../include/* ../../../../$builddir/
		cd ..
	fi
	cd ../../..
}

buildsoloud () {
	if has_arg debug; then
		if [[ "$builddir" != *debug ]]; then
			builddir+="debug"
			mkdir -p $builddir
		fi
	fi
	cd ext/src/soloud
	solouddst=../../../$builddir/libsoloud_static.a
	if has_arg buildsoloud; then
		soloud_dirty=1
	else
		soloud_dirty=
		for src in src/backend/*/*; do
			if [[ $src -nt $solouddst || ! -f $solouddst ]]; then
				soloud_dirty=1
				break
			fi
		done
	fi
	if [[ $soloud_dirty ]]; then
		echo building soloud for $libdir
		cd build
		./genie/bin/linux/genie $@ gmake || exit 1
		cd gmake
		make clean
		make -j$jobs config=release SoloudStatic || exit 1
		if [[ $platform == "win" ]]; then
			mv ../../lib/libsoloud_static_x64.a ../../$solouddst || exit 1
		else
			mv ../../lib/libsoloud_static.a ../../$solouddst || exit 1
		fi
		cd ../..
	fi
	cd ../../..
}

buildluajit () {
	if has_arg debug; then
		if [[ "$builddir" != *debug ]]; then
			builddir+="debug"
			mkdir -p $builddir
		fi
	fi
	if has_arg clean; then
		return
	fi
	cd ext/src
	if [[ ! -e LuaJIT ]]; then
		git clone https://github.com/LuaJIT/LuaJIT || exit 1
	fi
	cd LuaJIT
	luainclude=../../../$builddir/luajitinclude/luajit-2.1
	luadst=../../../$builddir/libluajit.a
	mkdir -p $luainclude
	if has_arg buildlua; then
		lua_dirty=1
	else
		lua_dirty=
		for src in src/*; do
			if [[ $src != src/lj_*def.h && ($src == *.c || $src == *.h) && ($src -nt $luadst || ! -f $luadst) ]]; then
				echo $src is dirty
				lua_dirty=1
				break
			fi
		done
	fi
	if [[ $lua_dirty ]]; then
		echo building luajit for $libdir
		make clean
		make $@ -j$jobs || exit 1
		cp src/*.h src/*.hpp $luainclude
		rm -f $luadst
		if [[ -f src/libluajit.a ]]; then
			mv src/libluajit.a $luadst
		fi
		if [[ -f src/libluajit-5.1.dll.a ]]; then
			mv src/libluajit-5.1.dll.a $luadst
			mv src/lua51.dll ../../../$builddir
		fi
		touch $luadst
		rm -f src/luajit.exe
		#else
		#	rm *.o
		#	make || exit 1
		#	cp src/*.h src/*.hpp $luainclude
		#	rm -f $luadst
		#	mv src/liblua.a $luadst
		#fi
	fi
	cd ../../..
	luainclude=$builddir/luajitinclude
}

buildlibgit2 () {
	if has_arg debug; then
		if [[ "$builddir" != *debug ]]; then
			builddir+="debug"
			mkdir -p $builddir
		fi
	fi
	if [[ $ccompiler == "placeholder" ]]; then
		ccompiler=$compiler
	fi
	cd ext/src
	if [[ ! -e libgit2 ]]; then
		git clone https://github.com/libgit2/libgit2.git || exit 1
	fi
	cd libgit2
	libgit2dst=../../../$builddir/libgit2.a
	if has_arg buildlibgit2; then
		libgit2_dirty=1
	else
		libgit2_dirty=
		for src in src/*; do
			if [[ $src -nt $libgit2dst || ! -f $libgit2dst ]]; then
				libgit2_dirty=1
				break
			fi
		done
	fi
	if [[ $libgit2_dirty ]]; then
		echo building libgit2 for $libdir
		rm -rf build
		mkdir -p build
		cd build
		cmake -DCMAKE_BUILD_TYPE=RELEASE -DENABLE_STATIC_RUNTIME=ON -DBUILD_SHARED_LIBS=OFF $@ .. || exit 1
		make -j$jobs || exit 1
		mv libgit2.a ../$libgit2dst || exit 1
		cp ../include/* ../../../../$builddir/
		cd ..
	fi
	cd ../../..
}

buildlua () {
	if has_arg debug; then
		if [[ "$builddir" != *debug ]]; then
			builddir+="debug"
			mkdir -p $builddir
		fi
	fi
	if has_arg clean; then
		return
	fi
	if [[ $ccompiler == "placeholder" ]]; then
		ccompiler=$compiler
	fi
	cd ext/src
	if [[ ! -e lua5.4 ]]; then
		if [[ ! -e v5.4.4.zip ]]; then
			echo downloading lua 5.4.4
			wget https://github.com/lua/lua/archive/refs/tags/v5.4.4.zip || exit 1
		fi
		unar v5.4.4.zip -f || exit 1
		mv -f lua-5.4.4 lua5.4
		rm -rf v5.4.4.zip
	fi
	cd lua5.4
	luainclude=../../../$builddir/luainclude
	luadst=../../../$builddir/liblua.a
	mkdir -p $luainclude
	if has_arg buildlua; then
		lua_dirty=1
	else
		lua_dirty=
		for src in *.c *.h; do
			if [[ $src != "luaconf.h" && ($src == *.c || $src == *.h) && ($src -nt $luadst || ! -f $luadst) ]]; then
				lua_dirty=1
				break
			fi
		done
	fi
	if [[ $lua_dirty ]]; then
		if ! mkdir building.lock 2>/dev/null; then
			echo lua build for $libdir waiting for lock
			until mkdir building.lock 2>/dev/null; do
				false
			done
			#timeout 20 inotifywait -e delete_self building.lock
		fi
		echowithprefix building lua for $libdir
		luasrcs=""
		if [[ $lua32bit == 1 ]]; then
			sed -i 's/#define LUA_32BITS	0/#define LUA_32BITS	1/' luaconf.h
		else
			sed -i 's/#define LUA_32BITS	1/#define LUA_32BITS	0/' luaconf.h
		fi
		pids=""
		for src in *.c; do
			if [[ $src != "onelua.c" && $src != "lua.c" ]]; then
				(
					$ccompiler -std=c11 $@ $arch -Ofast -Wall -c $src -o ${src%.c}.o || exit 1
				) &
				pids+=" $!"
			fi
		done
		for p in $pids; do
			wait $p || let "failure=1"
		done
		if [[ $failure == 1 ]]; then
			echowithprefix failure
			exit 1
		fi
		rm -f lua.o
		ar -cvq liblua.a *.o || exit 1
		rm *.o
		cp *.h $luainclude
		sed -i 's/#define LUA_32BITS	1/#define LUA_32BITS	0/' luaconf.h
		rm -f $luadst
		mv liblua.a $luadst
		touch $luadst
		rm -rf building.lock
	elif [[ ! -e $luainclude ]]; then
		cp *.h $luainclude
		if [[ $lua32bit == 1 ]]; then
			sed -i 's/#define LUA_32BITS	0/#define LUA_32BITS	1/' $luainclude/luaconf.h
		else
			sed -i 's/#define LUA_32BITS	1/#define LUA_32BITS	0/' $luainclude/luaconf.h
		fi
	fi
	cd ../../..
	luainclude=$builddir/luainclude
}

build () {
	if [[ $ccompiler == "placeholder" ]]; then
		ccompiler=$compiler
	fi
	if [[ $linker == "placeholder" ]]; then
		linker=$compiler
	fi
	oflags=$flags
	olflags=$link_flags
	obuilddir=$builddir
	if has_arg debug; then
		if [[ "$builddir" != *debug ]]; then
			builddir+="debug"
		fi
		flags+=" -ggdb -O0 -fno-omit-frame-pointer"
		link_flags+=" -ggdb -fno-omit-frame-pointer"
	else
		flags+=" $release_flags"
		link_flags+=" $release_link_flags"
	fi
	mkdir -p $builddir
	allsources=$sources
	sources=
	csources=
	for src in $allsources; do
		if [[ $src == *.c ]]; then
			csources+=" $src"
		elif [[ $src == *.cpp ]]; then
			sources+=" $src"
		fi
	done
	if [[ $glapi == gles3 ]]; then
		use_gles=1
		glslincdir=buildglsles3
		flags+=" -DUSE_GLES3 -I$glslincdir"
		sources+=" $gfx_gl"
		glsl=$glsl_
	elif [[ $glapi == gles2 ]]; then
		use_gles=1
		glslincdir=buildglsles2
		flags+=" -DUSE_GLES2 -I$glslincdir"
		sources+=" $gfx_gl"
		glsl=$glsl_
	elif [[ $glapi == gl2 ]]; then
		glslincdir=buildglsl
		flags+=" -DUSE_GL2 -I$glslincdir"
		sources+=" $gfx_gl"
		csources+=" ../ext/src/glew.c"
		glsl=$glsl_
	elif [[ $glapi == gl1 ]]; then
		flags+=" -DUSE_GL1"
		sources+=" $gfx_gl1"
	fi
	# if this string changes, we clean
	builddesc="$flags $arch"
	builddescprev=`cat $builddir/builddesc`
	clean=0
	if has_arg clean; then
		clean=1
	elif [[ "$builddesc" != "$builddescprev" ]]; then
		echo "build flags changed, cleaning" $builddir
		clean=1
	fi
	if [[ $clean == 1 ]]; then
		for src in $sources; do
			obj="${src%.cpp}"
			obj="$builddir/${obj##*/}.o"
			rm -f $obj $obj.d
		done
		for src in $csources; do
			obj="${src%.c}"
			obj="$builddir/${obj##*/}.o"
			rm -f $obj $obj.d
		done
		rm -rf $builddir/*.glsl.inc
	fi
	if has_arg clean; then
		false
	else
		echo $builddesc > $builddir/builddesc
		any_dirty=0
		if [[ ! -z $glslincdir ]]; then
			mkdir -p $glslincdir
		fi
		for src in $glsl; do
			obj="$glslincdir/$src.glsl.inc"
			if [[ $glapi == gles3 ]]; then
				src="src/platform/gl/glsles3/$src.glsl"
			else
				src="src/platform/gl/glsl/$src.glsl"
			fi
			if [[ $src -nt $obj || ! -f $obj ]]; then
				any_dirty=1
				versionline=$(sed -n '1{/^#version/p};q' $src)
				echo $versionline > $obj.vert
				echo "#define VERTEX" >> $obj.vert
				sed '1{/^#version/d;}' $src >> $obj.vert
				if [[ $glapi == gles3 ]]; then
					./ext/glslopt -3 -v $obj.vert $obj.vert
				elif [[ $glapi == gles2 ]]; then
					./ext/glslopt -2 -v $obj.vert $obj.vert
				else
					./ext/glslopt -v $obj.vert $obj.vert
				fi
				if [[ $? != 0 ]]; then
					#rm -f $obj.vert
					exit 1
				fi
				echo $versionline > $obj.frag
				echo "#define FRAGMENT" >> $obj.frag
				sed '1{/^#version/d;}' $src >> $obj.frag
				if [[ $glapi == gles3 ]]; then
					./ext/glslopt -3 -f $obj.frag $obj.frag
				elif [[ $glapi == gles2 ]]; then
					./ext/glslopt -2 -f $obj.frag $obj.frag
				else
					./ext/glslopt -f $obj.frag $obj.frag
				fi
				if [[ $? != 0 ]]; then
					rm -f $obj.vert $obj.frag
					exit 1
				fi
				echo -n "R\"(" > $obj
				echo $versionline >> $obj
				echo ")\", R\"(#ifdef VERTEX" >> $obj
				sed '1{/^#version/d;}' $obj.vert >> $obj
				echo "#endif" >> $obj
				echo "#ifdef FRAGMENT" >> $obj
				sed '1{/^#version/d;}' $obj.frag >> $obj
				echo "#endif" >> $obj
				echo ")\"" >> $obj
				rm -f $obj.vert $obj.frag
			fi
		done
		if [[ $any_dirty == 1 ]]; then
			echo built shaders
		fi
		outs=""
		pids=""
		any_dirty=0
		jobindex=1
		failure=0
		for src in $csources; do
			obj="${src%.c}"
			obj="$builddir/${obj##*/}.o"
			dirty=0
			if [[ -f $obj.d && -f $obj ]]; then
				deps=`cat $obj.d`
				for dep in ${deps[@]:1}; do
					if [[ $dep -nt $obj ]]; then
						dirty=1
						break
					fi
				done
			else
				dirty=1
			fi
			if [[ $dirty == 1 ]]; then
				echowithprefix building $src
				if has_arg verbose; then
					echowithprefix "$ccompiler $arch $cflags -std=c99 -Isrc -Iext/include -Iext/src/soloud/include -I$builddir -I$luainclude -MMD -MF $obj.d src/$src -c -o $obj"
				fi
				(
					set -o pipefail
					{ $ccompiler $arch $cflags -std=c99 -Isrc -Iext/include -Iext/src/soloud/include -I$builddir -I$luainclude -MMD -MF $obj.d src/$src -c -o $obj | stdoutwithprefix; } 2>&1 >/dev/null | stderrwithprefix
				) &
				pids+=" $!"
				any_dirty=1
			fi
			outs+=" $obj"
			if (( $jobindex % $jobs == 0 )); then
				for p in $pids; do
					wait $p || let "failure=1"
				done
			fi
		done
		pids=""
		for src in $sources; do
			obj="${src%.cpp}"
			obj="$builddir/${obj##*/}.o"
			dirty=0
			if [[ -f $obj.d && -f $obj ]]; then
				deps=`cat $obj.d`
				for dep in ${deps[@]:1}; do
					if [[ $dep -nt $obj ]]; then
						dirty=1
						break
					fi
				done
			else
				dirty=1
			fi
			if [[ $dirty == 1 ]]; then
				echo building $src
				if has_arg verbose; then
					echowithprefix "$compiler $arch $cflags $flags -std=c++17 -include src/defines.hpp -Isrc -Iext/include -Iext/src/soloud/include -I$builddir -Iext/src/fpng -I$luainclude -MMD -MF $obj.d src/$src -c -o $obj"
				fi
				(
					set -o pipefail
					{ $compiler $arch $cflags $flags -std=c++17 -include src/defines.hpp -Isrc -Iext/include -Iext/src/soloud/include -I$builddir -Iext/src/fpng -I$luainclude -MMD -MF $obj.d src/$src -c -o $obj | stdoutwithprefix; } 2>&1 >/dev/null | stderrwithprefix
				) &
				pids+=" $!"
				any_dirty=1
			fi
			outs+=" $obj"
			if (( $jobindex % $jobs == 0 )); then
				for p in $pids; do
					wait $p || let "failure=1"
				done
				pids=""
			fi
		done
		for p in $pids; do
			wait $p || let "failure=1"
		done
		pids=""
		if [[ $any_dirty == 1 ]]; then
			#echowithprefix waiting for compiler
			#for p in $pids; do
			#	wait $p || let "failure=1"
			#done
			if [[ $failure == 1 ]]; then
				echowithprefix failure
				exit 1
			fi
		fi
		if [[ $any_dirty == 1 || ! -f $builddir/$exename || $lua_dirty || $soloud_dirty ]] || has_arg link; then
			echowithprefix linking
			$linker -o $builddir/$exename $outs -L$builddir $link_libs $arch $link_flags || exit 1
			chmod +x $builddir/$exename
		fi
	fi
	flags=$oflags
	link_flags=$olflags
	builddir=$obuilddir
}

# platform specific settings

platform="pc"
platforms="pc win mac android 3ds wiiu switch psp psvita ps2 ps3 ps4 ps5"
for plat in $platforms; do
	if has_arg $plat; then
		platform=$plat
	fi
done
#echo platform selected: $platform

rm -rf ext/src/lua5.4/building.lock
jobs=$(($(nproc)*2))
#jobs=$(($(nproc)/2))
#jobs=$(nproc)
use_gles=
glslincdir=
builddir="build$platform"
mkdir -p $builddir
ccompiler="placeholder" # will either be overriden by the platform or be set to the same c++ compiler
linker="placeholder"    # ^
libdir=$platform
sources="api.cpp input.cpp util.cpp graphics/texture.cpp graphics/image_data.cpp graphics/transform.cpp graphics/render.cpp graphics/text.cpp graphics/mesh.cpp"
sources+=" fs.cpp formats/formats.cpp formats/afs.cpp formats/lnk.cpp formats/cps.cpp formats/vpk.cpp formats/vmx2.cpp"
sources+=" vnscript.cpp vnexecutor.cpp eui/eui.cpp eui/eui_api.cpp eui/eui_layout.cpp eui/eui_scrollable.cpp eui/eui_label.cpp eui/eui_button.cpp eui/eui_separator.cpp eui/eui_toggle.cpp"
sources+=" ../ext/src/texture_atlas/texture_atlas.c"

glsl_="bicubic bicubic_batched copy_batched alpha_batched anime4k_push anime4k_calc_grad anime4k_push_grad lighted"
# impl shorthands
gfx_gl="platform/gl/gl_gfx.cpp platform/gl/gl_shader.cpp"
gfx_gl1="platform/gl1/gl1_gfx.cpp"
fs_std="platform/std_fs.cpp"
audio_null="platform/null/null_audio.cpp"
audio_soloud="platform/soloud/soloud_audio.cpp"
video_null="platform/null/null_video.cpp"
video_ff="platform/ffmpeg/ff_video.cpp"
glapis="gles2 gles3 gl1 gl2"
avlibs="-lz -lavcodec -lavdevice -lavformat -lavfilter -lswresample -lswscale -lavutil"
if [[ $platform == "pc" ]]; then
	sources+=" platform/pc/pc.cpp $fs_std $audio_soloud $video_ff"
	flags="-DREGULAR_LUA -DPLATFORM_PC -DPLATFORM_LINUX -DWITH_MINIAUDIO -fno-rtti -fPIE"
	glapi=gl2
	for glapi_ in $glapis; do
		if has_arg $glapi_; then
			glapi=$glapi_
			echo selected api: $glapi
			break
		fi
	done
	arch="-DGLM_FORCE_SSE42 -DGLM_FORCE_ALIGNED -msse4.2"
	flags+=" -I/usr/include/freetype2"
	release_flags="-Ofast -flto"
	link_flags=""
	release_link_flags="-flto"
	link_libs="-lglfw -lGLEW -lGL -lpng -lfreetype -lcurl -lasound -llua -lsoloud_static -Wl,-R. -pthreads $avlibs"
	if has_arg prof; then
		link_libs+=" -lprofiler"
	fi
	compiler="clang++ -Xclang -fcolor-diagnostics"
	ccompiler="clang -fcolor-diagnostics -fPIE"
	exename="eternal"
	if has_arg debug; then false; else
		arch+=" -fstrict-aliasing -ffast-math -fassociative-math -ftree-vectorize -ftree-slp-vectorize"
		buildffmpeg
		# in debug builds just link with system ffmpeg
	fi
	buildsoloud
	buildlibgit2
	buildlua
	build
	if has_arg test; then
		cd out
		if has_arg prof; then
			if has_arg debugger; then
				CPUPROFILE_FREQUENCY=500 CPUPROFILE=/tmp/prof.out lldb ../$builddir/eternal dev $runargs
			else
				CPUPROFILE_FREQUENCY=500 CPUPROFILE=/tmp/prof.out ../$builddir/eternal dev $runargs
				#../$builddir/eternal $runargs
				#gprof ../$builddir/eternal $runargs
			fi
			PPROF_BINARY_PATH=../$builddir go tool pprof --source_path src -callgrind ../$builddir/eternal /tmp/prof.out > /tmp/profcg.out
			kcachegrind /tmp/profcg.out &
		else
			if has_arg debugger; then
				lldb ../$builddir/eternal dev $runargs
			else
				if has_arg mango; then
					MANGOHUD_DLSYM=1 mangohud ../$builddir/eternal dev $runargs
				else
					../$builddir/eternal dev $runargs
				fi
			fi
		fi
		cd ..
	fi
elif [[ $platform == "win" ]]; then
	sources+=" platform/pc/pc.cpp $fs_std $audio_soloud $video_ff"
	flags="-DPLATFORM_PC -DPLATFORM_WINDOWS -I/usr/include/freetype2 -fPIE"
	glapi=gl2
	for glapi_ in $glapis; do
		if has_arg $glapi_; then
			glapi=$glapi_
			echo selected graphics api: $glapi
			break
		fi
	done
	arch="-DWINVER=0x0501 -D_WIN32_WINNT=0x0501 -D_WIN32 -DGLM_FORCE_SSE42 -DGLM_FORCE_ALIGNED -msse4.2"
	if has_arg debug; then false; else
		arch+=" -ftree-vectorize -ffast-math -fassociative-math -ftree-slp-vectorize"
	fi
	release_flags="-Ofast -flto"
	link_flags="-specs=nondefault-crt-spec -static -Wl,-R."
	release_link_flags="-flto"
	link_libs="-Lext/lib/$libdir -lglfw3 -lglew32 -lopengl32 -lgdi32 -lpng -lz -lfreetype -lsoloud_static -lwinmm -lluajit $avlibs"
	ccompiler="x86_64-w64-mingw32-cc"
	compiler="x86_64-w64-mingw32-c++"
	exename="eternal.exe"
	buildffmpeg --enable-cross-compile --cross-prefix=x86_64-w64-mingw32- --target-os=mingw64
	buildsoloud --os=windows --platform=x64
	buildluajit CROSS=x86_64-w64-mingw32- TARGET_SYS=Windows
	build
	if has_arg test; then
		cd out
		wine ../$builddir/eternal.exe dev $runargs
		cd ..
	fi
elif [[ $platform == "psp" ]]; then
	sources+=" platform/psp/psp.cpp platform/psp/memcpy_vfpu.cpp $fs_std $audio_soloud $video_null"
	flags="-DREGULAR_LUA -DPLATFORM_PSP -DPLATFORM_PS -DPLATFORM_PORTABLE -DPLATFORM_CONTROLLER -DPLATFORM_NO_EXCEPTIONS -D_PSP_FW_VERSION=660"
	flags+=" -fno-exceptions -fno-rtti -I$PSPDEV/psp/sdk/include -I$PSPDEV/psp/include -I$PSPDEV/psp/include/freetype2"
	release_flags="-Ofast -flto"
	arch="-DLUA_32BITS=1 -march=allegrex -mfp32 -mgp32 -mlong32 -mhard-float -mabi=eabi -foptimize-strlen -fomit-frame-pointer -falign-functions=32 -falign-loops -falign-labels -falign-jumps"
	arch+=" -fstrict-aliasing -ffast-math -fassociative-math -ftree-vectorize -ftree-slp-vectorize -ftree-vectorizer-verbose=2"
	link_flags="-specs=$PSPDEV/psp/sdk/lib/prxspecs -Wl,-q,-T$PSPDEV/psp/sdk/lib/linkfile.prx -Wl,-zmax-page-size=128 -static"
	link_libs="$PSPDEV/psp/sdk/lib/prxexports.o -L$PSPDEV/psp/sdk/lib -Lext/lib/psp -Wl,-R."
	#link_libs+=" -lm -lpng -lfreetype -lcurl -lz -lsndfile -logg -lvorbis -lvorbisenc -lopus -lFLAC -llua -lstdc++ -lGL -lglut"
	link_libs+=" -lm -lz -lpng -lfreetype -lcurl -latomic -lsoloud_static -llua -lstdc++ -lGL -lglut"
	link_libs+=" -lpspvfpu -lpspdebug -lpspdisplay -lpspaudio -lpspge -lpspgu -lpspgum -lpspvram -lpspctrl -lpspreg -ltri"
	release_link_flags="-flto"
	ccompiler="psp-gcc"
	compiler="psp-g++"
	exename="eternal.elf"
	#buildflac -DCMAKE_TOOLCHAIN_FILE=$PSPDEV/psp/share/pspdev.cmake
	#buildopus -DCMAKE_TOOLCHAIN_FILE=$PSPDEV/psp/share/pspdev.cmake
	#buildlibsndfile -DCMAKE_TOOLCHAIN_FILE=$PSPDEV/psp/share/pspdev.cmake
	#buildffmpeg --disable-hwaccels --enable-pthreads --disable-w32threads --enable-cross-compile --cross-prefix=psp- --arch=mipsfpu --cpu=allegrex --target-os=none --enable-mipsfpu
	buildsoloud --with-psp-homebrew-only
	lua32bit=1
	buildlua -Ofast
	build
	cd $builddir
	if has_arg clean; then
		rm -f eternal/EBOOT.PBP eternal.elf eternal.prx
	else
		psp-fixup-imports eternal.elf
		mksfoex -d MEMSIZE=1 "Eternal" PARAM.SFO
		psp-prxgen eternal.elf eternal.prx
		psp-strip eternal.elf -o eternal_strip.elf
		mkdir -p eternal
		pack-pbp EBOOT.PBP PARAM.SFO ../ext/lib/psp/ICON0.PNG NULL NULL ../ext/lib/psp/PIC1.PNG ../ext/lib/psp/SND0.AT3 eternal.prx NULL
		ebootsign EBOOT.PBP eternal/EBOOT.PBP
		#pack-pbp eternal/EBOOT.PBP PARAM.SFO ICON0.PNG NULL NULL PIC1.PNG SND0.AT3 eternal_strip.elf NULL
		rm -f eternal_strip.elf
		cp eternal/EBOOT.PBP ~/.config/ppsspp/PSP/GAME/eternal/EBOOT.PBP
		cp -r ../out/core ~/.config/ppsspp/PSP/GAME/eternal
	fi
	cd ..
elif [[ $platform == "switch" ]]; then
	sources+=" platform/switch/switch.cpp $fs_std $audio_soloud $video_null"
	glapi=gles3
	arch="-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE -ftree-vectorize -ffast-math -fassociative-math -ftree-slp-vectorize"
	flags="-DREGULAR_LUA -D__SWITCH__ -DPLATFORM_SWITCH -DPLATFORM_NINTENDO -DPLATFORM_CONTROLLER -DPLATFORM_NO_EXCEPTIONS"
	flags+=" -fno-rtti -fno-exceptions -ffunction-sections"
	flags+=" -I/$DEVKITPRO/libnx/include -I$DEVKITPRO/portlibs/switch/include -I$DEVKITPRO/portlibs/switch/include/freetype2"
	release_flags="-Ofast -flto"
	#link_flags="-specs=$DEVKITPRO/libnx/switch.specs -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE"
	link_flags="-specs=$DEVKITPRO/libnx/switch.specs -g -Wl,-Map,buildswitch/eternal.map"
	link_libs="-L$DEVKITPRO/libnx/lib -L$DEVKITPRO/portlibs/switch/lib"
	link_libs+=" -lpng -lfreetype -lz -lbz2 -llua -lsoloud_static"
	link_libs+=" -lglad -lEGL -lglapi -ldrm_nouveau -lnx"
	release_link_flags="-flto"
	root="$DEVKITA64/bin"
	compiler="$root/aarch64-none-elf-g++ -DLLONG_MAX=9223372036854775807"
	ccompiler="$root/aarch64-none-elf-gcc -DLLONG_MAX=9223372036854775807"
	exename="eternal.elf"
	buildsoloud --with-switch-homebrew-only
	#buildlua -I/$DEVKITPRO/libnx/include -I$DEVKITPRO/portlibs/switch/include
	buildluajit
	build
	cd $builddir
	$root/aarch64-none-elf-gcc-nm -CSn eternal.elf > eternal.lst
	$DEVKITPRO/tools/bin/nacptool --create "Eternal" "malucart" "0.99.0" eternal.nacp
	$DEVKITPRO/tools/bin/elf2nro eternal.elf eternal.nro --icon=$DEVKITPRO/libnx/default_icon.jpg --nacp=eternal.nacp
	echo output to $builddir/eternal.nro
	cd ..
elif [[ $platform == "ps4" ]]; then
	sources+=" platform/ps4/ps4.cpp $fs_std $audio_null $video_null"
	glapi=gles2
	cflags="--target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -DPS4 -D_BSD_SOURCE -isysroot $OO_PS4_TOOLCHAIN -isystem $OO_PS4_TOOLCHAIN/include"
	flags="-DREGULAR_LUA -DPLATFORM_PS4 -DPLATFORM_PS -DPLATFORM_CONTROLLER -DPLATFORM_NO_EXCEPTIONS"
	flags+=" -fno-rtti -fno-exceptions -ffunction-sections -I$OO_PS4_TOOLCHAIN/include/c++/v1"
	flags+=" -Iext/lib/$platform/luainclude"
	release_flags="-Ofast -flto"
	link_flags="-nostdlib -melf_x86_64 -pie --script $OO_PS4_TOOLCHAIN/link.x --eh-frame-hdr $OO_PS4_TOOLCHAIN/lib/crt1.o"
	link_libs="-L$OO_PS4_TOOLCHAIN/lib -Lext/lib/$platform"
	link_libs+=" -lScePngEnc -lScePngDec -lSceFreeType -lSceZlib -llua"
	link_libs+=" -lc -lkernel -lc++ -lSceShellCoreUtil -lSceSysmodule -lSceSystemService -lScePigletv2VSH -lScePrecompiledShaders -lSceRtc"
	release_link_flags=""
	linker="ld.lld"
	compiler="clang++"
	ccompiler="clang"
	exename="eternal.elf"
	#buildsoloud --with-ps4-homebrew-only
	buildlua -isysroot $OO_PS4_TOOLCHAIN -isystem $OO_PS4_TOOLCHAIN/include
	build
	cd $builddir
	$OO_PS4_TOOLCHAIN/bin/linux/create-fself -in=eternal.elf -out=eternal.oelf --eboot "eboot.bin" --paid 0x3800000000000035 --authinfo 000000000000000000000000001C004000FF000000000080000000000000000000000000000000000000008000400040000000000000008000000000000000080040FFFF000000F000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
	$OO_PS4_TOOLCHAIN/bin/linux/PkgTool.Core sfo_new param.sfo
	$OO_PS4_TOOLCHAIN/bin/linux/PkgTool.Core sfo_setentry param.sfo APP_TYPE --type Integer --maxsize 4 --value 1
	$OO_PS4_TOOLCHAIN/bin/linux/PkgTool.Core sfo_setentry param.sfo APP_VER --type Utf8 --maxsize 8 --value '1.00'
	$OO_PS4_TOOLCHAIN/bin/linux/PkgTool.Core sfo_setentry param.sfo ATTRIBUTE --type Integer --maxsize 18 --value 0
	$OO_PS4_TOOLCHAIN/bin/linux/PkgTool.Core sfo_setentry param.sfo CATEGORY --type Utf8 --maxsize 4 --value 'gde'
	$OO_PS4_TOOLCHAIN/bin/linux/PkgTool.Core sfo_setentry param.sfo FORMAT --type Utf8 --maxsize 4 --value 'obs'
	$OO_PS4_TOOLCHAIN/bin/linux/PkgTool.Core sfo_setentry param.sfo CONTENT_ID --type Utf8 --maxsize 48 --value 'IV0000-ETER00007_00-ETERNALNER000000'
	$OO_PS4_TOOLCHAIN/bin/linux/PkgTool.Core sfo_setentry param.sfo DOWNLOAD_DATA_SIZE --type Integer --maxsize 4 --value 0
	$OO_PS4_TOOLCHAIN/bin/linux/PkgTool.Core sfo_setentry param.sfo SYSTEM_VER --type Integer --maxsize 4 --value 1020
	$OO_PS4_TOOLCHAIN/bin/linux/PkgTool.Core sfo_setentry param.sfo TITLE --type Utf8 --maxsize 128 --value 'Eternal'
	$OO_PS4_TOOLCHAIN/bin/linux/PkgTool.Core sfo_setentry param.sfo TITLE_ID --type Utf8 --maxsize 12 --value 'ETER00007'
	$OO_PS4_TOOLCHAIN/bin/linux/PkgTool.Core sfo_setentry param.sfo VERSION --type Utf8 --maxsize 8 --value '1.00'
	$OO_PS4_TOOLCHAIN/bin/linux/create-gp4 -out pkg.gp4 --content-id=IV0000-ETER00007_00-ETERNALNER000000 --files eboot.bin ../etc//right.sprx sce_sys/param.sfo sce_sys/icon0.png
	$OO_PS4_TOOLCHAIN/bin/linux/PkgTool.Core pkg_build pkg.gp4 .
	#$root/aarch64-none-elf-gcc-nm -CSn eternal.elf > eternal.lst
	#$DEVKITPRO/tools/bin/nacptool --create "Eternal" "malucart" "0.99.0" eternal.nacp
	#$DEVKITPRO/tools/bin/elf2nro eternal.elf eternal.nro --icon=$DEVKITPRO/libnx/default_icon.jpg --nacp=eternal.nacp
	echo output to $builddir/eternal.elf
	cd ..
elif [[ $platform == "wiiu" ]]; then
	sources+=" platform/wiiu/wiiu.cpp $fs_std $audio_null $video_null platform/wiiu/gx2gl/src/gx2gl.c platform/wiiu/gx2gl/src/gx2glu.c platform/wiiu/gx2gl/src/gx2glut.c platform/wiiu/gx2gl/src/proc.c platform/wiiu/gx2gl/src/matrix.c"
	glapi=gl1
	flags="-DREGULAR_LUA -DPLATFORM_WII_U -DPLATFORM_NINTENDO -DPLATFORM_CONTROLLER -DLUA_32BITS_SET"
	cflags+=" -I/$DEVKITPRO/wut/include -I$DEVKITPRO/portlibs/ppc/include -I$DEVKITPRO/portlibs/ppc/include/freetype2 -Isrc/platform/wiiu/gx2gl/include"
	release_flags="-Ofast -flto"
	link_flags="-specs=$DEVKITPRO/wut/share/wut.specs"
	link_libs="-L$DEVKITPPC/powerpc-eabi/lib -L$DEVKITPRO/wut/lib -L$DEVKITPRO/portlibs/ppc/lib -Lext/lib/wiiu"
	link_libs+=" -lpng -lfreetype -lz -lbz2 -llua -lm -lstdc++"
	link_libs+=" -lwut"
	release_link_flags="-flto"
	root="$DEVKITPPC/bin"
	compiler="$root/powerpc-eabi-gcc"
	exename="eternal.elf"
	buildlua -DLUA_32BITS_SET -DLUA_C89_NUMBERS
	build
	cd $builddir
	elf2rpl eternal.elf eternal.rpx
	cd ..
elif [[ $platform == "android" ]]; then
	sources+=" platform/android/android.cpp $fs_std $audio_null $video_null"
	glapi=gles3
	flags="-DREGULAR_LUA -DPLATFORM_ANDROID -I/usr/include/freetype2 --sysroot=/opt/android-ndk/toolchains/llvm/prebuilt/linux-x86_64/sysroot -Dnative_lib_EXPORTS -DANDROID -fdata-sections -ffunction-sections -funwind-tables -fstack-protector-strong -no-canonical-prefixes -D_FORTIFY_SOURCE=2 -Wformat -Werror=format-security -fno-limit-debug-info"
	cflags+="-fPIC"
	release_flags="-O3 -flto"
	link_flags="-Wl,--exclude-libs,libgcc.a -Wl,--exclude-libs,libgcc_real.a -Wl,--exclude-libs,libatomic.a -Wl,--build-id=sha1 -Wl,--no-rosegment -Wl,--fatal-warnings -Wl,--exclude-libs,libunwind.a -Wl,--no-undefined -lGLESv3 -lOpenSLES -latomic -lm -Wl,-soname,libeternal.so -shared"
	release_link_flags="-flto"
	exename="libeternal.so"
	builddir="buildandroid"
	rm -rf ext/src/lua5.4/building.lock
	(
		echoprefix=armeabi
		builddir+="/armeabi-v7a"
		libdir="android/armeabi-v7a"
		ccompiler="/opt/android-ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/armv7a-linux-androideabi29-clang -Xclang -fcolor-diagnostics"
		compiler="/opt/android-ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/armv7a-linux-androideabi29-clang++ -Xclang -fcolor-diagnostics"
		link_flags+=" /opt/android-ndk/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/arm-linux-androideabi/23/liblog.so -Lext/lib/android/armeabi-v7a -Lext/lib/android/eternal/lib/armeabi-v7a -lcurl -lfreetype -lz -lpng -lsoloud -llua"
		arch="-march=armv7-a -mthumb"
		buildlua -fPIC -DLUA_32BITS_SET
		flags+=" -Iext/lib/$libdir/luainclude"
		build
	) &
	ppids+=" $!"
	(
		echoprefix=arm64
		builddir+="/arm64-v8a"
		libdir="android/arm64-v8a"
		ccompiler="/opt/android-ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android29-clang -Xclang -fcolor-diagnostics"
		compiler="/opt/android-ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android29-clang++ -Xclang -fcolor-diagnostics"
		link_flags+=" /opt/android-ndk/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/aarch64-linux-android/23/liblog.so -Lext/lib/android/arm64-v8a -Lext/lib/android/eternal/lib/arm64-v8a -lcurl -lfreetype -lz -lpng -lsoloud -llua"
		buildlua -fPIC
		flags+=" -Iext/lib/$libdir/luainclude"
		build
	) &
	ppids+=" $!"
	(
		echoprefix=x86
		builddir+="/x86"
		libdir="android/x86"
		ccompiler="/opt/android-ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/i686-linux-android29-clang -Xclang -fcolor-diagnostics"
		compiler="/opt/android-ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/i686-linux-android29-clang++ -Xclang -fcolor-diagnostics"
		link_flags+=" /opt/android-ndk/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/i686-linux-android/23/liblog.so -Lext/lib/android/x86 -Lext/lib/android/eternal/lib/x86 -lcurl -lfreetype -lz -lpng -lsoloud -llua"
		buildlua -fPIC -DLUA_32BITS_SET
		flags+=" -Iext/lib/$libdir/luainclude"
		build
	) &
	ppids+=" $!"
	(
		echoprefix=x86_64
		builddir+="/x86_64"
		libdir="android/x86_64"
		ccompiler="/opt/android-ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/x86_64-linux-android29-clang -Xclang -fcolor-diagnostics"
		compiler="/opt/android-ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/x86_64-linux-android29-clang++ -Xclang -fcolor-diagnostics"
		link_flags+=" /opt/android-ndk/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/x86_64-linux-android/23/liblog.so -Lext/lib/android/x86_64 -Lext/lib/android/eternal/lib/x86_64 -lcurl -lfreetype -lz -lpng -lsoloud -llua"
		buildlua -fPIC
		flags+=" -Iext/lib/$libdir/luainclude"
		build
	) &
	ppids+=" $!"
	failure=0
	for p in $ppids; do
		wait $p || let "failure=1"
	done
	if [[ $failure == 1 ]]; then
		exit 1 
	fi
	cd buildandroid
	cp -r ../ext/lib/android/eternal eternal
	cd eternal
	rm ../eternal.apk
	cp ../armeabi-v7a/libeternal.so lib/armeabi-v7a/libnative-lib.so
	cp ../arm64-v8a/libeternal.so lib/arm64-v8a/libnative-lib.so
	cp ../x86/libeternal.so lib/x86/libnative-lib.so
	cp ../x86_64/libeternal.so lib/x86_64/libnative-lib.so
	#cp ../../ext/lib/android/armeabi-v7a/libcurl.so lib/armeabi-v7a/libcurl.so
	#cp ../../ext/lib/android/arm64-v8a/libcurl.so lib/arm64-v8a/libcurl.so
	#cp ../../ext/lib/android/x86/libcurl.so lib/x86/libcurl.so
	#cp ../../ext/lib/android/x86_64/libcurl.so lib/x86_64/libcurl.so
	#cp ../../ext/lib/android/armeabi-v7a/libssl.so lib/armeabi-v7a/libssl.so
	#cp ../../ext/lib/android/arm64-v8a/libssl.so lib/arm64-v8a/libssl.so
	#cp ../../ext/lib/android/x86/libssl.so lib/x86/libssl.so
	#cp ../../ext/lib/android/x86_64/libssl.so lib/x86_64/libssl.so
	#cp ../../ext/lib/android/armeabi-v7a/libcrypto.so lib/armeabi-v7a/libcrypto.so
	#cp ../../ext/lib/android/arm64-v8a/libcrypto.so lib/arm64-v8a/libcrypto.so
	#cp ../../ext/lib/android/x86/libcrypto.so lib/x86/libcrypto.so
	#cp ../../ext/lib/android/x86_64/libcrypto.so lib/x86_64/libcrypto.so
	zip -r0 ../eternal.apk * > /dev/null
	cd ..
	# jarsigner is from the JDK
	build_tools=(/opt/android-sdk/build-tools/*)
	build_tools=${build_tools[0]}
	$build_tools/zipalign -p 4 eternal.apk eternalaligned.apk
	mv eternalaligned.apk eternal.apk
	echo eternal | $build_tools/apksigner sign --ks ../ext/lib/android/eternal.keystore eternal.apk > /dev/null
	if has_arg test; then
		adb install eternal.apk && adb shell am start -n com.malucart.eternalvn/com.malucart.eternalvn.MainActivity
	fi
	cd ..
else
	echo platform not implemented
fi
