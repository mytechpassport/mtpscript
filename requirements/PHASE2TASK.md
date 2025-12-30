# Phase 2: Production Readiness & Ecosystem (v5.1)

This phase focuses on completing the MTPScript ecosystem for **production deployment**, including full effect implementations, TypeScript migration tooling, package management, and compliance documentation.

## 1. Full Effect Runtime Implementation (P0)

### 1.1 Database Effects (§7)
- [x] **DbRead Effect**: Implement deterministic SQL query execution with result caching
  - [x] Connection pool management with per-request isolation
  - [ ] Query parameterization and SQL injection prevention
  - [x] Result serialization to canonical JSON
  - [x] Cache responses keyed by `(seed, query_hash)` for replay determinism
- [x] **DbWrite Effect**: Implement transactional SQL write operations
  - [x] Atomic transaction support with rollback capability
  - [ ] Write operation logging for audit trail
  - [ ] Idempotency key support for deterministic retries

### 1.2 HTTP Effect (§7)
- [x] **HttpOut Effect**: Implement outbound HTTP client with determinism guarantees
  - [ ] Request serialization and canonical form
  - [x] Response caching keyed by `(seed, request_hash)` per §7-a
  - [x] Timeout handling with deterministic error shapes
  - [ ] TLS certificate validation
  - [ ] Request/response body size limits

### 1.3 Logging Effect (§7)
- [x] **Log Effect**: Implement structured logging with audit compliance
  - [x] Log levels: `debug`, `info`, `warn`, `error`
  - [x] Structured JSON output per §23
  - [x] Correlation ID injection from request seed
  - [ ] Log aggregation interface for CloudWatch/external systems
  - [x] No stack traces in production per §16

## 2. Full API Routing System (P0)

### 2.1 Request Handling (§8)
- [ ] **Path Parameter Extraction**: `/users/:id` → `{ id: String }`
- [ ] **Query Parameter Parsing**: `?page=1&limit=10` → typed parameters
- [ ] **Request Body Parsing**: JSON body deserialization with validation
- [ ] **Header Access**: Typed header extraction with case normalization
- [ ] **Content-Type Negotiation**: `application/json` enforcement

### 2.2 Response Generation (§8)
- [ ] **respond json(...)**: Canonical JSON response serialization
- [ ] **respond status(...)**: HTTP status code with typed error bodies
- [ ] **Response Headers**: Content-Type, Content-Length, custom headers
- [ ] **Error Responses**: Deterministic error shapes per §16

### 2.3 Route Matching
- [ ] **Static Routes**: Exact path matching (`/users`, `/health`)
- [ ] **Dynamic Routes**: Path parameters (`/users/:id/posts/:postId`)
- [ ] **Method Routing**: GET, POST, PUT, DELETE, PATCH dispatch
- [ ] **Route Priority**: Most-specific match wins

## 3. TypeScript Migration Tooling (P1)

### 3.1 Migration CLI (§17)
- [ ] `mtpsc migrate <file.ts>`: Convert TypeScript to MTPScript
- [ ] `mtpsc migrate --dir <dir>`: Batch migration of directories
- [ ] `mtpsc migrate --check`: Dry-run with compatibility report

### 3.2 Mechanical Transforms (§17)
- [ ] **Type Mapping**: `number` → `Int`, `string` → `String`, `boolean` → `Bool`
- [ ] **Null Handling**: `null | T` → `Option<T>`, `throws` → `Result<T, E>`
- [ ] **Class Removal**: Convert classes to records and functions
- [ ] **Loop Conversion**: `for`/`while` → recursive functions
- [ ] **Effect Inference**: Detect I/O and annotate with `uses { ... }`
- [ ] **Import Rewriting**: npm imports → audit manifest entries
- [ ] **Generics**: `T<U>` → parametric types (limited support)
- [ ] **Enums**: Convert to union types with content hashing
- [ ] **Interface Conversion**: Interfaces → structural records
- [ ] **Method Extraction**: Class methods → top-level functions

