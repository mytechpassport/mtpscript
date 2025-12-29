# **How to Learn MTPScript: A Comprehensive Guide**

## 1. Getting Started with MTPScript

### 1.1 Installing MTPScript

First, you need to install the MTPScript runtime and compiler. You can download the latest version from the [official MTPScript website](https://mtpscript.org/download).

```sh
# Download and install MTPScript runtime
curl -sS https://mtpscript.org/install | sh
```

### 1.2 Setting Up Your Environment

Create a new directory for your project and initialize it with the MTPScript package manager.

```sh
mkdir my-mtpscript-project
cd my-mtpscript-project
mtp init
```

This will create a basic project structure and a `mtp.toml` configuration file.

---

## 2. Understanding Types in MTPScript

MTPScript has a strong type system with primitive and composite types.

### 2.1 Primitive Types

```mtp
// Primitive types
let intExample: Int = 42
let boolExample: Bool = true
let stringExample: String = "Hello, MTPScript!"
```

### 2.2 Composite Types

#### Records

```mtp
// Define a record type
type User {
  id: Int
  name: String
  email: String
}

// Create an instance of User
let user: User = {
  id: 1,
  name: "John Doe",
  email: "john.doe@example.com"
}
```

#### Algebraic Data Types

```mtp
// Define an algebraic data type
type Payment {
  | Card(number: String)
  | Wire(ref: String)
}

// Create instances of Payment
let cardPayment: Payment = Card("1234-5678-9101-1121")
let wirePayment: Payment = Wire("ABC123")
```

---

## 3. Language Methods and Features

### 3.1 Functions

```mtp
// Define a function
function add(a: Int, b: Int): Int {
  return a + b
}

// Call the function
let result: Int = add(5, 3)
```

### 3.2 Conditionals

```mtp
// Define a function with conditionals
function isPositive(num: Int): Bool {
  if num > 0 {
    return true
  } else {
    return false
  }
}

// Call the function
let positive: Bool = isPositive(10)
```

### 3.3 Pattern Matching

```mtp
// Define a function with pattern matching
function describePayment(payment: Payment): String {
  match payment {
    | Card(number) -> return "Card payment with number " + number
    | Wire(ref) -> return "Wire payment with reference " + ref
  }
}

// Call the function
let paymentDescription: String = describePayment(cardPayment)
```

### 3.4 Effects

```mtp
// Declare an effect
effect http

// Define a function that uses the http effect
function fetchUser(userId: Int): User uses { http } {
  // Simulate fetching a user from an HTTP API
  return http.get("https://api.example.com/users/" + userId.toString())
}
```

---

## 4. Setting Up an HTTP Server

### 4.1 Reference HTTP Server (Local / On-Prem)

MTPScript includes a reference HTTP server for local development and on-prem systems.

```mtp
// Define an HTTP API
api GET /hello {
  respond "Hello, World!"
}

// Start the server
mtp serve --port 8080
```

### 4.2 Serverless Setup (AWS Lambda)

For serverless deployment, you can use AWS Lambda with a custom runtime.

```mtp
// Define an HTTP API for serverless
api GET /hello {
  respond "Hello, World!"
}

// Deploy to AWS Lambda
mtp deploy --target lambda
```

---

## 5. CRUD Example

Let's create a simple CRUD (Create, Read, Update, Delete) example for managing users.

### 5.1 Define the User Type

```mtp
type User {
  id: Int
  name: String
  email: String
}
```

### 5.2 Create a User

```mtp
// Define a function to create a user
function createUser(name: String, email: String): User uses { db } {
  let user: User = {
    id: db.nextId(),
    name: name,
    email: email
  }
  db.insert(user)
  return user
}
```

### 5.3 Read a User

```mtp
// Define a function to read a user
function getUser(id: Int): Option<User> uses { db } {
  return db.find(id)
}
```

### 5.4 Update a User

```mtp
// Define a function to update a user
function updateUser(id: Int, name: String, email: String): Option<User> uses { db } {
  match db.find(id) {
    | some(user) -> {
      let updatedUser: User = {
        id: user.id,
        name: name,
        email: email
      }
      db.update(updatedUser)
      return some(updatedUser)
    }
    | none -> return none
  }
}
```

### 5.5 Delete a User

```mtp
// Define a function to delete a user
function deleteUser(id: Int): Bool uses { db } {
  return db.delete(id)
}
```

---

## 6. Calling the Package Manager

### 6.1 Adding Dependencies

You can add dependencies to your project using the MTPScript package manager.

```sh
# Add a dependency
mtp add github.com/example/lib@abc123
```

This will add the dependency to your `mtp.toml` file and vendor it at build time.

### 6.2 Using npm Packages

To use npm packages, you need to create a host adapter.

```js
// host/unsafe/example.js
import { exampleFunction } from "example-npm-package";

export function callExampleFunction() {
  return exampleFunction();
}
```

Then, inject the adapter as an effect in your MTPScript code.

```mtp
// Define a function that uses the npm package
function useExampleFunction(): String uses { unsafe } {
  return unsafe.callExampleFunction()
}
```

---

## 7. Folder Structure

A typical MTPScript project has the following folder structure:

```
my-mtpscript-project/
├── src/
│   ├── main.mtp
│   └── user.mtp
├── host/
│   └── unsafe/
│       ├── example.js
├── mtp.toml
└── package.json
```

### 7.1 `src/` Directory

This directory contains your MTPScript source files.

### 7.2 `host/unsafe/` Directory

This directory contains host adapters for npm packages.

### 7.3 `mtp.toml` File

This file contains configuration for the MTPScript package manager.

```toml
[package]
name = "my-mtpscript-project"
version = "0.1.0"

[dependencies]
example = "github.com/example/lib@abc123"
```

### 7.4 `package.json` File

This file contains configuration for npm dependencies.

```json
{
  "name": "my-mtpscript-project",
  "version": "0.1.0",
  "dependencies": {
    "example-npm-package": "^1.0.0"
  }
}
```