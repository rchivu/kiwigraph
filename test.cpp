#include "graph.h"

int main()
{
	KWGraph::IntGraph graph;
	KWGraph::IntPrinter printer(&graph);
	graph.InitializeGraph(8, 	KWGraph::GraphCreationFlags_Connected, 
								KWGraph::StorageType_AdjacencyList);
	
	graph.BFS(&printer);
	graph.DFS(&printer, KWGraph::DFSOrder_PreOrder);
	graph.DFS(&printer, KWGraph::DFSOrder_PostOrder);
}
