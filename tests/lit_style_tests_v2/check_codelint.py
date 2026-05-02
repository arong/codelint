#!/usr/bin/env python3

# Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

"""
Codelint Test Helper for Clang-Tidy

Provides a test harness for Codelint checks using the same infrastructure
as Clang-Tidy's check_clang_tidy.py script.

Usage:
    # From run_lit_tests.sh
    python3 check_codelint.py test_file.cpp check_name temp_dir
         --clang-tidy path_to_clang_tidy
         --plugin path_to_codelint_plugin
         --std c++17

    # From lit-style tests
    // RUN: %check_codelint %s check-name %t -- -std=c++17

    // RUN: %check_codelint %s readability-inconsistent-declaration-parameter-name %t -- \
    // RUN:   -config="{CheckOptions: {readability-inconsistent-declaration-parameter-name.Strict: true}}"
"""

import argparse
import os
import re
import subprocess
import sys
import difflib


class TryRunFailed(Exception):
    pass


def try_run(args, raise_error=True):
    result = subprocess.run(args, capture_output=True, text=True, timeout=600)
    if result.returncode != 0 and raise_error:
        output = result.stdout + result.stderr
        print(f"{' '.join(args)} failed:\n{output}")
        raise TryRunFailed(output)
    return result


def _remove_filecheck_content(text):
    return re.sub(r"// *CHECK-[A-Z0-9\-]*:[^\r\n]*", "//", text)


class MessagePrefix:
    def __init__(self, label):
        self.has_message = False
        self.prefixes = []
        self.label = label

    def check(self, file_check_suffix, input_text):
        prefix = self.label + file_check_suffix
        self.has_message = prefix in input_text
        if self.has_message:
            self.prefixes.append(prefix)
        return self.has_message


