# Codelint Feature Specification for AI Agents
# This file is machine-readable YAML - do not wrap in code blocks

version: "1.0.0"

# =============================================================================
# TOOL INFORMATION
# =============================================================================
tool:
  name: "codelint"
  description: "C++ static analysis tool using LLVM LibTooling to detect code issues and suggest fixes"
  namespace: "codelint::lint"
  language: "C++ (C++14/C++17/C++20)"
  build_system: "CMake"
  dependencies:
    - "LLVM/Clang 18.1"
    - "CLI11"
    - "RapidJSON"

# =============================================================================
# CHECKERS
# =============================================================================
checkers:
  - name: "init"
    description: "Variable initialization checker - recommends modern C++ best practices"
    check_types:
      - type: "init_uninitialized"
        severity: "warning"
        description: "Variable declared without explicit initialization"
        example_fix: "int c; -> int c{};"
      - type: "init_equals_syntax"
        severity: "warning"
        description: "Recommend using uniform initialization syntax '{}' instead of '='"
        example_fix: "int a = 5; -> int a{5};"
      - type: "init_unsigned_suffix"
        severity: "warning"
        description: "Unsigned integer should have 'U' suffix for literal values"
        example_fix: "unsigned int b = 42; -> unsigned int b{42U};"
      - type: "const_suggestion"
        severity: "hint"
        description: "Variable is not modified, recommend adding const"
        example_fix: "int x{42}; -> const int x{42};"
      - type: "can_be_constexpr"
        severity: "hint"
        description: "Compile-time constant, recommend using constexpr"
        example_fix: "const int x{42}; -> constexpr int x{42};"
    fixable: true
    skip_rules:
      - "auto declarations (type deduction requires '=' syntax)"
      - "for loop variables (e.g., for (int i = 0; ...))"
      - "union members (special initialization semantics)"
      - "enum class without zero value member"
      - "extern declarations"
      - "exception variables in catch blocks"

  - name: "global"
    description: "Global variable detector - finds all global variables in the project"
    check_types:
      - type: "global_variable"
        severity: "info"
        description: "Global variable detected - potential state management issue"
        example_fix: "N/A - informational only"
    fixable: false

  - name: "singleton"
    description: "Meyer's Singleton pattern detector"
    check_types:
      - type: "singleton_pattern"
        severity: "info"
        description: "Meyer's Singleton detected (static local variable returning reference)"
        example_fix: "N/A - informational only"
    fixable: false

  - name: "const"
    description: "Const correctness checker - analyzes variable mutability using CFG data flow analysis"
    check_types:
      - type: "can_be_const"
        severity: "warning"
        description: "Variable can be marked const (not modified after initialization)"
        example_fix: "int x{42}; -> const int x{42};"
      - type: "can_be_constexpr"
        severity: "hint"
        description: "Variable can be constexpr (compile-time constant)"
        example_fix: "const int x{42}; -> constexpr int x{42};"
      - type: "const_suggestion"
        severity: "hint"
        description: "Variable is not modified, recommend adding const"
        example_fix: "int x{42}; -> const int x{42};"
    fixable: true

# =============================================================================
# CLI OPTIONS
# =============================================================================
options:
  global:
    - name: "--version"
      type: "flag"
      description: "Show version information"
      example: "./codelint --version"
    - name: "-p, --path"
      type: "option"
      description: "Path to compile_commands.json directory"
      default: "."
      example: "./codelint -p build lint src/main.cpp"
    - name: "--output-json"
      type: "flag"
      description: "Output results in JSON format instead of text"
      example: "./codelint --output-json lint src/"

  lint_subcommand:
    - name: "files"
      type: "positional"
      description: "Source files or directories to check (0 or more)"
      example: "./codelint lint src/main.cpp"
    - name: "--only"
      type: "option"
      description: "Only run specific checkers (comma-separated)"
      example: "./codelint lint src/ --only=init,const"
    - name: "--exclude"
      type: "option"
      description: "Exclude specific checkers (comma-separated)"
      example: "./codelint lint src/ --exclude=global"
    - name: "--fix"
      type: "flag"
      description: "Automatically apply fixes where possible (init and const checkers only)"
      example: "./codelint lint src/ --fix"
    - name: "--inplace"
      type: "flag"
      description: "Modify files in-place (requires --fix)"
      example: "./codelint lint src/ --fix --inplace"
    - name: "--severity"
      type: "option"
      description: "Minimum severity to report (error, warning, info, hint)"
      default: "info"
      example: "./codelint lint src/ --severity=warning"

# =============================================================================
# OUTPUT SCHEMA
# =============================================================================
output_schema:
  format: "JSON"
  structure:
    issues:
      type: "array"
      description: "Array of lint issues found"
      item_properties:
        type: "string"
        severity: "string"
        checker: "string"
        name: "string"
        type_str: "string"
        file: "string"
        line: "integer"
        column: "integer"
        description: "string"
        suggestion: "string"
        fixable: "boolean"
    summary:
      type: "object"
      properties:
        errors: "integer"
        warnings: "integer"
        info: "integer"
        hints: "integer"
    fixed_count:
      type: "integer"
      description: "Number of issues fixed (when --fix is used)"
  json_example: |
    {
      "issues": [
        {
          "type": "init_equals_syntax",
          "severity": "warning",
          "checker": "init",
          "name": "a",
          "type_str": "int",
          "file": "/path/to/file.cpp",
          "line": 10,
          "column": 5,
          "description": "Variable should use '{}' syntax for initialization",
          "suggestion": "int a{5};",
          "fixable": true
        }
      ],
      "summary": {
        "errors": 0,
        "warnings": 5,
        "info": 2,
        "hints": 3
      },
      "fixed_count": 0
    }

# =============================================================================
# USAGE EXAMPLES
# =============================================================================
examples:
  - description: "Check single file"
    command: "./codelint -p build lint src/main.cpp"
  - description: "Check and auto-fix issues"
    command: "./codelint -p build lint src/main.cpp --fix"
  - description: "Check entire directory"
    command: "./codelint -p build lint src/"
  - description: "Run specific checkers only"
    command: "./codelint -p build lint src/ --only=init,const"
  - description: "Exclude certain checkers"
    command: "./codelint -p build lint src/ --exclude=global"
  - description: "JSON output for CI integration"
    command: "./codelint -p build --output-json lint src/ --severity=warning"
  - description: "Check with minimum severity filter"
    command: "./codelint -p build lint src/ --severity=warning"
  - description: "Find global variables in project"
    command: "./codelint -p build find_global src/"
  - description: "Find singleton patterns"
    command: "./codelint -p build find_singleton src/"

# =============================================================================
# FUTURE FEATURES
# =============================================================================
future_features:
  - name: "--scope"
    status: "planned"
    description: "Git-aware scope filtering - limit analysis to modified lines"
    modes:
      - "all: analyze entire file (default)"
      - "modified: working directory + staged changes (git diff HEAD)"
      - "commit:<hash>: specific commit changes"
      - "merge-base: CI/PR review (git diff origin/main...HEAD)"
    use_case: "CI integration, incremental analysis, pre-commit hooks"
  - name: "suppress_inline"
    status: "planned"
    description: "Suppress specific warnings inline using comments"
  - name: "config_file"
    status: "planned"
    description: "YAML/JSON configuration file for persistent settings"
  - name: "html_report"
    status: "planned"
    description: "Generate HTML report with interactive visualization"
