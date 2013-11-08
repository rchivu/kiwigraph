#ifndef KWGRAPH_H
#define KWGRAPH_H

#ifndef KWGRAPH_STRIP_INCLUDES
// These are the sort of things that people usually add to precompiled headers
// so we want to give them the option not to have thse includes
#    include <vector>
#    include <queue>
#    include <set>
#    include <algorithm>
#    include <assert.h>
#    include <cstdio>
#    include <cstdlib>
#    include <ctime>

#    if defined(__linux__) || defined(__APPLE__)
#        include <sys/time.h>
#        include <string.h>
#        include <errno.h>
#    endif

#endif

#include "platform.h"

namespace KWGraph
{
    static const int ROOT_ID = -1;
    static const int INVALID_ID = -2;

    enum GraphCreationFlags
    {
        GraphCreationFlags_Connected   = (1 << 0),
        GraphCreationFlags_Directed    = (1 << 1),
        GraphCreationFlags_Sparse      = (1 << 2),
        // Makes the graph algorithm generate the same graph every time
        GraphCreationFlags_Consistent  = (1 << 3),
        GraphCreationFlags_AllowCycles = (1 << 4)
    };    

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

    enum NodeAction
    {
        NodeAction_Continue,
        NodeAction_Abort,
        NodeAction_SkipChildren
    };

    template <typename T>
    struct Edge;

    template <typename T>
    class GraphVisitor;

    template <typename T>
    struct Node
    {
        Node() : parent(INVALID_ID) {}
        //We're not keeping pointers here because we don't know beforehand the 
        //number of edges that we have, so a realloc will ruin everything 
        std::vector<int> edges;
        T weight;
        float x;
        float y;
        int parent;
        int id;
    };

    template <typename T>
    struct Edge
    {
        Edge() : source(-1), 
                 destination(-1), 
                 directed(true) {}
        int      source;
        int      destination;
        T        weight;    
        bool     directed;
    };


    namespace
    {
        template <typename T>
        struct EdgeCreateData
        {
            std::vector< Edge<T> >  edges;
            Node<T>*                node;
            int                     nrNodes;
            int                     nrConnections;
            T                       weightScale;
            bool                    directed;
        };

        static int commonSeed = 1234;
        static bool isRandomEngineOn = false;

        static void  StartRandomEngine(bool isConsistent)
        {
            if(!isRandomEngineOn)
                return;

            time_t timeSeed = time(NULL);
            int seed = isConsistent ? commonSeed 
                                    : (int)timeSeed;
            isRandomEngineOn = true;
            srand(seed);
        }
        template <typename T>
        static void* CreateGraphEdges(void* userData)
        {
            EdgeCreateData<T>* data = static_cast<EdgeCreateData<T>*>(userData);
            //make edges in a local copy to avoid false sharing
            std::set<int> connections;
            std::vector< Edge<T> > outEdges;

            for(int edgeIt = 0; edgeIt < data->nrConnections; ++edgeIt)
            {
                int connectionNodeId = rand() % data->nrNodes;
                if(connections.find(connectionNodeId) != connections.end())
                    continue;
                Edge<T> newEdge;
                newEdge.weight = T(rand()/(float)RAND_MAX) * data->weightScale;
                newEdge.destination = connectionNodeId;
                newEdge.source = data->node->id;
                outEdges.push_back(newEdge);
                connections.insert(connectionNodeId);

            }
            data->edges = outEdges;
        }
    }

    // A general purpose representation for a graph, supports adjacency matrices
    // and adjacency lists for storage
    // NOTE: Most functions are overloaded on purpose to avoid the evils of 
    // default parameters    

    template <typename T>
    class Graph
    {
    private:
        // Keeping a linear matrix gets us a huge performance boost becasuse
        // it reduces the chances that we get a cache miss when we get an element
        std::vector< T >        m_matrix;
        std::vector< Node<T> >  m_nodes;
        std::vector< Edge<T> >  m_edges;
        StorageType             m_storageType;
        
