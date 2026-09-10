# zoom_filter_neon — Optimization Methodology & Journal

Method + journal for optimizing `Goom/zoom_filter_neon.c`.
Design intent: `docs/specs/2026-08-08-neon-zoom-filter-design.md`.
Correctness reference: `c_zoom()` in `Goom/filters.c:403` (pure C).

This file is both the **process** (how a new optimization path is run) and the
**journal** (every attempted path, its measured effect, and its fate). A path
that is not recorded here was never tested.

---

## 1. Ground rules (invariants that never change)

- **Correctness is the hard gate, speed is secondary.** A path that trades
  correctness for speed is rejected, not merged.
- **One accepted divergence vs `c_zoom`** (from the design spec, MMX-precedent):
  Alpha byte of dest is blended by NEON (C leaves it untouched). Excluded
  from conformance comparison. The former second divergence (saturating
  `vqmovn` narrow vs truncation) was removed by path 008: the blend narrows
  with a truncating `vshrn`, exactly C's cast. It was always bit-identical in
  practice (max channel sum 65025 < 65536) and never bought anything — do
  **not** reintroduce it expecting a gain.
- Kernel stays a drop-in: same signature as `zoom_filter_xmmx`, guarded by
  `#if defined(__aarch64__)`, scalar tail keeps C semantics for `bufsize % 4`.
- Every measured claim needs a conformance run + bench run at the exact commit.

## 2. Testing & benchmarking methodology

Standalone harness, no framework: `tests/zoom_filter/`.

### Build / run

```sh
cd tests/zoom_filter
make test          # build against real Goom/filters.c + run conformance
make bench         # build + run benchmark (default 300 frames)
./zoom_filter_test --bench 1000   # custom frame count
./zoom_filter_test --bench 1000 --smooth   # smooth/continuous warp field
```

The harness links the **real** `filters.c` (`zoom_filter_c`) and
`zoom_filter_neon.o` — conformance is against the true reference, not a copy.

`--smooth` uses `gen_smooth_walk`, a continuous random-walk field that models
the real goom warp's smoothness (`zoomVector` maps adjacent output pixels to
adjacent source pixels). It is a **diagnostic** for source-adjacency
optimizations; the default random `gen_smooth` field remains the primary
bench. As of path 003 the two fields give the same kernel speed and the same
conclusions (wide-load regresses on both), so `--smooth` is kept as a
verification tool, not the default.

### Conformance (default mode)

Deterministic xorshift32 (fixed seeds) — reproducible across runs and
platforms. Covers:

- Sizes: `512x288, 720x360, 409x256, 333x201, 64x48, 5x3` — includes odd
  sizes (`409`, `5x3`) that force the scalar tail.
- `buffratio` in `{0, 1, 32768, 65535, 65536, 131072}` — spans shift/overflow
  edges.
- Two field generators: **smooth** (identity-grid + random displacement,
  exercises clip + interior) and **adversarial** (negatives, way-out-of-range,
  `INT_MIN/MAX` overflow bait).
- **Pass:** ≥99% of pixels byte-exact on RGB, max per-channel delta ≤ 2,
  alpha ignored.

Current result is **100% exact, max delta 0** — the 99%/≤2 tolerance is
headroom, not a target. A new path should keep exactness at 100% where
possible; falling to 99%/delta-2 is the outer acceptable bound, not the norm.

### Benchmark (`--bench`)

- Sizes `512x288` and `720x360`; N frames (default 300).
- Times **only the kernel**: warp-field evolution (`gen_smooth`, `memcpy`)
  runs untimed before the clocked call.
- Accumulates a checksum over dest to defeat dead-code elimination.
- Report: `ms/frame` (C and NEON), `speedup = C/NEON`, checksum.

### Machine & noise control (record with every bench)

- Machine: **Apple M1 Pro (arm64)**, macOS Darwin 25.3.0, `cc` (clang) `-O2 -Wall`.
- **Use median-of-runs, not a single `--bench`.** Speedups < ~5% are within
  noise on this machine. For a candidate decision, take median of ≥5 runs of
  `--bench 300`; quote the median and the spread.
- Compare all candidates against the **same baseline commit** and the same
  run batch, so machine state is shared.
- Do not run other load while benchmarking.

## 3. Baselines (committed / accepted)

Recorded 2026-08-08 on M1 Pro, working tree at guard tightening
(`__aarch64__`), commit basis `d121bc3c`.

