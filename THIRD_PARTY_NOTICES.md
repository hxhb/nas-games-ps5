# Third-party notices

Game Center uses libsmb2 6.2, which is licensed under the GNU Lesser General Public
License, version 2.1 or later. The release source archive includes the complete
library sources and the licence at `.deps/libsmb2-6.2/LICENCE-LGPL-2.1.txt`.
Users may modify their copy of the library and reverse engineer the linked
executable to debug those modifications as allowed by the LGPL. Build and relink
instructions are in [docs/development.md](docs/development.md).

The PS5 Payload SDK v0.50 provides the PS5 toolchain and linked startup code. The
SDK release sources, including their GPL-3.0-or-later and other applicable licence
texts, are included in the release source archive under `dependencies/`.

The source headers for stb_image, stb_image_resize2 and stb_image_write are in
`apps/nasgames/third_party/stb/`. The headers contain their applicable notices
and licence text.

The source files for the hello and browser examples retain the original licence
and copyright notices from the PS5 Payload SDK examples.
