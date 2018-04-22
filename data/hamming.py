n = 5
instance = "cancer"
dataSize = "1000"

mapFile = open("mappings/" + instance + ".mapping")
mapping = dict()
rmapping = dict()


modelStringCache = dict()
modelStringCache["asia"] = "[asia][tub|asia][smoke][lung|smoke][bronc|smoke][either|lung:tub][xray|either][dysp|bronc:either]"
modelStringCache["alarm"] = "[HYPOVOLEMIA][LVFAILURE][ERRLOWOUTPUT][ERRCAUTER][INSUFFANESTH][ANAPHYLAXIS][KINKEDTUBE][FIO2][PULMEMBOLUS][INTUBATION][DISCONNECT][MINVOLSET][HISTORY|LVFAILURE][LVEDVOLUME|HYPOVOLEMIA:LVFAILURE][STROKEVOLUME|HYPOVOLEMIA:LVFAILURE][TPR|ANAPHYLAXIS][PAP|PULMEMBOLUS][SHUNT|INTUBATION:PULMEMBOLUS][VENTMACH|MINVOLSET][CVP|LVEDVOLUME][PCWP|LVEDVOLUME][VENTTUBE|DISCONNECT:VENTMACH][PRESS|INTUBATION:KINKEDTUBE:VENTTUBE][VENTLUNG|INTUBATION:KINKEDTUBE:VENTTUBE][MINVOL|INTUBATION:VENTLUNG][VENTALV|INTUBATION:VENTLUNG][PVSAT|FIO2:VENTALV][ARTCO2|VENTALV][EXPCO2|ARTCO2:VENTLUNG][SAO2|PVSAT:SHUNT][CATECHOL|ARTCO2:INSUFFANESTH:SAO2:TPR][HR|CATECHOL][HRBP|ERRLOWOUTPUT:HR][HREKG|ERRCAUTER:HR][HRSAT|ERRCAUTER:HR][CO|HR:STROKEVOLUME][BP|CO:TPR]"
modelStringCache["child"] = "[BirthAsphyxia][Disease|BirthAsphyxia][LVH|Disease][DuctFlow|Disease][CardiacMixing|Disease][LungParench|Disease][LungFlow|Disease][Sick|Disease][HypDistrib|DuctFlow:CardiacMixing][HypoxiaInO2|CardiacMixing:LungParench][CO2|LungParench][ChestXray|LungParench:LungFlow][Grunting|LungParench:Sick][LVHreport|LVH][Age|Disease:Sick][LowerBodyO2|HypDistrib:HypoxiaInO2][RUQO2|HypoxiaInO2][CO2Report|CO2][XrayReport|ChestXray][GruntingReport|Grunting]"
modelStringCache["sachs"] = "[PKC][Plcg][PIP3|Plcg][PKA|PKC][Jnk|PKA:PKC][P38|PKA:PKC][PIP2|PIP3:Plcg][Raf|PKA:PKC][Mek|PKA:PKC:Raf][Erk|Mek:PKA][Akt|Erk:PKA]"
modelStringCache["insurance"] = "[Age][Mileage][SocioEcon|Age][GoodStudent|SocioEcon:Age][RiskAversion|Age:SocioEcon][OtherCar|SocioEcon][VehicleYear|SocioEcon:RiskAversion][MakeModel|SocioEcon:RiskAversion][SeniorTrain|Age:RiskAversion][HomeBase|RiskAversion:SocioEcon][AntiTheft|RiskAversion:SocioEcon][RuggedAuto|MakeModel:VehicleYear][Antilock|MakeModel:VehicleYear][DrivingSkill|Age:SeniorTrain][CarValue|MakeModel:VehicleYear:Mileage][Airbag|MakeModel:VehicleYear][DrivQuality|DrivingSkill:RiskAversion][Theft|AntiTheft:HomeBase:CarValue][Cushioning|RuggedAuto:Airbag][DrivHist|DrivingSkill:RiskAversion][Accident|Antilock:Mileage:DrivQuality][ThisCarDam|Accident:RuggedAuto][OtherCarCost|Accident:RuggedAuto][MedCost|Accident:Age:Cushioning][ILiCost|Accident][ThisCarCost|ThisCarDam:CarValue:Theft][PropCost|OtherCarCost:ThisCarCost]"
modelStringCache["water"] = "[C_NI_12_00][CKNI_12_00][CBODD_12_00][CKND_12_00][CNOD_12_00][CBODN_12_00][CKNN_12_00][CNON_12_00][C_NI_12_15|C_NI_12_00][CKNI_12_15|CKNI_12_00][CBODD_12_15|C_NI_12_00:CKNI_12_00:CBODD_12_00:CNOD_12_00:CBODN_12_00][CKND_12_15|CKNI_12_00:CKND_12_00:CKNN_12_00][CNOD_12_15|CBODD_12_00:CNOD_12_00:CNON_12_00][CBODN_12_15|CBODD_12_00:CBODN_12_00:CNON_12_00][CKNN_12_15|CKND_12_00:CKNN_12_00][CNON_12_15|CNOD_12_00:CBODN_12_00:CKNN_12_00:CNON_12_00][C_NI_12_30|C_NI_12_15][CKNI_12_30|CKNI_12_15][CBODD_12_30|C_NI_12_15:CKNI_12_15:CBODD_12_15:CNOD_12_15:CBODN_12_15][CKND_12_30|CKNI_12_15:CKND_12_15:CKNN_12_15][CNOD_12_30|CBODD_12_15:CNOD_12_15:CNON_12_15][CBODN_12_30|CBODD_12_15:CBODN_12_15:CNON_12_15][CKNN_12_30|CKND_12_15:CKNN_12_15][CNON_12_30|CNOD_12_15:CBODN_12_15:CKNN_12_15:CNON_12_15][C_NI_12_45|C_NI_12_30][CKNI_12_45|CKNI_12_30][CBODD_12_45|C_NI_12_30:CKNI_12_30:CBODD_12_30:CNOD_12_30:CBODN_12_30][CKND_12_45|CKNI_12_30:CKND_12_30:CKNN_12_30][CNOD_12_45|CBODD_12_30:CNOD_12_30:CNON_12_30][CBODN_12_45|CBODD_12_30:CBODN_12_30:CNON_12_30][CKNN_12_45|CKND_12_30:CKNN_12_30][CNON_12_45|CNOD_12_30:CBODN_12_30:CKNN_12_30:CNON_12_30]"
modelStringCache["barley"] = "[jordtype][komm][forfrugt][pesticid][saatid][sort][nplac][saamng][tkvs][partigerm][nedbarea|komm][aar_mod|komm:jordtype][potnmin|jordtype:forfrugt][exptgens|jordtype:forfrugt:pesticid][rokap|jordtype][srtprot|sort][dg25|saatid][saakern|saamng:tkvs][frspdag|saatid][sorttkv|sort][srtsize|sort][nmin|jordtype:nedbarea][nopt|exptgens:pesticid][dgv1059|saatid:rokap][jordinf|frspdag][dgv5980|rokap][jordn|nmin:aar_mod:potnmin][mod_nmin|nmin:aar_mod][markgrm|partigerm:jordinf][bgbyg|dgv1059:dgv5980][ngodnt|forfrugt:exptgens:mod_nmin][ngodnn|nopt:jordn][antplnt|saakern:markgrm][ngodn|ngodnt:ngodnn][nprot|jordn:ngodn][ngtilg|ngodn:nplac:dg25][ntilg|ngtilg:jordn][aks_m2|antplnt:ntilg:dgv1059:sorttkv][keraks|ntilg:dgv1059:aks_m2][aks_vgt|ntilg:dgv5980:aks_m2][ksort|keraks:aks_vgt:srtsize][udb|aks_m2:aks_vgt][tkv|aks_m2:keraks:ntilg:sorttkv][slt22|keraks:aks_vgt:srtsize][s2225|keraks:aks_vgt:srtsize][s2528|keraks:aks_vgt:srtsize][protein|nprot:dgv1059:srtprot:ksort][spndx|ntilg:dgv5980:ksort]"
modelStringCache["cancer"] = "[Pollution][Smoker][Cancer|Pollution:Smoker][Xray|Cancer][Dyspnoea|Cancer]"
modelStringCache["earthquake"] = "[Burglary][Earthquake][Alarm|Burglary:Earthquake][JohnCalls|Alarm][MaryCalls|Alarm]"
modelStringCache["survey"] = "[A][S][E|A:S][O|E][R|E][T|O:R]"
modelStringCache["mildew"] = "[straaling_1][temp_1][meldug_1][lai_0][straaling_2][temp_2][straaling_3][temp_3][straaling_4][temp_4][middel_1][middel_2][middel_3][nedboer_1][nedboer_2][nedboer_3][lai_1|lai_0:meldug_1][foto_1|lai_1:temp_1:straaling_1][mikro_1|lai_1:temp_1:nedboer_1][dm_1|foto_1][meldug_2|middel_1:mikro_1:meldug_1][lai_2|lai_1:meldug_2][foto_2|lai_2:temp_2:straaling_2][mikro_2|lai_2:temp_2:nedboer_2][dm_2|foto_2:dm_1][meldug_3|middel_2:mikro_2:meldug_2][lai_3|lai_2:meldug_3][foto_3|lai_3:temp_3:straaling_3][mikro_3|lai_3:temp_3:nedboer_3][dm_3|foto_3:dm_2][meldug_4|middel_3:mikro_3:meldug_3][lai_4|lai_3:meldug_4][foto_4|lai_4:temp_4:straaling_4][dm_4|foto_4:dm_3][udbytte|dm_4]"


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
			#print("Parents of var ", mapping[varStr], ": ", end="")
			#print("")
			continue

		else:
			var, rest = varStr.split("|")
			pars = rest.split(":")

			intPars = []

			#print("Parents of var ", mapping[var], ": ", end="")
			for par in pars:
				intPars.append(mapping[par])

				network[mapping[par]][mapping[var]] = 1
			intPars.sort()
			#print(" ".join(map(str, intPars)))

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

	return (numMissing + numExtra + numReversed, numMissing, numExtra, numReversed)


