# SoundFont Manager interface 1.0

`SFMAN100.H` supplements the existing `sfman-1.01/SFMAN.H` common SDK types
with the missing `SFMANL100API` declaration. The declaration is an exact excerpt
from [Creative/E-mu's SFMAN.H as preserved by kX](https://github.com/kxproject/kx-audio-driver/blob/19d1e856533e475b702121c42fd9463855808947/h/sfman/SFMAN.H#L438-L481).
Only the file wrapper, include and provenance comment are added; the table
itself is unchanged (line endings normalized). The original copyright is retained.

The existing common header already declares `ID_SFMANL100API`. Interface 1.0
omits the 1.01 pathname query; its bank-clear callback is consequently at offset
0x34 on x86. The [pinned implementation](https://github.com/kxproject/kx-audio-driver/blob/19d1e856533e475b702121c42fd9463855808947/kxsfman/sfman32.cpp#L505-L551)
independently distinguishes the two tables and exports `SFMANAGER` data.

Do not replace common SDK types with the newer witness wholesale: its
pointer-width adaptations and other extensions are not the VC5 SDK contract.
