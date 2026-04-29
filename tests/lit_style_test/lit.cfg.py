# LLVM Lit Configuration for Codelint Tests
# Adapted from clang-tools-extra/test/lit.cfg.py

import lit.formats
import lit.util
import os
import sys

# Build configuration
build_dir = os.path.dirname(os.path.abspath(__file__))
project_root = os.path.dirname(os.path.dirname(build_dir))

# Find clang-tidy and plugin
llvm_dir = os.environ.get('LLVM_DIR', '/opt/homebrew/opt/llvm@21')
clang_tidy_path = os.path.join(llvm_dir, 'bin', 'clang-tidy')
plugin_path = os.path.join(project_root, 'build', 'lib')

# Find plugin file (.so or .dylib)
plugin_file = None
if os.path.isdir(plugin_path):
    for f in os.listdir(plugin_path):
        if f.startswith('codelint-plugin') and (f.endswith('.so') or f.endswith('.dylib')):
            plugin_file = os.path.join(plugin_path, f)
            break

# Name configuration
config.name = 'CODELINT'
config.test_format = lit.formats.ShTest(True)

# Test suffixes
config.suffixes = ['.cpp', '.c', '.h']

# Test directory
config.test_source_root = os.path.dirname(__file__)
config.test_exec_root = config.test_source_root

# Required tools
required_tools = ['clang-tidy', 'FileCheck']

# Substitution: %check_codelint
check_codelint_py = os.path.join(config.test_source_root, 'check_codelint.py')
config.substitutions.append(
    ('%check_codelint',
     f'{sys.executable} {check_codelint_py} '
     f'--clang-tidy {clang_tidy_path} '
     f'--plugin {plugin_file or "PATH_TO_PLUGIN"} '
     f'%s codelint %t')
)

# Substitution: %s = source file
config.substitutions.append(('%s', '${SOURCE_FILE}'))

# Substitution: %t = temp directory
config.substitutions.append(('%t', '${TMP_DIR}'))

# Features
config.available_features = []

# Check if FileCheck is available
if lit.util.which('FileCheck'):
    config.available_features.append('filecheck')
