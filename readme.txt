=======
AC3File
=======

AC3File is DirectShow AC3 and DTS file source filter. Required to playback 
raw AC3 and DTS files. Supports much of different format variations: 
8/14/16 bit big/low endian bitstreams.

Copyright (c) 2006 by Alexander Vigovsky (xvalex@mail.ru)


License:
========

Distributed under GNU General Public License version 2.
You may find it in GNU_eng.txt at english language and GNU_rus.txt at russian 
language. Russain language version is for information purpose only and english 
version have priority with all variant reading.

This application may solely be used for demonstration and educational purposes. 
Any other use may be prohibited by law in some coutries. The author has no 
liability regarding this application whatsoever. This application may be 
distributed freely unless prohibited by law.

This product distributed in hope it may be useful, but without any warranty; 
without even the implied warranty of merchantability or fitness for a 
particular purpose and compliance with any standards. I do not guarantee 
any support, bug correction, repair of lost data, I am not responsible 
for broken hardware and lost working time. And I am not responsible for 
legality of reproduced with this program multimedia production.


Compilation:
============

To compile this project at least VC++ 6.0 is required with PlatformSDK or 
DirectX SDK installed (at least one of them).

Include path should have this lines first:

...\...SDK\Samples\Multimedia\DirectShow\BaseClasses
...\...SDK\include

Libraries path should have this lines first:

...\...SDK\lib

Required libraries are strmbase.lib (release) and strmbasd.lib (debug) in
libraries path. If it is not shipped with SDK you can compile this project:
...\...SDK\Samples\Multimedia\DirectShow\BaseClasses\baseclasses.dsw
(read MSDN about how to setup environment to use DirectShow Base Classes)

AC3File project has dependency on the valib library project so 
by default directories should be configured as follows:

...\ac3file       - ac3file project
...\valib         - valib project
...\valib\lib     - valib library
...\valib\valib   - valib include & source files

You may checkout both modules from the CVS:
CVS root: :pserver:anonymous@cvs.sourceforge.net:/cvsroot/ac3filter
Modules: ac3file, valib


Links:
======

http://ac3filter.net                         - Project home page
http://ac3filter.net/forum                   - Support forum
http://sourceforge.net/projects/ac3filter    - Sourceforge project page


Donate:
=======

http://order.kagi.com/?6CZJZ&lang=en


Contact Author: 
===============

mailto:xvalex@mail.ru?Subject=AC3File

Please write in English or Russian. Subject should have 'AC3File' word 
otherwise it may be accidentally deleted with tons of other junk mail.
