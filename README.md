# AsciiBouncingBalls

Hab hier versucht aus dem 1D elastischen Stoß, den wir in der Oberstufe kennengelernt haben einen schrägen Stoß abzuleiten.
Leider ist die Energie in dem System nicht konstant, also irgendwo wird wohl noch ein kleiner Fehler sein, weil weicht nur langsam ab.

Die Geschwindigkeit der Bälle wird bei jedem Stoß ( Wand oder anderer Ball ) mit einer Dämpfung multipliziert ( typischer Wert: .8 ).

Ich habe ncurses verwendet, um Maus Events zu bekommen, leider hab ich das erst in der Mitte irgendwann gemacht und war dann zu faul um die ANSI Escape Codes rauszuschmeißen,
also benutze ich jetzt eine Mischung von beidem :)

GUI mit raylib war mir zu langweilig und was sieht schon besser aus als Grafiken, die im Terminal dargestellt sind.

## Installation von ncurses
Ich habe nur ein Macbook, also konnte ich es nicht auf anderen Plattformen testen / war mir zu viel Aufwand es zu auf anderen Plattformen zu testen. Deshalb:
hat ✨Chatty✨ für mich die Anleitung geschrieben, ich hoffe sie stimmt. Auf MacOs sollte es auf jedenfall funktionieren:)

Sorry, dass ich kein cmake oder so benutzt habe, aber das repo hat ja auch nur ein Sourcefile hehe

### Linux (Ubuntu / Debian / Arch / etc.)

Unter Linux ist ncurses in fast allen Paketmanagern direkt verfügbar:

    sudo apt install libncurses-dev   # Ubuntu / Debian
    sudo dnf install ncurses-devel    # Fedora
    sudo pacman -S ncurses            # Arch Linux

### macOS (Homebrew)

    brew install ncurses

**Compile-Beispiel (clang):**

    clang -o out main.c -lncursesw \
      -I/opt/homebrew/opt/ncurses/include \
      -L/opt/homebrew/opt/ncurses/lib

### Windows

Windows unterstützt ncurses nicht nativ. Zwei funktionierende Optionen:

#### MSYS2

    pacman -S mingw-w64-x86_64-ncurses
    x86_64-w64-mingw32-gcc -o out.exe main.c -lncursesw

#### Cygwin

    gcc -o out main.c -lncursesw
