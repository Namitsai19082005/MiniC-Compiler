import subprocess
import pytest
import os
import tempfile

# Resolve the minic binary path relative to this conftest
PROJECT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
MINIC = os.path.join(PROJECT_DIR, 'build', 'minic')


@pytest.fixture
def compile_and_run():
    """Fixture: compile .mc → .s → executable → capture stdout."""
    def _run(source_path, extra_args=None, input_text=None, expect_compile_fail=False):
        with tempfile.TemporaryDirectory() as tmpdir:
            asm_path = os.path.join(tmpdir, 'output.s')
            exe_path = os.path.join(tmpdir, 'output')

            # Resolve source path relative to tests/
            if not os.path.isabs(source_path):
                source_path = os.path.join(os.path.dirname(__file__), source_path)

            # Step 1: Compile .mc → .s
            cmd = [MINIC, source_path, '-o', asm_path]
            if extra_args:
                cmd.extend(extra_args)
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)

            if expect_compile_fail:
                return result.stderr, result.returncode

            assert result.returncode == 0, \
                f"Compilation failed (rc={result.returncode}):\n{result.stderr}"

            # Step 2: Assemble + link
            link_result = subprocess.run(
                ['gcc', '-no-pie', '-g', asm_path, '-o', exe_path, '-lc'],
                capture_output=True, text=True, timeout=30
            )
            assert link_result.returncode == 0, \
                f"Assembly/linking failed:\n{link_result.stderr}"

            # Step 3: Run executable
            run_result = subprocess.run(
                [exe_path], capture_output=True, text=True,
                input=input_text, timeout=10
            )
            return run_result.stdout, run_result.returncode
    return _run


@pytest.fixture
def compile_only():
    """Fixture: compile .mc → .s only (no assemble/link)."""
    def _run(source_path, extra_args=None):
        with tempfile.TemporaryDirectory() as tmpdir:
            asm_path = os.path.join(tmpdir, 'output.s')
            if not os.path.isabs(source_path):
                source_path = os.path.join(os.path.dirname(__file__), source_path)
            cmd = [MINIC, source_path, '-o', asm_path]
            if extra_args:
                cmd.extend(extra_args)
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
            asm_content = ''
            if os.path.exists(asm_path):
                with open(asm_path) as f:
                    asm_content = f.read()
            return result.returncode, result.stderr, asm_content
    return _run


@pytest.fixture
def compile_multi_tu():
    """Fixture: compile multiple TUs and link them."""
    def _run(main_source, link_sources, extra_args=None):
        with tempfile.TemporaryDirectory() as tmpdir:
            exe_path = os.path.join(tmpdir, 'output')

            # Resolve paths
            test_dir = os.path.dirname(__file__)
            main_path = main_source if os.path.isabs(main_source) else os.path.join(test_dir, main_source)
            link_paths = [
                s if os.path.isabs(s) else os.path.join(test_dir, s)
                for s in link_sources
            ]

            cmd = [MINIC, main_path, '-o', exe_path, '--link'] + link_paths
            if extra_args:
                cmd.extend(extra_args)
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)

            if result.returncode != 0:
                return '', result.returncode, result.stderr

            run_result = subprocess.run(
                [exe_path], capture_output=True, text=True, timeout=10
            )
            return run_result.stdout, run_result.returncode, ''
    return _run
