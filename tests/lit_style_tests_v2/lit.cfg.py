import lit.formats
import lit.util
import os
import sys

build_dir = os.path.dirname(os.path.abspath(__file__))
project_root = os.path.dirname(os.path.dirname(build_dir))

llvm_dir = os.environ.get('LLVM_DIR', '/opt/homebrew/opt/llvm@21')
clang_tidy_path = os.path.join(llvm_dir, 'bin', 'clang-tidy')
plugin_path = os.path.join(project_root, 'build', 'lib')

plugin_file = None
if os.path.isdir(plugin_path):
    for f in os.listdir(plugin_path):
        if f.startswith('codelint-core') and (f.endswith('.so') or f.endswith('.dylib')):
            plugin_file = os.path.join(plugin_path, f)
            break

config.name = 'CODELINT_LIT_V2'
config.test_format = lit.formats.ShTest(True)
config.suffixes = ['.cpp', '.c', '.h']
config.test_source_root = build_dir
config.test_exec_root = build_dir

check_codelint_py = os.path.join(build_dir, 'check_codelint.py')
config.substitutions.append(
    ('%check_codelint',
     f'{sys.executable} {check_codelint_py} '
     f'--clang-tidy {clang_tidy_path} '
     f'--plugin {plugin_file or "PATH_TO_PLUGIN"}')
)

config.available_features = []
if lit.util.which('FileCheck'):
    config.available_features.append('filecheck')
