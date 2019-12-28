OUTPUT="$(sos -f tests/.sosrc.test "python tests/divide_by_zero.py" | tail -1 | sed 's/\033\[[0-9;]*m//g')"
URL="https://stackoverflow.com/search?q=ZeroDivisionError%3A%20division%20by%20zero%20python"
if [[ "$OUTPUT" !=  *"$URL"* ]]; then
    echo "test failed";
    exit 1;
fi
echo "test passed!";
