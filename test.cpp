#include "graph.h"

int main()
{
	KWGraph::IntGraph graph;
	KWGraph::IntPrinter printer;
	graph.InitializeGraph(8, 	KWGraph::GraphCreationFlags_Connected, 
								KWGraph::IntGraph::StorageType_AdjacencyList);
	
	graph.BFS(&printer);
}
