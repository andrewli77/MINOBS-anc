base = "water"

inF = open("constraints/" + base + ".txt")


lines = [line.strip() for line in inF.read().split("\n") if (line.strip() != "") ]
count = 1

while len(lines) != 0:
	line = lines.pop(0)
	line = line.strip()

	if line == "":
		continue


	m = int(line)

	outF = open("constraints/" + base + "/" + base + "." + str(m) + "-" + str(count), "w")
	outF.write(str(m) + "\n")

	for j in range(m):
		line = lines.pop(0).strip()
		outF.write(line + "\n")

	count += 1