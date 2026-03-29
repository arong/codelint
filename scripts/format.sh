#!/bin/bash
set -e

echo "🧹 Cleaning whitespace..."

# Clean trailing whitespace
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
  -exec sed -i '' 's/[[:blank:]]*$//' {} +

# Ensure exactly one trailing newline
for file in $(find . -type f \( \
  -name '*.cpp' -o \
  -name '*.h' -o \
  -name '*.txt' -o \
  -name '*.md' -o \
  -name '*.sh' -o \
  -name '*.py' \
  \) \
  ! -path './build/*' \
  ! -path './.git/*'); do

  # Remove all trailing newlines
  while [ "$(tail -c 1 "$file" | od -An -tx1 | tr -d ' ')" = "0a" ]; do
    sed -i '' '$ d' "$file"
  done

  # Add exactly one newline
  echo "" >> "$file"
done

echo "✅ Whitespace cleanup complete!"
echo "📊 Files processed: $(find . -type f \( -name '*.cpp' -o -name '*.h' -o -name '*.txt' -o -name '*.md' -o -name '*.sh' -o -name '*.py' \) ! -path './build/*' ! -path './.git/*' | wc -l | tr -d ' ')"
