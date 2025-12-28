// Test large integer precision preservation
var largeInt = 9007199254740993; // 2^53 + 1, should lose precision in double
print("Original large int:", largeInt);

// Test with indirect eval of large number
var result = (1, eval)("9007199254740993");
print("Result from eval:", result);

// Check if they're equal
print("Equal?", largeInt === result);

// Test arithmetic with large numbers
var a = 9007199254740992; // 2^53, should be exact
var b = 1;
var sum = a + b;
print("2^53 + 1 =", sum);
print("Is 2^53 + 1 exact?", sum === 9007199254740993);
