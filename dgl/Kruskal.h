
#ifndef KRUSKAL_H_
#define KRUSKAL_H_

#include <vector>
#include "alg/Heap.h"
#include "mtl/Sort.h"
#include "graph/DynamicGraph.h"
#include "core/Config.h"
#include "MinimumSpanningTree.h"
#include "alg/DisjointSets.h"
#include <limits>
#include <algorithm>

namespace dgl{
template<class Status=MinimumSpanningTree::NullStatus>
class Kruskal:public MinimumSpanningTree{
public:

	DynamicGraph & g;
	Status &  status;
	int last_modification;
	int min_weight;
	int last_addition;
	int last_deletion;
	int history_qhead;

	int last_history_clear;
	bool hasParents;
	int INF;
	DisjointSets sets;
	std::vector<int> mst;
	std::vector<int> q;
	std::vector<int> check;
	const int reportPolarity;

	//std::vector<char> old_seen;
	std::vector<bool> in_tree;;
	std::vector<int> parents;
	std::vector<int> parent_edges;


    struct EdgeLt {
        const std::vector<int>&  edge_weights;

        bool operator () (int x, int y) const {
        	return edge_weights[x]<edge_weights[y];
        }
        EdgeLt(const std::vector<int>&  _edge_weights) : edge_weights(_edge_weights) { }
    };

	Heap<EdgeLt> edge_heap;
	std::vector<int> edge_list;

	std::vector<int> prev;

public:

	int stats_full_updates;
	int stats_fast_updates;
	int stats_fast_failed_updates;
	int stats_skip_deletes;
	int stats_skipped_updates;
	int stats_num_skipable_deletions;
	double mod_percentage;

	double stats_full_update_time;
	double stats_fast_update_time;

	Kruskal(DynamicGraph & graph, Status & _status=MinimumSpanningTree::nullStatus, int _reportPolarity=0 ):g(graph), status(_status), last_modification(-1),last_addition(-1),last_deletion(-1),history_qhead(0),last_history_clear(0),INF(0),reportPolarity(_reportPolarity),edge_heap(EdgeLt(g.weights)){

		mod_percentage=0.2;
		stats_full_updates=0;
		stats_fast_updates=0;
		stats_skip_deletes=0;
		stats_skipped_updates=0;
		stats_full_update_time=0;
		stats_fast_update_time=0;
		stats_num_skipable_deletions=0;
		stats_fast_failed_updates=0;
		min_weight=-1;
		hasParents=false;
	}

	void setNodes(int n){
		q.reserve(n);
		check.reserve(n);
		in_tree.resize(g.nEdgeIDs());

		INF=std::numeric_limits<int>::max();
		sets.AddElements(n);
		parents.resize(n);
		parent_edges.resize(n);
	}

	void update( ){
		static int iteration = 0;
		int local_it = ++iteration ;
#ifdef RECORD
		if(g.outfile){
			fprintf(g.outfile,"m\n");
			fflush(g.outfile);
		}
#endif
		if(last_modification>0 && g.modifications==last_modification){
			stats_skipped_updates++;
			return;
		}
		stats_full_updates++;
		
		if(last_deletion==g.deletions){
			stats_num_skipable_deletions++;
		}
		hasParents=false;
		sets.Reset();
		setNodes(g.nodes());

		min_weight=0;

		mst.clear();

		if(edge_list.size()<g.nEdgeIDs()){
			edge_list.clear();
			for(int i = 0;i<g.nEdgeIDs();i++){

					//edge_heap.insert(i);
					edge_list.push_back(i);

			}
			std::sort(edge_list.begin(),edge_list.end() ,EdgeLt(g.weights));
		}
		for(int i = 0;i<in_tree.size();i++)
			in_tree[i]=false;
		//while(edge_heap.size()){
		for(int i = 0;i<edge_list.size();i++){
			int edge_id = edge_list[i];//edge_heap.removeMin();
			if(!g.edgeEnabled(edge_id))
				continue;
			int u = g.getEdge(edge_id).from;
			int v = g.getEdge(edge_id).to;

			int set1 = sets.FindSet(u);
			int set2 = sets.FindSet(v);
			if(set1!=set2){
				assert(g.edgeEnabled(edge_id));
				in_tree[edge_id]=true;
				mst.push_back(edge_id);
				//if(reportPolarity>-1)
				//	status.inMinimumSpanningTree(edge_id,true);
				min_weight+=g.getWeight(edge_id);
				sets.UnionSets(set1,set2);
				assert(sets.FindSet(u)==sets.FindSet(v));
			}
		}

/*		if (sets.NumSets()>1)
			min_weight=INF;*/

		status.setMinimumSpanningTree(sets.NumSets()>1 ? INF: min_weight);

		//if(reportPolarity>-1){
		for(int i = 0;i<in_tree.size();i++){
			//Note: for the tree edge detector, polarity is effectively reversed.
			if(reportPolarity<1 && (!g.edgeEnabled(i) || in_tree[i]) ){
				status.inMinimumSpanningTree(i,true);
			}else if(reportPolarity>-1 && (g.edgeEnabled(i) && ! in_tree[i]) ){
				status.inMinimumSpanningTree(i,false);
			}
		}
		//}



		last_modification=g.modifications;
		last_deletion = g.deletions;
		last_addition=g.additions;

		history_qhead=g.history.size();
		last_history_clear=g.historyclears;

		assert(dbg_uptodate());

		;
	}
	std::vector<int> & getSpanningTree(){
		update();
		return mst;
	 }

