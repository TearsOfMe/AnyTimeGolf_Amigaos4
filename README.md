
Anytime Golf: Magic Touch
=========================

"Anytime Golf: Magic Touch" is an interactive golf simulation game for iPhone and iPad
developed by [Robert Rose](http://robertwrose.com/) and Jake Helms through Bork 3D LLC.

In 2015 Bork 3D released the source code and source art material for the game to the
public under a permissive BSD-style open source license.

The source code includes the [Bork 3D Game Engine](https://en.wikipedia.org/wiki/Bork3D_Game_Engine),
a C++ game engine capable of targeting iOS, MacOS and Windows.

![](art/marketing/screenshots/IMG_0033.PNG) ![](art/marketing/screenshots/IMG_0049.PNG)

Compilation
===========

iOS
---

Xcode is required.

Open code/game/golf.xcodeproj and compile the iPhone or iPad, Debug or Release targets.

MacOS
-----

Xcode7 is required.

Open code/game/macos/example_macos.xcodeproj and compile the Debug or Release targets.

Windows
-------

Visual Studio 2013 is required.

FreeImage.dll is required to run the game. FreeImage is distributed under
less-permissive license terms so must be downloaded separately. You can obtain FreeImage.dll
from http://freeimage.sourceforge.net/ Place the file in code\game.

Open code\game\win32\game.sln and compile the Debug or Release targets.

AmigaOS 4
---------

Ported to **AmigaOS 4** by **TearsOfMe** (based on the original work by Robert Rose and Jake Helms / Bork 3D LLC).

Key porting features and technical adaptations:
- **Graphics:** OpenGL ES 1.1 / 2.0 via [gl4es](https://github.com/ptitSeb/gl4es) on top of Warp3D Nova (RadeonHD / RadeonRX).
- **Audio & Input:** SDL2 audio subsystem and event loop integration with mouse / touch-drag swing controls.
- **Architecture:** Full Big-Endian PowerPC support (PowerPC 74xx / PA6T on AmigaOne X1000 / X5000 / Sam460):
  - In-place endian byte swapping for interleaved PowerVR POD model attributes and bone indices.
  - Endian-safe decompression of PowerVR PVRTC texture blocks (`PVRTCDecompress`).
  - Alignment-safe float access for odd-strided skinned meshes.
  - Native libpng texture decoding and font glyph UV mapping.

Stereoscopic 3D
===============

The branch 'stereo_3d' contains an experimental port of Anytime Golf for Windows that renders
the game in stereoscopic 3D on nVidia 3D Vision systems.

Thanks to @tliron for the example code that outlines how to render OpenGL in a manner
compatible with nVidia 3D Vision. See https://github.com/tliron/opengl-3d-vision-bridge

License
=======

Source code and source artwork covered by the original LICENSE file:
Copyright (c) 2008-2015, Bork 3D LLC. All rights reserved.

AmigaOS 4 port modifications and additions (c) 2026 TearsOfMe.

Music Copyright (c) [Mick Rippon](https://soundcloud.com/mickrip). All Rights Reserved.
