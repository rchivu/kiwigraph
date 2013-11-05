#include "graph.h"

enum PathState
{
	PathState_Done,
	PathState_Uninitialized,
	PathState_NoPath,
};
enum PathProfileId
{
	PathProfileId_BFSPath,
	PathProfileId_BFSTotal
};

static const char* profileNames[] = {"BFS Path", "BFS Total"};

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

	virtual void OnStartVisit() 
	{
		const char* profileName = profileNames[PathProfileId_BFSPath];
        KWGraph::StartMiniProfile(PathProfileId_BFSPath, profileName);
	}		
	
	virtual void OnEndComponentVisit() 
	{
		//We always start at the source, so if the path is uninitialized when we
		//reach the end of a component then there is no path to the destination
		if(m_pathState == PathState_Uninitialized)
		{
			printf("There is no path between %d and %d\n", 
				KWGraph::GraphVisitor<T>::m_visitSource, m_destination);	
			m_pathState = PathState_NoPath;
			if(KWGraph::IsInProgress(PathProfileId_BFSPath))
				KWGraph::EndMiniProfile(PathProfileId_BFSPath);
		}
	}
	virtual KWGraph::NodeAction OnNodeProcess(const KWGraph::Node<T>& node)
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
			KWGraph::EndMiniProfile(PathProfileId_BFSPath);
			return KWGraph::NodeAction_Abort;
		}
		return KWGraph::NodeAction_Continue;
	}
};
 
int main()
{
	KWGraph::IntGraph graph;
	KWGraph::IntPrinter printer(&graph, 1);
	graph.InitializeGraph(10000,KWGraph::GraphCreationFlags_Sparse    |
                                 KWGraph::GraphCreationFlags_Consistent|
                                 KWGraph::GraphCreationFlags_Connected, 
                                 KWGraph::StorageType_AdjacencyList);
	
	const char* profileName = profileNames[PathProfileId_BFSTotal];
    KWGraph::StartMiniProfile(PathProfileId_BFSTotal, profileName);

	BFSShortestPath<int> bfsShortPath(&graph, 1, 5);
	graph.BFS(&bfsShortPath);		
	KWGraph::EndMiniProfile(PathProfileId_BFSTotal);
}