        static const int m_maxSparseConnections = 10;
        static const float m_denseEdgeChance = 0.8f;

        void InvalidateParents()
        {
            size_t nrNodes = m_nodes.size();
            for(size_t nodeIt = 0; nodeIt < nrNodes; ++nodeIt)
            {
                m_nodes[nodeIt].parent = INVALID_ID;
            }
        }
        
        void InitGraphStorage(int size, float edgeChance, bool isDirected)
        {
            m_nodes.resize(size);    
            for(size_t nodeIt = 0; nodeIt < size; ++nodeIt)
            {
                m_nodes[nodeIt].id = nodeIt;
                m_nodes[nodeIt].edges.reserve(size * edgeChance);
            }

            size_t reserveSize = size * size * edgeChance * edgeChance; 
            if(!isDirected)
                reserveSize *= 2;
            m_edges.reserve(reserveSize);
        }

    public:
        typedef std::vector< Edge<T> > EdgeVector;
        typedef std::vector< Node<T> > NodeVector;

        // We want to keep both an adjacency matrix and a adjacency list 
        // For instance this way we can compare different implementations
        // for a certain algorithm

        inline const std::vector<T>& GetAdjacencyMatrix() const { return m_matrix; }
        inline const std::vector< Node<T> >& GetNodes() const { return m_nodes; }
        inline const std::vector< Edge<T> >& GetEdges() const { return m_edges; }
        inline std::vector<T>& GetAdjacencyMatrix() { return m_matrix; }
        inline std::vector< Node<T> >& GetNodes() { return m_nodes; }
        inline std::vector< Edge<T> >& GetEdges() { return m_edges; }

        inline size_t GetNrNodes() const { return m_nodes.size(); }
        inline size_t GetNrEdges() const { return m_edges.size(); }

        inline int GetMatrixIndex(int row, int col) const
        {
            int nrNodes = static_cast<int>(m_nodes.size());
            return row * nrNodes + col;
        }

        void AddNode(T weight)
        {
            Node<T> newNode;
            newNode.weight = weight;

            //TODO handle errors in case the vector cannot resize        
            m_nodes.push_back(newNode);
        }

        void AddNode(const Node<T>& node)
        {
            //TODO handle errors in case the vector cannot resize
            m_nodes.push_back(node);
        }

        void AddListEdge(Node<T>& source, Node<T>& destination, T weight, bool directed)
        {
            AddListEdge(source.id, destination.id, weight, directed);
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
            Edge<T> newEdge;
            newEdge.source = sourceId;
            newEdge.destination = destId;
            newEdge.directed = directed;
            newEdge.weight = weight;
            size_t edgeId = m_edges.size();
            m_edges.push_back(newEdge);
            //TODO handle errors in case the vector cannot resize
            m_nodes[sourceId].edges.push_back(edgeId);
            if(directed)
            {
                edgeId = m_edges.size();
                newEdge.source = destId;
                newEdge.destination = sourceId;
                m_edges.push_back(newEdge);
                m_nodes[destId].edges.push_back(edgeId);
            }
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
            size_t expectedSize = m_nodes.size() * m_nodes.size();

            // Because of branch prediction we can safely ignore the slowdown
            // that this if introduces because the chances of this being true
            // are very slim
            if(m_matrix.size() != expectedSize)
                AllocAdjacencyMatrix();
        
            size_t nrNodes = m_nodes.size();
            int srcRow = static_cast<int>(sourceId / (float)nrNodes);
            int dstRow = static_cast<int>(destId / (float)nrNodes);
            int srcCol = sourceId % nrNodes;
            int dstCol = destId % nrNodes;
            int srcToDstIndex = GetMatrixIndex(srcRow, dstCol);
            m_matrix[srcToDstIndex] = weight;
            if(directed)
            {
                int dstToSrcIndex = GetMatrixIndex(dstRow, srcCol);
                m_matrix[dstToSrcIndex] = weight;
            }
        }

