# GLM-Review Reference

Detailed reference for the `glm-review` skill.

---

## CLI Options

| Option | Description | Default |
|--------|-------------|---------|
| `--target` | Files/directories to review (glob pattern) | Required |
| `--api-key` | GLM4.7 API key | Required |
| `--prompt-file` | Custom prompt file path | `prompts/default.txt` |
| `--chunk-size` | Max tokens per request | 4000 |
| `--output` | Output file path | `review_output.md` |
| `--model` | Model identifier | `glm-4.7` |
| `--base-url` | API base URL | Configured in script |
| `--dry-run` | Show files and token count without calling API | False |
| `--simulate` | Use mock client for testing | False |

---

## Review Modes

### File-Level Review

Review individual files or multiple specific files:

```bash
# Single file
./skills/glm-review/scripts/review.py \
  --target "src/class_a.cpp" \
  --api-key "$API_KEY"

# Multiple files
./skills/glm-review/scripts/review.py \
  --target "src/class_a.cpp,src/class_b.cpp" \
  --api-key "$API_KEY"
```

### Directory Scan

Review all C++ files matching a glob pattern:

```bash
# All .cpp files
./skills/glm-review/scripts/review.py \
  --target "src/**/*.cpp" \
  --api-key "$API_KEY"

# Both .cpp and .h files
./skills/glm-review/scripts/review.py \
  --target "src/**/*.{cpp,h}" \
  --api-key "$API_KEY"
```

### Custom Prompts

Use different prompts for specialized reviews:

```bash
# Use built-in safety-focused prompt
./skills/glm-review/scripts/review.py \
  --target "src/**/*" \
  --prompt-file "skills/glm-review/prompts/safety-review.txt" \
  --api-key "$API_KEY"

# Use performance-focused prompt
./skills/glm-review/scripts/review.py \
  --target "src/**/*" \
  --prompt-file "skills/glm-review/prompts/performance-review.txt" \
  --api-key "$API_KEY"
```

---

## Output Format

Review results are written to Markdown with structured sections:

```markdown
# Code Review Report

## File: src/class_a.cpp

### Issues Found
- Line 23: Uninitialized variable `count`
- Line 45: Potential integer overflow

### Suggestions
- Use `{}` initialization for all variables
- Consider bounds checking for array access

### Code Quality Score: 7/10
```

---

## Chunking Strategy

### Token-Aware Chunking

- **Default chunk size**: 4000 tokens per API request
- **File pairing**: Groups 1-3 `.cpp` files with corresponding `.h` headers
- **Sequential processing**: Processes chunks sequentially to avoid rate limits

### Chunk Size Configuration

```bash
# Smaller chunks for faster feedback
./skills/glm-review/scripts/review.py \
  --target "src/**/*" \
  --chunk-size 2000 \
  --api-key "$API_KEY"

# Larger chunks for comprehensive review
./skills/glm-review/scripts/review.py \
  --target "src/**/*" \
  --chunk-size 6000 \
  --api-key "$API_KEY"
```

---

## Cost Management

### Token Usage

The review displays token usage per chunk:

```
Chunk 1/3: 3500 tokens in, 1200 tokens out
Chunk 2/3: 3800 tokens in, 1150 tokens out
Chunk 3/3: 4200 tokens in, 1300 tokens out
Total: 11500 tokens in, 3650 tokens out
```

### Rate Limit Handling

- Sequential processing with automatic retries
- Exponential backoff on API rate limits (1s -> 2s -> 4s)
- Progress tracking with resume capability

---

## Custom Prompts

Create custom prompts in `prompts/` directory.

### Example: Security Review

```text
Review this C++ code for security vulnerabilities:
- Buffer overflows
- Use-after-free bugs
- Integer overflows
- Injection attacks
- Cryptographic issues

Provide:
1. Severity level (Critical/High/Medium/Low)
2. Attack scenario
3. Recommended fix with code example
```

### Example: Performance Review

```text
Review this C++ code for performance issues:
- Unnecessary copies
- Missing const correctness
- Inefficient algorithms
- Memory allocation patterns
- Loop optimizations

Provide:
1. Performance impact estimate
2. Optimized code example
3. Benchmarking suggestions
```

---

## Git Integration

### Review Staged Changes

```bash
# Get staged files and run review
git diff --name-only --cached | grep -E '\.(cpp|h|hpp)$' | \
  xargs ./skills/glm-review/scripts/review.py \
  --target "$(cat -)" \
  --api-key "$API_KEY"
```

### Review Recent Commits

```bash
# Review files changed in last 3 commits
./skills/glm-review/scripts/review.py \
  --target "$(git diff --name-only HEAD~3 HEAD | grep -E '\.(cpp|h|hpp)$' | tr '\n' ',')" \
  --api-key "$API_KEY"
```

---

## Troubleshooting

| Issue | Solution |
|-------|----------|
| `ModuleNotFoundError: openai` | Install prerequisites: `pip install -r skills/glm-review/requirements.txt` |
| `API key required` | Set `--api-key` argument or `GLM_API_KEY` environment variable |
| `Rate limit exceeded` | Reduce `--chunk-size` or wait between runs |
| `No files found` | Check glob pattern; `src/**/*.cpp` requires shell expansion |
| `Output file not created` | Check write permissions for output directory |
| `Chunk too large` | Reduce `--chunk-size` or split target files |

---

## Best Practices

1. **Start with small chunks**: Use `--chunk-size 2000` for initial review
2. **Use specific targets**: Review changed files rather than entire codebase
3. **Customize prompts**: Tailor prompts to your code quality standards
4. **Iterative review**: Address issues in first pass, then re-review
5. **Track token usage**: Monitor API costs with chunk-by-chunk reporting
6. **Use `--dry-run` first**: Preview files and token count before spending API credits
7. **Use `--simulate` for testing**: Validate your setup without API calls

---

## Limitations

- **V1 Scope**: Single prompt per review session (no conversation)
- **Output format**: Markdown only (no JSON/SARIF)
- **File size**: Limited by token count per chunk
- **Language**: Optimized for C++ (C++14/17/20)
- **Files > 5000 lines**: Skipped with warning
