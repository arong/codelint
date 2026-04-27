---
name: glm-review
description: >
  AI-powered C++ code review using GLM4.7. Token-aware chunked processing for large codebases (200+ files), markdown output.
  When to use: Use this skill when you need to review C++ code quality, detect bugs, or get improvement suggestions.
  Ideal for: PR review, codebase audit, security review, or performance analysis of C++ projects.
  Not for: Non-C++ languages, real-time analysis, or SARIF/JSON output (markdown only).
---

# GLM-Review Skill

AI-powered C++ code review using GLM4.7 model via OpenAI-compatible API.

---

## When to Use

| Scenario | Use This Skill |
|----------|----------------|
| Review C++ code for bugs and quality issues | Yes |
| PR review before merging | Yes |
| Security or performance audit | Yes (with custom prompts) |
| Large codebase (100+ files) | Yes (chunked processing) |
| Non-C++ languages | No |
| Real-time analysis needed | No (API calls take time) |
| SARIF/JSON output required | No (markdown only) |

---

## Quick Start

```bash
# 1. Install prerequisites
pip install -r skills/glm-review/requirements.txt

# 2. Set API key
export GLM_API_KEY="your-api-key"

# 3. Run review
./skills/glm-review/scripts/review.py \
  --target "src/**/*.cpp" \
  --api-key "$GLM_API_KEY"
```

---

## Core Commands

### Review Files

```bash
# Review single file
./skills/glm-review/scripts/review.py --target "src/main.cpp" --api-key "$GLM_API_KEY"

# Review directory (glob pattern)
./skills/glm-review/scripts/review.py --target "src/**/*.cpp" --api-key "$GLM_API_KEY"

# Review with custom output
./skills/glm-review/scripts/review.py \
  --target "src/**/*.cpp" \
  --api-key "$GLM_API_KEY" \
  --output "review-report.md"
```

### Dry Run (Preview)

```bash
# Preview files and token count without API calls
./skills/glm-review/scripts/review.py \
  --target "src/**/*.cpp" \
  --api-key "$GLM_API_KEY" \
  --dry-run
```

### Test Mode (No API)

```bash
# Test with mock client (no API charges)
./skills/glm-review/scripts/review.py \
  --target "src/**/*.cpp" \
  --simulate
```

---

## See Also

- **Full Reference**: [reference.md](reference.md) - CLI options, review modes, chunking, custom prompts, troubleshooting
- **Prompts**: `prompts/review-system.md` - Default review prompt
