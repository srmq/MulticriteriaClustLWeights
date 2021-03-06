/*
 * Algoritmo.cpp
 *
 *  Created on: 12/07/2009
 *      Author: Filipe
 */

#include "Algoritmo.h"
#include "FuzzyIndices.h"
#include "FuzzyCluster.h"
#include "ConfusionMatrix.h"
#include <memory>
#include <vector>
#include <limits>

MinCostFlow Algoritmo::mcf;
string Algoritmo::saida = "";

Algoritmo::Algoritmo(size_t inicializacoes, size_t clusters, size_t prototipos, size_t m, size_t limite, const Repositorio& repositorio, ostream& out) :
	inicializacoes(inicializacoes), clusters(clusters), prototipos(prototipos), repositorio(repositorio), out(out) {
	melhor = atual = Resultado(clusters, prototipos, repositorio.tabela.size(), repositorio.tabela[0].n, m, 0);
	this->individuos = this->repositorio.tabela[0].n;
	this->limite = limite;
}

Algoritmo::~Algoritmo() {
}

int Algoritmo::getBestClusterIndex(int n) { // n is an element
        double bestUi = std::numeric_limits<double>::min();
	    const int clusterNumber = this->melhor.U[0].size();
	    int bestCluster = -1;

        for (int i = 0; i < clusterNumber; i++) {
            double Uni = this->melhor.U[n][i];
            if (Uni > bestUi) {
                bestUi = Uni;
                bestCluster = i;
            }
        }
        return bestCluster;
}

int Algoritmo::getAprioriPartition(util::FuzzyIndices &fuzzyIndices, int n) {
        const size_t prioriClusterNumber = this->repositorio.dados.prioriCluster.size();
        double bestUi = std::numeric_limits<double>::min();
	    int bestPartition = -1;

    	for (size_t i = 0; i < prioriClusterNumber; i++) { //apriori cluster i
            double Uni = fuzzyIndices.getPertinenceDegree(std::make_pair(n, i));
            if (Uni > bestUi) {
                bestPartition = i;
                bestUi = Uni;
            }
    	}
    	return bestPartition;

}

