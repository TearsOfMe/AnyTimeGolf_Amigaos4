# Anytime Golf auf AmigaOS 4

Dieser Port bringt **Anytime Golf: Magic Touch** (Bork 3D Game Engine) nativ auf **AmigaOS 4 (PowerPC)**.

Gepflegt und portiert von **TearsOfMe**: https://github.com/TearsOfMe/AnyTimeGolf_Amigaos4

---

## Enthaltene Executables

Das Paket stellt zwei optimierte Programmversionen bereit:

### 1. `golf_amigaos4` (Hardware-beschleunigt – Empfohlen)
- **Renderer:** [GL4ES](https://github.com/ptitSeb/gl4es) (OpenGL 2.1 / ES 2.0 Wrapper) auf Basis von **Warp3D Nova**.
- **Bibliotheken:** `libSDL2_gl4es`, `libgl4es`, `libGLU_gl4es`.
- **Einsatzbereich:** Systeme mit moderner Grafikkarte und Warp3D Nova (z. B. AmigaOne X1000, X5000, A1222+, SAM460 mit RadeonHD oder RadeonRX).
- **Vorteile:** Maximale Darstellungsqualität, hardwarebeschleunigtes Alpha-Blending, Mipmapping und flüssige Framerate.

### 2. `golf_amigaos4_soft` (Eingebetteter TinyGL CPU-Software-Rasterizer -> SDL2)
- **Renderer:** Integrierter, nativer **TinyGL CPU-Software-Rasterizer** (Open-Source 3D Engine in reinem C).
- **Framebuffer:** Rendert alle 3D-Geometrien, Texturen und Alpha-Blending direkt per CPU in einen RAM-Framebuffer und gibt ihn über ein Standard-SDL2-2D-Texture-Streaming aus.
- **Treiberunabhängig:** Benötigt **weder MiniGL noch Warp3D noch Warp3D Nova**.
- **Einsatzbereich:** Garantiert lauffähig auf **allen** AmigaOS 4 Installationen, Grafikkarten (auch ohne 3D-Treiber) und Emulatoren (wie **QEMU** oder **WinUAE**).

---

## Verzeichnisstruktur & Speicherstände

```
AnytimeGolf/
├── golf_amigaos4        # Hardwarebeschleunigte Binary (GL4ES / Warp3D Nova)
├── golf_amigaos4_soft   # Software- / MiniGL-Fallback-Binary
├── data/                # Spielressourcen (Texturen, 3D-Modelle, Sounds, UI)
├── save/                # Automatisch erstellter Ordner für Spielstände
├── README.md
└── LICENSE
```

### Spielstände & Einstellungen (`save/`)
Alle Spielstände und Konfigurationen (Tour-Fortschritt, Soundeinstellungen, Spielzustände) werden sauber im Unterordner `PROGDIR:save/` gespeichert (`save_GOLF_GS_*.dat`).
- Der Ordner `save` wird beim ersten Spielstart automatisch angelegt.
- Ältere Spielstände direkt im Hauptverzeichnis werden beim Laden automatisch erkannt (Rückwärtskompatibilität).

---

## Steuerung & Tastaturkürzel

- **Maus (Touch-Emulation):**
  - **Linke Maustaste:** Zielen, Schlägerauswahl, Klick auf UI-Elemente.
  - **Schlag ausführen:** "Swing"-Button anklicken und mit gedrückter Maustaste nach hinten und zügig nach vorne ziehen (Magic Touch Gestensteuerung).
- **Tastatur:**
  - `ESC`: Spiel beenden / Menü
  - `F11` oder `ALT + ENTER`: Vollbild / Fenstermodus umschalten
  - `U` oder `F`: Bildschirmausrichtung kippen (Upside-Down Toggle)

---

## Umgebungsvariablen (Optionale Konfiguration)

Über Shell-Variablen können Fenstergröße und Verhalten angepasst werden:

```shell
# Fenstergröße manuell vorgeben (Standard: 768x1024 bzw. Desktop-angepasst)
setenv GOLF_WIDTH 1024
setenv GOLF_HEIGHT 768

# Vollbildmodus erzwingen
setenv GOLF_FULLSCREEN 1

# Bildschirm kopfüber starten (falls gewünscht)
setenv GOLF_UPSIDEDOWN 1

# Für golf_amigaos4_soft: Reines CPU-Rendering in Mesa erzwingen
setenv LIBGL_ALWAYS_SOFTWARE 1
```

---

## Kompilierung im Docker-Container

Der Cross-Compiler im Container `4530b990e258` baut mit dem Skript `ports/amigaos4/build-in-container.sh` automatisch beide Binaries:

```sh
docker exec 4530b990e258 /opt/code/golf/ports/amigaos4/build-in-container.sh
```
