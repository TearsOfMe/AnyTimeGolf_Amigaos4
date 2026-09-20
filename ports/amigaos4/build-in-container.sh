#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
SDK=${AMIGAOS4_SDK:-/opt/ppc-amigaos/ppc-amigaos/SDK}
CXX=${CXX:-/opt/ppc-amigaos/bin/ppc-amigaos-g++}
CC=${CC:-/opt/ppc-amigaos/bin/ppc-amigaos-gcc}
BUILD="$ROOT/build-amigaos4"
OBJ="$BUILD/obj"

rm -rf "$BUILD"
mkdir -p "$OBJ"

FLAGS="-std=c++11 -O2 -fpermissive -Wno-narrowing -D_GLIBCXX_USE_CXX11_ABI=0 -DRUDE_AMIGAOS4 -DRUDE_IPAD=1 -DNO_RUDETWEAKER"
FLAGS="$FLAGS -I$ROOT/ports/amigaos4/compat"
FLAGS="$FLAGS -I$ROOT/code/engine -I$ROOT/code/game/Classes"
FLAGS="$FLAGS -I$ROOT/code/lib/pvrt -I$ROOT/code/lib/pvrt/OGLES"
FLAGS="$FLAGS -I$ROOT/code/lib/bullet/src"
FLAGS="$FLAGS -I$ROOT/code/lib/bullet/src/LinearMath"
FLAGS="$FLAGS -I$ROOT/code/lib/bullet/src/BulletCollision"
FLAGS="$FLAGS -I$ROOT/code/lib/bullet/src/BulletDynamics"
FLAGS="$FLAGS -I$SDK/newlib/include -I$SDK/local/newlib/include/SDL2"
FLAGS="$FLAGS -I$SDK/local/common/include"

compile_cxx()
{
    source=$1
    object="$OBJ/$(basename "$source" .cpp).o"
    echo "CXX $source"
    "$CXX" $FLAGS -c "$source" -o "$object"
}

for source in "$ROOT"/code/engine/*.cpp "$ROOT"/code/game/Classes/*.cpp; do
    case "$source" in
        */RudeRegistryCF.cpp|*/RudeRegistryWin.cpp|*/RudeRegistrySymbian.cpp|\
        */SoundEngine.cpp|*/MikSound.cpp|*/RBEffectsMgr.cpp|*/RBTButton.cpp|\
        */RBTTitle.cpp|*/SoundProvider.cpp|*/SoundStream.cpp)
            continue
            ;;
    esac
    compile_cxx "$source"
done

compile_cxx "$ROOT/ports/amigaos4/main.cpp"

for source in "$ROOT"/code/lib/pvrt/*.cpp; do
    case "$source" in
    esac
    compile_cxx "$source"
done

for source in \
    "$ROOT"/code/lib/pvrt/OGLES/PVRTTextureAPI.cpp \
    "$ROOT"/code/lib/pvrt/OGLES/PVRTPrint3DAPI.cpp
do
    compile_cxx "$source"
done

echo "CC ports/amigaos4/gthread_stub.c"
"$CC" -c "$ROOT/ports/amigaos4/gthread_stub.c" -o "$OBJ/gthread_stub.o"

for source in $(find "$ROOT/code/lib/bullet/src" -type f -name '*.cpp' | sort); do
    case "$source" in
        */Demos/*|*/Extras/*|*/Bullet-C-API.cpp)
            continue
            ;;
    esac
    object="$OBJ/$(basename "$source" .cpp).bullet.o"
    echo "CXX $source"
    "$CXX" $FLAGS -c "$source" -o "$object"
done

echo "LINK golf_amigaos4"
"$CXX" -o "$BUILD/golf_amigaos4" \
    -use-dynld \
    $(find "$OBJ" -maxdepth 1 -name '*.o' ! -name '*.bullet.o' | sort) \
    $(find "$OBJ" -maxdepth 1 -name '*.bullet.o' | sort) \
    -L"$SDK/local/newlib/lib" \
    -lSDL2_image -lSDL2 -lGL -lGLU \
    -ltiff -lwebpdemux -lwebp -lwebpmux -lsharpyuv \
    -ljpeg -lpng -lz -lbz2 -lm \
    -L/opt/ppc-amigaos/lib/gcc/ppc-amigaos/11.5.0/newlib \
    -Wl,-Bstatic -lstdc++ -lgcc -Wl,-Bdynamic \
    -Wl,-Map,"$BUILD/golf_amigaos4.map"

cp -R "$ROOT/code/game/Data" "$BUILD/data"
cp -R "$ROOT/code/game/Resources-iPad" "$BUILD/data/Resources-iPad"
echo "Built $BUILD/golf_amigaos4"
