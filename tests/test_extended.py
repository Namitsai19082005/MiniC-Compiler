"""Additional end-to-end tests for broader coverage."""
import pytest


class TestBasicsExtended:
    def test_zero(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/basics/zero.mc')
        assert stdout.strip() == '0'

    def test_precedence(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/basics/precedence.mc')
        lines = stdout.strip().split('\n')
        assert lines[0] == '14'   # 2 + 3*4 = 14
        assert lines[1] == '20'   # (2+3)*4 = 20

    def test_parens(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/basics/parens.mc')
        assert stdout.strip() == '21'  # (1+2)*(3+4) = 21

    def test_complex_expr(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/basics/complex_expr.mc')
        assert stdout.strip() == '6'  # 1 + 2*3 - 1 = 6

    def test_return_code(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/basics/return_code.mc')
        assert rc == 42

    def test_char_type(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/basics/char_type.mc')
        assert stdout.strip() == '65'


class TestControlFlowExtended:
    def test_nested_if(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/control/nested_if.mc')
        assert stdout.strip() == '2'

    def test_if_no_else(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/control/if_no_else.mc')
        assert stdout.strip() == '1'

    def test_early_return(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/control/early_return.mc')
        lines = stdout.strip().split('\n')
        assert lines[0] == '42'
        assert lines[1] == '42'


class TestFunctionsExtended:
    def test_fibonacci(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/functions/fibonacci.mc')
        assert stdout.strip() == '55'

    def test_void_func(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/functions/void_func.mc')
        lines = stdout.strip().split('\n')
        assert lines == ['1', '2', '3', '4', '5']

    def test_nested_call(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/functions/nested_call.mc')
        assert stdout.strip() == '42'

    def test_power(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/functions/power.mc')
        lines = stdout.strip().split('\n')
        assert lines[0] == '1024'
        assert lines[1] == '243'

    def test_gcd(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/functions/gcd.mc')
        lines = stdout.strip().split('\n')
        assert lines[0] == '6'
        assert lines[1] == '25'


class TestPointersExtended:
    def test_double_ptr(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/pointers/double_ptr.mc')
        assert stdout.strip() == '10'


class TestScopesExtended:
    def test_loop_scope(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/scopes/loop_scope.mc')
        lines = stdout.strip().split('\n')
        assert lines == ['0', '10', '20']


class TestRegisterSpillExtended:
    def test_heavy_pressure(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/register_spill/heavy_pressure.mc')
        assert stdout.strip() == '210'  # sum 1..20

    def test_deep_expr(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/register_spill/deep_expr.mc')
        # ((1+2)*3+4)*(5+6)-((7*8)+9) = (9+4)*11-(56+9) = 13*11-65 = 143-65 = 78
        assert stdout.strip() == '78'
