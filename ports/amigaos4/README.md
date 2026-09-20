# Anytime Golf auf AmigaOS 4

Dieser Port verwendet:

- SDL2 für Fenster, Eingaben und den Hauptloop
- SDL2_image für PNG-Ressourcen
- Mesa/GL für den vorhandenen OpenGL-1.x-Renderer
- einen erzwungenen nicht-beschleunigten SDL-GL-Kontext für Software-Rendering

## Voraussetzungen

Auf dem Build-System müssen ein AmigaOS-4-Cross-Compiler, die SDK-Header und
Bibliotheken für SDL2, SDL2_image, Mesa/GL, GLU und Bullet installiert sein.
Die CMake-Pakete müssen die Targets `SDL2::SDL2`, `SDL2_image::SDL2_image`,
`OpenGL::GL`, `OpenGL::GLU` sowie die Bullet-Targets bereitstellen.

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
