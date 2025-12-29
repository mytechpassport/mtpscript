# **MTPScript Language Specification**
**Version 5.1**

---

## 0. Executive Definition (One-Page)

MTPScript is a **serverless-first, deterministic API language** for regulated environments.
It compiles to a **constrained JavaScript subset**, executed by a **sealed MicroQuickJS runtime** with:

* Zero ambient authority
* Zero hidden I/O
* Zero cross-request state
* Explicit capability declaration
* Per-request sandbox isolation ➜

JavaScript is **not** the language—it is an **execution encoding**.

---

## 0-a. Execution-Isolation Model ➜

MTPScript guarantees **per-request sandbox isolation** with **sub-millisecond reuse overhead**:

1. Build-time snapshot (`app.msqs`) – immutable, 150–400 kB
2. Runtime `clone_vm()` – copy-on-write, ≤ 60 µs **best-case**; **≤ 1 ms worst-case under EFS/FUSE cold page fault**
3. VM discarded after every request – no `fork()`, **secure wipe performed on pages that touched PCI-classified data**
4. Host effects re-injected per VM – static initialisers run **once per VM**, **after** effect seed injection, never reused

This removes cold-start latency while satisfying **“No cold state reuse”**.

---

## 0-b. Deterministic Seed Algorithm ➜

The **deterministic seed** injected into every VM is the **SHA-256 hash** of the **canonical concatenation**:

```
seed = SHA-256(
  AWS_Request_Id       ||
  AWS_Account_Id       ||
  Function_Version     ||
  "mtpscript-v5.1"     ||   // literal constant
  Snapshot_Content_Hash // SHA-256 of app.msqs
)
```

All conforming runtimes **must** produce the **same 32-byte seed** for the **same input byte sequence**; this seed is **never reused** across requests.

---

## 1. Design Goals (Hard Constraints)

### 1.1 Primary Goals
* Deterministic execution semantics (bit-exact SHA-256 response) ➜
* Explicit authority via effects
* Strong auditability
* Serverless suitability (AWS Lambda custom runtime)
* Mechanical migration from TypeScript APIs

### 1.2 Explicit Non-Goals
* Classes & inheritance
* Reflection / introspection
* Metaprogramming / macros
* Dynamic code loading
* Shared mutable state
* Threads or concurrency primitives
* Implicit coercions or I/O
* Floating-point math

---

## 2. Determinism Model (Auditor-Safe) ➜

| Guarantee | Status | Normative Requirement |
|-----------|--------|------------------------|
| Deterministic execution | ✅ | Same input bytes → same output bytes (SHA-256) |
| Deterministic hashing | ✅ | FNV-1a 64-bit + **deterministic CBOR (RFC 7049 §3.9)** |
| Deterministic equality | ✅ | Structural, total |
| Deterministic serialization | ✅ | Canonical JSON (RFC 8785) **+ duplicate-key rejection** |
| Deterministic API behaviour | ✅ | Host effects replay-identical **using seed per §0-b** |
| Bit-identical JS | ❌ Not claimed | – |
| Bit-identical VM bytecode | ❌ Not claimed | – |

---

## 3. Syntax & Grammar (Locked)

*(EBNF unchanged except pipeline associativity – left-associative ➜)*
**Addition:**
```
expr ::= ...
       | await expr   // only inside `uses { Async }`
```

---

## 4. Type System

### 4.1 Primitive Types
| Type | Notes |
|------|-------|
| `Int` | Signed 64-bit, checked overflow |
| `Bool` | `true` / `false` only |
| `String` | Immutable UTF-8 |

### 4.2 Composite Types
Records and algebraic data types (unchanged).

### 4.3 No `null`, No `undefined`
Use `Option<T>` and `Result<T, E>`.

---

## 4-a. Decimal / Money ➜

```mtp
type Decimal {
  value: String   // canonical integer significand, **1–34 digits**, no leading zeros
  scale: Int      // 0 ≤ scale ≤ 28 (IEEE-754-2008 decimal128)
}
```

