#include <vector>
#include <queue>
#include <algorithm>
#include <assert.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>

namespace KWGraph
{
	enum GraphCreationFlags
	{
		GraphCreationFlags_Connected  	= (1 << 0),
		GraphCreationFlags_Directed 	= (1 << 1),
		GraphCreationFlags_Sparse 		= (1 << 2),
		// Makes the graph algorithm generate the same graph every time
		GraphCreationFlags_Consistent 	= (1 << 3),
		GraphCreationFlags_AllowCycles 	= (1 << 4)
	};	

	template <typename T>
	struct Edge;

	template <typename T>
	class GraphVisitor;

	template <typename T>
	struct Node
	{
		//We're not keeping pointers here because we don't know beforehand the 
		//number of edges that we have, so a realloc will ruin everything 
		std::vector< Edge<T> > edges;
		T weight;
		float x;
		float y;
		int id;
	};

	template <typename T>
	struct Edge
	{
		Edge() : source(NULL), 
				 destination(NULL), 
				 directed(true) {}
		Node<T>* source;
		Node<T>* destination;
		T		 weight;	
		bool 	 directed;
	};

	namespace
	{
		static int commonSeed = 1234;
		static bool isRandomEngineOn = false;
	}

	// A general purpose representation for a graph, supports adjacency matrices
	// and adjacency lists for storage
 	// NOTE: Most functions are overloaded on purpose to avoid the evils of 
	// default parameters
		

	template <typename T>
	class Graph
	{
	public:
		enum StorageType
		{
			StorageType_None = 0,
			StorageType_AdjacencyList = 1 << 0,
			StorageType_AdjacencyMatrix = 1 << 0
		};

		enum DFSOrder
		{
			DFSOrder_PreOrder,
			DFSOrder_PostOrder
		};

	private:
		// Keeping a linear matrix gets us a huge performance boost becasuse
		// it reduces the chances that we get a cache miss when we get an element
		std::vector< T > 		matrix;
		std::vector< Node<T> > 	nodes;
		std::vector< Edge<T> >  edges;
		StorageType 			storageType;
	public:
		// We want to keep both an adjacency matrix and a adjacency list 
		// For instance this way we can compare different implementations
		// for a certain algorithm

		inline std::vector<T>& GetAdjacencyMatrix() { return matrix; }
		inline std::vector< Node<T> >& GetNodes() { return nodes; }
		inline std::vector< Edge<T> >& GetEdges() { return edges; }
		inline size_t GetNrNodes() { return nodes.size(); }
		inline size_t GetNrEdges() { return edges.size(); }

		int GetNodeIndex(const Node<T>& node)
		{
			typename std::vector< Node< T> >::iterator nodeIt = 
				std::find(nodes.begin(), nodes.end(), node);
			return nodeIt - nodes.begin();
		}

		inline int GetMatrixIndex(int row, int col)
		{
			int nrNodes = static_cast<int>(nodes.size());
			return row * nrNodes + col;
		}

		void AddNode(T weight)
		{
			Node<T> newNode;
			newNode.weight = weight;

			//TODO handle errors in case the vector cannot resize		
			nodes.push_back(newNode);
		}

		void AddNode(const Node<T>& node)
		{
			//TODO handle errors in case the vector cannot resize
			nodes.push_back(node);
		}

		void AddListEdge(Node<T>& source, Node<T>& destination, T weight, bool directed)
		{
			Edge<T> newEdge;
			newEdge.source = &source;
			newEdge.destination = &destination;
			newEdge.directed = directed;
			newEdge.weight = weight;
			
			size_t edgeId = edges.size();
			edges.push_back(newEdge);
			//TODO handle errors in case the vector cannot resize
			source.edges.push_back(edges[edgeId]);
			
			if(directed)
			{
				newEdge.source = &destination;
				newEdge.destination = &source;
				edgeId = edges.size();
				edges.push_back(newEdge);
				destination.edges.push_back(edges[edgeId]);
			}
		}

		void AddListEdge(Node<T>& source, Node<T>& destination, T weight)
		{
			AddListEdge(source, destination, weight, true);
		}

		void AddListEdge(Node<T>& source, Node<T>& destination)
		{
			AddListEdge(source, destination, 1, true);
		}

		void AddListEdge(int sourceId, int destId, T weight, bool directed)
		{
			Node<T>& source = nodes[sourceId];
			Node<T>& destination = nodes[destId];
			AddListEdge(source, destination, weight, directed);
		}