        void AddMatrixEdge(Node<T>& src, Node<T>& dest, T weight, bool directed)
        {
            int sourceId = src.id;
            int destinationId = dest.id;
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

        void AddEdge(Node<T>& source, Node<T>& dest, T weight, bool directed)
        {
            assert(m_storageType != StorageType_None);

            if(m_storageType & StorageType_AdjacencyList)
                AddListEdge(source, dest, weight, directed);
            if(m_storageType & StorageType_AdjacencyMatrix)
                AddMatrixEdge(source, dest, weight, directed);
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
            assert(m_storageType != StorageType_None);

            if(m_storageType & StorageType_AdjacencyList)
                AddListEdge(sourceId, destId, weight, directed);
            if(m_storageType & StorageType_AdjacencyMatrix)
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
            m_matrix.resize(0);
            //TODO handle errors in case the vector cannot resize
            m_matrix.resize(m_nodes.size() * m_nodes.size());
        }

        static void SetRandomEngineSeed(int seed) { commonSeed = seed; }        
        void InitializeGraph(int size, int flags, T weightScale, StorageType storage)
        {
            bool isSparse       = flags & GraphCreationFlags_Sparse;
            bool isCyclic       = flags & GraphCreationFlags_AllowCycles;
            bool isDirected     = flags & GraphCreationFlags_Directed;
            bool isConnected    = flags & GraphCreationFlags_Connected;
            bool isConsistent   = flags & GraphCreationFlags_Consistent;        

            m_storageType = storage;
            StartRandomEngine(isConsistent);

            float edgeChance = (isSparse) ? m_maxSparseConnections / (float)size
                                          : m_denseEdgeChance;

            InitGraphStorage(size, edgeChance, isDirected);
            for(size_t nodeIt = 0; nodeIt < size; ++nodeIt)
            {
                Node<T>& newNode = m_nodes[nodeIt];            
                newNode.weight = T(rand() / (float)RAND_MAX) * weightScale;
                for(size_t edgeIt = 0; edgeIt < size; ++edgeIt)
                {
                    float randomChance = rand() / (float)RAND_MAX;        
                    if(randomChance < edgeChance)
                    {
                        if(!isCyclic && edgeIt == nodeIt)
                            continue;
                        T edgeWeight = T(rand() / (float)RAND_MAX) * weightScale;
                        AddEdge(nodeIt, edgeIt, edgeWeight, isDirected);
                    }
                }

                if(isConnected && newNode.edges.size() == 0)
                {
                    int connectionIndex = rand() % size;
                    T edgeWeight = T(rand() / (float)RAND_MAX) * weightScale;

                    while(!isCyclic && connectionIndex == nodeIt)
                        connectionIndex = rand() % size;

                    AddEdge(nodeIt, connectionIndex, edgeWeight, isDirected);
                }
            }
        }

        void ThreadedInitializeGraph(int size, int flags, T weightScale, StorageType storage, int nrThreads)
        {
            bool isSparse       = flags & GraphCreationFlags_Sparse;
            bool isCyclic       = flags & GraphCreationFlags_AllowCycles;
            bool isDirected     = flags & GraphCreationFlags_Directed;
            bool isConnected    = flags & GraphCreationFlags_Connected;
            bool isConsistent   = flags & GraphCreationFlags_Consistent;        

            m_storageType = storage;
            StartRandomEngine(isConsistent);
            float edgeChance = (isSparse) ? m_maxSparseConnections / (float)size
                                          : m_denseEdgeChance;
            const int nrConnections = edgeChance * size;
            InitGraphStorage(size, edgeChance, isDirected);
            std::vector<PThreadID> threads;
            std::vector< EdgeCreateData<T> > edgeData;
            threads.resize(nrThreads);
            int crThreadId = 0;
            edgeData.resize(size);
            for(size_t nodeIt = 0; nodeIt < size; ++nodeIt)
            {
                EdgeCreateData<T>& data = edgeData[nodeIt];
                data.node = &m_nodes[nodeIt];
                data.nrNodes = size;
                data.nrConnections = nrConnections;
                data.weightScale = weightScale;
                data.directed = isDirected;

                PWaitOnThread(threads[crThreadId], NULL);
                PStartThread((void*)&edgeData[nodeIt], CreateGraphEdges<T>, threads[crThreadId]);

                crThreadId = (crThreadId + 1) % nrThreads;
            }
            for(size_t threadIt = 0; threadIt < nrThreads; ++threadIt)
                PWaitOnThread(threads[crThreadId], NULL);

            for(size_t nodeIt = 0; nodeIt < size; ++nodeIt)
            {
                EdgeCreateData<T>& data = edgeData[nodeIt];
                for(size_t dataIt = 0; dataIt < data.edges.size(); ++dataIt)
                {
                    int edgeId = m_edges.size();
                    m_edges.push_back(data.edges[dataIt]);
                    data.node->edges.push_back(edgeId);
                }
            }

        }
        void BFSAddNextComponentNode(GraphVisitor<T>* visitor, 
                                     std::queue<Node<T>*>& visitQueue,
                                     std::vector<bool>& visitedNodes)
        {
            typename KWGraph::Graph<T>::NodeVector& graphNodes = GetNodes();
            size_t nrNodes = graphNodes.size();
            if(visitor)
                visitor->OnEndComponentVisit();

            for(size_t nodeIt = 0; nodeIt < nrNodes; ++nodeIt)
            {
                if(visitedNodes[nodeIt] == false)
                {
                    if(visitor)
                        visitor->OnStartComponentVisit();
                    graphNodes[nodeIt].parent = ROOT_ID;
                    visitQueue.push(&graphNodes[nodeIt]);
                    break;
                }
            }
        }

        void BFS(GraphVisitor<T>* visitor)
        {
            size_t nrNodes = GetNrNodes();
            if(nrNodes == 0)
                return;

            InvalidateParents();

            std::queue < Node<T>* > visitQueue;
            std::vector<bool> visitedNodes;
            typename KWGraph::Graph<T>::NodeVector& graphNodes = GetNodes();
            int visitSource = 0;
            if(visitor)
            {
                visitSource = visitor->GetVisitSource();
                visitSource = (visitSource < 0) ? 0 : visitSource;                
            }
            visitQueue.push(&graphNodes[visitSource]);
            graphNodes[visitSource].parent = ROOT_ID;
            visitedNodes.resize(nrNodes, false);
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
                {
                    NodeAction action = visitor->OnBeginNodeProcess(*crNode);
                    if(action == NodeAction_Abort)
                        break;
                    if(action == NodeAction_SkipChildren)
                    {
                        if(visitQueue.empty())
                            BFSAddNextComponentNode(visitor, visitQueue, visitedNodes);
                        continue;
                    }
                }
            
                if(visitedNodes[crNode->id])
                {
                    if(visitor)
                    {
                        NodeAction action = visitor->OnNodeAlreadyVisited(*crNode);
                        if(action == NodeAction_Abort)
                            break;
                    }
                    if(visitQueue.empty())
                        BFSAddNextComponentNode(visitor, visitQueue, visitedNodes);
                    continue;
                }

                visitedNodes[crNode->id] = true;
                if(visitor)        
                {
                    NodeAction action = visitor->OnNodeProcess(*crNode);
                    if(action == NodeAction_Abort)
                        break;
                    if(action == NodeAction_SkipChildren)
                    {
                        if(visitQueue.empty())
                            BFSAddNextComponentNode(visitor, visitQueue, visitedNodes);
                        continue;
                    }
                }

                for(size_t edgeIt = 0; edgeIt < crNode->edges.size(); ++edgeIt)
                {
                    int edgeIdx = crNode->edges[edgeIt];
                    Edge<T>& crEdge = m_edges[edgeIdx];
                    Node<T>& nextNode = m_nodes[crEdge.destination];
                    if(nextNode.parent == INVALID_ID)
                        nextNode.parent = crNode->id;
                    visitQueue.push(&nextNode);
                }
            
                if(visitor)
                {
                    NodeAction action = visitor->OnEndNodeProcess(*crNode);
                    if(action == NodeAction_Abort)
                        break;
                }

                if(visitQueue.empty())
                {
                    BFSAddNextComponentNode(visitor, visitQueue, visitedNodes);
                }
            }

            if(visitor)
            {
                visitor->OnEndComponentVisit();
                visitor->OnEndVisit();
            }
        }