| Path | Conformance | 512x288 (ms/fr) | 720x360 (ms/fr) | Speedup 512x288 | Speedup 720x360 |
|------|-------------|-----------------|-----------------|-----------------|-----------------|
| **baseline** (committed kernel) | 100% / delta 0 | C 1.28, NEON 0.47 | C 2.40, NEON 0.71 | **2.71x** | **3.38x** |
| **new** (path 001+002) | 100% / delta 0 | C 1.28, NEON 0.38 | C 2.34, NEON 0.68 | **3.37x** | **3.44x** |
| **new2** (path 007 spreadtab) | 100% / delta 0 | C 1.28, NEON 0.34 | C 2.34, NEON 0.57 | **3.76x** | **4.10x** |
| **new3** (path 008) | 100% / delta 0 | C 1.00, NEON 0.22 | C 1.76, NEON 0.39 | **4.45x** | **4.54x** |

`new3` row re-measured 2026-09-10 on M1 Pro / Darwin 25.6.0 / clang `-O2`
(`make bench`, 300 frames), same tree, same batch as its baseline (4.10x /
4.01x re-measured back-to-back). Absolute NEON time 0.246→0.221 (512x288,
**1.116x**) and 0.437→0.381 (720x360, **1.149x**), median-of-9 interleaved.

New2-baseline measured in a 5-run interleaved fair batch vs the **original**
committed kernel (f82af05d): NEON absolute time **0.45→0.34** (512x288,
**1.32x**) and **0.77→0.57** (720x360, **1.35x**). Cumulative from the
original: path 001+002 (~1.2x) then path 007 spreadtab (~1.1x more). The
remaining gap to the 2x goal is bounded by the irreducible per-pixel work:
coordinate interpolation + blend ALU (~58% of time), scattered source gather
(~21%), and coefficient index extraction + table load (~21%) — see 008.
The remaining gap to the stated 2x goal is bounded by the scattered-gather,
issue-bound structure of the kernel — see journal entries 003–006 for the
rejected paths and the reason (each added instructions or branch
mispredicts on the random-displacement warp field, which the goom FX uses).

Path 008 re-measured the per-bucket profile by **isolation** instead of
sampling (force one term to a constant, keep everything else, accept a
deliberately wrong output): with `vt` forced to 0 (coefficient-table load +
extraction gone) the kernel gains **1.25–1.32x**; with `pos = loop+b`
(coalesced source gather, all 24 loads/pixel-group kept) it gains **1.11x**.
So the coefficient path is the larger single bucket and the scattered
*addressing* of the source gather is worth ~10%, while the 16 8-byte source
loads themselves are cheap. Removing both lets clang auto-vectorize the
gathers entirely (2.35x) — an unreachable figure for a real scatter. After
path 008 the 8-pixel loop is ~129 instructions against an algorithmic floor
of ~125, so the next win must come from a different data flow, not from
better instruction selection.

Any future candidate must beat this per-pixel speedup with equal or better
conformance to be kept.

---

## 4. Optimization process (a new path)

Run as a loop, one candidate family at a time. Each iteration ends with a
commit of the kept winner.

### Step 0 — snapshot baseline
Ensure the tree is at the baseline commit. Record `git rev-parse HEAD`.
Run `make test` (green) + `make bench` (median of 5) → this is the reference.

### Step 1 — generate ideas (3 subagents)
Spawn **3 parallel subagents**, each tasked to return concrete optimization
ideas. Instruct them to:
- Read `Goom/zoom_filter_neon.c`, `Goom/filters.c` (c_zoom), the harness, and
  **this journal** — ideas already attempted (kept or rejected) are out of
  scope unless they *improve* a rejected one.
- Return a ranked list of concrete, patchable ideas (not essays). Each idea:
  file/region, the specific transform, and a one-line predicted effect.
