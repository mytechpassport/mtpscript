# MTPScript

A deterministic programming language designed for serverless deployment with guaranteed reproducible builds and runtime isolation. Compiles to an intermediate representation and executes in a hardened, embeddable runtime engine.

## Quick Start

### Building from Source

```bash
# Clone and build
git clone <repository>
cd mtpscript
make
```

This creates:
- `mtpsc` - MTPScript compiler, type checker, and development tools
- `mtpjs` - High-performance runtime engine for executing compiled code

### Running MTPScript

#### One-command execution (recommended)
```bash
./mtpsc run hello.mtps
```
Compiles and executes MTPScript code in a single step.

#### Development workflow
```bash
# Compile MTPScript to intermediate representation
./mtpsc compile hello.mtps > hello.js

# Execute with runtime engine
./mtpjs hello.js
```

#### Production deployment
```bash
# Create signed snapshot (.msqs file)
./mtpsc snapshot hello.mtps

# Run the immutable snapshot
./mtpjs -b app.msqs
```

#### What is a snapshot?
A **snapshot** (`.msqs` file) is a cryptographically signed, immutable bundle containing:
- Compiled bytecode ready for execution
- Embedded runtime dependencies
- ECDSA-P256 signature for integrity verification
- Deterministic build metadata

Snapshots enable **sub-millisecond cold starts** and **secure deployment** - the same snapshot runs identically across all environments.

## CLI Commands

### Compiler (`mtpsc`)

```bash
Usage: mtpsc <command> [options] <file>

Commands:
  compile <file>     Compile MTPScript to JavaScript intermediate representation
  run <file>         Compile and immediately execute (development shortcut)
  check <file>       Type check and validate effects without compilation
  openapi <file>     Generate OpenAPI 3.0 specification from type annotations
  snapshot <file>    Create signed .msqs snapshot for production deployment
  serve <file>      Start local development server with hot reload (future)
```

#### Compilation Process
MTPScript uses a multi-stage compilation pipeline:

1. **Lexing & Parsing** → Convert source to AST
2. **Type Checking** → Structural typing with effect validation
3. **Code Generation** → Deterministic JavaScript output (no `eval`, `class`, `this`, loops)
4. **Bytecode Compilation** → Convert to runtime engine bytecode
5. **Snapshot Creation** → Bundle bytecode with signature and metadata

### Runtime Engine (`mtpjs`)

The runtime engine executes compiled MTPScript code with strict resource controls and security isolation.

```bash
Usage: mtpjs [options] [file [args]]

Core execution:
  -b, --allow-bytecode  Load and execute .msqs snapshot or bytecode file
  [file]                Execute compiled JavaScript intermediate representation

Development tools:
  -h, --help            Show help
  -e, --eval EXPR       Evaluate JavaScript expression
  -i, --interactive     Start interactive REPL
  -I, --include file    Include additional JavaScript file

Resource controls:
  -d, --dump            Show memory usage statistics after execution
      --memory-limit n  Limit memory usage to 'n' bytes (default: 16MB)
  --no-column           Strip column info from debug output

Bytecode operations:
  -o FILE               Compile JavaScript to bytecode file
  -m32                  Generate 32-bit compatible bytecode
```

#### Runtime Architecture
- **Tracing garbage collector** with compacting (no memory fragmentation)
- **Fixed memory buffer** allocation (no system malloc in hot path)
- **Stack-based bytecode** execution (no CPU stack usage)
- **UTF-8 string handling** with WTF-8 encoding
- **Sub-millisecond startup** from snapshots

## Examples

### Basic Function
```mtp
// hello.mtps
fn main() {
    console.log("Hello from MTPScript!");
}
```

Run it:
```bash
./mtpsc run hello.mtps
```

### HTTP Handler (for serverless deployment)
```mtp
fn handle(request) {
    return {
        status: 200,
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ message: "Hello World" })
    };
}
```

### With Effects and Types
```mtp
effect DbRead;
effect DbWrite;

fn getUser(id: String) -> Result<User, String> {
    // Database read effect
    let user = DbRead.query("SELECT * FROM users WHERE id = ?", [id]);
    match user {
        Some(u) => Ok(u),
        None => Err("User not found")
    }
}

fn createUser(name: String) -> Result<UserId, String> {
    // Database write effect
    let result = DbWrite.execute(
        "INSERT INTO users (name) VALUES (?)",
        [name]
    );
    Ok(result.lastInsertId)
}
```

## Architecture

### Compilation Pipeline
```
MTPScript Source
       ↓
    Lexer/Parser
       ↓
Type Checker & Effect Analysis
       ↓
Deterministic JavaScript Generation
       ↓
Bytecode Compilation
       ↓
.msqs Snapshot (signed & immutable)
```

The compilation process ensures:
- **No dynamic features**: Generated JS forbids `eval`, `class`, `this`, loops
- **Effect safety**: All side effects (DbRead, DbWrite, HttpOut, Log) are explicitly tracked
- **Deterministic output**: Same source always produces identical bytecode
- **Type safety**: Structural typing prevents runtime type errors

### Production Runtime
- **Isolated execution**: Fresh runtime instance per request
- **Memory containment**: Fixed buffer allocation prevents resource exhaustion
- **Secure cleanup**: Memory wiping after each execution
- **Cryptographic verification**: ECDSA signature validation before execution
- **Gas metering**: Deterministic execution limits with precise error reporting

### Deployment Targets
- **AWS Lambda**: Custom runtime with <1ms cold start from snapshots
- **Local development**: Identical semantics to production via `mtpsc serve`
- **Embedded systems**: Minimum 10kB RAM for resource-constrained devices

## Language Features

### Type System
- **Structural typing** with immutability by default
- **Algebraic data types**: `Option<T>`, `Result<T, E>`
- **Decimal arithmetic** (IEEE-754-2008 decimal128)
- **Effect tracking** for side effects (DbRead, DbWrite, HttpOut, Log)

### Pipeline Operator
```mtp
// Left-associative pipeline (like F#)
data
|> validate
|> transform
|> save
```

### Pattern Matching
```mtp
match result {
    Ok(data) => process(data),
    Err(msg) => logError(msg)
}
```

### Module System
- **Static imports** with git-hash pinned dependencies
- **Deterministic builds** with signed package manifests
- **No runtime network access** for dependencies

## Development Workflow

### Local Development
```bash
# Type check without compilation
./mtpsc check app.mtps

# Compile and run in one command
./mtpsc run app.mtps

# Or compile to inspect generated code
./mtpsc compile app.mtps > app.js
./mtpjs app.js
```

### API Documentation
```bash
# Generate OpenAPI spec from type annotations
./mtpsc openapi app.mtps > api.json
```

### Production Deployment
```bash
# Create signed snapshot for deployment
./mtpsc snapshot app.mtps

# Verify snapshot integrity
./mtpjs -b app.msqs
```

### Development Server (Future)
```bash
# Start local server with hot reload (when implemented)
./mtpsc serve app.mtps
```

## Testing

```bash
# Run unit tests
make test

# Run compiler tests
./mtpsc_test

# Run microbenchmarks
make microbench
```

## Security & Compliance

- **SOC 2, SOX, ISO 27001, PCI-DSS** compliant
- **Reproducible builds** with containerized build pipeline
- **Runtime isolation** with per-request VM cloning
- **Deterministic execution** with cryptographic seed generation
- **Memory safety** with bounds-checked operations

## License

MIT License - see LICENSE file for details.


