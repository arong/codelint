#!/usr/bin/env python3
"""
Codelint Test Helper (check_codelint.py)
=========================================

This script is adapted from clang-tidy's check_clang_tidy.py to provide
a simplified lit-based testing approach for codelint checks.

Usage:
    // RUN: %check_codelint %s codelint-init %t -- -std=c++17

CHECK Directives:
    // CHECK-MESSAGES: :[[@LINE-1]]:col: warning: message [check-name]
    // CHECK-FIXES: expected_fixed_code
    // CHECK-MESSAGES-NOT: message that should NOT appear

Key Features:
- Filters CHECK lines from test files
- Runs clang-tidy with codelint plugin
- Uses FileCheck to verify messages and fixes
- Supports line references (@LINE+N, @LINE-N)
- Supports test suffixes for multiple scenarios
"""

import subprocess
import sys
import os
import re
import tempfile
import argparse


def create_temp_file_with_content(content):
    """Create a temporary file with the given content."""
    fd, path = tempfile.mkstemp(suffix='.cpp')
    with os.fdopen(fd, 'w') as f:
        f.write(content)
    return path


def get_check_flag(check_name):
    """Get the clang-tidy --checks flag for a codelint check."""
    all_checks = (
        'codelint-init,codelint-lint-code,-codelint-global,'
        '-codelint-singleton,-codelint-strict-bool-condition,'
        '-codelint-signed-to-unsigned-return,-codelint-global-const-string'
    )

    if check_name == 'codelint-init':
        return '-*,' + check_name
    elif check_name == 'codelint-lint-code':
        return '-*,' + check_name
    elif check_name.startswith('codelint-'):
        return '-*,' + check_name
    else:
        return check_name


def run_clang_tidy(source_file, check_name, temp_dir, clang_tidy_path,
                   plugin_path, extra_args, standards):
    """Run clang-tidy on the source file and return the output."""
    check_flag = get_check_flag(check_name)

    cmd = [
        clang_tidy_path,
        '-p', temp_dir,
        '--load=' + plugin_path,
        '--checks=' + check_flag,
        '--header-filter=.*',
    ]

    # Add fix if we need to verify fixes
    if standards:
        for std in standards.split(','):
            cmd_extra = cmd + ['--', '-std=' + std] + extra_args
            result = subprocess.run(cmd_extra, capture_output=True, text=True)
            if result.returncode != 0 and 'error' not in result.stderr.lower():
                continue
            break

    # Run without fix first to get messages
    result = subprocess.run(
        cmd + [source_file, '--'] + ['-std=c++17'] + extra_args,
        capture_output=True,
        text=True
    )

    return result.stdout + result.stderr


def run_clang_tidy_with_fix(source_file, check_name, temp_dir, clang_tidy_path,
                            plugin_path, extra_args):
    """Run clang-tidy with --fix and return the fixed content."""
    check_flag = get_check_flag(check_name)

    # Create a copy since --fix modifies the file
    fd, temp_copy = tempfile.mkstemp(suffix='.cpp')
    with open(source_file, 'r') as f:
        content = f.read()
    with os.fdopen(fd, 'w') as f:
        f.write(content)

    cmd = [
        clang_tidy_path,
        '-p', temp_dir,
        '--load=' + plugin_path,
        '--checks=' + check_flag,
        '--header-filter=.*',
        '--fix',
        '--fix-errors',
        temp_copy,
        '--',
        '-std=c++17'
    ] + extra_args

    subprocess.run(cmd, capture_output=True)

    with open(temp_copy, 'r') as f:
        fixed_content = f.read()

    os.unlink(temp_copy)
    return fixed_content


def filter_messages(output):
    """Filter codelint warnings/errors from clang-tidy output."""
    lines = []
    in_codelint_warning = False
    count = 0

    for line in output.split('\n'):
        # Check if this is a codelint warning/error line
        if re.search(r'(warning|error):.*\[codelint-', line):
            in_codelint_warning = True
            count = 4  # Include next 4 lines (context)
            lines.append(line)
        elif in_codelint_warning:
            lines.append(line)
            count -= 1
            if count <= 0:
                in_codelint_warning = False

    return '\n'.join(lines)


def resolve_line_reference(line_ref, current_line):
    """Resolve @LINE+N or @LINE-N reference to actual line number."""
    match = re.match(r'@LINE([+-]\d+)?', line_ref)
    if not match:
        return line_ref

    offset = match.group(1)
    if offset is None:
        return current_line

    offset_val = int(offset)
    return current_line + offset_val


def expand_check_line(line, line_num, lines):
    def replace_line_ref(match):
        line_ref = match.group(1)
        resolved_line = resolve_line_reference('@' + line_ref, line_num)
        return str(resolved_line)

    result = re.sub(r'\[@(LINE[+-]?\d*)\]', replace_line_ref, line)
    return result


