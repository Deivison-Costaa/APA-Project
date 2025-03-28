import os
import random

# Configuration
n = 120  # Number of flights
m = 30  # Number of runways

# Generate arrays
random.seed(42)  # For reproducibility (optional)

r = [random.randint(0, 100) for _ in range(n)]
c = [random.randint(5, 30) for _ in range(n)]
p = [random.randint(10, 200) for _ in range(n)]

# Generate symmetric t matrix with zeros on diagonal
t = [[0] * n for _ in range(n)]
for i in range(n):
    for j in range(i + 1, n):
        val = random.randint(5, 30)
        t[i][j] = val
        t[j][i] = val

# Create directory if needed
os.makedirs("Instances", exist_ok=True)

# Write to file
with open("Instances/instance3.txt", "w") as f:
    f.write(f"{n}\n")
    f.write(f"{m}\n\n")
    
    f.write(" ".join(map(str, r)) + "\n")
    f.write(" ".join(map(str, c)) + "\n")
    f.write(" ".join(map(str, p)) + "\n\n")
    
    for row in t:
        f.write(" ".join(map(str, row)) + "\n")

print(f"Instance generated successfully at Instances/instance3.txt")