        NodeAction DFSStep(Node<T>& node, GraphVisitor<T>* visitor, 
                     std::vector<bool>& visited, DFSOrder order, int parent)
        {
            bool skipChildren = false;

            if(visited[node.id])
            {
                if(visitor)
                    return visitor->OnNodeAlreadyVisited(node);
            }
        
            visited[node.id] = true;
            node.parent = parent;
            if(visitor)
            {
                NodeAction action = visitor->OnBeginNodeProcess(node);
                if(action != NodeAction_Continue)
                    return action;
            }
            if(order == DFSOrder_PreOrder && visitor)
            {
                NodeAction action = visitor->OnNodeProcess(node);
                if(action != NodeAction_Continue)
                    return action;
            }
        
            for(size_t edgeIt = 0; edgeIt < node.edges.size() && !skipChildren; ++edgeIt)
            {
                int edgeIdx = node.edges[edgeIt];
                Edge<T>& edge = m_edges[edgeIdx];
                Node<T>& nextNode = m_nodes[edge.destination];
                NodeAction action = DFSStep(nextNode, visitor, visited, order, node.id);
                if(action == NodeAction_Abort)
                    return action;
            }
        
            if(visitor)
            {
                NodeAction action;
                if(order == DFSOrder_PostOrder)
                {
                    action = visitor->OnNodeProcess(node);
                    if(action == NodeAction_Abort)
                        return action;
                }

                action = visitor->OnEndNodeProcess(node);
                if(action == NodeAction_Abort)
                    return action;
            }
        }

