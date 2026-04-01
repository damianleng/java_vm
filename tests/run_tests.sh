#!/bin/bash
# Run all JVM tests. Must be executed from the project root: ./tests/run_tests.sh

PASS=0
FAIL=0

# ── helpers ──────────────────────────────────────────────────────────────────

check() {
    local name="$1"
    local expected="$2"
    local actual
    actual=$(./jvm "tests/classes/${name}.class" 2>&1)
    if [ "$actual" = "$expected" ]; then
        echo "PASS  $name"
        PASS=$((PASS + 1))
    else
        echo "FAIL  $name"
        echo "      expected: $(echo "$expected" | head -1)$([ $(echo "$expected" | wc -l) -gt 1 ] && echo ' ...')"
        echo "      got:      $(echo "$actual"   | head -1)$([ $(echo "$actual"   | wc -l) -gt 1 ] && echo ' ...')"
        FAIL=$((FAIL + 1))
    fi
}

# ── compile ───────────────────────────────────────────────────────────────────

if ! command -v javac &> /dev/null; then
    echo "Error: javac not found. Install the JDK or run inside Docker."
    exit 1
fi

mkdir -p tests/classes
echo "Compiling test files..."
javac -d tests/classes tests/*.java
echo ""

# ── tests ─────────────────────────────────────────────────────────────────────

check Hello "42"

check Arithmetic "8
6
21
5
1"

check Loops "15
3
2
1"

check Objects "7
30"

check Arrays "6
5"

# ── summary ───────────────────────────────────────────────────────────────────

echo ""
echo "Results: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
