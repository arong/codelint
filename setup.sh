#!/bin/bash
# One-time setup after cloning the repository

set -e

echo "🔧 Setting up development environment..."

# Configure git hooks path
git config core.hooksPath .githooks
echo "✅ Git hooks configured (.githooks)"

# Check if hooks are executable
chmod +x .githooks/pre-commit .githooks/commit-msg
echo "✅ Hooks made executable"

echo ""
echo "📋 Active hooks:"
echo "   - pre-commit: clang-format, whitespace checks, sensitive files"
echo "   - commit-msg: format validation, branch restriction (AI: develop only)"
echo ""
echo "✨ Setup complete! Run 'git commit' to trigger hooks."
