#!/bin/bash
cd /mnt/c/Users/Manas/OneDrive/Desktop/MiniC_Compiler
PASS=0
FAIL=0
TOTAL=0

for f in tests/inputs/*/*.mc; do
    NAME=$(basename "$f" .mc)
    DIR=$(basename $(dirname "$f"))
    TOTAL=$((TOTAL+1))
    
    # Skip multi_tu (needs special handling) and error tests
    if [ "$DIR" = "multi_tu" ] || [ "$DIR" = "errors" ]; then
        continue
    fi
    
    # Compile
    if ! ./wsl_build/minic "$f" -o /tmp/test_out.s 2>/dev/null; then
        echo "COMPILE_FAIL: $DIR/$NAME"
        FAIL=$((FAIL+1))
        continue
    fi
    
    # Assemble & Link
    if ! gcc -no-pie -g /tmp/test_out.s -o /tmp/test_out 2>/dev/null; then
        echo "LINK_FAIL: $DIR/$NAME"
        FAIL=$((FAIL+1))
        continue
    fi
    
    # Run
    OUTPUT=$(/tmp/test_out 2>&1)
    RC=$?
    
    if [ $RC -eq 139 ]; then
        echo "SEGFAULT: $DIR/$NAME"
        FAIL=$((FAIL+1))
    else
        echo "OK: $DIR/$NAME -> $OUTPUT (rc=$RC)"
        PASS=$((PASS+1))
    fi
done

echo ""
echo "=============================="
echo "RESULTS: $PASS passed, $FAIL failed out of $TOTAL total"
echo "=============================="
