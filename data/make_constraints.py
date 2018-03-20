


def parseConstraints():
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


def parseConstraintsCaMML():
	lines = [line.strip() for line in inF.read().split("\n") if (line.strip() != "") ]
	count = 1

	mapping = []
	mapFile = open("mappings/" + base + ".mapping")
	mapping = [line.strip() for line in mapFile.read().split("\n") if (line.strip() != "")]
	print(mapping)

	while len(lines) != 0:
		line = lines.pop(0)
		line = line.strip()

		if line == "":
			continue


		m = int(line)

		outF = open("constraints/" + base + "-CaMML" + "/" + base + "." + str(m) + "-" + str(count), "w")

		for j in range(m):
			line = lines.pop(0).strip()
			a, b = line.split(" ")
			outF.write(mapping[int(a)] + " => " + mapping[int(b)] + " 1.0;\n")

		count += 1


base = "asia"

inF = open("constraints/" + base + ".txt")

parseConstraintsCaMML()