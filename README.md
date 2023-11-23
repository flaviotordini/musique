<p align="center">
<img src="https://flavio.tordini.org/files/products/musique.png">
</p>

# Musique
Musique is a music player built for speed, simplicity and style. It is written in C++ using the Qt framework. Contributing is welcome, especially in the Linux desktop integration area.

## Translating Musique to your language
Translations are done at https://www.transifex.com/flaviotordini/musique/
Just register and apply for a language team. Please don't request translation merges on GitHub.

## Build instructions
To compile Musique you need at least Qt 6.0. The following Qt modules are needed: core, gui, widgets, network, sql (using the Sqlite plugin), declarative, dbus. You also need TagLib: http://taglib.github.io and MPV >= 0.29.0: https://mpv.io/

To be able to build on a Debian (or derivative) system:

	sudo apt install build-essential libgl-dev qt6-base-dev-tools qmake6 qt6-declarative-dev libmpv-dev libtag1-dev libqt6sql6-sqlite qt6-l10n-tools

Clone from Github:

    git clone --recursive https://github.com/flaviotordini/musique.git

Compiling:

    qmake && make

On Debian, qmake has been renamed to qmake6 for Qt 6.

Running:

	build/target/musique

Installing on Linux:

    sudo make install

This is for packagers. End users should not install applications in this way.