		void AddListEdge(int sourceId, int destId, T weight)
		{
			AddListEdge(sourceId, destId, weight, true);
		}

		void AddListEdge(int sourceId, int destId)
		{
			AddListEdge(sourceId, destId, 1, true);
		}

		void AddMatrixEdge(int sourceId, int destId, T weight, bool directed)
		{
			// We're using size_t here to avoid a cast
			// There is no way this can be negative so there is no side effect
			size_t expectedSize = nodes.size() * nodes.size();

			// Because of branch prediction we can safely ignore the slowdown
			// that this if introduces because the chances of this being true
			// are very slim
			if(matrix.size() != expectedSize)
				AllocAdjacencyMatrix();
			
			size_t nrNodes = nodes.size();
			int srcRow = static_cast<int>(sourceId / (float)nrNodes);
			int dstRow = static_cast<int>(destId / (float)nrNodes);
			int srcCol = sourceId % nrNodes;
			int dstCol = destId % nrNodes;
			int srcToDstIndex = GetMatrixIndex(srcRow, dstCol);
			matrix[srcToDstIndex] = weight;
			if(directed)
			{
				int dstToSrcIndex = GetMatrixIndex(dstRow, srcCol);
				matrix[dstToSrcIndex] = weight;
			}
		}

		void AddMatrixEdge(Node<T>& source, Node<T>& destination, T weight, bool directed)
		{
			int sourceId = GetNodeIndex(source);
			int destinationId = GetNodeIndex(destination);
			AddMatrixEdge(sourceId, destinationId, weight, directed);
		}

		void AddMatrixEdge(Node<T>& source, Node<T>& destination, T weight)
		{
			AddMatrixEdge(source, destination, weight, true);
		}

		void AddMatrixEdge(Node<T>& source, Node<T>& destination)
		{
			AddMatrixEdge(source, destination, 1, true);
		}

		void AddEdge(Node<T>& source, Node<T>& destination, T weight, bool directed)
		{
			assert(storageType != StorageType_None);

			if(storageType & StorageType_AdjacencyList)
				AddListEdge(source, destination, weight, directed);
			if(storageType & StorageType_AdjacencyMatrix)
				AddMatrixEdge(source, destination, weight, directed);
		}

		void AddEdge(Node<T>& source, Node<T>& destination, T weight)
		{
			AddEdge(source, destination, weight, true);
		}

		void AddEdge(Node<T>& source, Node<T>& destination)
		{
			
			AddEdge(source, destination, 1, true);			
		}

		void AddEdge(int sourceId, int destId, T weight, bool directed)
		{
			assert(storageType != StorageType_None);
	
			if(storageType & StorageType_AdjacencyList)
				AddListEdge(sourceId, destId, weight, directed);
			if(storageType & StorageType_AdjacencyMatrix)
				AddMatrixEdge(sourceId, destId, weight, directed);
		}

		void AddEdge(int sourceId, int destId, T weight)
		{
			AddEdge(sourceId, destId, weight, true);
		}

		void AddEdge(int sourceId, int destId)
		{
			AddEdge(sourceId, destId, 1, true);
		}

		void AllocAdjacencyMatrix()
		{
			// Make sure to avoid the realloc
			matrix.resize(0);
			//TODO handle errors in case the vector cannot resize
			matrix.resize(nodes.size() * nodes.size());
		}


		static void SetRandomEngineSeed(int seed) { commonSeed = seed; }		
		void InitializeGraph(int size, int flags, StorageType storage)
		{
			bool isSparse 		= flags & GraphCreationFlags_Sparse;
			bool isCyclic 		= flags & GraphCreationFlags_AllowCycles;
		 	bool isDirected 	= flags & GraphCreationFlags_Directed;
			bool isConnected 	= flags & GraphCreationFlags_Connected;
			bool isConsistent 	= flags & GraphCreationFlags_Consistent;		

			storageType = storage;

			if(!isRandomEngineOn)
			{
				time_t timeSeed = time(NULL);
				int seed = isConsistent ? commonSeed 
										: (int)timeSeed;
				isRandomEngineOn = true;
				srand(seed);
			}

			nodes.resize(size);	

			for(size_t nodeIt = 0; nodeIt < size; ++nodeIt)
			{
				Node<T>& newNode = nodes[nodeIt];
				newNode.id = nodeIt;
				newNode.weight = T(rand() / (float)RAND_MAX);
				float edgeChance = (isSparse) 	? 0.2f
												: 0.8f;
				for(size_t edgeIt = 0; edgeIt < size; ++edgeIt)
				{
					newNode.edges.reserve(size * edgeChance);
					float randomChance = rand() / (float)RAND_MAX;		
					if(randomChance < edgeChance)
					{
						if(!isCyclic && edgeIt == nodeIt)
							continue;
						T edgeWeight = T(rand() / (float)RAND_MAX);
						AddEdge(nodeIt, edgeIt, edgeWeight, isDirected);
					}
				}
				if(isConnected && newNode.edges.size() == 0)
					AddEdge(nodeIt, rand() % size);
			}
		}

