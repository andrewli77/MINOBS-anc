n = 20
instance = "child"


mapFile = open("mappings/" + instance + ".mapping")
mapping = dict()
rmapping = dict()


modelStringCache = dict()
modelStringCache["asia"] = "[asia][tub|asia][smoke][lung|smoke][bronc|smoke][either|lung:tub][xray|either][dysp|bronc:either]"
modelStringCache["alarm"] = "[HYPOVOLEMIA][LVFAILURE][ERRLOWOUTPUT][ERRCAUTER][INSUFFANESTH][ANAPHYLAXIS][KINKEDTUBE][FIO2][PULMEMBOLUS][INTUBATION][DISCONNECT][MINVOLSET][HISTORY|LVFAILURE][LVEDVOLUME|HYPOVOLEMIA:LVFAILURE][STROKEVOLUME|HYPOVOLEMIA:LVFAILURE][TPR|ANAPHYLAXIS][PAP|PULMEMBOLUS][SHUNT|INTUBATION:PULMEMBOLUS][VENTMACH|MINVOLSET][CVP|LVEDVOLUME][PCWP|LVEDVOLUME][VENTTUBE|DISCONNECT:VENTMACH][PRESS|INTUBATION:KINKEDTUBE:VENTTUBE][VENTLUNG|INTUBATION:KINKEDTUBE:VENTTUBE][MINVOL|INTUBATION:VENTLUNG][VENTALV|INTUBATION:VENTLUNG][PVSAT|FIO2:VENTALV][ARTCO2|VENTALV][EXPCO2|ARTCO2:VENTLUNG][SAO2|PVSAT:SHUNT][CATECHOL|ARTCO2:INSUFFANESTH:SAO2:TPR][HR|CATECHOL][HRBP|ERRLOWOUTPUT:HR][HREKG|ERRCAUTER:HR][HRSAT|ERRCAUTER:HR][CO|HR:STROKEVOLUME][BP|CO:TPR]"
modelStringCache["child"] = "[BirthAsphyxia][Disease|BirthAsphyxia][LVH|Disease][DuctFlow|Disease][CardiacMixing|Disease][LungParench|Disease][LungFlow|Disease][Sick|Disease][HypDistrib|DuctFlow:CardiacMixing][HypoxiaInO2|CardiacMixing:LungParench][CO2|LungParench][ChestXray|LungParench:LungFlow][Grunting|LungParench:Sick][LVHreport|LVH][Age|Disease:Sick][LowerBodyO2|HypDistrib:HypoxiaInO2][RUQO2|HypoxiaInO2][CO2Report|CO2][XrayReport|ChestXray][GruntingReport|Grunting]"
modelStringCache["sachs"] = "[PKC][Plcg][PIP3|Plcg][PKA|PKC][Jnk|PKA:PKC][P38|PKA:PKC][PIP2|PIP3:Plcg][Raf|PKA:PKC][Mek|PKA:PKC:Raf][Erk|Mek:PKA][Akt|Erk:PKA]"
modelStringCache["insurance"] = "[Age][Mileage][SocioEcon|Age][GoodStudent|SocioEcon:Age][RiskAversion|Age:SocioEcon][OtherCar|SocioEcon][VehicleYear|SocioEcon:RiskAversion][MakeModel|SocioEcon:RiskAversion][SeniorTrain|Age:RiskAversion][HomeBase|RiskAversion:SocioEcon][AntiTheft|RiskAversion:SocioEcon][RuggedAuto|MakeModel:VehicleYear][Antilock|MakeModel:VehicleYear][DrivingSkill|Age:SeniorTrain][CarValue|MakeModel:VehicleYear:Mileage][Airbag|MakeModel:VehicleYear][DrivQuality|DrivingSkill:RiskAversion][Theft|AntiTheft:HomeBase:CarValue][Cushioning|RuggedAuto:Airbag][DrivHist|DrivingSkill:RiskAversion][Accident|Antilock:Mileage:DrivQuality][ThisCarDam|Accident:RuggedAuto][OtherCarCost|Accident:RuggedAuto][MedCost|Accident:Age:Cushioning][ILiCost|Accident][ThisCarCost|ThisCarDam:CarValue:Theft][PropCost|OtherCarCost:ThisCarCost]"

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

	print(numMissing + numExtra + numReversed)
	return (numMissing + numExtra + numReversed, numMissing, numExtra, numReversed)


trueBN = model2network(modelStringCache[instance])

modelFile = open(instance + "_results")

scoreTotals = [0 for i in range(200)]
shdTotals = [0 for i in range(200)]
missingTotals = [0 for i in range(200)]
extraTotals = [0 for i in range(200)]
reversedTotals = [0 for i in range(200)]
counts = [0 for i in range(200)]

while True:
	line1 = modelFile.readline()
	if (line1.strip() == ""):
		break

	model = modelFile.readline()
	line3 = modelFile.readline()

	size = int(line1)
	score = int(line3)

	scoreTotals[size] += score


	info = hammingDAG(trueBN, model2network(model))
	shdTotals[size] +=  info[0]
	missingTotals[size] += info [1]
	extraTotals[size] += info [2]
	reversedTotals[size] += info [3]
	counts[size] += 1

for i in range(200):
	if counts[i] != 0:
		#assert(counts[i] == 1 or counts[i] == 5)
		print("Size: %d \t Avg Score: %f \t Avg SHD %f \t Avg Missing %f \t Avg Extra %f \t Avg Reversed %f" %(i, scoreTotals[i]/counts[i], shdTotals[i]/counts[i], missingTotals[i]/counts[i], extraTotals[i]/counts[i], reversedTotals[i]/counts[i]))