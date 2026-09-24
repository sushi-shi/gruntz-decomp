# Registrar local ownership selects nested inline cuts

tags: cpp:macro cpp:local cpp:reference cpp:template | asm:call asm:push | topic:inline-budget topic:evaluation-order

`RegisterGruntActions` (0x5be30) is exact using the same typed name and handler
accessors at all nineteen sites. No raw/typed API selector or hand-expanded
construction loop is needed. The shared ZTools accessor, placement-new and
error-helper family remains unchanged from `c7b4df129`.

## Controlled composition

The starting inline `ToActHandler` cast has 749 instructions against retail's
734. Converting the helper to a macro alone declines too many nested indexers
(686 instructions). That first dip does not reject the macro boundary.

All rows below use the macro conversion and sibling registration scopes:

| Caller-owned entities / assignment form | Instructions | Typed handler / name calls |
| --- | ---: | ---: |
| Named name-slot reference | 702 | 3 / 3 |
| Add the converted `CActHandler` value | 718 | 2 / 2 |
| Add one registry reference outside all nineteen registrations | 734 | 2 / 1 |
| Replace the name-slot reference with a handler-slot reference; assign the name in one expression | **734, exact** | **2 / 1** |

The penultimate row has the complete retail call set and control-flow
skeleton, but its named `CString&` sequences the final name lookup before
the right-hand-side literal is pushed. Retail pushes the literal before the
lookup. The final row keeps a named destination at the handler store instead:

```cpp
// In the registrar:
CActReg& registry = CActRegPool<CGrunt>::s_table;

// In each registration macro's create arm:
ActInsertId(key, g_typeCounter);
id = g_typeCounter;
g_typeColl[g_typeCounter] = key;
++g_typeCounter;

// In the conversion and store macros:
CActHandler converted = ToActHandler(handler);
CActHandler& slot = registry[id];
slot = converted;
```

`ToActHandler` is the reviewed primary-base `static_cast<CActHandler>` macro,
not a union view; see the [real-VC5 conversion controls](pmf-si-base-4byte-under-mi.md).
Each local owns a real receiver, converted value or returned destination. No
unused declaration, forcing pragma or fabricated cost remains. This is a
retail-exact reconstruction, not a claim that the game's registrar source
survives.

## Evidence and negative controls

The exact body is 2,533 bytes, 734 instructions, 113 calls, 73 branches, one
return and 358 relocations. All 340 semantic referents agree in order,
including nineteen handler targets. The retained typed handler calls are at
R/S; the sole retained typed name call is at S. The other 35 indexer sites
retain the nested raw call through compiler expansion.

Adding both destination references overshoots the desired inline population.
Replacing the name destination with a named string-key input gives 718
instructions: final assignment order is right but R's name accessor still
does not expand. A source-typed eight-byte PMF local introduces unwanted
growth/error expansions. A whole find-or-register-name inline helper retains
nine calls absent from retail. These controls rule out treating an equal
local count, a single better percentage or an extra helper as sufficient.
No numerical compiler budget was inferred or claimed measured.

The old special final-S macro is unnecessary on this composition; all nineteen
sites use one registration macro. This falsifies the earlier claim that the
last source site needs a distinct accessor or index spelling.

## Detection and safe reverse use

When a repeated typed operation expands differently only near the end of a
large caller, inspect the actual receiver/result ownership and conversion
boundary before manufacturing different APIs for those sites. Preserve a
structurally converging base and compose independently justified locals. Once
the call set agrees, compare argument scheduling from the first divergence:
a named returned reference can impose a sequence absent from one expression.
Equal instruction totals are not enough; changed same-sized call targets can
be visible only in the ordered-referent audit.