trueBN = model2network(modelStringCache[instance])


modelFile = open("bdeu/"+instance + "_" + dataSize +"_results")

scoreTotals = [0 for i in range(600)]
shdTotals = [0 for i in range(600)]
missingTotals = [0 for i in range(600)]
extraTotals = [0 for i in range(600)]
reversedTotals = [0 for i in range(600)]
tmTotals = [0 for i in range(600)]
counts = [0 for i in range(600)]
models = [[] for i in range(600)]

lastSz = -1

while True:
	line1 = modelFile.readline()

	if (not line1):
		break

	if (line1.strip() == "" or line1[0] == "#"):
		continue



	model = modelFile.readline()
	line3 = modelFile.readline()
	line4 = modelFile.readline()

	#print(model)

	

	size = int(line1)
	score = int(line3)
	tm = sum(map(float, line4.strip().split(",")))

	scoreTotals[size] += score


	if (size != lastSz):
		lastSz = size
 
	info = hammingDAG(trueBN, model2network(model))
	shdTotals[size] +=  info[0]
	missingTotals[size] += info [1]
	extraTotals[size] += info [2]
	reversedTotals[size] += info [3]
	tmTotals[size] += tm
	counts[size] += 1
	models[size].append(model)

	#print(size, info)

parsedFile = open(instance + "_results_parsed", "w")
for i in range(600):
	if counts[i] != 0:
		#assert(counts[i] == 1 or counts[i] == 5)
		print("Size: %d \t Avg Score: %f \t Avg SHD: %f \t Avg Missing: %f \t Avg Extra: %f \t Avg Reversed: %f \t Samples: %d \t t: %f\n" %(i, scoreTotals[i]/counts[i], shdTotals[i]/counts[i], missingTotals[i]/counts[i], extraTotals[i]/counts[i], reversedTotals[i]/counts[i], counts[i], tmTotals[i]/counts[i]))

		parsedFile.write(str(i) + "\n")
		for model in models[i]:
			parsedFile.write("\"" + model.strip() + "\",\n")
parsedFile.close()