class CheckRunner:
    def __init__(self, args, extra_args):
        self.input_file_name = args.test_file
        self.check_name = args.check_name
        self.temp_file_name = args.temp_dir
        self.clang_tidy_binary = args.clang_tidy
        self.plugin_path = args.plugin
        self.std = args.std
        self.check_suffix = args.check_suffix or ['']
        if isinstance(self.check_suffix, str):
            self.check_suffix = [s.strip() for s in self.check_suffix.split(',')]
        self.export_fixes = args.export_fixes

        file_name_with_extension = self.input_file_name
        _, extension = os.path.splitext(file_name_with_extension)
        if extension not in ['.c', '.hpp', '.m', '.mm', '.cu']:
            extension = '.cpp'
        self.temp_file_name = os.path.join(self.temp_file_name, 'test' + extension)

        self.clang_tidy_extra_args = list(extra_args) if extra_args else []
        self.clang_extra_args = []

        if '--' in self.clang_tidy_extra_args:
            i = self.clang_tidy_extra_args.index('--')
            self.clang_extra_args = self.clang_tidy_extra_args[i + 1:]
            self.clang_tidy_extra_args = self.clang_tidy_extra_args[:i]

        if not any(re.match(r'^-?-config(-file)?=', arg)
                   for arg in self.clang_tidy_extra_args):
            self.clang_tidy_extra_args.append('--config={}')

        self.clang_extra_args.append(f'-std={self.std}')

        self.input_text = ''
        self.has_check_fixes = False
        self.has_check_messages = False
        self.has_check_notes = False
        self.expect_no_diagnosis = False
        self.fixes = MessagePrefix('CHECK-FIXES')
        self.messages = MessagePrefix('CHECK-MESSAGES')
        self.notes = MessagePrefix('CHECK-NOTES')
        self.match_partial_fixes = args.match_partial_fixes

    def read_input(self):
        with open(self.input_file_name, 'r', encoding='utf-8') as f:
            self.input_text = f.read()

    def get_prefixes(self):
        for suffix in self.check_suffix:
            if suffix and not re.match(r'^[A-Z0-9\-]+$', suffix):
                sys.exit(
                    'Only A..Z, 0..9 and "-" are allowed in check suffixes, '
                    f'but "{suffix}" was given')

            file_check_suffix = ('-' + suffix) if suffix else ''

            has_check_fix = self.fixes.check(file_check_suffix, self.input_text)
            self.has_check_fixes = self.has_check_fixes or has_check_fix

            has_check_message = self.messages.check(file_check_suffix, self.input_text)
            self.has_check_messages = self.has_check_messages or has_check_message

            has_check_note = self.notes.check(file_check_suffix, self.input_text)
            self.has_check_notes = self.has_check_notes or has_check_note

            if has_check_note and has_check_message:
                sys.exit(
                    f"Please use either {self.notes.prefix} or {self.messages.prefix} "
                    f"but not both")

            if not has_check_fix and not has_check_message and not has_check_note:
                self.expect_no_diagnosis = True

        expect_diagnosis = (
            self.has_check_fixes or self.has_check_messages or self.has_check_notes)
        if self.expect_no_diagnosis and expect_diagnosis:
            sys.exit(
                f"{self.fixes.prefix}, {self.messages.prefix} or "
                f"{self.notes.prefix} not found in the input")
        assert expect_diagnosis or self.expect_no_diagnosis

    def _filter_prefixes(self, prefixes, check_file):
        if check_file == self.input_file_name:
            content = self.input_text
        else:
            with open(check_file, 'r', encoding='utf-8') as f:
                content = f.read()
        return [p for p in prefixes if p in content]

    def prepare_test_inputs(self):
        cleaned_test = _remove_filecheck_content(self.input_text)

        temp_dir = os.path.dirname(self.temp_file_name)
        os.makedirs(temp_dir, exist_ok=True)

        with open(self.temp_file_name, 'w', encoding='utf-8') as f:
            f.write(cleaned_test)
            f.truncate()

        self.original_file_name = self.temp_file_name + '.orig'
        with open(self.original_file_name, 'w', encoding='utf-8') as f:
            f.write(cleaned_test)
            f.truncate()

    def run_clang_tidy(self):
        args = [
            self.clang_tidy_binary,
            self.temp_file_name,
        ]

        if self.plugin_path:
            args.append('--load=' + self.plugin_path)

        if self.export_fixes is not None:
            args.append(f'--export-fixes={self.export_fixes}')
        else:
            args.append('--fix')
            args.append('--fix-errors')

        args.append(f'--checks=-*,{self.check_name}')
        args.extend(self.clang_tidy_extra_args)
        args.append('--')
        args.extend(self.clang_extra_args)

        print(f"Running {repr(args)}...")
        try:
            process_output = subprocess.check_output(
                args, stderr=subprocess.STDOUT, timeout=600
            ).decode(errors='ignore')
        except subprocess.CalledProcessError as e:
            process_output = e.output.decode(errors='ignore')
        except subprocess.TimeoutExpired:
            process_output = 'TIMEOUT'

        print("------------------------ clang-tidy output -----------------------")
        print(process_output)
        print("------------------------------------------------------------------")

        diff_result = subprocess.run(
            ['diff', '-u', self.original_file_name, self.temp_file_name],
            capture_output=True, text=True)
        diff_output = diff_result.stdout or ''

        print("------------------------------ Fixes -----------------------------")
        print(diff_output)
        print("------------------------------------------------------------------")

        return process_output, diff_output

    def check_no_diagnosis(self, clang_tidy_output):
        clean_output = clang_tidy_output.strip()
        if clean_output:
            sys.exit(f"No diagnostics were expected, but found:\n{clean_output}")

    def check_fixes(self):
        if not self.has_check_fixes:
            return

        active_prefixes = self._filter_prefixes(
            self.fixes.prefixes, self.input_file_name)
        if not active_prefixes:
            return

        filecheck_args = [
            'FileCheck',
            f'--input-file={self.temp_file_name}',
            self.input_file_name,
            f'--check-prefixes={",".join(active_prefixes)}',
        ]
        filecheck_args.append('--strict-whitespace')

        try_run(filecheck_args)

    def check_messages(self, clang_tidy_output):
        if not self.has_check_messages:
            return

        active_prefixes = self._filter_prefixes(
            self.messages.prefixes, self.input_file_name)
        if not active_prefixes:
            return

        # Filter to only keep actual diagnostic lines (file:line:col: level: message [check])
        # Remove metadata lines like "Error while processing" and "N errors generated"
        diagnostic_pattern = re.compile(r'^.*:\d+:\d+: (warning|error|note): .*$')
        filtered_output = [
            line for line in clang_tidy_output.splitlines()
            if diagnostic_pattern.match(line)
        ]

        messages_file = self.temp_file_name + '.msg'
        with open(messages_file, 'w', encoding='utf-8') as f:
            f.write('\n'.join(filtered_output))

        try_run([
            'FileCheck',
            f'--input-file={messages_file}',
            self.input_file_name,
            f'--check-prefixes={",".join(active_prefixes)}',
            '-implicit-check-not={{warning|error}}:',
        ])

    def check_notes(self, clang_tidy_output):
        if not self.has_check_notes:
            return

        active_prefixes = self._filter_prefixes(
            self.notes.prefixes, self.input_file_name)
        if not active_prefixes:
            return

        filtered_output = [
            line for line in clang_tidy_output.splitlines()
            if 'note: FIX-IT applied' not in line
        ]

        notes_file = self.temp_file_name + '.notes'
        with open(notes_file, 'w', encoding='utf-8') as f:
            f.write('\n'.join(filtered_output))

        try_run([
            'FileCheck',
            f'--input-file={notes_file}',
            self.input_file_name,
            f'--check-prefixes={",".join(active_prefixes)}',
            '-implicit-check-not={{note|warning|error}}:',
        ])

    def run(self):
        self.read_input()
        if self.export_fixes is None:
            self.get_prefixes()
        self.prepare_test_inputs()
        clang_tidy_output, diff_output = self.run_clang_tidy()

        if self.expect_no_diagnosis:
            self.check_no_diagnosis(clang_tidy_output)
        elif self.export_fixes is None:
            self.check_fixes()
            self.check_messages(clang_tidy_output)
            self.check_notes(clang_tidy_output)


def csv_list(string):
    return string.split(',')


def parse_arguments():
    parser = argparse.ArgumentParser(
        description='Codelint test helper for clang-tidy checks.')
    parser.add_argument('test_file', help='Path to the test file')
    parser.add_argument('check_name', help='Name of the codelint check to run')
    parser.add_argument('temp_dir', help='Temporary directory for test files')
    parser.add_argument('--clang-tidy', default='clang-tidy',
                        help='Path to clang-tidy binary')
    parser.add_argument('--plugin', required=True,
                        help='Path to codelint plugin (.so or .dylib)')
    parser.add_argument('--std', default='c++17',
                        help='C++ standard to use')
    parser.add_argument('-check-suffix', '-check-suffixes',
                        default=[''], type=csv_list,
                        help='Comma-separated list of FileCheck suffixes')
    parser.add_argument('--export-fixes', default=None,
                        help='Export fixes to YAML file instead of verifying')
    parser.add_argument('--match-partial-fixes', action='store_true',
                        help='Allow partial line matches for fixes')

    args, extra_args = parser.parse_known_args()
    return args, extra_args


def main():
    args, extra_args = parse_arguments()
    CheckRunner(args, extra_args).run()


if __name__ == '__main__':
    main()
