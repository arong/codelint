#!/bin/bash
# Clean up text files: remove trailing whitespace and ensure trailing newline
# Usage: ./scripts/cleanup-text.sh [directory]

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TARGET_DIR="${1:-$PROJECT_ROOT}"
EXCLUDE_FILE="$(dirname "${BASH_SOURCE[0]}")/.format-exclude"

echo "Cleaning text files in $TARGET_DIR..."

# Load exclude patterns
declare -a EXCLUDE_PATTERNS
EXCLUDE_PATTERNS=("build/" ".sisyphus/" "node_modules/" ".git/")

if [ -f "$EXCLUDE_FILE" ]; then
    while IFS= read -r pattern; do
        [[ -z "$pattern" || "$pattern" =~ ^# ]] && continue
        EXCLUDE_PATTERNS+=("$pattern")
    done < "$EXCLUDE_FILE"
fi

should_exclude() {
    local file="$1"
    for pattern in "${EXCLUDE_PATTERNS[@]}"; do
        if [[ "$file" == *"$pattern"* ]]; then
            return 0
        fi
    done
    return 1
}

find "$TARGET_DIR" -type f \( -name "*.md" -o -name "*.txt" -o -name "*.py" -o -name "*.sh" -o -name "*.yml" -o -name "*.yaml" -o -name "*.json" -o -name "CMakeLists.txt" -o -name ".gitignore" \) | while read -r file; do
    if should_exclude "$file"; then
        echo -e "${GREEN}  Skip: $file${NC}"
        continue
    fi

    if [ -f "$file" ]; then
        MODIFIED=0
        if grep -qE '[[:space:]]+$' "$file" 2>/dev/null; then
            sed -i '' 's/[[:space:]]*$//' "$file"
            echo -e "${BLUE}  Fixed whitespace: $file${NC}"
            MODIFIED=1
        fi
        if [ -s "$file" ] && [ "$(tail -c 1 "$file" | wc -l)" -eq 0 ]; then
            echo "" >> "$file"
            echo -e "${BLUE}  Added newline: $file${NC}"
            MODIFIED=1
        fi
        [ $MODIFIED -eq 0 ] && echo -e "${GREEN}  Clean: $file${NC}"
    fi
done

echo -e "${GREEN}Done!${NC}"