        void DFS(GraphVisitor<T>* visitor, DFSOrder order)
        {
            size_t nrNodes = m_nodes.size();
            if(nrNodes == 0)
                return;

            InvalidateParents();

            if(visitor)
                visitor->OnStartVisit();

            std::vector<bool> visited;
            visited.resize(m_nodes.size(), false);
            bool allNodesVisited = false;
            int visitSource;
            if(visitor)
                visitSource = visitor->GetVisitSource();
            visitSource    = (visitSource < 0) ? 0 : visitSource;

            while(!allNodesVisited)
            {
                allNodesVisited = true;
            
                if(visitor)
                        visitor->OnStartComponentVisit();
                if(!visited[visitSource])
                    DFSStep(m_nodes[visitSource], visitor, visited, order, visitSource);

                for(size_t nodeIt = 0 ; nodeIt < visited.size(); ++nodeIt)
                {
                    if(visited[nodeIt])
                        continue;

                    allNodesVisited = false;

                    NodeAction action = DFSStep(m_nodes[nodeIt], visitor, visited, order, nodeIt);
                    if(action == NodeAction_Abort)
                    {
                        allNodesVisited = true;
                        break;
                    }    
                }
                if(visitor)
                        visitor->OnEndComponentVisit();
            }
        
            if(visitor)
                visitor->OnEndVisit();
        }
    };