def extract_check_lines(test_lines, suffix=None):
    check_prefix = 'CHECK'
    if suffix:
        check_prefix = f'CHECK-{suffix}'

    messages_pattern = re.compile(
        rf'//\s*{check_prefix}(-MESSAGES|-MESSAGES-NOT|-FIXES|-FIXES-NOT)?:(.*)$'
    )

    check_lines = []
    for i, line in enumerate(test_lines):
        match = messages_pattern.search(line)
        if match:
            check_type = match.group(1) or '-MESSAGES'
            check_content = match.group(2)

            # Expand @LINE references
            expanded_content = expand_check_line(check_content, i + 1, test_lines)

            check_lines.append((check_type, expanded_content))

    return check_lines


def write_filecheck_input(check_lines, output_file):
    """Write FileCheck input file."""
    with open(output_file, 'w') as f:
        for check_type, content in check_lines:
            # FileCheck expects CHECK: or CHECK-NOT:
            if '-NOT' in check_type:
                f.write(f'CHECK-NOT: {content}\n')
            else:
                f.write(f'CHECK: {content}\n')


def run_filecheck(input_file, check_file):
    """Run FileCheck to verify output."""
    cmd = ['FileCheck', '--input-file', input_file, check_file]
    result = subprocess.run(cmd, capture_output=True, text=True)
    return result.returncode == 0, result.stdout + result.stderr


def main():
    parser = argparse.ArgumentParser(description='Codelint Test Helper')
    parser.add_argument('test_file', help='Path to the test file')
    parser.add_argument('check_name', help='Name of the codelint check')
    parser.add_argument('temp_dir', help='Temporary directory for compile commands')
    parser.add_argument('--clang-tidy', default='clang-tidy',
                        help='Path to clang-tidy binary')
    parser.add_argument('--plugin', required=True,
                        help='Path to codelint plugin')
    parser.add_argument('--extra-arg', action='append', default=[],
                        help='Extra arguments to pass to clang-tidy')
    parser.add_argument('--std', default='c++17',
                        help='C++ standard version')
    parser.add_argument('--check-suffix', default=None,
                        help='Run only tests with this suffix')
    parser.add_argument('--verify-fix', action='store_true',
                        help='Also verify code fixes')

    args = parser.parse_args()

    # Read test file
    with open(args.test_file, 'r') as f:
        test_content = f.read()

    test_lines = test_content.split('\n')

    # Find RUN line to get test configuration
    run_pattern = re.compile(r'//\s*RUN:\s*(.+)')
    run_args = None
    for line in test_lines:
        match = run_pattern.match(line)
        if match:
            run_args = match.group(1)
            break

    # Extract check lines
    check_lines = extract_check_lines(test_lines, args.check_suffix)

    # Separate message and fix checks
    message_checks = [(t, c) for t, c in check_lines if 'FIXES' not in t]
    fix_checks = [(t, c) for t, c in check_lines if 'FIXES' in t]

    # Run clang-tidy to get messages
    messages_output = run_clang_tidy(
        args.test_file,
        args.check_name,
        args.temp_dir,
        args.clang_tidy,
        args.plugin,
        args.extra_arg,
        None
    )

    # Filter to just codelint warnings
    filtered_messages = filter_messages(messages_output)

    if not message_checks:
        if filtered_messages.strip():
            print("FAIL: Expected no warnings but got:")
            print(filtered_messages)
            return 1
        print("PASS: No warnings (as expected)")
        return 0

    message_check_file = create_temp_file_with_content('\n'.join(
        [f'CHECK: {content}' for _, content in message_checks]
    ))
    message_input_file = create_temp_file_with_content(filtered_messages)

    msg_ok, msg_result = run_filecheck(message_input_file, message_check_file)

    # Clean up message check temp files
    os.unlink(message_check_file)
    os.unlink(message_input_file)

    # If fix verification is enabled
    fix_ok = True
    fix_result = ""
    if args.verify_fix and fix_checks:
        fixed_content = run_clang_tidy_with_fix(
            args.test_file,
            args.check_name,
            args.temp_dir,
            args.clang_tidy,
            args.plugin,
            args.extra_arg
        )

        fix_input_file = create_temp_file_with_content(fixed_content)
        fix_check_file = create_temp_file_with_content('\n'.join(
            [f'CHECK: {content}' for _, content in fix_checks]
        ))

        fix_ok, fix_result = run_filecheck(fix_input_file, fix_check_file)

        os.unlink(fix_input_file)
        os.unlink(fix_check_file)

    # Report results
    if msg_ok and (not args.verify_fix or fix_ok):
        return 0
    else:
        print("FileCheck failed!")
        if not msg_ok:
            print("Messages check failed:")
            print(msg_result)
        if not fix_ok:
            print("Fix check failed:")
            print(fix_result)
        return 1


if __name__ == '__main__':
    sys.exit(main())
