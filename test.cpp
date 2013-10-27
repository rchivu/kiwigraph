#include "graph.h"

int main()
{
	KWGraph::IntGraph graph;
	KWGraph::IntPrinter printer;
	graph.InitializeGraph(8, 	KWGraph::GraphCreationFlags_Connected, 
								KWGraph::StorageType_AdjacencyList);
	
	graph.BFS(&printer);
	printf("\n");
	graph.DFS(&printer, KWGraph::DFSOrder_PreOrder);
	printf("\n");		
	graph.DFS(&printer, KWGraph::DFSOrder_PostOrder);
}
