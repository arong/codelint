# codelint-strict-bool-condition

Enforces that only `bool` type expressions are used in condition statements.

## Description

This check prevents dangerous implicit conversions from integers, pointers,
and other types in condition expressions (if, while, for, do-while, ternary).

Implicit conversions in conditions are dangerous because:
1. Integer `0` is treated as `false`, non-zero as `true` - confusing
2. Pointer `nullptr` is `false`, non-null is `true` - intent unclear
3. Assignment in conditions returns a value - easy to miss `=` vs `==`
4. `strcmp()` returns `0` for equal strings - inverted logic causes bugs

## Examples

### Example 1: Integer in condition

```cpp
// Before - DANGEROUS
int status = 0;
if (status) {  // Only false when status == 0
    // ...
}

// After - CORRECT
int status = 0;
if (status != 0) {  // Explicit comparison
    // ...
}
```

### Example 2: Pointer in condition

```cpp
// Before - DANGEROUS
char* ptr = buffer;
while (ptr) {  // Confusing: checks if ptr != nullptr
    // ...
}

// After - CORRECT
char* ptr = buffer;
while (ptr != nullptr) {  // Clear intent
    // ...
}
```

### Example 3: Assignment in condition

```cpp
// Before - DANGEROUS
while (result = get_next()) {  // Easy to miss = vs ==
    // ...
}

// After - CORRECT
while ((result = get_next()) != nullptr) {  // Explicit comparison
    // ...
}
```

### Example 4: strcmp in condition (classic bug)

```cpp
// Before - BUG! strcmp returns 0 for equal strings
if (strcmp(a, b)) {  // WRONG: true when strings are DIFFERENT
    // handle_equal_strings();  // Bug: handles different strings!
}

// After - CORRECT
if (strcmp(a, b) == 0) {  // Explicit comparison to 0
    handle_equal_strings();  // Correct: handles equal strings
}
```

## Checked Statement Types

| Statement | Checked Expression |
|-----------|-------------------|
| `if` | Condition expression |
| `while` | Loop condition |
| `for` | Loop condition (not init or increment) |
| `do-while` | Loop condition |
| Ternary `?:` | Condition expression |

## Valid Conditions

These patterns are correct and will not trigger warnings:

```cpp
// Bool variables
bool ready = true;
if (ready) { }  // ✅ OK

// Explicit comparisons (produce bool)
if (status == 0) { }  // ✅ OK
while (ptr != nullptr) { }  // ✅ OK
if (count > 0) { }  // ✅ OK

// Bool expressions
while (!done) { }  // ✅ OK
if (a && b) { }  // ✅ OK (a and b must be bool)
```

## Why This Matters

| Problem | Risk |
|---------|------|
| `if (count)` | Treats 0 as false, non-zero as true - confusing |
| `if (ptr)` | Null check works, but intent unclear |
| `while (result = func())` | Assignment returns value - easy to confuse `=` vs `==` |
| `if (strcmp(a, b))` | **BUG!** strcmp returns 0 for equal strings |

## Limitations

- Does not check conditions in `?:` operator operands
- Does not detect bool variables assigned from integer expressions

## See Also

- [readability-implicit-bool-conversion](https://clang.llvm.org/extra/clang-tidy/checks/readability/implicit-bool-conversion.html)
- [bugprone-implicit-widening-of-multiplication-result](https://clang.llvm.org/extra/clang-tidy/checks/bugprone/implicit-widening-of-multiplication-result.html)
