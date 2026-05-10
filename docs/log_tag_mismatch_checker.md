# Log Tag Mismatch Checker

## What it does

Checks that log tags (`[FunctionName]`) in logging statements match the name of the
enclosing function. This helps catch copy-paste errors when moving log statements
between functions.

## Example

```cpp
void Foo::FuncA() {
    ILOG("[FuncB] enters");     // Warning: log tag 'FuncB' does not match enclosing function 'FuncA'
    ILOG("[FuncA] finished");   // OK
}
```

## Options

| Name | Default | Description |
|------|---------|-------------|
| `LogMacroNames` | `"*LOG*,*log*"` | Comma-separated patterns for log macro names |
| `AllowQualifiedName` | `true` | Allow `[Class::Func]` format for member functions |

## Supported Patterns

- `[FuncName]` - Simple function name match
- `[Class::FuncName]` - Qualified name match (when enabled)
- No tag - No warning (logs without tags are allowed)