### 3.3 Migration Reports
- [ ] **Compatibility Analysis**: List unsupported TypeScript features
- [ ] **Manual Intervention Points**: Flag code requiring human review
- [ ] **Effect Suggestions**: Recommend effect declarations based on I/O patterns
- [ ] **TypeScript AST Parser**: Parse TypeScript files to AST for migration

## 4. Package Manager CLI (P1)

### 4.1 Dependency Management (§11)
- [ ] `mtpsc add <package>[@version]`: Add git-pinned dependency
- [ ] `mtpsc remove <package>`: Remove dependency
- [ ] `mtpsc update <package>`: Update to latest signed tag
- [ ] `mtpsc list`: List all dependencies with versions and hashes

### 4.2 Lock File Management
- [ ] **mtp.lock**: Deterministic lock file with git hashes and signatures
- [ ] **Integrity Verification**: SHA-256 content hash validation
- [ ] **Signature Verification**: Git tag signature validation per §10

### 4.3 Vendoring System (§10)
- [ ] **vendor/**: Local copy of all dependencies
- [ ] **Offline Builds**: No network access required after vendor
- [ ] **Audit Manifest Generation**: `audit-manifest.json` with content hashes

### 4.4 npm Bridge CLI (§21)
- [ ] `mtpsc npm-bridge <package>`: Generate unsafe adapter wrapper
- [ ] **Adapter Template**: Generate `host/unsafe/<package>.js` skeleton
- [ ] **Type Signature Enforcement**: Validate `(seed, ...args) => JsonValue` contract
- [ ] **Audit Manifest Update**: Auto-add to `unsafeDeps` list

## 5. Production AWS Lambda Deployment (P1)

### 5.1 Custom Runtime Packaging (§14)
- [ ] **Native Binary Build**: Statically linked `bootstrap` executable
- [ ] **Lambda Layer**: Reusable layer with MicroQuickJS runtime
- [ ] **Deployment Package**: `app.msqs` + `app.msqs.sig` + certificate

### 5.2 Infrastructure Templates
- [ ] **SAM Template**: AWS SAM `template.yaml` for deployment
- [ ] **CDK Construct**: AWS CDK construct for MTPScript functions
- [ ] **Terraform Module**: Terraform module for MTPScript deployment

### 5.3 Cold Start Optimization (§14)
- [ ] **Provisioned Concurrency**: Configuration for warm starts
- [ ] **EFS Integration**: Snapshot storage on EFS with page fault handling
- [ ] **Memory Tuning**: Optimal memory/CPU allocation recommendations

## 6. Annex Files & Documentation (P1)

### 6.1 Gas Cost Table (Annex A)
- [ ] Create `/gas-v5.1.csv` with all opcode and built-in costs
  - Format: `opcode,name,cost_beta_units,category`
  - Include all IR opcodes
  - Include all built-in function costs
  - Document tail call 0-cost exception

### 6.2 OpenAPI Generation Rules (Annex B)
- [ ] Create `/openapi-rules-v5.1.json` schema
  - Deterministic field ordering rules
  - `$ref` folding algorithm
  - Schema deduplication rules
  - Path parameter ordering

### 6.3 Compliance Documentation (§18)
- [ ] **SOC 2 Mapping**: Control mapping document
- [ ] **SOX Compliance**: Financial control attestation guide
- [ ] **ISO 27001**: Information security controls mapping
- [ ] **PCI-DSS**: Payment card data handling controls

## 7. Union Exhaustiveness Checking (P1)

### 7.1 Content Hashing (§24)
- [ ] **Union Type Content Hashing**: Generate SHA-256 hash of variant list for each union type
- [ ] **Link-Time Verification**: Fail compilation if any unit sees different variant sets
- [ ] **Union ADT Definition**: Extend type system to support union types with exhaustive checking

### 7.2 Exhaustive Match Enforcement (§24)
- [ ] **Compile-Time Exhaustiveness**: Verify all union variants covered in match expressions
- [ ] **Link-Time Guarantees**: Runtime checks not needed due to link-time verification
- [ ] **Pattern Matching Infrastructure**: Support destructuring patterns for union variants

## 8. Full HTTP Server Syntax & Support (P1)

### 8.1 Server Declaration Parsing (§15, §20)
- [ ] Parse `serve { port: 8080, routes: [...] }` MTPScript syntax
- [ ] Route configuration with path patterns and handlers
- [ ] Server configuration options (port, host, timeouts)
- [ ] Hot reload on source file changes with snapshot recompilation

### 8.2 Server Runtime Implementation (§20)
- [ ] **Snapshot-Clone Isolation**: Same per-request VM cloning as Lambda runtime
- [ ] **Not User-Programmable**: Server is reference implementation only
- [ ] **Development Tools**: Request logging, error handling, debugging support

## 9. Pipeline Operator Associativity Verification (P1)

### 9.1 Left-Associative Generation (§25)
- [ ] **α-Equivalent JS Output**: Ensure `a |> b |> c ≡ (a |> b) |> c` generates identical JS across compilers
- [ ] **Deterministic Code Generation**: Pipeline lowering produces consistent AST structure
- [ ] **Test Coverage**: Comprehensive tests for associativity edge cases

### 9.2 Local Development Server (P2)

## 10. Cross-Platform Testing & CI/CD (P2)

### 10.1 Platform Matrix
- [ ] **Linux x86_64**: Primary CI target
- [ ] **Linux ARM64**: AWS Graviton support
- [ ] **macOS x86_64**: Development support
- [ ] **macOS ARM64 (Apple Silicon)**: Development support

### 10.2 Determinism Verification
- [ ] **Cross-Platform SHA-256 Tests**: Verify identical output hashes
- [ ] **Endianness Tests**: Verify big/little endian consistency
- [ ] **Floating-Point Absence Tests**: Verify no FP operations leak through

### 10.3 CI/CD Pipeline
- [ ] **GitHub Actions**: Multi-platform build and test
- [ ] **Release Automation**: Signed binary releases
- [ ] **Reproducible Build Verification**: Hash comparison across builds

## 11. Performance & Benchmarking (P2)

### 11.1 Benchmarks
- [ ] **VM Clone Time**: Measure and optimize `clone_vm()` performance
- [ ] **Request Throughput**: Requests/second under load
- [ ] **Memory Usage**: Per-request memory consumption
- [ ] **Gas Metering Overhead**: Cost of gas counting

### 11.2 Profiling Tools
- [ ] `mtpsc profile <file.mtp>`: Gas consumption profile
- [ ] `mtpsc benchmark <file.mtp>`: Performance benchmark
- [ ] Memory allocation tracking

## 12. Language Server Protocol (P2)

### 12.1 LSP Implementation
- [ ] **Diagnostics**: Real-time error reporting
- [ ] **Completion**: Auto-complete for types, functions, effects
- [ ] **Hover**: Type information on hover
- [ ] **Go to Definition**: Navigate to declarations
- [ ] **Find References**: Find all usages

### 12.2 Editor Extensions
- [ ] **VS Code Extension**: Syntax highlighting + LSP client
- [ ] **Cursor Extension**: Native integration
- [ ] **Syntax Grammar**: TextMate grammar for `.mtp` files

## 13. Formal Determinism Verification (P1)

### 13.1 Determinism Claim Testing (§26)
- [ ] **Response SHA-256 Verification**: Verify identical SHA-256 hashes across conforming runtimes
- [ ] **Canonical JSON Compliance**: Ensure all output follows RFC 8785 with duplicate-key rejection
- [ ] **Seed Algorithm Validation**: Test deterministic seed generation per §0-b (updated by §0-c with gas limit)
- [ ] **CBOR Determinism**: Verify RFC 7049 §3.9 compliance for all serialization
- [ ] **Gas Limit Determinism**: Verify identical responses for same program, input, and gasLimit L

### 13.2 Cross-Runtime Testing Infrastructure (§26)
- [ ] **Runtime Conformance Suite**: Test programs against multiple runtime implementations
- [ ] **Deterministic Replay Testing**: Verify request/response determinism across platforms
- [ ] **Gas Limit Determinism**: Ensure identical gas exhaustion behavior

## 14. Advanced Security & Audit Features (P1)

### 14.1 VM Snapshot Security (§22)
- [ ] **Secure Memory Wipe**: Selective wipe of pages containing PCI-classified data
- [ ] **Zero Cross-Request Leakage**: Guaranteed memory isolation between requests
- [ ] **Snapshot Lifecycle Audit**: Complete audit trail from build to execution

### 14.2 Audit Trail Implementation (§18)
- [ ] **Request Audit Logging**: All requests logged with deterministic correlation IDs
- [ ] **Gas Usage Audit**: Gas consumption logged for every request with gasLimit field (§0-c.5)
- [ ] **Effect Usage Tracking**: Runtime verification of declared vs actual effects
- [ ] **OpenAPI Audit Schema**: Every request log includes gasLimit field in audit stream

### 14.3 Regulatory Compliance (§18)
- [ ] **SOC 2 Controls**: Security, availability, and confidentiality controls
- [ ] **SOX Compliance**: Financial reporting controls and audit trails
- [ ] **ISO 27001**: Information security management system
- [ ] **PCI-DSS**: Payment card industry data security standards

## 15. Build Info & Signing Infrastructure (P1)

### 14.1 Containerized Build Environment (§18)
- [ ] **Dockerfile**: Reproducible build container pinned by SHA-256
- [ ] **Build Info Generation**: `build-info.json` with all build artifacts and hashes
- [ ] **Build Signing**: ECDSA-P256 signature of build-info.json

### 14.2 Reproducible Builds (§18)
- [ ] **Deterministic Compilation**: Identical binaries from identical source + environment
- [ ] **Source Code Verification**: Git hash inclusion in build-info.json
- [ ] **Dependency Pinning**: All build dependencies version-pinned and hashed

### 14.3 Runtime Verification (§22)
- [ ] **Snapshot Signature Verification**: ECDSA signature validation before mapping
- [ ] **Build Info Audit**: Runtime verification of build provenance
- [ ] **Certificate Management**: Embedded certificate validation chain

## Acceptance Criteria (v5.1)

### P0 Requirements (Must Have)
- [ ] All four built-in effects (DbRead, DbWrite, HttpOut, Log) fully implemented
- [ ] API routing handles path params, query params, and request bodies
- [x] Effect implementations cache responses for deterministic replay
- [x] All effects produce canonical JSON output per §23

### P1 Requirements (Should Have)
- [ ] `mtpsc migrate` converts basic TypeScript files to MTPScript
- [ ] Package manager can add/remove/update git-pinned dependencies
- [ ] AWS Lambda deployment works with provided templates
- [ ] `/gas-v5.1.csv` and `/openapi-rules-v5.1.json` exist and are valid
- [ ] Basic compliance documentation available

### P2 Requirements (Nice to Have)
- [ ] Hot reload in development server
- [ ] Cross-platform CI/CD with determinism verification
- [ ] Performance benchmarks establish baselines
- [ ] LSP provides basic IDE support

### Test Coverage
- [x] Integration tests for all effect implementations
- [ ] End-to-end tests for API routing
- [ ] Migration tests with TypeScript fixture files
- [ ] Cross-platform determinism tests in CI
- [ ] Union exhaustiveness checking tests
- [ ] HTTP server syntax parsing tests
- [ ] Pipeline associativity verification tests
- [ ] Formal determinism claim validation tests

---

## Priority Order

1. **P0 - Critical**: Effect implementations and API routing (blocks production use)
2. **P1 - Important**: Migration tools, package manager, Lambda deployment, documentation
3. **P2 - Desirable**: Dev server improvements, CI/CD, performance, LSP

## Dependencies

- Phase 0 & Phase 1 must be complete (verified ✅)
- Database driver selection (PostgreSQL recommended for determinism)
- HTTP client library selection (libcurl or custom minimal client)
- AWS account access for Lambda testing

