#!/bin/bash
# Auto-fix trailing whitespace and trailing newlines

set -e

echo "🔧 Fixing trailing whitespace..."

# Fix trailing whitespace
find . -type f \( \
  -name '*.cpp' -o \
  -name '*.h' -o \
  -name '*.txt' -o \
  -name '*.md' -o \
  -name '*.sh' -o \
  -name '*.py' -o \
  -name '*.yml' -o \
  -name '*.yaml' \
\) \
  ! -path './build/*' \
  ! -path './.git/*' \
  ! -path './third_party/*' \
  -exec sed -i '' 's/[[:blank:]]*$//' {} +

echo "✅ Trailing whitespace fixed"

echo "🔧 Fixing trailing newlines..."

# Fix trailing newlines (ensure single newline at EOF)
find . -type f \( \
  -name '*.cpp' -o \
  -name '*.h' -o \
  -name '*.txt' -o \
  -name '*.md' -o \
  -name '*.sh' -o \
  -name '*.py' \
\) \
  ! -path './build/*' \
  ! -path './.git/*' \
  ! -path './third_party/*' \
  -exec sh -c '
    for file; do
      # Remove multiple trailing newlines
      while [ "$(tail -c 1 "$file" | od -An -tx1 | tr -d " \n")" = "0a" ] && \
            [ "$(tail -c 2 "$file" | od -An -tx1 | tr -d " \n")" = "0a0a" ]; do
        sed -i "" -e :a -e "/^\n*$/{$d;N;ba" -e "}" "$file" 2>/dev/null || true
      done

      # Ensure file ends with newline
      if [ -n "$(tail -c 1 "$file" 2>/dev/null)" ] && [ "$(tail -c 1 "$file" | od -An -tx1 | tr -d " \n")" != "0a" ]; then
        echo "" >> "$file"
      fi
    done
  ' sh {} +

echo "✅ Trailing newlines fixed"
echo "✨ Formatting complete!"
