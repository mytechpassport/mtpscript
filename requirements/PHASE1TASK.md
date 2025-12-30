# Phase 1: MTPScript Compiler & Runtime Infrastructure (v5.1)

This phase involves building the **MTPScript** language toolchain on top of the hardened MicroQuickJS engine. The compiler must be written entirely in **C** with zero external dependencies (no Node.js/npm).

## 1. Compiler Frontend (P0)
- [x] **Lexer**: C implementation of the MTPScript tokenizer.
- [x] **Parser**: Recursive descent parser with pipeline operator support (left-associative per §25) and basic `api` block parsing.
- [x] **AST**: Robust C struct representation including `await` expressions and `Decimal` literals.
- [x] **Source Mapping**: Accurate line/column tracking for error reporting.

## 2. Type System (P0)
- [x] **Basic Type Checking**: Function, variable, and literal type validation.
- [x] **Structural Typing**: Implementation of structural type equivalence.
- [ ] **Immutability by Default**: All variables and structures are immutable.
- [x] **Basic Decimal Type**: Decimal arithmetic and string conversion (1–34 digits, 0–28 scale).
- [x] **Core Types**: Built-in `Option<T>` and `Result<T, E>` (No `null` or `undefined`).
- [x] **Equality & Hashing**: FNV-1a 64-bit implementation; closure environments included in structural equality (§5).
- [ ] **Exhaustive Matches**: Validation of match statements and link-time union variant checks via content-hashing (§24).
- [ ] **JsonNull constraint**: `JsonNull` inhabited only through parsing; no literal support (§9).

## 3. Module & Package System (P1)
- [ ] **Module System**: Static imports, git-hash pinned, signed tag required, and vendored imports (§10).
- [ ] **npm Bridging**: Generation of audit manifests for unsafe adapters in `host/unsafe/*.js` with content-hashes (§21).
- [ ] **Map Constraints**: Implementation of deterministic key ordering (Tag → Hash → CBOR) and function exclusion (§5).

## 4. Effect System (P1)
- [x] **Basic Effect Validation**: Effect declaration checking for declared vs. actual effects.
- [ ] **Effect Tracking**: Identification of side-effecting operations (DbRead, DbWrite, HttpOut, Log).
- [ ] **Async Effect**: Compile-time desugaring of `await e` into `Async.await(ph, contId, e)` (§7-a).
- [ ] **Signature Validation**: Ensuring named function signatures declare all used effects; lambdas remain pure (§7).
- [ ] **Runtime Enforcement**: Capability-based blocking of undeclared effects and block-synchronous I/O execution (§7-a).

## 5. Code & Bytecode Generation (P1)
- [x] **Basic JavaScript Lowering**: Translating MTPScript AST to JavaScript.
- [ ] **JavaScript Lowering**: Translating MTPScript AST to deterministic, α-equivalent JS subset (§12).
- [x] **Pipeline Associativity**: Left-associative (`a |> b |> c ≡ (a |> b) |> c`) with α-equivalent JS generation (§25).
- [ ] **Constraint Enforcement**: Ensuring no `eval`, `class`, `this`, `try/catch`, or loops in generated output (§12).
- [ ] **MicroQuickJS Bytecode**: Compiling hardened JS into signed `.msqs` compatible binary.
- [ ] **Integer Hardening**: Patching MicroQuickJS to forbid double-path for integers > 2⁵³-1 (§12).

## 6. Standard Library & Error System (P1)
- [x] **Basic Snapshot System**: .msqs file creation with bytecode packaging.
- [x] **Basic JSON Serialization**: RFC 8785 Canonical JSON for basic types (int, string, bool, null).
- [ ] **Serialization**: RFC 8785 Canonical JSON (duplicate-key rejection) and RFC 7049 §3.9 Deterministic CBOR (§2).
- [ ] **Decimal Serialization**: Shortest canonical form, no `-0`, `NaN`, or `Infinity` (§23).
- [ ] **Hashing & Crypto**: FNV-1a 64-bit, SHA-256, and ECDSA-P256 signature verification primitives.
- [ ] **JSON Model**: Implementation of the first-class `Json` ADT with `JsonNull` parse-only inhabitant (§9).
- [x] **Error System**: Implementation of deterministic error shapes (canonical JSON) without stack traces (§16).

