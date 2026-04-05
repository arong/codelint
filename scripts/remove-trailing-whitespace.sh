#!/bin/bash
# Script to remove trailing whitespace from all files
# Usage: ./scripts/remove-trailing-whitespace.sh [directory]
# If no directory specified, uses current directory

set -euo pipefail

TARGET_DIR="${1:-.}"
echo "🧹 Removing trailing whitespace from all files in $TARGET_DIR..."

# File extensions to process
EXTENSIONS=(
    "*.cpp"
    "*.cc"
    "*.cxx"
    "*.h"
    "*.hpp"
    "*.hh"
    "*.hxx"
    "*.c"
    "*.txt"
    "*.md"
    "*.sh"
    "*.py"
    "*.yml"
    "*.yaml"
    "*.json"
    "*.cmake"
    "CMakeLists.txt"
)

# Build find pattern
FIND_PATTERN=""
for ext in "${EXTENSIONS[@]}"; do
    if [[ -z "$FIND_PATTERN" ]]; then
        FIND_PATTERN="-name '$ext'"
    else
        FIND_PATTERN="$FIND_PATTERN -o -name '$ext'"
    fi
done

# Count of files modified
MODIFIED_COUNT=0

# Process each file
while IFS= read -r -d '' file; do
    # Skip binary files and hidden directories
    if [[ "$file" == *"/\."* ]] || [[ "$file" == *"/build/"* ]] || [[ "$file" == *"/.git/"* ]] || [[ "$file" == *"/.sisyphus/"* ]]; then
        continue
    fi

    # Check if file has trailing whitespace
    if grep -q '[[:blank:]]$' "$file" 2>/dev/null; then
        # Remove trailing whitespace
        sed -i '' 's/[[:space:]]*$//' "$file"
        echo "  ✓ Fixed: $file"
        MODIFIED_COUNT=$((MODIFIED_COUNT + 1))
    fi
done < <(eval "find $TARGET_DIR -type f \( $FIND_PATTERN \) -print0" 2>/dev/null || true)

if [[ $MODIFIED_COUNT -eq 0 ]]; then
    echo "✅ No trailing whitespace found in any files"
else
    echo "✅ Fixed trailing whitespace in $MODIFIED_COUNT file(s)"
fi

exit 0
