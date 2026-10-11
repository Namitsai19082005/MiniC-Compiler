"""End-to-end tests: compile .mc → .s → executable → assert stdout."""
import pytest


class TestBasics:
    def test_hello(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/basics/hello.mc')
        assert stdout.strip() == '42'
        assert rc == 0

    def test_arithmetic(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/basics/arithmetic.mc')
        assert stdout.strip() == '30'
        assert rc == 0

    def test_divmod(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/basics/divmod.mc')
        lines = stdout.strip().split('\n')
        assert lines[0] == '33'
        assert lines[1] == '1'

    def test_unary(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/basics/unary.mc')
        lines = stdout.strip().split('\n')
        assert lines[0] == '-5'
        assert lines[1] == '1'
        assert lines[2] == '-1'

    def test_bitwise(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/basics/bitwise.mc')
        lines = stdout.strip().split('\n')
        assert lines[0] == '2'   # 6 & 3
        assert lines[1] == '7'   # 6 | 3
        assert lines[2] == '5'   # 6 ^ 3
        assert lines[3] == '24'  # 6 << 2
        assert lines[4] == '3'   # 6 >> 1

    def test_comparison(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/basics/comparison.mc')
        lines = stdout.strip().split('\n')
        assert lines == ['1', '1', '1', '1', '1', '1', '0']

    def test_logical(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/basics/logical.mc')
        lines = stdout.strip().split('\n')
        assert lines == ['1', '0', '1', '0']

    def test_compound_assign(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/basics/compound_assign.mc')
        lines = stdout.strip().split('\n')
        assert lines[0] == '15'   # 10 + 5
        assert lines[1] == '12'   # 15 - 3
        assert lines[2] == '24'   # 12 * 2
        assert lines[3] == '6'    # 24 / 4


class TestControlFlow:
    def test_if_else(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/control/if_else.mc')
        assert stdout.strip() == '1'

    def test_while_loop(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/control/while_loop.mc')
        assert stdout.strip() == '55'

    def test_for_loop(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/control/for_loop.mc')
        assert stdout.strip() == '55'

    def test_nested_loops(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/control/nested_loops.mc')
        assert stdout.strip() == '25'


class TestFunctions:
    def test_basic_func(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/functions/basic_func.mc')
        assert stdout.strip() == '7'

    def test_recursion(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/functions/recursion.mc')
        assert stdout.strip() == '120'

    def test_multi_func(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/functions/multi_func.mc')
        lines = stdout.strip().split('\n')
        assert lines[0] == '10'   # clamp(15, 0, 10)
        assert lines[1] == '0'    # clamp(-5, 0, 10)
        assert lines[2] == '5'    # clamp(5, 0, 10)


class TestPointers:
    def test_basic_ptr(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/pointers/basic_ptr.mc')
        lines = stdout.strip().split('\n')
        assert lines[0] == '10'
        assert lines[1] == '20'

    def test_swap(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/pointers/swap.mc')
        lines = stdout.strip().split('\n')
        assert lines[0] == '20'
        assert lines[1] == '10'


class TestScopes:
    def test_nested_shadow(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/scopes/nested_shadow.mc')
        lines = stdout.strip().split('\n')
        assert lines[0] == '20'
        assert lines[1] == '10'

    def test_deep_nesting(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/scopes/deep_nesting.mc')
        lines = stdout.strip().split('\n')
        assert lines == ['3', '2', '1']


class TestOptimizations:
    def test_const_fold(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/optimizations/const_fold.mc')
        lines = stdout.strip().split('\n')
        assert lines[0] == '5'
        assert lines[1] == '20'
        assert lines[2] == '4'

    def test_copy_prop(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/optimizations/copy_prop.mc')
        assert stdout.strip() == '15'

    def test_dead_code(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/optimizations/dead_code.mc')
        assert stdout.strip() == '3'


class TestRegisterSpill:
    def test_many_vars(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/register_spill/many_vars.mc')
        assert stdout.strip() == '120'


class TestDebugInfo:
    def test_dwarf_sections(self, compile_only):
        rc, stderr, asm = compile_only('inputs/basics/hello.mc')
        assert rc == 0
        assert '.file 1' in asm
        assert '.loc 1' in asm
        assert '.debug_info' in asm
        assert '.debug_abbrev' in asm
        assert 'DW_TAG_compile_unit' in asm
        assert 'DW_TAG_subprogram' in asm
