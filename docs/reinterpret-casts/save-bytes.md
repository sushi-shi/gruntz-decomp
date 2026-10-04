# Save-slot byte transforms

## Every site

Both casts convert a pointer to a live `SaveSlot` into `u8*`. [Ints.h](../../include/Ints.h#L5) defines `u8` as **unsigned char**, not an unrelated integer alias.

| Site | Current snippet | Object and lifetime |
| --- | --- | --- |
| [CSaveGame::ComputeAll](../../src/Io/SaveGame.cpp#L144) | `sum += Encode(reinterpret_cast<u8*>(GetSlot(i)));` | A member of the owner's `m_slots` array, transformed in place before writing the save file. |
| [CSaveGame::Verify](../../src/Io/SaveGame.cpp#L158) | `sum += Decode(reinterpret_cast<u8*>(GetSlot(i)));` | The same live slot, decoded in place and included in the stored checksum comparison. |

[GetSlot](../../src/Io/SaveGame.cpp#L297) bounds-checks its index and returns `&m_slots[i]`. Both callers iterate exactly `SAVE_SLOT_COUNT`; the [array declaration and record](../../include/Io/SaveGame.h#L33) share that count. Neither cast allocates storage or changes ownership.

## Representation, bounds, and mutation

[Encode and Decode](../../src/Io/SaveGame.cpp#L269) traverse `sizeof(SaveSlot)` bytes, XOR each byte with its index, and accumulate a weighted sum of the plaintext byte. They intentionally modify the object representation. Calling this only a checksum view would omit that important effect: `Verify` is not a const operation or an idempotent inspection of an already-decoded slot.

Access to object representation through `unsigned char` is the relevant aliasing permission. This is fundamentally different from interpreting an arbitrary byte array as a newly created `SaveSlot`. No stricter alignment is required for these byte accesses, and the live array element supplies the entire extent. Equal sizes alone are not the justification.

The record currently contains integer/b32 fields, character arrays, and unions, with no pointer, `CString`, virtual table, or destructor-managed payload. [Save](../../src/Io/SaveGame.cpp#L100) encodes, writes the slots, then decodes through `Verify`; [Load](../../src/Io/SaveGame.cpp#L83) reads the representation and decodes before normal use. Structured field reads during the encoded interval would be inappropriate. Exception/failure paths between encoding and decoding need their own review; the cast does not provide transaction safety. The fixed MSVC/x86 layout and byte order are part of the save-file contract, not portable serialization guarantees.

The whole representation includes any padding that the target layout has. The casts do not initialize padding, validate a loaded file, or establish which union member future code may read. Initial setup zeroes complete slots, but `Load` ignores short-read counts; truncated input is a separate existing source risk. This audit did not independently establish that risk's retail provenance and does not change it. The byte access itself is not a confirmed reconstruction defect.

## History and external comparison

[9af23eff6](https://github.com/sushi-shi/gruntz-decomp/commit/9af23eff6819ebdb2d0a6b95ccd6f2b1e1caf4cc) first reconstructed both operations with C-style `unsigned char*` casts after correcting the class from a misattributed file-I/O owner to `CSaveGame`. Its evidence was the retail slot loop and encode/decode operations. [bee8f0886](https://github.com/sushi-shi/gruntz-decomp/commit/bee8f0886cb36d95b3d469c80f1d3b80d4bec882) changed type spellings to the fixed-width aliases. Current cast blame is [6d500a7ea](https://github.com/sushi-shi/gruntz-decomp/commit/6d500a7eab0cec44cf10b2e6e983867f4f5645bc), the mechanical C-style-to-named-cast sweep; it did not introduce this representation access. No original Gruntz source signature is claimed.

Surviving related LithTech [NOLF CRC32::CalcDataCRC](https://github.com/osgcc/no-one-lives-forever/blob/dfbe22fb4cc01bf7e5f54a79174fa8f108dd2f54/NOLF/Shared/CRC32.cpp#L62-L95) converts a generic input pointer into a character pointer and processes its bytes. This is a related-source example of a byte-oriented checksum boundary, **not** the source of Gruntz's weighted XOR transform: it neither mutates the input nor supplies this save-record layout. [zlib's Adler routine](https://github.com/madler/zlib/blob/51b7f2abdade71cd9bb0e7a373ef2610ec6f9daf/adler32.c#L61-L90) likewise consumes a byte buffer and explicit extent; its algorithm is different.

**Disposition:** retain both casts. Retyping the routines to `SaveSlot*` would move the byte conversion into their implementations rather than eliminate the representation boundary; original signature evidence would be needed. Encoding a separate serialized buffer could improve exception behavior and portability, but would change the observed in-place mutation and file contract. Neither change is justified merely by the presence of `reinterpret_cast`.