- **Do not edit code and do not run the full suite.** They propose; the lead
  evaluates. (They may reason over the harness's measurement protocol.)

Use 3 distinct angles so coverage is broad:
1. **NEON idiom / instruction selection** — narrower ops, `vld`/`vst` width,
   `vqtbl` vs shift-broadcast, saturating ops, avoiding `vget/vset` lane moves.
2. **Memory layout / scheduling** — prefetch, gather cost, dependency chains,
   load grouping, loop unrolling, `restrict`/alignment, pointer vs index.
3. **Algorithmic / loop restructure** — scalar-gather reduction, tail folding,
   multi-row batching, buffratio special-casing, output-batching.

### Step 2 — evaluate each idea
For each idea, decide: **promising**, **uncertain**, or **unlikely**.
- Promising → implement as an **isolated, revertible change** (own edit; keep
  baseline intact for comparison; prefer a `#ifdef`-toggleable variant so
  coexistence tests stay clean).
- Correctness gate first: `make test` must stay ≥99%/≤2, and ideally 100%/0.
- Then measure: median-of-5 `make bench` vs the Step-0 baseline. Same commit
  basis, same batch.
- Record the result in the journal (section 6) regardless of outcome.
- **Keep** if it improves speed with equal/better conformance. **Reject** if
  not — but write down the concrete mechanism you believe limited it (e.g.
  "gather latency dominates", "store-forwarding stall") so it can be revisited.

### Step 3 — commit the best single
Pick the best kept idea, commit it with the journal entry (message references
the path id + measured effect, e.g. `zoom_filter_neon: path 003 — 2.71x→3.4x`).

### Step 4 — check coexistence
Multiple kept ideas often combine. Run a final subagent or the lead to pick
**orthogonal** winners and test them jointly:
- Apply all kept ideas together, `make test` + median-of-5 `make bench`.
- If the joint result is faster than any single and still conformance-clean,
  keep the combination. Otherwise drop the conflicting member(s).
- Commit the combined result as the new baseline.

### Step 5 — new baseline
The committed winner becomes the baseline for the next iteration. Update
section 3's table. Repeat.

---

## 5. Process improvements (build on the above)

Adopted into the loop above and worth restating as deliberate choices:

1. **Correctness gate strictly before timing.** Never let a tempting speedup
   slide past the delta bound — measure correctness, then speed. A faster-but-
   divergent kernel is a bug, not a win.
2. **Median-of-5, same-batch, same-commit.** Sub-5% deltas are noise on this
   machine; single-shot benches mislead. Always compare within one batch.
3. **Isolation + toggleability.** Every candidate is a revertible edit (ideally
   `#ifdef`-toggleable) so coexistence and bisection are trivial.
4. **Specialized subagents, not generic ones.** Three fixed angles (instruction
   selection / memory-scheduling / algorithm) cover the space without overlap.
5. **Negative results carry a hypothesis.** A rejected path's recorded
   "why" (mechanism, not just the number) is what makes it re-explorable later.
6. **Every entry stamped with the commit.** Revert/bisect to the exact tested
   state at any time.
7. **Versioned baseline table.** The journal's baseline row is the contract a
   candidate must beat; keeping it current prevents regressions creeping in.

Optional, if it proves worthwhile: a `make bench` smoke threshold (e.g. fail
if speedup drops below a committed floor) as a cheap perf-regression guard —
no framework, matching project convention.

---

## 6. Journal

Chronological. Each entry: **id**, **date**, **commit**, **what**, **why
(predicted)**, **conformance**, **measured effect**, **status**, **notes**.

<!-- TEMPLATE
### [NNN] — <short name>
- date / commit:
- what:
- predicted effect:
- conformance:
- measured (median-of-5 vs baseline at time):
- status: kept | rejected
- notes / why-rejected hypothesis:
-->

### 000 — baseline (committed kernel)
- date: 2026-08-08 / commit basis: `d121bc3c` (working tree guard `__aarch64__`)
- what: NEON kernel, 4 px/iter, vld2 deinterleave, scalar gather, u16 blend,
  vqmovn narrow, scalar tail.
- conformance: **100% / delta 0** (20,929,680 px).
- measured: 512x288 2.71x (C 1.28 / NEON 0.47 ms/fr); 720x360 3.38x
  (C 2.40 / NEON 0.71 ms/fr).
- status: **kept (baseline)**.

<!-- Append new entries here; keep the baseline row in section 3 in sync. -->

### 001 — branch-free gather + register extraction (kept)
- date: 2026-08-08 / commit: (this optimization round)
- what: Replace the cbz/cbnz decision-tree gather with a branch-free masked
  gather: flat index `((px&15)<<4)|(py&15)`, unconditional load
  `precal[flat] & ~clip` (clip lanes are all-ones/zero, so `& ~clip` yields 0
  exactly like c_zoom). Drop the `pos[]/clipped[]` stack round-trip; extract
  lanes straight from vectors via `vgetq_lane` (unrolled for const indices).
  Fold the clip mask into `pos` once (`pos = (px>>4 + prevX*(py>>4)) & ~clip`).
  Add `__restrict` to expix1/expix2/lbruS/lbruD; `vmull_u8(vget_low)+vmlal_high`
  instead of `vmull+vmlal` on high/low; single 16-byte store via
  `vqmovn_high_u16`.
- predicted effect: the 4 coeff loads become independent (parallel L1 hits)
  instead of a serialized branch chain; fewer stores→load round-trips.
- conformance: **100% / delta 0**.
- measured: (with 002) see table row `new`.
- status: **kept**.
- notes: the compiler further vectorizes the coeff gather into `ld1.s` lane
  inserts on a u32x4 — good codegen already.

### 002 — 8 px/iteration (kept)
- date: 2026-08-08 / commit: (this round)
- what: Widen the main loop to process 8 px/iter (two independent 4px
  batches: 2x vld2q each of lbruS/lbruD, 8-way coord/clip/pos/idx, 8 coeff
  gathers, blend as two 4px groups each stored with one 16-byte store). 4px
  remainder loop + scalar tail kept for `bufsize % 8`.
- predicted effect: doubles in-flight source-gather MLP (16 expix1 loads +
  8 coeff loads overlap) and amortizes loop/vector prologue over 8 px.
- conformance: **100% / delta 0**.
- measured: fair A/B 4px 0.40 → 8px 0.37 ms/fr @512 (~6%). 16px was worse
  (register spills) — see 005.
- status: **kept**.

### 003 — paired wide-load (rejected)
- date: 2026-08-08
- what: When `pos[b]==pos[a]+1` (source-adjacent output pair), replace the
  two 8-byte row loads with one 16-byte load + two `vext` shuffles.
- predicted effect: halve the expix1 gather count.
- conformance: **100% / delta 0**.
- measured: **regression** 0.37 → 0.50 ms/fr @512 (median-of-5) on the random
  gen_smooth field. Also **regressed on a smooth/continuous field** (the
  `--smooth` gen_smooth_walk bench, which models the real goom warp's
  continuity: pos[k+1]==pos[k]+1 mostly): 0.32 → 0.42 ms/fr. So the shuffle
  + branch overhead (2 vext + 2 vcombine + 2 vget_low per pair) outweighs the
  gather savings even when adjacency holds ~100% of the time.
- status: **rejected**.
- notes / why: the bench warp field (gen_smooth, amplitude 48) has random
  per-pixel displacement, so source-adjacency holds only ~50–60% of the time;
  the data-dependent branch mispredicts and the `vext` shuffles + fallback
  bloat the loop. Confirms the kernel is **issue/throughput-bound, not
  gather-latency-bound** on the goom FX warp. The smooth-field test proves the
  source gather cannot be cheaply reduced by wide loads even on the ideal
  (adjacent) warp — the shuffle cost exceeds the gather saving.

### 004 — software prefetch of expix1 (rejected)
- date: 2026-08-08
- what: `__builtin_prefetch` both expix1 rows for all 8 px right after pos
  extraction, before coeff gather + blend.
- predicted effect: hide ~14-cycle L2 gather latency.
- conformance: **100% / delta 0**.
- measured: **regression** 0.38 → 0.42 ms/fr @512 (median-of-5).
- status: **rejected**.
- notes / why: the 16 prfm add issue pressure; the kernel was already not
  latency-bound (see 003), so the extra instructions only slowed it.

### 005 — 16 px/iteration (rejected)
- date: 2026-08-08
- what: Widen to 16 px/iter (four 4px batches).
- predicted effect: more MLP.
- conformance: **100% / delta 0**.
- measured: **regression** 0.37 → 0.41 ms/fr @512 (fair A/B vs 8px).
- status: **rejected**.
- notes / why: register pressure (27+ live vectors in coord phase near the
  32-reg file) causes spills; 8px is the MLP sweet spot.

### 006 — vzip reduction + -O3/-mcpu (rejected / not kept)
- date: 2026-08-08
- what: Replace the `vcombine_u16(vadd low,high)` pairwise reduce with
  vzip1/vzip2/vadd/vuzp1/vuzp2; and tested -O3 / -mcpu=apple-m1 for the
  kernel object.
- predicted effect: drop slow mov.d lane moves; better scheduling.
- conformance: **100% / delta 0**.
- measured: vzip **regression** (more ops, ~0.42 vs 0.37 @512). -O3 alone
  ~2–3% within noise; -mcpu=apple-m1 not portable (would break other ARM
  targets), not adopted.
- status: **rejected**.
- notes / why: the compiler's mov.d reduction is already cheap; the extra
  permute ops cost more than they save. The kernel is at its practical
  ceiling for the current algorithm (~1.2x); reaching the 2x goal would need
  a fundamentally different data flow (the gathers are irreducible: ~2 L1/L2
  loads + a scalar coeff gather per output pixel).



### 007 — pre-expanded coefficient table (spreadtab) (kept)
- date: 2026-08-08 / commit: `ad88daa5`
- what: At kernel entry, expand precalCoef into `uint8_t spreadtab[512][16]`
  where `spreadtab[i]` = the exact byte spread `[c1,c1,c1,c1, c2,c2,c2,c2,
  c3,c3,c3,c3, c4,c4,c4,c4]` (what the old vdup+vqtbl1q produced) and
  `spreadtab[i+256]` = all zero. The blend loads `vC = spreadtab[idx]` with
  one `vld1q`, where `idx = flat + (clip&256)` folds the clip bit into the
  table offset (clipped lane → all-zero spread). Eliminates the per-pixel
  scalar coeff gather, the `& ~clip` mask, the `vdup` lane move, and the
  `vqtbl1q` spread — four operations per pixel replaced by one load. ~0.4%
  one-time setup per call (8KB memset + 256 fills).
- predicted effect: coefficient handling was the dominant cost (profiling:
  coeff gather+mask+dup+tbl ≈ 24% of time; with spreadtab it drops to ~7%
  for the vt-extraction + 16-byte load).
- conformance: **100% / delta 0**.
- measured: 5-run fair batch vs **original** kernel (f82af05d): NEON
  0.45→0.34 (512, **1.32x**), 0.77→0.57 (720, **1.35x**). Cumulative ~1.35x.
- status: **kept**.
- notes: `vld1q` compiles to `ldr q, [base, wN, uxtw #4]` — the vt index is
  used directly as a scaled byte offset, no address arithmetic. Profiling of
  the new kernel: base (coord+blend+pos-extract+store) ≈58%, scattered source
  gather ≈21%, vt-extraction + spreadtab load ≈21%. The source gather (2x
  8-byte L1/L2 loads per pixel) and the index extraction are irreducible for
  a scattered warp; the blend multiply/reduce is minimal ALU. This is the
  practical ceiling for the current algorithm — 2x would require eliminating
  the per-pixel gathers, which the goom FX's random-displacement warp makes
  impossible without an unsafe data-dependent branch.

### 008 — exact ALU simplifications: vsli index, bsl clip fold, vqsub threshold (kept)
- date: 2026-09-10 / commit: (this round)
- what: three algebraically **exact** substitutions, two places each (8px
  loop and 4px remainder loop):
  1. flat precalCoef index: `((px&15)<<4)|(py&15)` (vand+vshl+vand+vorr) →
     `vsliq_n_u32(py, px&15, 4)` — insert `(px&15)` above the low nibble of
     `py`, which also supplies the `py&15` mask for free.
  2. clip fold into the table offset: `vidx + (clip&256)` (vand+vadd) →
     `vbslq_u32(clip, vdupq_n_u32(256), vidx)` (one select).
  3. threshold + narrow: `vshrq_n_u16(x - ((x>5)?5:0), 8)` then saturating
     `vqmovn_u16` → `vshrn_n_u16(vqsubq_u16(x,5), 8)`. `vqsub` saturates at 0
     and differs from C only for `x <= 5`, where both forms shift to 0 so the
     stored byte is 0 either way; `vshrn` is the truncating narrow that C's
     `unsigned char` cast performs. This also **removes divergence #2** from
     the file header (the saturating narrow): the blend is now bit-exact, and
     alpha remains the only divergence.
- predicted effect: cut the vector-ALU op count in the two non-gather buckets
  (index build, post-blend), which are the port-limited part of the loop.
- conformance: **100% / delta 0** (20,929,680 px). Independently verified
  algebraically, not just on the harness fields: threshold identity checked
  over the entire u16 domain (0 mismatches / 65536 inputs), vsli over the
  full 16x16 nibble lattice plus `INT_MIN`/`INT_MAX`/0xffffffff probes, bsl
  against `idx + (mask&256)` for both mask states.
- measured (median-of-9, 400 frames, single-process interleaved A/B vs the
  HEAD kernel, same batch): NEON 0.246→0.221 ms/fr (512x288, **1.116x**) and
  0.437→0.381 (720x360, **1.149x**). Harness `make bench`: 512x288 speedup
  4.10x→**4.45x**, 720x360 4.01x→**4.54x**. Attribution: vqsub alone
  1.061x/1.088x, +bsl 1.089x/1.118x, +vsli 1.116x/1.149x. All byte-exact
  against the baseline variant on full 32-bit pixels.
- status: **kept**.
- notes: measured with a *single-process interleaved multi-variant rig*: every
  candidate is instantiated from one copy of the kernel via a name macro plus
  a `V_*` toggle, all variants time one frame each in round-robin, results are
  medians of N repeats, and each variant's output is byte-compared against the
  baseline variant. This gives same-batch comparisons (the journal's noise
  rule) and turns correctness into a per-candidate gate. The rig is a
  throwaway; the harness stays the committed instrument.
- coexistence: `vmull`+`vmull_high`+`vaddq` instead of `vmull`+`ext`+
  `vmlal_high`, the combine-then-add reduce, the packed-u64 extraction, and
  64-byte table alignment were each re-tested **on top of** 008 and all gave
  nothing (1.117x vs 1.116x, i.e. within noise) — 008 is kept alone.

### 009 — inline-asm fused pair load (rejected)
- date: 2026-09-10
- what: replace `vcombine_u8(vld1_u8(p), vld1_u8(p+prevX))` (which clang
  expands to 2 `ldr d` + `mov.16b` + `mov.d` register juggling) with an
  asm-pinned `ld1 {%0.8b},[p]` / `ld1 {%0.d}[1],[p+stride]` pair.
- predicted effect: save the two register-move ops per gathered pixel.
- conformance: 100% / delta 0 (byte-exact).
- measured: **regression**, 0.982x (512) / 0.990x (720) alone; 0.976x when
  combined with the reduce rewrite.
- status: **rejected**.
- notes / why: the asm barrier prevents clang from scheduling the loads against
  the surrounding blend and forces both halves into one register early, which
  costs more than the moves it removes. clang's mov sequence is fine.

### 010 — packed (pos | vt<<32) u64 lane extraction (rejected)
- date: 2026-09-10
- what: `vzip1q_u32`/`vzip2q_u32` the pos and vt vectors so a `u64` lane read
  yields both indices, halving SIMD→GPR transfers (8 instead of 16 per 8 px).
- predicted effect: the 16 `umov`/`fmov` per 8 px are a scarce-port bottleneck.
- conformance: 100% / delta 0.
- measured: **no gain**, 1.000x–1.031x. A diagnostic that removed *only* the
  vt extraction (keeping the 16-byte table load, wrong output) gained just
  1.05–1.07x, while removing the table load as well gained 1.25–1.32x.
- status: **rejected**.
- notes / why: the kernel is not transfer-throughput-bound; the coefficient
  *load* plus its live-state pressure is what the vt path actually costs. Also
  relevant: fire-and-forget idea 006 (vzip reduce) failed for the same reason.

### 011 — vld1q x2 + vuzp instead of vld2q coordinate loads (rejected)
- date: 2026-09-10
- what: `vld2q_s32` (deinterleaving load) → two `vld1q_s32` + `vuzp1q`/`vuzp2q`.
- predicted effect: Apple's `ld2` is a permute-class load; plain loads + uzp
  might be cheaper.
- conformance: 100% / delta 0.
- measured: **no gain**, 0.994x–1.025x across batches (sign flips run to run).
- status: **rejected**.
- notes / why: on Firestorm `ld2.4s` is evidently *not* penalised enough to
  beat two loads plus two shuffles.

### 012 — vmull/vmull_high/vaddq instead of ext+vmull/vmlal_high (rejected)
- date: 2026-09-10
- what: `vaddq_u16(vmull_u8(lo,lo), vmull_high_u8(vP,vC))` to avoid the
  `ext.16b` clang emits when materialising `vget_high_u8`.
- predicted effect: move work off the shuffle port onto the multiply port.
- conformance: 100% / delta 0.
- measured: 0.998x (512) / 1.021x (720) alone — at the noise floor; no gain
  on top of 008.
- status: **rejected** (not merged; op count is identical).
- notes / why: `ext` and the extra `vaddq` are interchangeable in cost here.

### 013 — 64-byte alignment of `spreadtab` (rejected)
- date: 2026-09-10
- what: `__attribute__((aligned(64)))` on the stack table, on the theory that
  `uint8_t[512][16]` has alignment 1 and every 16-byte `vld1q` could be
  misaligned or line-straddling.
- predicted effect: cheaper coefficient loads.
- conformance: 100% / delta 0.
- measured: **no effect**, 0.999x–1.024x; no gain on top of 008.
- status: **rejected**.
- notes / why: clang already places the 8KB local on a 16-byte boundary and
  the loads are 16-byte aligned by construction (index scaled by 16).


