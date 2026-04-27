---
name: glm-review
description: C++ code review using GLM4.7 OpenAI-compatible API. Token-aware chunking, markdown output, sequential processing.
---

# GLM-Review Skill

AI-powered C++ code review using GLM4.7 model via OpenAI-compatible API. Provides intelligent code quality analysis with configurable prompts and token management.

---

## Quick Start

```bash
# 1. Install prerequisites
pip install openai tiktoken

# 2. Run review on C++ files
./skills/glm-review/scripts/glm_review.py \
  --target "src/**/*.cpp" \
  --api-key "your-api-key"

# 3. Review single file
./skills/glm-review/scripts/glm_review.py \
  --target "src/main.cpp" \
  --api-key "your-api-key"

# 4. Review with custom prompt
./skills/glm_review/scripts/glm_review.py \
  --target "src/**/*" \
  --prompt-file "skills/glm-review/prompts/safety-review.txt" \
  --api-key "your-api-key"
```

---

## Prerequisites

```bash
pip install openai tiktoken
```

- **openai**: API client for GLM4.7 OpenAI-compatible endpoint
- **tiktoken**: Token counting for cost management and chunking

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

---

## Review Modes

### File-Level Review

```bash
# Review individual files
./skills/glm-review/scripts/glm_review.py \
  --target "src/class_a.cpp" \
  --api-key "$API_KEY"

# Review multiple specific files
./skills/glm-review/scripts/glm_review.py \
  --target "src/class_a.cpp,src/class_b.cpp" \
  --api-key "$API_KEY"
```

### Directory Scan

```bash
# Review all C++ files in directory
./skills/glm-review/scripts/glm_review.py \
  --target "src/**/*.cpp" \
  --api-key "$API_KEY"

# Review both .cpp and .h files
./skills/glm-review/scripts/glm_review.py \
  --target "src/**/*.{cpp,h}" \
  --api-key "$API_KEY"
```

### Custom Prompts

```bash
# Use built-in safety-focused prompt
./skills/glm-review/scripts/glm_review.py \
  --target "src/**/*" \
  --prompt-file "skills/glm-review/prompts/safety-review.txt" \
  --api-key "$API_KEY"

# Use performance-focused prompt
./skills/glm-review/scripts/glm_review.py \
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

---
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
./skills/glm-review/scripts/glm_review.py \
  --target "src/**/*" \
  --chunk-size 2000 \
  --api-key "$API_KEY"

# Larger chunks for comprehensive review
./skills/glm-review/scripts/glm_review.py \
  --target "src/**/*" \
  --chunk-size 6000 \
  --api-key "$API_KEY"
```

---

## Cost Management

### Token Usage

```bash
# Review displays token usage per chunk
# Example output:
# Chunk 1/3: 3500 tokens in, 1200 tokens out
# Chunk 2/3: 3800 tokens in, 1150 tokens out
# Chunk 3/3: 4200 tokens in, 1300 tokens out
# Total: 11500 tokens in, 3650 tokens out
```

### Rate Limit Handling

- Sequential processing with automatic retries
- Exponential backoff on API rate limits
- Progress tracking with resume capability

---

## Custom Prompts

Create custom prompts in `prompts/` directory:

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
# Get staged files
git diff --name-only --cached | grep -E '\.(cpp|h|hpp)$'

# Run review on staged changes
./skills/glm-review/scripts/glm_review.py \
  --target "$(git diff --name-only --cached | grep -E '\.(cpp|h|hpp)$' | tr '\n' ',')" \
  --api-key "$API_KEY"
```

### Review Recent Commits

```bash
# Review files changed in last 3 commits
./skills/glm-review/scripts/glm_review.py \
  --target "$(git diff --name-only HEAD~3 HEAD | grep -E '\.(cpp|h|hpp)$' | tr '\n' ',')" \
  --api-key "$API_KEY"
```

---

## Troubleshooting

| Issue | Solution |
|-------|----------|
| `ModuleNotFoundError: openai` | Install prerequisites: `pip install openai tiktoken` |
| `API key required` | Set `--api-key` argument or environment variable |
| `Rate limit exceeded` | Reduce `--chunk-size` or use sequential mode (default) |
| `No files found` | Check glob pattern: `src/**/*.cpp` requires shell expansion |
| `Output file not created` | Check write permissions for output directory |

---

## Best Practices

1. **Start with small chunks**: Use `--chunk-size 2000` for initial review
2. **Use specific targets**: Review changed files rather than entire codebase
3. **Customize prompts**: Tailor prompts to your code quality standards
4. **Iterative review**: Address issues in first pass, then re-review
5. **Track token usage**: Monitor API costs with chunk-by-chunk reporting

---

## Limitations

- **V1 Scope**: Single prompt per review session (no conversation)
- **Output format**: Markdown only (no JSON/SARIF)
- **File size**: Limited by token count per chunk
- **Language**: Optimized for C++ (C++14/17/20)

---

## License

MIT License