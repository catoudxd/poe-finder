# Poe Finder (Dusklight-Mod)

Listet alle 60 Poe-Seelen aus Twilight Princess mit Fundort auf und zeigt anhand der
Spielstand-Flags, welche noch fehlen. Gebaut gegen Dusklight v2.0.3.

## Benutzen

1. `poe_finder.dusk` in den Mods-Ordner kopieren (oder auf das Dusklight-Fenster ziehen):
   - Windows: `%APPDATA%\TwilitRealm\Dusklight\mods`
   - Linux: `~/.local/share/TwilitRealm/Dusklight/mods`
   - macOS: `~/Library/Application Support/TwilitRealm/Dusklight/mods`
2. Spielstand laden, dann im Dusklight-Menü **Mods → Poe Finder** öffnen.
3. Oben steht die Liste der fehlenden Poes, darunter alle 60 nach Gebiet.

## Selbst bauen

Native Mods müssen pro Plattform kompiliert werden.

- **Über GitHub (alle Plattformen, kein Compiler nötig):** diesen Ordner als Repository zu GitHub
  hochladen. Der mitgelieferte Workflow (`.github/workflows/build.yml`) baut automatisch;
  unter *Actions → letzter Lauf → Artifacts* liegt `mod-combined` mit einer `.dusk` für
  Windows, Linux, macOS, Android und iOS.
- **Lokal:** `cmake -B build -G Ninja && cmake --build build` (Windows: in der
  "x64 Native Tools"-Eingabeaufforderung von Visual Studio 2022). Ergebnis: `build/mods/poe_finder.dusk`.