void Algoritmo::executar() {


	this->out << "Resultados:\n\n";

	this->out << "# Numero de inicializacoes: " << this->inicializacoes << "\n";
	this->out << "# Numero de individuos: " << this->individuos << "\n";
	this->out << "# Numero de clusters: " << this->clusters << "\n";
	this->out << "# Numero de prototipos: " << this->prototipos << "\n";
	this->out << "# " << repositorio << "\n";

	this->out << "\n\n";

	for (size_t i = 1; i <= this->inicializacoes; i++) {
		this->atual.init(i);
		while (this->inicializacao())
			;

		// ------------------------------------------------
		size_t iteracao = 1;
		bool repete;
		// ------------------------------------------------

		// ------------------------------------------------
		// - IMPRESSAO ------------------------------------
		printLinha1(this->out);
		printInicializacaoHeader(this->out, i);
		printLinha2(this->out);

		print2(this->out);

		printLinha2(this->out);
		print3(this->out, iteracao++, this->atual.J, this->atual.CR);
		// ------------------------------------------------

		do {
			this->etapa1();
			repete = this->etapa2();

			// ------------------------------------------------
			// - IMPRESSAO ------------------------------------
			if (repete) {
				printLinha2(this->out);
				print3(this->out, iteracao++, this->atual.J, this->atual.CR);
			}
			// ------------------------------------------------

		} while (repete && iteracao <= limite);

		// ------------------------------------------------
		// - IMPRESSAO ------------------------------------
		printLinha1(this->out);
		this->out << "\n";
		// ------------------------------------------------
	}

	// ------------------------------------------------
	// - IMPRESSAO ------------------------------------
	this->out << "\n\n===================================================";
	this->out << "===================================================\n\n";

	// parte 1
	this->out << "# Melhor inicializacao: " << this->melhor.id << "\n";
	this->out << "# Melhor J: " << scientific << this->melhor.J << "\n";
	this->out << "# CR: " << scientific << this->melhor.CR << "\n";

	this->out << "\n";

	// parte 2
	size_t k = this->melhor.cluster.size();
	size_t n = this->repositorio.tabela[0].n;

	size_t c[n], p[n];
	for (size_t i = 0; i < this->melhor.cluster.size(); i++) {
		tr( (this->melhor.cluster[i]) ,iter) {
			c[*iter] = i;
		}
	}
	if (this->repositorio.rotulado) {
		for (size_t i = 0; i < this->repositorio.dados.prioriCluster.size(); i++) {
			tr( (this->repositorio.dados.prioriCluster[i]) ,iter) {
				p[*iter] = i;
			}
		}
	} else {
		memset(p, -1, sizeof(p));
	}

	printLinha1(this->out, k);
	printRelacaoHeader(this->out, k);

	for (size_t i = 0; i < n; i++) {
		printLinha2(this->out, k);
		print2(this->out, i, k, this->melhor.U[i], c[i], p[i]);
	}
	printLinha1(this->out, k);
	this->out << "\n";

	// parte 3
	this->out << "# Solucao:\n";
	for (size_t i = 0; i < this->clusters; i++) {
		this->out << "Cluster[" << (i + 1) << "] :\n";
		this->out << this->melhor[i] << "\n\n";
	}

	if (this->repositorio.rotulado) {
		this->out << "# Priori Cluster:\n";
		for (size_t i = 0; i < this->repositorio.dados.prioriCluster.size(); i++) {
			this->out << "Cluster[" << (i + 1) << "] :\n";
			this->out << this->repositorio.dados.prioriCluster[i] << "\n\n";
		}
	}

	this->out << "# Coeficientes clusters-tabelas:\n";
	this->out << fixed << setprecision(6);
	for (size_t i = 0; i < this->melhor.coeficiente.size(); i++) {
		this->out << "Cluster #" << (i+1) << "\n";
		for(size_t j = 0; j < this->melhor.coeficiente[i].size(); j++) {
			this->out << "\ttabela[" << j << "] : " << this->melhor.coeficiente[i][j] << "\n";
		}
	}
	this->out << "\n";

	// ------------------------------------------------
	// confusing matrix
	this->imprimirMatriz(out);

	// ------------------------------------------------
	// access file
	this->printAcessFile(c);

	//srmq
	{
	    const int clusterNumber = this->melhor.U[0].size();
	    const int numberElements = this->melhor.U.size();
	    const int pTables = this->melhor.coeficiente[0].size();
        std::shared_ptr<std::vector<util::FuzzyCluster>> bestClusters(new std::vector<util::FuzzyCluster>());
        for (int i = 0; i < clusterNumber; i++) {
                util::FuzzyCluster c(numberElements, pTables);
                for (int n = 0; n < numberElements; n++) {
                    c.updateMemberhipDegree(n, this->melhor.U[n][i]);
                }
                bestClusters->push_back(c);
        }
        util::FuzzyIndices fuzzyIndices(bestClusters);
        const size_t prioriClusterNumber = this->repositorio.dados.prioriCluster.size();
    	for (size_t i = 0; i < prioriClusterNumber; i++) { //apriori cluster i
            for (Cluster::iterator it = this->repositorio.dados.prioriCluster[i].begin();
					it != this->repositorio.dados.prioriCluster[i].end(); it++) {
				fuzzyIndices.insertPertinenceDegree(std::make_pair((*it), i), 1.0);
            }
    	}

        util::ConfusionMatrix confusionMatrix(clusterNumber, prioriClusterNumber);
        for (int n = 0; n < numberElements; n++) {  //n is an object
            confusionMatrix.putObject(n,
                    getBestClusterIndex(n),
                    getAprioriPartition(fuzzyIndices, n));
        }
        this->out << ">>>>>>>>>>>> The F-Measure is: ";
        this->out << confusionMatrix.fMeasureGlobal() << std::endl;
        this->out << ">>>>>>>>>>>> The CR-Index  is: ";
        this->out << confusionMatrix.CRIndex() << std::endl;
        this->out << ">>>>>>>>>>>> OERC Index    is: ";
        this->out << confusionMatrix.OERCIndex() << std::endl;
        this->out << ">>>>>>>>>>>> NMI  Index    is: ";
        this->out << confusionMatrix.nMIIndex() << std::endl;


        util::FuzzyIndices::CampelloIndices campIndices(fuzzyIndices);
        this->out << ">>>>>>>>>>>> Campello's Rand Index            is: ";
        this->out << campIndices.randIndex() << std::endl;
        this->out << ">>>>>>>>>>>> Campello's Adjusted Rand Index   is: ";
        this->out << campIndices.adjustedRandIndex() << std::endl;
        this->out << ">>>>>>>>>>>> Campello's Jaccard Coef          is: ";
        this->out << campIndices.jaccardCoef() << std::endl;
        this->out << ">>>>>>>>>>>> Campello's Fowlkes-Mallows Index is: ";
        this->out << campIndices.fowlkesMallowsIndex() << std::endl;
        this->out << ">>>>>>>>>>>> Campello's Minkowski Measure     is: ";
        this->out << campIndices.minkowskiMeasure() << std::endl;
        util::FuzzyIndices::AndersonIndices andIndices(fuzzyIndices);
        this->out << ">>>>>>>>>>>> (Anderson)Fuz Rand Index Rifqi   is: ";
        this->out << andIndices.sFRHRIndex() << std::endl;
        this->out << ">>>>>>>>>>>> Anderson's Rand Index (4a)       is: ";
        this->out << andIndices.randIndex_4a() << std::endl;
        this->out << ">>>>>>>>>>>> Anderson's Johnson Index (4b)    is: ";
        this->out << andIndices.jmcabIndex_4b() << std::endl;
        this->out << ">>>>>>>>>>>> Anderson's Hubert  Index (4c)    is: ";
        this->out << andIndices.hubertIndex_4c() << std::endl;
        this->out << ">>>>>>>>>>>> Anderson's Wallace Index (4d1)   is: ";
        this->out << andIndices.wallaceIndex_4d1() << std::endl;
        this->out << ">>>>>>>>>>>> Anderson's Wallace Index (4d2)   is: ";
        this->out << andIndices.wallaceIndex_4d2() << std::endl;
        this->out << ">>>>>>>>>>>> Anderson's FowlkesM Index (4e)   is: ";
        this->out << andIndices.fowlkesMallow_4e() << std::endl;
        this->out << ">>>>>>>>>>>> Anderson's Jacard Index (4f)     is: ";
        this->out << andIndices.jacard_4f() << std::endl;
        this->out << ">>>>>>>>>>>> Anderson's Wallace Index (4g)    is: ";
        this->out << andIndices.adjRandHA_4g() << std::endl;
        this->out << ">>>>>>>>>>>> Anderson's Adj Rand Campello(4h) is: ";
        this->out << andIndices.adjRandCampello_4h() << std::endl;
        this->out << ">>>>>>>>>>>> Anderson's Adj Rand Brower  (4i) is: ";
        this->out << andIndices.adjRandBrouwer_4i() << std::endl;
        this->out << ">>>>>>>>>>>> Anderson's Minkowski        (4j) is: ";
        this->out << andIndices.minkowski_4j() << std::endl;
        this->out << ">>>>>>>>>>>> Anderson's Hubert's Gamma   (4k) is: ";
        this->out << andIndices.hGamma_4k() << std::endl;
        this->out << ">>>>>>>>>>>> Anderson's Yule             (4l) is: ";
        this->out << andIndices.yule_4l() << std::endl;
        this->out << ">>>>>>>>>>>> Anderson's Chi-Squared      (4m) is: ";
        this->out << andIndices.chisq_4m() << std::endl;
        this->out << ">>>>>>>>>>>> Anderson's Goodman-Kruskal  (4n) is: ";
        this->out << andIndices.goodmnKrkl_4n() << std::endl;
	}

}

