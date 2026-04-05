#!/bin/bash
# Format all C/C++ files in the project using clang-format
# Usage: ./scripts/format-all.sh [directory]

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TARGET_DIR="${1:-$PROJECT_ROOT}"
EXCLUDE_FILE="$(dirname "${BASH_SOURCE[0]}")/.format-exclude"

echo "Formatting all C/C++ files in $TARGET_DIR..."
echo ""

if ! command -v clang-format &> /dev/null; then
    echo -e "${RED}Error: clang-format is not installed${NC}"
    exit 1
fi

echo "Using: $(clang-format --version)"

# Load exclude patterns from .format-exclude file
declare -a EXCLUDE_PATTERNS
EXCLUDE_PATTERNS=("build/" ".sisyphus/" "node_modules/" ".git/")

if [ -f "$EXCLUDE_FILE" ]; then
    while IFS= read -r pattern; do
        [[ -z "$pattern" || "$pattern" =~ ^# ]] && continue
        EXCLUDE_PATTERNS+=("$pattern")
    done < "$EXCLUDE_FILE"
fi

# Function to check if file should be excluded
should_exclude() {
    local file="$1"
    for pattern in "${EXCLUDE_PATTERNS[@]}"; do
        if [[ "$file" == *"$pattern"* ]]; then
            return 0
        fi
    done
    return 1
}

PROCESSED=0
MODIFIED=0

find "$TARGET_DIR" -type f \( -name "*.cpp" -o -name "*.cc" -o -name "*.cxx" -o -name "*.h" -o -name "*.hpp" -o -name "*.hh" -o -name "*.hxx" -o -name "*.c" \) | while read -r file; do
    if should_exclude "$file"; then
        echo -e "${GREEN}  Skip (excluded): $file${NC}"
        continue
    fi

    if ! clang-format --dry-run --Werror "$file" &> /dev/null; then
        if clang-format -i "$file" 2>/dev/null; then
            echo -e "${BLUE}  Formatted: $file${NC}"
            echo "MODIFIED" >> /tmp/format_count_$$
        fi
    else
        echo -e "${GREEN}  Already formatted: $file${NC}"
    fi
    echo "PROCESSED" >> /tmp/format_count_$$
done

if [ -f /tmp/format_count_$$ ]; then
    MODIFIED=$(grep -c "MODIFIED" /tmp/format_count_$$ || echo 0)
    PROCESSED=$(grep -c "PROCESSED" /tmp/format_count_$$ || echo 0)
    rm -f /tmp/format_count_$$
fi

echo ""
echo "========================================"
echo "  Total: $PROCESSED | Formatted: $MODIFIED"
echo "========================================"
echo -e "${GREEN}Done!${NC}"
