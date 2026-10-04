# Network item data and session records

These four expressions cross Win32 or DirectPlay boundaries. They do not reinterpret one engine class as another. The contracts below are specific to the 32-bit target; matching pointer widths alone would not prove ownership, object lifetime, or alias safety.

## Every site

| Site | Current expression | Actual conversion | Storage owner |
| --- | --- | --- | --- |
| [PopulateProviderList](../../src/Net/NetMgr.cpp#L253) | `reinterpret_cast<LPARAM>(provider)` | `CNetProviderNode*` → SDK `LPARAM` (`LONG` on this target) | A node allocated by `AddProvider`, owned by `m_providers`; the list box holds a non-owning cookie. |
| [ReadProviderSelection](../../src/Net/NetMgr.cpp#L286) | `reinterpret_cast<CNetProviderNode*>(itemData)` | `i32` → `CNetProviderNode*` | Recovers that cookie after `ListBox_GetItemData`; no new object is constructed. |
| [ReadSessionSelection](../../src/Net/NetMgr.cpp#L431) | `reinterpret_cast<CNetSessionListNode*>(itemData)` | `i32` → `CNetSessionListNode*` | Recovers a node owned by `m_sessionListings`. |
| [CreateSession](../../src/Net/NetMgr.cpp#L494) | `reinterpret_cast<LPDPSESSIONDESC2>(descriptionBytes)` | `u8*` (`unsigned char*`) → SDK `DPSESSIONDESC2*` | Temporary `new u8[descriptionSize]` buffer, filled by DirectPlay; copied before `delete[]`. |

## List-box cookies

The actual build SDK's `msvc/include/WINDEF.H` defines `LPARAM` and `LRESULT` as `LONG`. Its `WINDOWSX.H` lines 1057–1058 implement the `LB_GETITEMDATA`/`LB_SETITEMDATA` message macros. These APIs store an application value; they do not construct or own a `CNetProviderNode`.

[AddProvider and ClearProviders](../../src/Net/NetMgr.cpp#L199) allocate/delete provider nodes. [AddSessionListing and ClearSessionListings](../../src/Net/NetMgr.cpp#L341) do the same for session nodes. [PopulateSessionList](../../src/Net/NetMgr.cpp#L377) stores session node addresses through a `MsgParam` union; that is the corresponding producer even though it is not a spelled `reinterpret_cast`. It is marked as having no retail caller. These address cookies must not outlive their manager-owned nodes.

The selection readers reject an invalid selection, a selection beyond the current manager list count, `LB_ERR`, and zero item data. These checks do **not** prove that an arbitrary nonzero cookie belongs to the list, nor prevent stale cookies after a separate list clear. No new use-after-free path is established here; validity depends on the UI population/clear ordering. The casts themselves perform no misaligned memory access. Dereferencing the recovered pointer is sound only when it is the original live node of the stated type.

The pointer-to-integer-to-pointer route is supported by this Win32 implementation and its pointer-sized integer representation. The intermediate `i32` would truncate pointers in a 64-bit port. A port would need a pointer-sized item-data type throughout the API and return-value chain. Replacing these conversions with union punning or `static_cast` through unrelated types would not improve the contract. The comments at the two readers were corrected from “combo box” to “list box”; the actual messages were already list-box messages.

### Provenance

- The provider producer's C-style pointer-to-message-parameter conversion appeared in [45d9a04a1](https://github.com/sushi-shi/gruntz-decomp/commit/45d9a04a10fafb3c71c86f68e17df9f65ffb56df), which implemented the earlier API-caller stub in a provisional `LobbyGroupList.cpp` owner. [b189247e6](https://github.com/sushi-shi/gruntz-decomp/commit/b189247e63a1c3a4d52a9f4036b4c29654ceb849) rehomed it to NetMgr, retaining that conversion. [159ba712c](https://github.com/sushi-shi/gruntz-decomp/commit/159ba712c3f8c90e83c754a8418995c59f457ea3) converted the numeric cast to `reinterpret_cast`. [ae473e7ed](https://github.com/sushi-shi/gruntz-decomp/commit/ae473e7edb37015b022950a97a666cd15296fc2f) later recovered the cursor helpers and current provider local. Latest line blame, [9cf3a7be1](https://github.com/sushi-shi/gruntz-decomp/commit/9cf3a7be1eee6a0f25a701db31c69abfda22cfb5), is the SDK message-macro adoption, not the cast's origin.
- Both selection readers were reconstructed as raw integer latches in [d1aa60021](https://github.com/sushi-shi/gruntz-decomp/commit/d1aa6002146cf8e11acf17ea976e91add7499ba7). The early comment called the item data an index, but the producers store addresses. [c586b5e4c](https://github.com/sushi-shi/gruntz-decomp/commit/c586b5e4c9acee1da22420f88c9f283f7d214840) recovered provider/session ownership with typed `AddrWord` unions. [3b09d668f](https://github.com/sushi-shi/gruntz-decomp/commit/3b09d668f2f3f65c69c40ad95f8e9afc5cdc30f0) removed those union seams and introduced the current direct casts. Its stated reason was to expose the actual API/ABI conversions.

These are reconstruction commits, not preserved original NetMgr source declarations. A [DirectX 6.1 SDK sample](https://github.com/NickCis/directx-6-1-sdk-samples/blob/fcd9eac7bb7d85a331dabd4521d67891692b2255/dplay/src/dpchat/dialog.cpp#L312-L320) similarly stores an allocated connection pointer in control item data, then [retrieves/frees it](https://github.com/NickCis/directx-6-1-sdk-samples/blob/fcd9eac7bb7d85a331dabd4521d67891692b2255/dplay/src/dpchat/dialog.cpp#L528-L561). That example uses a combo box; it corroborates the cookie pattern, not Gruntz's node lifetime or original spelling.

## DirectPlay's variable-size descriptor

[CreateSession](../../src/Net/NetMgr.cpp#L474) queries the required size, allocates a byte array, passes it to `GetSessionDesc`, checks the second result, and then casts the buffer head. The build SDK's `dx/Include/dplay.h` declares `GetSessionDesc(LPVOID, LPDWORD)` and `LPDPSESSIONDESC2` as a pointer to its native descriptor. A fixed `new DPSESSIONDESC2` would omit the variable trailing strings. This is an SDK-produced record, not arbitrary file bytes guessed to have that layout.

[CNetSessionListNode::Initialize](../../src/Net/NetMgr.cpp#L1049) copies the fixed descriptor and duplicates its nonempty session-name/password strings into separate arrays. Therefore deleting `descriptionBytes` afterward does not leave those two pointers dangling. The node's destructor releases the copied strings. No pointer into the temporary buffer is deliberately retained by this path.

The heap allocation at its base supplies the target ABI's required alignment. This establishes neither a universal C++ object-lifetime rule nor bounds by itself: the code relies on DirectPlay's successful output contract to supply at least a complete descriptor and valid strings. It does not independently check `descriptionSize >= sizeof(DPSESSIONDESC2)` and ignores the first probe's HRESULT. The second call is checked and a size-change/error exits before decoding. These are existing API-contract dependencies, not newly demonstrated malformed-output exploits.

The underlying byte-buffer path first appeared in [f2343d423](https://github.com/sushi-shi/gruntz-decomp/commit/f2343d4233d2bb950cda0953ddafa1edb13150fc), with an untyped blob and incorrectly named DirectPlay slots. [6368ca83b](https://github.com/sushi-shi/gruntz-decomp/commit/6368ca83bc7324c8fe1e23966a648f9a4bacffaa) typed the descriptor consumer and introduced the cast, citing the variable-size record-plus-strings layout. Latest blame [c586b5e4c](https://github.com/sushi-shi/gruntz-decomp/commit/c586b5e4c9acee1da22420f88c9f283f7d214840) corrected API/owner names and used the SDK type.

[Wine's pinned DirectPlay client tests](https://github.com/wine-mirror/wine/blob/455e3509b98a6919fd4ad1def4803e08c41c03b2/dlls/dplayx/tests/dplayx.c#L4580-L4639) allocate output storage, exercise insufficient-size cases, and read descriptor fields/string pointers after successful calls. This supports the SDK output-buffer interpretation; it does not prove every Windows provider behaves correctly or give Gruntz portable C++ lifetime semantics.

**Disposition:** retain all four casts. A typed aligned storage adapter or copy into a live descriptor could make the last boundary more explicit, but must preserve the trailing-string lifetime and actual SDK output contract. Extra validation would be a separate behavior change requiring retail/source evidence. No SDK structure or COM signature should be rewritten to hide these conversions.
