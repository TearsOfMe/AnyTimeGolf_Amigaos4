# Anytime Golf auf AmigaOS 4

Dieser Port verwendet:

- SDL2 (libSDL2_gl4es) für Fenster, Eingaben und den Hauptloop
- SDL2_image für PNG-Ressourcen
- GL4ES (Mesa-basierter OpenGL 2.1 Wrapper) für Hardware-beschleunigtes Rendering via Warp3D Nova / OGLES2
- optional LIBGL_ALWAYS_SOFTWARE=1 für Software-Rendering

## Voraussetzungen

Auf dem Build-System müssen ein AmigaOS-4-Cross-Compiler, die SDK-Header und
Bibliotheken für SDL2_gl4es, SDL2_image, GL4ES (`libgl4es`), `libGLU_gl4es` und Bullet installiert sein.

## Cross-Compile im vorhandenen Docker-Container

Der bereitgestellte Container `4530b990e258eda651876f79799ec39b9b993394c8ae72f72a619b9b63ebd2de`
enthält den PPC-AmigaOS4-GCC 11.5.0 sowie SDL2-/Mesa-/Bullet-SDK-Dateien.
Der Quellbaum kann zum Bauen nach `/opt/code/golf` kopiert werden:

```sh
docker cp . 4530b990e258eda651876f79799ec39b9b993394c8ae72f72a619b9b63ebd2de:/opt/code/golf
docker exec 4530b990e258eda651876f79799ec39b9b993394c8ae72f72a619b9b63ebd2de \
  /opt/ppc-amigaos/bin/ppc-amigaos-g++ --version
```

Das Image stellt kein CMake bereit. Für reproduzierbare Builds werden deshalb
die CMake-Quelldateien und die identischen Compiler-/Include-/Linker-Optionen
als Grundlage für ein natives Container-Buildskript verwendet.

## Cross-Compile (CMake)

```sh
cmake -S ports/amigaos4 -B build-amigaos4 \
  -DCMAKE_TOOLCHAIN_FILE=ports/amigaos4/amigaos4-toolchain.cmake \
  -DCMAKE_PREFIX_PATH=/opt/amigaos4/SDK/local
cmake --build build-amigaos4 --parallel
```

Das erzeugte Programm erwartet `data/` neben der ausführbaren Datei. Die
Ressourcen werden beim Build automatisch aus `code/game/Data` und
`code/game/Resources-iPad` kopiert.

Spielstände werden unter AmigaOS 4 über `PROGDIR:` als Dateien
`save_<app>_<name>.dat` neben der ausführbaren Datei gespeichert. Fehlt ein
Spielstand oder hat er eine unerwartete Größe, wird er als nicht vorhanden
behandelt und der jeweilige Standardzustand verwendet.

## Hinweise zum Renderer

`SDL_GL_ACCELERATED_VISUAL=0` fordert einen Software-Kontext an. Bei Mesa kann
zusätzlich die Laufzeitvariable `LIBGL_ALWAYS_SOFTWARE=1` gesetzt werden. Der
Port nutzt bewusst den bestehenden Fixed-Function-Renderer; Shader oder
GPU-spezifische Erweiterungen sind nicht erforderlich.