    template<typename T>
    class GraphVisitor
    {
    protected:
        Graph<T>*   m_graph;
        int         m_visitSource;
    public:
        GraphVisitor(Graph<T>* graph) : m_visitSource(-1),
                                        m_graph(graph) {}
        GraphVisitor(Graph<T>* graph, int source) : m_visitSource(source),
                                                    m_graph(graph) {}
        void SetVisitSource(int source){ m_visitSource = source; }
        int GetVisitSource(){ return m_visitSource; }
        //Called exactly once at the begining of the visit
        virtual void OnEndVisit() {}
        //Called exactly once at the end of the visit
        virtual void OnStartVisit() {}
        //Called once when a new component is found in the graph
        virtual void OnEndComponentVisit() {}
        //Called once when the visit ends for each separate component
        virtual void OnStartComponentVisit() {}
        //Called when the node has been reached
        virtual NodeAction OnBeginNodeProcess(const Node<T>& node) 
        {
            return NodeAction_Continue;
        }
        //Called when the node is being processed        
        virtual NodeAction OnNodeProcess(const Node<T>& node) 
        {
            return NodeAction_Continue;
        }
        //Called when the node and its' descendents have been processed        
        virtual NodeAction OnEndNodeProcess(const Node<T>& node) 
        {
            return NodeAction_Continue;
        }
        //Called when a node that has been already visited is found
        //Essentially this means we found a cycle in the graph
        virtual NodeAction OnNodeAlreadyVisited(const Node<T>& node) 
        {
            return NodeAction_Continue;
        };
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
        IntPrinter(IntGraph* graph) : IntGraphVisitor(graph){}
        IntPrinter(IntGraph* graph, int source) : IntGraphVisitor(graph, source){}

        virtual void OnEndComponentVisit() 
        {
            printf("\n"); 
        }

        virtual NodeAction OnNodeProcess(const IntNode& node) 
        {
            printf("%d ", node.id);
            return NodeAction_Continue;
        }
    };

#ifndef KWGRAPH_SKIP_MINI_PROFILER
    namespace
    {
        typedef long long ProfileDataType;
        struct ProfileData
        {
            //This only supposed to display static text from .DATA
            //so no need to make a deep copy with a std::string
            const char* name;
            ProfileDataType time;
            bool inProgress;
        };
        typedef std::vector<ProfileData> ProfileDataList;
        typedef std::vector<ProfileData>::iterator ProfileDataIt;
    
        ProfileDataList g_profileData;


#if defined(__linux__) || defined(__APPLE__)            
        static inline ProfileDataType GetProfileTime()
        {

            struct timeval crTime;
            int err = gettimeofday(&crTime, NULL);
            if(err == 0)
                return crTime.tv_usec + crTime.tv_sec * 1000000;
            else 
                return -1;
        }
    
        static inline float GetSeconds(ProfileDataType time)
        {
            return time * 0.0000001f;
        }
#elif defined(_WIN32)
#       include <Mmsystem.h>
        static inline ProfileDataType GetProfileTime()
        {
            return timegettime();
        }
    
        static inline float GetSeconds(ProfileDataType time)
        {
            return time * 0.001f;
        }
#endif
    };

    static inline void StartMiniProfile(int profileId, const char* name)
    {
        ProfileDataType profileTime = GetProfileTime();
        if(profileId >= g_profileData.size())
            g_profileData.resize(profileId + 1);

        if(profileTime < 0)
            printf("Failed to start profile: %s\n", strerror(errno));

        ProfileData& profileData = g_profileData[profileId];
        profileData.time = profileTime;
        profileData.name = name;
        profileData.inProgress = profileData.time >= 0;
    }

    static inline void EndMiniProfile(int profileId)
    {
        ProfileData& profileData = g_profileData[profileId];
        if(!profileData.inProgress)
            return;

        ProfileDataType profileTime = GetProfileTime() - profileData.time;
        profileData.inProgress = false;
        float timeInSeconds = GetSeconds(profileTime);        
        if(profileData.name)
            printf("Total time spent in test %s: %g seconds\n", 
                                             profileData.name, timeInSeconds);
        else
            printf("Total time spent in test: %g seconds\n", timeInSeconds);
    }

    static inline bool IsInProgress(int profileId)
    {
        return g_profileData[profileId].inProgress;
    }

#endif
}

#endif
