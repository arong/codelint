#!/usr/bin/env python3
"""
Convert existing CodeLintTest to lit-style tests.

Usage:
    python convert_to_lit.py <checker_name>

Example:
    python convert_to_lit.py init_checker
"""

import os
import sys
import re
import glob


def parse_check_output(check_output_file):
    """Parse check-output/*.txt and extract CHECK-MESSAGES lines."""
    checks = []
    if not os.path.exists(check_output_file):
        return checks

    with open(check_output_file, 'r') as f:
        content = f.read()

    for line in content.split('\n'):
        if not line.strip():
            continue
        # Parse: path:line:col: level: message [check-name]
        # Example: tests/CodeLintTest/src/init_checker/src/std.cpp:3:5: warning: ...
        match = re.match(r'.+\.cpp:(\d+):(\d+):\s*(warning|error):\s*(.+)\s*\[codelint-.+\]', line)
        if match:
            line_num = int(match.group(1))
            col = int(match.group(2))
            level = match.group(3)
            message = match.group(4)
            checks.append((line_num, col, level, message))

    return checks


def convert_to_lit(source_file, check_output_file, fixed_file, output_file, check_name):
    """Convert a test to lit-style."""
    if not os.path.exists(source_file):
        return False

    with open(source_file, 'r') as f:
        source_lines = f.readlines()

    checks = parse_check_output(check_output_file)

    # Build lit-style test
    output_lines = []
    output_lines.append(f'// RUN: %check_codelint %s {check_name} %t -- -std=c++17\n')

    check_by_line = {}
    for line_num, col, level, message in checks:
        if line_num not in check_by_line:
            check_by_line[line_num] = []
        check_by_line[line_num].append((col, level, message))

    for i, line in enumerate(source_lines, 1):
        if line.strip().startswith('// RUN:') or line.strip().startswith('// CHECK-'):
            continue
        output_lines.append(line)

        if i in check_by_line:
            for col, level, message in check_by_line[i]:
                rel_line = '@LINE'
                check_line = f'// CHECK-MESSAGES: :[{rel_line}]:{col}: {level}: {message} [{check_name}]\n'
                output_lines.append(check_line)

    # Add CHECK-FIXES if fixed file exists
    if fixed_file and os.path.exists(fixed_file):
        with open(fixed_file, 'r') as f:
            fixed_lines = f.readlines()

        # Find differences and add CHECK-FIXES
        # For now, just add the full expected fixed content as a comment block
        output_lines.append('\n')
        output_lines.append('// === Expected Fixed Output ===\n')
        for line in fixed_lines[:20]:  # Limit to first 20 lines for readability
            if line.strip() and not line.strip().startswith('//'):
                # Escape special characters for CHECK-FIXES
                escaped = line.replace('{{', '{{{{').replace('}}', '}}}}')
                output_lines.append(f'// CHECK-FIXES: {escaped.rstrip()}\n')

    with open(output_file, 'w') as f:
        f.writelines(output_lines)

    return True


def convert_checker(checker_name):
    """Convert all tests for a checker."""
    project_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    old_test_dir = os.path.join(project_root, 'tests', 'CodeLintTest', 'src', checker_name)
    new_test_dir = os.path.join(project_root, 'tests', 'lit_style_tests_v2', checker_name)

    if not os.path.exists(old_test_dir):
        print(f"Checker directory not found: {old_test_dir}")
        return 0

    src_dir = os.path.join(old_test_dir, 'src')
    check_output_dir = os.path.join(old_test_dir, 'check-output')
    fixed_dir = os.path.join(old_test_dir, 'fixed')

    if not os.path.exists(src_dir):
        print(f"Source directory not found: {src_dir}")
        return 0

    # Determine check name from checker name
    check_name_map = {
        'init_checker': 'codelint-init',
        'lint_code_checker': 'codelint-lint-code',
        'global_checker': 'codelint-global',
        'singleton_checker': 'codelint-singleton',
        'strict_bool_condition_checker': 'codelint-strict-bool-condition',
        'signed_to_unsigned_checker': 'codelint-signed-to-unsigned-return',
        'global_const_string_checker': 'codelint-global-const-string',
    }
    check_name = check_name_map.get(checker_name, checker_name)

    converted = 0
    for src_file in glob.glob(os.path.join(src_dir, '*.cpp')):
        test_name = os.path.basename(src_file)
        check_output_file = os.path.join(check_output_dir, test_name.replace('.cpp', '.txt'))
        fixed_file = os.path.join(fixed_dir, test_name) if os.path.exists(fixed_dir) else None
        output_file = os.path.join(new_test_dir, test_name)

        if convert_to_lit(src_file, check_output_file, fixed_file, output_file, check_name):
            print(f"Converted: {test_name}")
            converted += 1

    return converted


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python convert_to_lit.py <checker_name>")
        print("Example: python convert_to_lit.py init_checker")
        sys.exit(1)

    checker_name = sys.argv[1]
    converted = convert_checker(checker_name)
    print(f"\nTotal converted: {converted} files")