bool Algoritmo::inicializacao() {
	this->atual.clear();
	this->atual.srand(this->repositorio);
	for (size_t i = 0; i < this->atual.cluster.size(); i++) {
		if (this->atual[i].size() == 0) {
			return true;
		}
	}
	print7("~~ ######################################### ~~");
	return false;
}

void Algoritmo::etapa1() {
	this->atual.atualizaCluster(this->repositorio);

}

bool Algoritmo::etapa2() {

	if(!this->atual.atualizaCoeficiente(this->repositorio)) {
		return false;
	}

	this->atual.atualizarU(this->repositorio);
	double dif = this->atual.J;
	dif -= this->atual.atualizaJ(this->repositorio);
	this->atual.atualizaCR(this->repositorio);
	if (this->atual < this->melhor) {
		this->melhor = this->atual;
	}
	dbg7(atual.J);
	return (cmp(dif) != 0);
}

Algoritmo::operator string() const {
	os out;
	_dbg(out,this->inicializacoes);
	_dbg(out,this->clusters);
	_print(out,this->repositorio);
	return out.str();
}

void Algoritmo::printAcessFile(size_t c[]) {
	// ---------------------------------------------------------------------
	// arquivo para ser usado pelo acess da microsoft
	string nome_arquivo = Algoritmo::saida;
	for (int i = nome_arquivo.size() - 1; i >= 0; i--) {
		if (nome_arquivo[i] == '.') {
			nome_arquivo.erase(i, nome_arquivo.size() - i);
			break;
		}
	}
	nome_arquivo += "_acess.txt";

	ofstream extra(nome_arquivo.c_str(), ios::out);
	for (size_t i = 0; i < individuos; i++) {
		extra << (i + 1) << " " << (c[i] + 1) << "\n";
	}
	extra.close();
	// ---------------------------------------------------------------------
}

#include "validation.h"