		void BFS(GraphVisitor<T>* visitor)
		{
			size_t nrNodes = GetNrNodes();
			if(nrNodes == 0)
				return;

			std::queue < Node<T>* > visitQueue;
			std::vector<bool> visitedNodes;
			std::vector< Node<T> >& graphNodes = GetNodes();
			visitQueue.push(&graphNodes[0]);
			
			visitedNodes.resize(nrNodes);
			if(visitor)
			{
				visitor->OnStartVisit();
				visitor->OnStartComponentVisit();
			}
			while(!visitQueue.empty())
			{
				Node<T>* crNode = visitQueue.front();
				visitQueue.pop();
				if(visitor)
					visitor->OnBeginNodeProcess(*crNode);
				if(visitedNodes[crNode->id])
				{
					if(visitor)
						visitor->OnNodeAlreadyVisited(*crNode);
					continue;
				}

				visitedNodes[crNode->id] = true;
				if(visitor)		
					visitor->OnNodeProcess(*crNode);
				for(size_t edgeIt = 0; edgeIt < crNode->edges.size(); ++edgeIt)
				{
					Edge<T>& edge = crNode->edges[edgeIt];
					visitQueue.push(edge.destination);
				}
				visitor->OnEndNodeProcess(*crNode);

				if(visitQueue.empty())
				{
					if(visitor)
						visitor->OnEndComponentVisit();
					for(size_t nodeIt = 0; nodeIt < nrNodes; ++nodeIt)
					{
						if(visitedNodes[nodeIt] == 0)
						{
							if(visitor)
								visitor->OnStartComponentVisit();
							visitQueue.push(&graphNodes[nodeIt]);
							break;
						}
					}
				}
			}
			if(visitor)
			{
				visitor->OnEndComponentVisit();
				visitor->OnEndVisit();
			}

		}

		void DFS(GraphVisitor<T>* visitor)
		{
			
		}
	};

	template<typename T>
	class GraphVisitor
	{
	public:
		//Called exactly once at the begining of the visit
		virtual void OnEndVisit() = 0;
		//Called exactly once at the end of the visit
		virtual void OnStartVisit() = 0;
		//Called once when a new component is found in the graph
		virtual void OnEndComponentVisit() = 0;
		//Called once when the visit ends for each separate component
		virtual void OnStartComponentVisit() = 0;
		//Called when the node has been reached
		virtual void OnBeginNodeProcess(const Node<T>& node) = 0;
		//Called when the node is being processed		
		virtual void OnNodeProcess(const Node<T>& node) = 0;
		//Called when the node and its' descendents have been processed		
		virtual void OnEndNodeProcess(const Node<T>& node) = 0;
		//Called when a node that has been already visited is found
		//Essentially this means we found a cycle in the graph
		virtual void OnNodeAlreadyVisited(const Node<T>& node) = 0;
	};

	typedef Node<int> IntNode;
	typedef Node<float> FloatNode;
	typedef Graph<int> IntGraph;
	typedef Graph<float> FloatGraph;
	typedef GraphVisitor<int> IntGraphVisitor;
	typedef GraphVisitor<float> FloatGraphVisitor;

	class IntPrinter : public IntGraphVisitor
	{
	public:
		virtual void OnEndVisit() {}
		virtual void OnStartVisit() {}
		virtual void OnEndComponentVisit() { printf("\n"); }
		virtual void OnStartComponentVisit() {}
		virtual void OnBeginNodeProcess(const IntNode& node){}
		virtual void OnNodeProcess(const IntNode& node) {printf("%d ", node.id);}
		virtual void OnEndNodeProcess(const IntNode& node) {}
		virtual void OnNodeAlreadyVisited(const IntNode& node) {}
	};
}


