#include "graph.h"

enum PathState
{
	PathState_Done,
	PathState_Uninitialized,
	PathState_NoPath,
};

template <typename T>
class BFSShortestPath : public KWGraph::GraphVisitor<T>
{
private:
	int       m_destination;
	PathState m_pathState;
public:
	BFSShortestPath(KWGraph::Graph<T>* graph, int source, int destination) : 
		KWGraph::GraphVisitor<T>(graph, source),
		m_destination(destination),
		m_pathState(PathState_Uninitialized) {}

	PathState GetPathState(){ return m_pathState; }
	virtual void OnEndComponentVisit() 
	{
		//We always start at the source, so if the path is uninitialized when we
		//reach the end of a component then there is no path to the destination
		if(m_pathState == PathState_Uninitialized)
		{
			printf("There is no path between %d and %d\n", 
				KWGraph::GraphVisitor<T>::m_visitSource, m_destination);	
			m_pathState = PathState_NoPath;
		}
	}
	virtual void OnNodeProcess(const KWGraph::Node<T>& node)
	{
		if(node.id == m_destination && m_pathState == PathState_Uninitialized)
		{
			m_pathState = PathState_Done;
			typename KWGraph::Graph<T>::NodeVector& nodes = 
				KWGraph::GraphVisitor<T>::m_graph->GetNodes();
			int crNodeId = node.id;
			printf("Shortest path between %d and %d: ", 
				KWGraph::GraphVisitor<T>::m_visitSource, m_destination);
			while(crNodeId != KWGraph::ROOT_ID)
			{
				printf("%d ", crNodeId);
				const KWGraph::Node<T>& crNode = nodes[crNodeId];
				crNodeId = crNode.parent; 
			}
			printf("\n");
		}
	}
};
 
int main()
{
	KWGraph::IntGraph graph;
	KWGraph::IntPrinter printer(&graph, 1);
	graph.InitializeGraph(8, 	KWGraph::GraphCreationFlags_Sparse    |
								KWGraph::GraphCreationFlags_Connected, 
								KWGraph::StorageType_AdjacencyList);
	graph.BFS(&printer);



	BFSShortestPath<int> bfsShortPath(&graph, 1, 5);
	graph.BFS(&bfsShortPath);		
}
