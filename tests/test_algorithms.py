"""Algorithm and edge-case tests."""
import pytest


class TestAlgorithms:
    def test_primes(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/algorithms/primes.mc')
        assert stdout.strip() == '25'  # 25 primes under 100

    def test_fib_iter(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/algorithms/fib_iter.mc')
        lines = stdout.strip().split('\n')
        # First 20 Fibonacci numbers
        fib = [0, 1]
        for i in range(18):
            fib.append(fib[-1] + fib[-2])
        assert lines == [str(f) for f in fib]

    def test_sum_n(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/algorithms/sum_n.mc')
        assert stdout.strip() == '5050'  # sum 1..100

    def test_palindrome(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/algorithms/palindrome.mc')
        lines = stdout.strip().split('\n')
        assert lines[0] == '1'   # 12321 is palindrome
        assert lines[1] == '0'   # 12345 is not

    def test_digit_sum(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/algorithms/digit_sum.mc')
        lines = stdout.strip().split('\n')
        assert lines[0] == '15'  # 1+2+3+4+5
        assert lines[1] == '45'  # 9+9+9+9+9

    def test_collatz(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/algorithms/collatz.mc')
        assert stdout.strip() == '111'  # collatz(27) takes 111 steps


class TestBasicsEdge:
    def test_empty_main(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/basics/empty_main.mc')
        assert rc == 0

    def test_negative(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/basics/negative.mc')
        assert stdout.strip() == '-101'

    def test_large_numbers(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/basics/large_numbers.mc')
        assert stdout.strip() == '1000000000000'

    def test_multi_assign(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/basics/multi_assign.mc')
        lines = stdout.strip().split('\n')
        assert lines == ['10', '20', '30']

    def test_string_print(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/basics/string_print.mc')
        assert stdout.strip() == 'hello world'

    def test_neg_compare(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/basics/neg_compare.mc')
        lines = stdout.strip().split('\n')
        assert lines == ['1', '1', '1', '1', '1', '1']


class TestControlFlowEdge:
    def test_short_circuit(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/control/short_circuit.mc')
        lines = stdout.strip().split('\n')
        assert lines == ['1', '1']  # first and third conditions are true

    def test_cond_in_loop(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/control/cond_in_loop.mc')
        assert stdout.strip() == '2450'  # sum of even numbers 0..98

    def test_while_powers(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/control/while_powers.mc')
        lines = stdout.strip().split('\n')
        assert lines == ['1', '2', '4', '8', '16', '32', '64', '128', '256', '512', '1024']

    def test_triple_nested(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/control/triple_nested.mc')
        lines = stdout.strip().split('\n')
        assert len(lines) == 10  # C(5,2) = 10 combos summing to 3


class TestFunctionsEdge:
    def test_four_ops(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/functions/four_ops.mc')
        lines = stdout.strip().split('\n')
        assert lines == ['13', '7', '30', '3']

    def test_func_with_loop(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/functions/func_with_loop.mc')
        lines = stdout.strip().split('\n')
        assert lines[0] == '1024'  # 1 * 2^10
        assert lines[1] == '96'    # 3 * 2^5

    def test_ackermann(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/functions/ackermann.mc')
        assert stdout.strip() == '61'  # ackermann(3,3) = 61


class TestOptimizationsEdge:
    def test_complex_fold(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/optimizations/complex_fold.mc')
        # (1+2)*(3-1) + (2*3) - (1+2+3)*2 + 1 = 3*2 + 6 - 12 + 1 = 1
        assert stdout.strip() == '1'

    def test_dead_chain(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/optimizations/dead_chain.mc')
        assert stdout.strip() == '600'

    def test_copy_chain(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/optimizations/copy_chain.mc')
        assert stdout.strip() == '5'


class TestScopesEdge:
    def test_cross_scope(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/scopes/cross_scope.mc')
        assert stdout.strip() == '10'  # 1+2+3+4


class TestPointersEdge:
    def test_ptr_modify(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/pointers/ptr_modify.mc')
        lines = stdout.strip().split('\n')
        assert lines[0] == '15'   # 5 + 10
        assert lines[1] == '25'   # 15 + 10

    def test_ptr_reassign(self, compile_and_run):
        stdout, rc = compile_and_run('inputs/pointers/ptr_reassign.mc')
        lines = stdout.strip().split('\n')
        assert lines == ['10', '20', '30']
