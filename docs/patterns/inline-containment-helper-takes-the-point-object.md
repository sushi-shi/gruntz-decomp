# A short-circuit inline containment helper takes the point object, not two scalars

## Detection signature

An open-coded four-bound containment test is structurally correct but uses an extra
callee-saved register. Moving the expression into an inline helper that takes `x` and
`y` separately removes that register, but the candidate still loads both coordinates
before the first comparison. Retail instead loads `y` only after the lower-X test has
succeeded.

That load boundary distinguishes these source shapes:

```cpp
rect.Contains(node->m_x, node->m_y); // both actual arguments are evaluated first
rect.Contains(node);                 // fields are read inside the short-circuit body
```

The second form can also be a point reference or an equivalent macro. The instructions
prove the argument's abstraction boundary, not the original identifier or whether the
owner was the rectangle versus the point type.

## Controlled A/B

`CWwdGridIter::GetNext` at `0x191c30` provided three controlled states under the pinned
VC5 `/O2` build:

| Source shape | Size / instructions | Fuzzy |
|---|---:|---:|
| open-coded four comparisons | `0xcf` / 85 | 94.244% |
| inline `Contains(x, y)` | `0xca` / 81 | 95.490% |
| inline `Contains(node)` | retail `0xcc` / 82 | 100.000% exact |

The scalar helper eagerly loaded `node->m_y` beside `node->m_x`. The object helper left
the Y load after the first conditional branch, exactly matching retail, while preserving
the inclusive maximum endpoints required by the WWD grid. Reusing the same object helper
in neighboring `CWwdGrid::Query` was byte-flat at 99.92%, a negative control against the
claim that every open-coded occurrence must move.

## Safe reverse use

When retail delays a field load across an earlier short-circuit comparison but a scalar-
argument inline hoists it, test a helper that receives the owning object or aggregate and
reads its fields internally. Keep the semantic boundary honest: do not replace half-open
Win32 `PtInRect` with an inclusive helper, and do not invent a point aggregate unless
complete-object evidence supports one. Treat an exact expansion as proof of the source
shape, not proof of an unattested historical name.
