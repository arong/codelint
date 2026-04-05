#!/bin/bash
# Install git hooks for the repository

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOOKS_DIR="$REPO_ROOT/.git/hooks"

echo "Installing git hooks..."

if [ ! -d "$REPO_ROOT/.git" ]; then
    echo -e "${RED}Error: Not a git repository${NC}"
    exit 1
fi

# Install pre-commit hook
cat > "$HOOKS_DIR/pre-commit" << 'HOOK_EOF'
#!/bin/bash
set -euo pipefail
REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'
EXCLUDE_FILE="$REPO_ROOT/scripts/.format-exclude"
declare -a EXCLUDE_PATTERNS
EXCLUDE_PATTERNS=("build/" ".sisyphus/" "node_modules/" ".git/" "third-party/" "vendor/" "include/CLI/")
if [ -f "$EXCLUDE_FILE" ]; then
    while IFS= read -r pattern; do
        [[ -z "$pattern" || "$pattern" =~ ^# ]] && continue
        EXCLUDE_PATTERNS+=("$pattern")
    done < "$EXCLUDE_FILE"
fi
should_exclude() {
    local file="$1"
    for pattern in "${EXCLUDE_PATTERNS[@]}"; do
        [[ "$file" == *"$pattern"* ]] && return 0
    done
    return 1
}
echo "Running pre-commit auto-fixes..."
STAGED_FILES=$(git diff --cached --name-only --diff-filter=ACM)
if [ -z "$STAGED_FILES" ]; then
    echo -e "${GREEN}No files to check${NC}"
    exit 0
fi
FILES_MODIFIED=0
for file in $STAGED_FILES; do
    if should_exclude "$file"; then continue; fi
    if [[ "$file" =~ \.(png|jpg|gif|ico|pdf|bin|lock)$ ]]; then continue; fi
    if [ -f "$file" ]; then
        if grep -qE '[[:space:]]+$' "$file" 2>/dev/null; then
            sed -i '' 's/[[:space:]]*$//' "$file"
            echo -e "${BLUE}  Fixed: $file${NC}"
            FILES_MODIFIED=1
        fi
        if [ -s "$file" ] && [ "$(tail -c 1 "$file" | wc -l)" -eq 0 ]; then
            echo "" >> "$file"
            FILES_MODIFIED=1
        fi
    fi
done
for file in $STAGED_FILES; do
    if should_exclude "$file"; then continue; fi
    if [[ ! "$file" =~ \.(cpp|cc|cxx|h|hpp|c)$ ]]; then continue; fi
    if [ -f "$file" ] && command -v clang-format &> /dev/null; then
        if ! clang-format --dry-run --Werror "$file" &> /dev/null; then
            clang-format -i "$file"
            echo -e "${BLUE}  Formatted: $file${NC}"
            FILES_MODIFIED=1
        fi
    fi
done
if [ $FILES_MODIFIED -eq 1 ]; then
    for file in $STAGED_FILES; do [ -f "$file" ] && git add "$file"; done
    echo -e "${GREEN}✅ Auto-fixes applied!${NC}"
else
    echo -e "${GREEN}✅ Checks passed!${NC}"
fi
exit 0
HOOK_EOF

chmod +x "$HOOKS_DIR/pre-commit"
echo -e "${GREEN}✓ Installed: pre-commit${NC}"

# Install commit-msg hook
cat > "$HOOKS_DIR/commit-msg" << 'HOOK_EOF'
#!/bin/bash
set -euo pipefail
BRANCH=$(git rev-parse --abbrev-ref HEAD)
if [[ "$BRANCH" =~ ^(develop|main|master|production)$ ]]; then
    echo "BLOCKED: AI cannot commit to $BRANCH"
    exit 1
fi
exit 0
HOOK_EOF

chmod +x "$HOOKS_DIR/commit-msg"
echo -e "${GREEN}✓ Installed: commit-msg${NC}"

echo -e "${GREEN}✅ Done!${NC}"