* Rounding: **round-half-even**; **ties to even** as required by IEEE-754-2008 **clause 4.3.2**
* Overflow: `Result<Decimal, Overflow>`
* Comparison: **constant-time** algorithm; normalise to **larger scale**, then compare significands
* Serialization: shortest canonical string (no trailing zeros)

---

## 5. Equality, Ordering & Hashing ➜

* Equality: structural, total, no reference identity
* Ordering: only `Int` and `String`
* Hash: FNV-1a 64-bit of **deterministic CBOR (RFC 7049 §3.9)**
* Map key order:
  1. Type tag
  2. Hash
  3. CBOR byte-wise tie-break

Functions **excluded** from map keys.
**Closure environments are included in structural equality.**

---

## 6. Control Flow & Execution

* All values immutable
* `if` must have `else`, both branches same type
* Pattern matches exhaustive, compiler-checked
* Recursion bounded by **gas** (10 M β-reductions) ➜
  **Gas cost table appended in Annex A – every IR opcode and built-in carries a fixed cost; tail calls cost 0.**

---

## 7. Effect System (Authority Model)

Effects represent **capabilities**.
Lambdas are **pure**; only named functions may use effects.
Host effects **must** be deterministic functions of their arguments + **request seed per §0-b** ➜

**Built-in effects:**

| Effect | Capability |
|---|---|
| `DbRead`, `DbWrite` | SQL execution |
| `HttpOut` | Outbound HTTP |
| `Log` | Structured logging |
| `Async` | ➜ **Deterministic async I/O** (see §7-a) |

---

## 7-a. Async Effect (Deterministic Await) ➜

```mtp
effect Async {
  await<T>(promiseHash: String, contId: Int, effectArgs: Json): Result<T, Err>
}
```

**Surface syntax:**

```mtp
api POST /invoice
uses { Async, DbWrite } {
  let rate = await httpGet("https://fx.example.com/usd-eur")   // desugars to Async.await
  let total = amount * rate
  DbWrite.insert("invoice", total)
  respond json({ total })
}
```

**Compile-time desugaring:**

```
let x = await e
≡
let contId   = freshInt()
let ph       = sha256(cbor(e))
let x        = Async.await(ph, contId, e)
```

**Host adapter contract:**

1. Block-synchronously execute the I/O.
2. Cache response bytes keyed by `(seed, contId)`.
3. Return **identical** bytes on every replay.
4. **No JavaScript event loop visible inside VM.**

---

## 8. API System (First-Class)

```mtp
api POST /users
uses { db, log } {
  let user = db.insert(...)
  log.info("created user", user)
  respond json(user)
}
```

* Compile-time OpenAPI generation **with deterministic field ordering and $ref folding rules (Annex B)**
* No hidden middleware

---

## 9. JSON Model ➜

```mtp
type Json {
  | JsonNull           // **inhabited only through parsing; no MTPScript literal produces JsonNull**
  | JsonBool(Bool)
  | JsonInt(Int)
  | JsonDecimal(Decimal)
  | JsonString(String)
  | JsonArray(List<Json>)
  | JsonObject(Map<String, Json>)  // **duplicate keys rejected at parse time**
}
```

* Parsing returns `Result`
* Output **canonical JSON** (RFC 8785 + Decimal form)

---

## 10. Module System

* Static imports only
* Git-hash pinned, **signed tag required**, vendored at build
* Order-independent compilation

---

## 11. Package Manager (v1)

* Git-hash based, **git-tag signature required**, no runtime network
* npm bridge via **explicit unsafe adapters**
* Produces audit manifest:

```json
{ "unsafeDeps": ["uuid@9.0.1"] }
```

---

## 12. Compilation Pipeline

```
MTPScript
 → AST
 → Typed IR
 → Effect-checked IR
 → Deterministic JS Subset
 → MicroQuickJS Bytecode
 → VM Snapshot (.msqs) ➜ **ECDSA-P256 signature appended**
```

Forbidden JS: `eval`, `class`, `this`, `try/catch`, loops, global mutation.
**MicroQuickJS patched to forbid double-path for integers > 2⁵³-1.**