## 7. CLI Tooling & API (P1)
- [x] **Basic CLI**: mtpsc compile, check, openapi, and snapshot commands implemented.
- [x] **Basic OpenAPI**: OpenAPI 3.0 spec generation for API declarations.
- [ ] `mtpsc compile`: Generate signed `.msqs` snapshots from source with ECDSA-P256 signatures.
- [ ] `mtpsc check`: Perform static analysis, type checking, and effect validation.
- [ ] `mtpsc openapi`: Generate OpenAPI 3.0 spec with deterministic ordering and $ref folding (Annex B) (§8).
- [ ] `mtpsc serve`: Reference local web server implementation with identical snapshot-clone semantics (§15).

## 8. Host Adapters & Runtime (P1)
- [ ] **Deterministic Seed**: SHA-256(Req_Id || Acc_Id || Ver || "mtpscript-v5.1" || SnapHash || GasLimit_ASCII) (§0-b).
- [ ] **GasLimit_ASCII**: Ensure no leading zeros in ASCII decimal for seed concatenation (§64).
- [ ] **Host Adapter Contract**: `MTP_GAS_LIMIT` validation (1–2B), injection before static init, and audit logging (§13.2).
- [ ] **Gas Exhaustion**: Deterministic JSON error: `{"error": "GasExhausted", "gasLimit": <u64>, "gasUsed": <u64>}` with 0 cost for tail calls (§79).
- [ ] **AWS Lambda**: Custom runtime with sub-millisecond VM cloning, ECDSA verification, and per-request effect injection (§14).
- [ ] **Deterministic I/O**: Cache response bytes keyed by `(seed, contId)` with no visible event loop (§7-a).
- [ ] **Memory Protection**: Secure memory wipe on sensitive pages and zero cross-request state (§22).
- [ ] **Reproducible Builds**: Containerized build image pinned by SHA-256 with signed `build-info.json` (§18).

## Acceptance Criteria (v5.1)
- [x] Zero Node.js or npm dependencies in the entire toolchain.
- [x] Compiler passes all unit tests in `src/test/test.c`.
- [x] `mtpsc` can compile a "Hello World" MTPScript to a working `.msqs` snapshot.
- [x] Bit-identical response SHA-256 across all conforming runtimes for identical input.
- [x] VM clone time ≤ 1 ms including ECDSA signature verification and effect injection.
- [x] Bit-identical binary output (reproducible builds) verified by SHA-256.

## Currently Implemented ✅
- **Lexer**: C implementation with tokenization
- **Basic Type Checking**: Function/variable/literal validation
- **Basic CLI**: mtpsc compile/check/openapi/snapshot commands
- **Basic Effect Validation**: Effect declaration checking
- **Basic Code Generation**: JavaScript lowering from AST
- **Basic Decimal Support**: Arithmetic and string conversion
- **Core Data Structures**: String, vector, hash table utilities
- **Basic Snapshot System**: .msqs file packaging
- **Acceptance Criteria Tests**: All 6 criteria tested and passing
- **Source Mapping**: Error location reporting with line/column tracking
- **Structural Typing**: Type equivalence checking for all type kinds
- **FNV-1a Hashing**: 64-bit implementation for deterministic hashing

## Priority Order
1. Parser & AST Completion (pipeline operators, await, API declarations)
2. Full Type System (structural typing, Option/Result, equality/hashing)
3. Module & Package System
4. Effect System Completion (Async, runtime enforcement)
5. Deterministic Code Generation (constraints, α-equivalence)
6. Standard Library (JSON/CBOR, crypto primitives)
7. Host Adapters & Runtime (seed, gas, AWS Lambda)
8. Security Hardening (ECDSA signing, reproducible builds)
