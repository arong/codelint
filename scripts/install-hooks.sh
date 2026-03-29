#!/bin/bash
set -e

HOOK_DIR="$(git rev-parse --git-dir)/hooks"
ROOT_DIR="$(git rev-parse --show-toplevel)"

echo "🔧 Installing git hooks..."

# Copy pre-commit hook
cat > "$HOOK_DIR/pre-commit" << 'EOF'
#!/bin/bash

# Pre-commit hook to enforce whitespace rules
# Checks staged files for trailing whitespace and ensures exactly one trailing newline

files=$(git diff --cached --name-only --diff-filter=ACM | grep -E '\.(cpp|h|txt|md|sh|py)$')

if [ -z "$files" ]; then
  exit 0
fi

errors=0
fixed=0

for file in $files; do
  if [ -f "$file" ]; then
    needs_fix=false

    # Check trailing whitespace
    if grep -q '[[:blank:]]$' "$file"; then
      echo "  ⚠️  Trailing whitespace in $file"
      needs_fix=true
    fi

    # Check trailing newline
    if [ -s "$file" ]; then
      last_char=$(tail -c 1 "$file" | od -An -tx1 | tr -d ' \n')
      last_two=""
      if [ $(wc -c < "$file") -ge 2 ]; then
        last_two=$(tail -c 2 "$file" | od -An -tx1 | tr -d ' \n')
      fi

      # Check for multiple trailing newlines
      if [ "$last_two" = "0a0a" ]; then
        echo "  ⚠️  Multiple trailing newlines in $file"
        needs_fix=true
      # Check for missing newline
      elif [ "$last_char" != "0a" ]; then
        echo "  ⚠️  Missing trailing newline in $file"
        needs_fix=true
      fi
    fi

    if $needs_fix; then
      # Fix trailing whitespace
      sed -i '' 's/[[:blank:]]*$//' "$file"

      # Fix trailing newline
      while [ "$(tail -c 1 "$file" | od -An -tx1 | tr -d ' \n')" = "0a" ]; do
        sed -i '' '$ d' "$file"
      done
      echo "" >> "$file"

      # Re-stage the fixed file
      git add "$file"
      fixed=$((fixed + 1))
    fi
  fi
done

if [ $fixed -gt 0 ]; then
  echo "✅ Fixed $fixed file(s). Proceeding with commit..."
fi

exit 0
EOF

chmod +x "$HOOK_DIR/pre-commit"

echo "✅ Git hooks installed successfully!"
echo "   Pre-commit hook: $HOOK_DIR/pre-commit"