---

## 13. Runtime Model ➜

* One **fresh VM** per request (snapshot clone)
* Fixed memory budget (no shared heap)
* VM discarded after response; **secure wipe executed on sensitive pages**
* Host effects injected **per VM**, **after** static init, **deterministic seed per §0-b**

---

## 14. Serverless Deployment (AWS Lambda)

* Custom runtime ships **native binary** + **app.msqs** + **signature certificate**
* Cold-start ≤ 1 ms **best-case**; **≤ 2 ms worst-case** under EFS page fault
* No Node.js, no state reuse
**Runtime verifies ECDSA signature of app.msqs before mapping; abort on mismatch.**

---

## 15. Local Web Server (Reference)

```mtp
serve { port: 8080, routes: [...] }
```

Identical semantics to Lambda; uses same snapshot clone path.

---

## 16. Error System

* Typed error codes
* No stack traces in prod
* Deterministic error shapes (canonical JSON)

---

## 17. TypeScript → MTPScript Migration

Mechanical transforms (unchanged).

---

## 18. Security & Audit Posture

Supports SOC 2, SOX, ISO 27001, PCI-DSS.
Authority explicit, behaviour deterministic, runtime sealed, surface minimal.
**Reproducible builds enforced by containerised build image pinned by SHA-256 and signed build-info.json.**

---

## 19. Final Positioning Statement

> MTPScript is a serverless-first, deterministic API language that uses JavaScript as a constrained execution format under a sealed, per-request sandbox runtime, designed for regulated environments where auditability and explicit authority matter more than dynamism.

---

## 20. HTTP Server Support

Same snapshot-clone isolation; not user-programmable.

---

## 21. npm Bridging (Unsafe Boundary) ➜

* Adapters live **outside** MTPScript in `host/unsafe/*.js`
* Adapters **must** be **pure functions** of arguments + **deterministic seed (§0-b)**
* **Type signature** enforced:
```js
function adapterName(seed: Uint8Array, ...args: JsonValue[]): JsonValue
```
* No `require()` inside MTPScript, no shared state, no exceptions escaping
* Audit manifest lists every unsafe dependency **and its content-hash**

---

## 22. VM Snapshot Lifecycle ➜

```
Build
  └── mtp compile --snapshot app.mtp → app.msqs
  └── sign app.msqs with ECDSA-P256 → app.msqs.sig

Runtime (per request)
  ├── verify app.msqs.sig against embedded certificate
  ├── map app.msqs read-only
  ├── clone_vm()               // 60 µs–1 ms COW
  ├── inject deterministic effects **after** static init
  ├── execute
  └── drop_vm() **+ secure wipe on sensitive pages**
```

No memory-wipe for non-sensitive data; **zero cross-request leakage**.

---

## 23. Canonical JSON Output ➜

* Object keys ordered by §5 rules
* Decimal shortest form, no `-0`, no `NaN`, no `Infinity`
* Array order preserved from source literal (left-associative)
**Output byte sequence is hashed with SHA-256 to produce the deterministic claim in §26.**

---

## 24. Union Exhaustiveness (Link-Time) ➜

* Union carries content-hash of variant list
* Link step fails if any unit sees different variant set
* Guarantees exhaustive matches without runtime checks

---

## 25. Pipeline Operator Associativity ➜

Left-associative:
`a |> b |> c ≡ (a |> b) |> c`
Generated JS **α-equivalent** across all compilers.

---

## 26. Formal Determinism Claim ➜

> For every MTPScript program P, compiler version C, and input byte sequence I, the SHA-256 of the HTTP response body is identical across all conforming runtimes **after canonical JSON encoding per §23**, **using the deterministic seed algorithm per §0-b**, **and assuming deterministic CBOR per §2**.

---

**Annex A – Gas Cost Table (Normative)**
(available as machine-readable CSV in repo `/gas-v5.1.csv`)

**Annex B – Deterministic OpenAPI Generation Rules**
(available as JSON schema in repo `/openapi-rules-v5.1.json`)

--------------------------------------------------------
End of Specification 5.1