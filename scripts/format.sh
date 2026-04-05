#!/bin/bash
# Format staged C/C++ files using clang-format
# Usage: ./scripts/format.sh [files...]
# If no files specified, formats all staged C/C++ files

set -euo pipefail

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if clang-format is installed
if ! command -v clang-format &> /dev/null; then
    echo -e "${RED}Error: clang-format is not installed${NC}"
    echo "Please install clang-format:"
    echo "  macOS: brew install clang-format"
    echo "  Ubuntu/Debian: sudo apt install clang-format"
    exit 1
fi

# Get files to format
if [ $# -gt 0 ]; then
    # Use provided files
    FILES=("$@")
else
    # Get staged C/C++ files
    FILES=()
    while IFS= read -r -d '' file; do
        FILES+=("$file")
    done < <(git diff --cached --name-only -z | grep -zE '\.(cpp|cc|cxx|h|hpp|hh|hxx|c)$' || true)
fi

if [ ${#FILES[@]} -eq 0 ]; then
    echo -e "${GREEN}No C/C++ files to format${NC}"
    exit 0
fi

echo "Formatting C/C++ files with clang-format..."

# Track if any files were modified
MODIFIED=0
FAILED=0

for file in "${FILES[@]}"; do
    # Skip deleted files
    if [ ! -f "$file" ]; then
        continue
    fi

    # Check if file needs formatting
    if ! clang-format --dry-run --Werror "$file" &> /dev/null; then
        echo "  Formatting: $file"
        if clang-format -i "$file"; then
            MODIFIED=$((MODIFIED + 1))
        else
            echo -e "${RED}  Failed to format: $file${NC}"
            FAILED=$((FAILED + 1))
        fi
    fi
done

if [ $FAILED -gt 0 ]; then
    echo -e "${RED}Failed to format $FAILED file(s)${NC}"
    exit 1
fi

if [ $MODIFIED -eq 0 ]; then
    echo -e "${GREEN}All files are already properly formatted${NC}"
else
    echo -e "${GREEN}Formatted $MODIFIED file(s)${NC}"
    echo -e "${YELLOW}Please re-stage the formatted files with: git add <files>${NC}"
fi

exit 0
