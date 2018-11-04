<p align="center">
<img src="https://flavio.tordini.org/files/products/musique.png">
</p>

# Musique
Musique is a music player built for speed, simplicity and style. It is written in C++ using the Qt framework. Contributing is welcome, especially in the Linux desktop integration area.

## Translating Musique to your language
Translations are done at https://www.transifex.com/projects/p/musique/
Just register and apply for a language team. Please don't request translation merges on GitHub.

## Build instructions
To compile Musique you need at least Qt 5.0. The following Qt modules are needed: core, gui, widgets, network, sql (using the Sqlite plugin), script, dbus. You also need TagLib: http://taglib.github.io

To be able to build on a Debian (or derivative) system:

	$ sudo apt-get install build-essential qttools5-dev-tools qt5-qmake libphonon4qt5-dev libqt5sql5-sqlite qt5-default libtag1-dev

Compiling:

    $ qmake
    $ make

Beware of the Qt 4 version of qmake!

Running:

	$ build/target/musique

Installing on Linux:
    
    $ sudo make install

This is for packagers. End users should not install applications in this way.

## A word about Phonon on Linux
To be able to actually listen to music you need a working Phonon setup.
Please don't contact me about this, ask for help on your distribution support channels.

## Legal Stuff
Copyright (C) 2010 Flavio Tordini

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