void Algoritmo::imprimirMatriz(ostream &out) {
	double calculaErro(int individuos, vector<vector<int> > &table);
	double fMeasure(vector<vector<int> > &table);

	//int k = (int) melhorCluster.size();
	//int p = dados.getNumPrioriCluster();
	const Array<Cluster>& C = this->melhor.cluster;
	const Array<Cluster>& P = this->repositorio.dados.prioriCluster;

	int k = C.size();
	int p = P.size();

	vector<vector<int> > table;
	table.resize(k + 1);
	for (int i = 0; i <= k; i++) {
		table[i].resize(p + 1);
	}

	// table[i][j] = cluster[i] && prioriCluster[j]
	for (int i = 0; i < k; i++) {
		for (int j = 0; j < p; j++) {
			table[i][j] = (C[i] && P[j]).size();
		}
	}

	// table[i][p] = melhorCluster[i].tamanho
	for (int i = 0; i < k; i++) {
		table[i][p] = C[i].size();
	}

	// table[k][i] = prioriCluster[i].tamanho
	for (int i = 0; i < p; i++) {
		table[k][i] = P[i].size();
	}

	table[k][p] = this->individuos;

	//--------------------------------------------------------------------------------
	out << "--- Confusing Matrix ------------------------------\n";

	for (int i = 0; i < p; i++) {
		out << '\t' << i;
	}
	out << '\n';

	for (int i = 0; i < k; i++) {
		out << i << '\'';
		for (int j = 0; j < p; j++) {
			out << '\t' << table[i][j];
		}
		out << '\n';
	}

	double error1 = calculaErro(individuos, table);
	double fmed = fMeasure(table);

	//--------------------------------------------------------------------------------
	out << "\n# error: " << fixed << setprecision(2) << (100 * error1) << "%\n";
	out << "# F measure: " << fixed << setprecision(6) << fmed << "\n";
	//--------------------------------------------------------------------------------


	// more:
	Array< Array<double> > PRIORI;
	PRIORI.resize(individuos);
	for (size_t i = 0; i < individuos; i++) {
		PRIORI[i].resize(p);
		for(int k=0;k<p;k++) {
		  PRIORI[i][k] = P[k].count(i) ? 1.0 : 0.0;
		}
	}
	double campello = Validation::fuzzy_rand_index_campello(PRIORI,this->melhor.U);
	double hullermeier = Validation::fuzzy_rand_index_hullermeier(PRIORI,this->melhor.U);

	// frigui-campello
	out << "# fuzzy_rand_index_campello: " << fixed << setprecision(6) << campello << "\n";
	// hullermeier
  out << "# fuzzy_rand_index_hullermeier: " << fixed << setprecision(6) << hullermeier << "\n";
}

double calculaErro(int individuos, vector<vector<int> > &table) {

	MinCostFlow &mcf = Algoritmo::mcf;

	int N = table.size() - 1;
	int M = table[0].size() - 1;

	int source = 0, sink = N + M + 1;

	mcf.init(N + M + 2);

	for (int i = 0; i < N; i++) {
		for (int j = 0; j < M; j++) {
			mcf.add_edge(i + 1, j + N + 1, 1, -table[i][j]);
		}
	}

	for (int i = 0; i < N; i++) {
		mcf.add_edge(source, i + 1, 1, 0);
	}

	if (N == M) {
		for (int i = 0; i < M; i++) {
			mcf.add_edge(i + N + 1, sink, 1, 0);
		}
	} else {
		for (int i = 0; i < M; i++) {
			mcf.add_edge(i + N + 1, sink, N, 0);
		}
	}

	pair<int, int> ret = mcf.solve(source, sink);
	assert(ret.first == N);

	dbg7(ret.first);
	dbg7(ret.second);

	return 1 + double(ret.second) / individuos;
}

double fMeasure(vector<vector<int> > &table) {
	double F = 0;

	int K = table.size() - 1;
	int P = table[0].size() - 1;

	for(int j = 0; j < P; j++) {
		double vmax = 0, rappel, precision;
		for(int i = 0; i < K; i++) if(table[i][j] != 0) {
			rappel = double(table[i][j]) / table[K][j];
			precision = double(table[i][j]) / table[i][P];
			vmax = max(vmax, 2 * rappel * precision / (rappel + precision));
			//printf("rappel = %d/%d, precision = %d/%d\n",table[i][j],table[K][j],table[i][j],table[i][P]);
		}
		F  += vmax * table[K][j];
		//printf(" %lf * %d\n",vmax, table[K][j]);
	}
	//printf("--> indi = %d\n",table[K][P]);
	return F / table[K][P];
}

ostream& operator<<(ostream& out, Algoritmo& a) {
	return out << (string(a));
}
