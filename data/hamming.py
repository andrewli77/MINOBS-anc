n = 8
instance = "asia"



cmpModels = ["[asia][tub|smoke][smoke][lung|smoke][bronc|smoke][either|tub:lung][xray|either][dysp|bronc:either]"]
mapFile = open("mappings/" + instance + ".mapping")
mapping = dict()
rmapping = dict()


modelStringCache = dict()
modelStringCache["asia"] = "[asia][tub|asia][smoke][lung|smoke][bronc|smoke][either|lung:tub][xray|either][dysp|bronc:either]"
modelStringCache["alarm"] = "[HYPOVOLEMIA][LVFAILURE][ERRLOWOUTPUT][ERRCAUTER][INSUFFANESTH][ANAPHYLAXIS][KINKEDTUBE][FIO2][PULMEMBOLUS][INTUBATION][DISCONNECT][MINVOLSET][HISTORY|LVFAILURE][LVEDVOLUME|HYPOVOLEMIA:LVFAILURE][STROKEVOLUME|HYPOVOLEMIA:LVFAILURE][TPR|ANAPHYLAXIS][PAP|PULMEMBOLUS][SHUNT|INTUBATION:PULMEMBOLUS][VENTMACH|MINVOLSET][CVP|LVEDVOLUME][PCWP|LVEDVOLUME][VENTTUBE|DISCONNECT:VENTMACH][PRESS|INTUBATION:KINKEDTUBE:VENTTUBE][VENTLUNG|INTUBATION:KINKEDTUBE:VENTTUBE][MINVOL|INTUBATION:VENTLUNG][VENTALV|INTUBATION:VENTLUNG][PVSAT|FIO2:VENTALV][ARTCO2|VENTALV][EXPCO2|ARTCO2:VENTLUNG][SAO2|PVSAT:SHUNT][CATECHOL|ARTCO2:INSUFFANESTH:SAO2:TPR][HR|CATECHOL][HRBP|ERRLOWOUTPUT:HR][HREKG|ERRCAUTER:HR][HRSAT|ERRCAUTER:HR][CO|HR:STROKEVOLUME][BP|CO:TPR]"
modelStringCache["child"] = "[BirthAsphyxia][Disease|BirthAsphyxia][LVH|Disease][DuctFlow|Disease][CardiacMixing|Disease][LungParench|Disease][LungFlow|Disease][Sick|Disease][HypDistrib|DuctFlow:CardiacMixing][HypoxiaInO2|CardiacMixing:LungParench][CO2|LungParench][ChestXray|LungParench:LungFlow][Grunting|LungParench:Sick][LVHreport|LVH][Age|Disease:Sick][LowerBodyO2|HypDistrib:HypoxiaInO2][RUQO2|HypoxiaInO2][CO2Report|CO2][XrayReport|ChestXray][GruntingReport|Grunting]"
modelStringCache["sachs"] = "[PKC][Plcg][PIP3|Plcg][PKA|PKC][Jnk|PKA:PKC][P38|PKA:PKC][PIP2|PIP3:Plcg][Raf|PKA:PKC][Mek|PKA:PKC:Raf][Erk|Mek:PKA][Akt|Erk:PKA]"

for i in range(n):
	var = mapFile.readline().strip()
	rmapping[i] = var
	mapping[var] = i

def model2network(model):
	network = [[0]*n for j in range(n)]

	for i in range(n):
		varStr = model[1:model.find("]")]
		model = model[model.find("]") + 1:]


		if (varStr.find("|") == -1):
			continue

		else:
			var, rest = varStr.split("|")
			pars = rest.split(":")

			for par in pars:
				network[mapping[par]][mapping[var]] = 1

	return network


def hammingDAG(trueBN, learnedBN):
	numMissing = 0
	numExtra = 0
	numReversed = 0

	for i in range(n):
		for j in range(n):
			if learnedBN[i][j] == 1 and trueBN[i][j] == 0:
				# Check first for if the reverse arc is present.
				if trueBN[j][i] == 1:
					numReversed += 1
				else:
					numExtra += 1

			elif learnedBN[i][j] == 0 and trueBN[i][j] == 1:
				# Avoid double counting the number of reversed arcs
				if learnedBN[j][i] != 1:
					numMissing += 1

	print("Number of missing arcs: ", numMissing)
	print("Number of extra arcs: ", numExtra)
	print("Number of reversed arcs: ", numReversed)
	print("Hamming distance: ", numMissing + numExtra + numReversed)

	return (numMissing, numExtra, numReversed, numMissing + numExtra + numReversed)


trueBN = model2network(modelStringCache[instance])
missing = 0
extra = 0
reversed = 0
shd = 0


for cmpModel in cmpModels:
	learnedBN = model2network(cmpModel)
	a,b,c,d = hammingDAG(trueBN, learnedBN)
	missing += a
	extra += b
	reversed += c
	shd += d

print("Avg Missing Edges: ", missing/len(cmpModels))
print("Avg Extra Edges: ", extra/len(cmpModels))
print("Avg Reversed Edges: ", reversed/len(cmpModels))
print("Avg SHD: ", shd/len(cmpModels))