# VC5 template members can inline without the inline keyword

tags: cpp:template cpp:inline cpp:container | asm:call asm:coff | topic:compiler-model topic:source-oracle

The old rule that `/Ob1` excludes every unmarked function was too broad.
MSVC 5.0 SP3 expands an instantiated template member whose out-of-class
body does not contain `inline`. An equivalent ordinary member remains a call.
This is a source-category distinction; no `/Ob2` assumption is required.

The controlled TU is:

```cpp
template<class T> struct Array {
    T* m_data;
    T& operator[](int i);
};
template<class T> T& Array<T>::operator[](int i) { return m_data[i]; }
struct Plain {
    int* m_data;
    int& At(int i);
};
int& Plain::At(int i) { return m_data[i]; }
int UseTemplate(Array<int>* a, int i) { return (*a)[i]; }
int UsePlain(Plain* a, int i) { return a->At(i); }
```

Compile the same file with the pinned compiler and `/nologo /c /O2 /MT`,
first with `/Ob1`, then with `/Ob0`. Inspect the caller's actual COFF relocation
sites and instructions, not merely the presence of a callee definition:

| Caller | `/Ob1` | `/Ob0` |
| --- | --- | --- |
| `UseTemplate` | 13 bytes; indexed load; zero relocations | 17 bytes; call to `Array<int>::operator[]` |
| `UsePlain` | 17 bytes; call to `Plain::At` | 17 bytes; call to `Plain::At` |

Sizes exclude alignment padding. The `/Ob1` template caller is
`8b442404 8b542408 8b08 8b0491 c3`. Both retained calls have a REL32
relocation at caller offset 10. The paired non-template body and `/Ob0` build
are negative controls against attributing this to general automatic inlining.

This explains why restoring a real template can move callers even when both
old and new emitted accessors match retail exactly. In the container audit,
the fake CString leaf and its hand-expanded accessors had hidden the original
instantiation boundary. Both recovered `zDArray` index bodies remain exact;
exposing the template/erased helper bodies changes caller expansion choices.
The controlled application results are in [the template audit](../template-model-audit.md).

For reverse use, check the actual template definition and instantiation,
including body visibility, before rejecting a candidate for lacking `inline`.
Do not infer a particular template argument from its ability to expand.
Recheck the full caller family against retail after restoring a template;
current-score movement does not invalidate independently proved type ownership.

`gruntz walls inline-model` already receives candidacy explicitly; its `marked`
field must not become a source-keyword eligibility filter. The CLI integration
control `test_unmarked_template_candidate_reaches_cli_prediction` passes an
unmarked eligible template and an unmarked ineligible ordinary member through
the public `--spec` path. Its small cost is a synthetic predictor input, not a
claimed measurement of this probe's C1 estimate. The real-compiler A/B above
separately establishes eligibility. Ordinary non-template `/Ob1` controls in
earlier patterns remain valid within that scope.