	int getParent(int node){
		update();
		//kruskals doesn't actually give us the parents, normally. need to construct it on demand, here.
		if (!hasParents)
			buildParents();
		return parents[node];
	}
	 int getParentEdge(int node){
		 if(getParent(node)!=-1)
			 return parent_edges[node];
		 else
			 return -1;
	 }
	bool edgeInTree(int edgeid){
		update();
		return in_tree[edgeid];
	}
	bool dbg_mst(){

		return true;
	}


	int weight(){

		update();

		assert(dbg_uptodate());
		if(sets.NumSets()>1)
			return INF;
		return min_weight;
	}
	int forestWeight(){
		update();
		assert(dbg_uptodate());
		return min_weight;
	}
	 int numComponents(){
		 update();
		 return sets.NumSets();
	 }
	int getComponent(int node){
		update();
		return sets.FindSet(node);
	}
	int getRoot(int component=0){
		update();
		if (!hasParents)
			buildParents();
		int u =0;
		if(sets.NumSets()>1)
			u=sets.GetElement(component);

		while(int p =getParent(u)!=-1){
			u=p;
		}
		assert(getParent(u)==-1);
		return u;
	}

	bool dbg_uptodate(){
#ifndef NDEBUG
		int sumweight = 0;
		in_tree.resize(g.nEdgeIDs());
		for(int i = 0;i<g.edges();i++){
			if(in_tree[i]){
				sumweight+= g.getWeight(i);
			}
		}
		assert(sumweight ==min_weight || min_weight==INF);


#endif
		return true;
	};
private:

	void buildParents(){
		hasParents=true;
		for(int i = 0;i<parents.size();i++){
			parents[i]=-1;
			parent_edges[i]=-1;
		}
		q.clear();
		for(int i = 0;i<sets.NumSets();i++){
			int root = sets.GetElement(i);//chose an element arbitrarily from the nth set.
			int set = sets.FindSet(root);
			q.push_back(root);
			assert(parents[root]==-1);
			while(q.size()){
				int u = q.back();
				assert(sets.FindSet(u)==set);
				q.pop_back();
				for (int j = 0;j<g.nIncident(u,true);j++){
					int edge = g.incident(u,j,true).id;
					int to = g.incident(u,j,true).node;
					if(edge<0)
						continue;
					if(edge>=in_tree.size())
						exit(3);
					if(in_tree[edge]){

						if(parents[to]==-1 &&  to!=root){
							assert(to!=root);
							//int st = sets.FindSet(to);
							assert(sets.FindSet(to)==set);
							parents[to]=u;
							parent_edges[to] = edge;
							q.push_back(to);
						}
					}
				}
			}
			assert(parents[root]==-1);
		}
/*
#ifndef NDEBUG
		int rootcount =0;
		for(int i = 0;i<parents.size();i++){
			if(parents[i]==-1)
				rootcount++;
		}
		assert(rootcount==1);
#endif
*/
	}
};
};
#endif