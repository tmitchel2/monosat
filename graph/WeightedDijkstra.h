/*
 * Dijkstra.h
 *
 *  Created on: 2013-05-28
 *      Author: sam
 */

#ifndef WEIGHTED_DIJKSTRA_H_
#define WEIGHTED_DIJKSTRA_H_

#include "mtl/Vec.h"
#include "mtl/Heap.h"
#include "DynamicGraph.h"
#include "Reach.h"
#include <limits>
using namespace Minisat;
namespace Minisat{
template<class EdgeStatus=DefaultEdgeStatus, class Weight=vec<double> >
class WeightedDijkstra:public Reach{
public:
	DynamicGraph<EdgeStatus> & g;
	Weight & weight;
	int last_modification;
	int last_addition;
	int last_deletion;
	int history_qhead;

	int last_history_clear;

	int source;
	double INF;


	vec<int> old_dist;
	vec<int> changed;

	vec<double> dist;
	vec<int> prev;
	struct DistCmp{
		vec<double> & _dist;
		 bool operator()(int a, int b)const{
			return _dist[a]<_dist[b];
		}
		 DistCmp(vec<double> & d):_dist(d){};
	};
	Heap<DistCmp> q;



public:

	WeightedDijkstra(int s,DynamicGraph<EdgeStatus> & graph, Weight & _weight):g(graph),weight(_weight), last_modification(-1),last_addition(-1),last_deletion(-1),history_qhead(0),last_history_clear(0),source(s),INF(0),q(DistCmp(dist)){

		marked=false;
		mod_percentage=0.2;
		stats_full_updates=0;
		stats_fast_updates=0;
		stats_skip_deletes=0;
		stats_skipped_updates=0;
		stats_full_update_time=0;
		stats_fast_update_time=0;
		INF= std::numeric_limits<double>::infinity();
	}
	//Dijkstra(const Dijkstra& d):g(d.g), last_modification(-1),last_addition(-1),last_deletion(-1),history_qhead(0),last_history_clear(0),source(d.source),INF(0),q(DistCmp(dist)),stats_full_updates(0),stats_fast_updates(0),stats_skip_deletes(0),stats_skipped_updates(0),stats_full_update_time(0),stats_fast_update_time(0){marked=false;};

/*	void setWeight(int node, double w){
		while(weight.size()<=g.nodes){
			weight.push(1);
		}
		weight[node]=w;

	}*/


	void setSource(int s){
		source = s;
		last_modification=-1;
		last_addition=-1;
		last_deletion=-1;
	}
	int getSource(){
		return source;
	}

	void updateFast(){
		stats_fast_updates++;
		double start_time = cpuTime();



		/*for(int i = 0;i<g.nodes;i++)
					changed.push(i);*/
		assert(last_deletion==g.deletions);
		last_modification=g.modifications;
		last_addition=g.additions;

		dist.growTo(g.nodes);
		prev.growTo(g.nodes);
		while(weight.size()<=g.nodes){
			weight.push(1);
		}



		q.clear();
		if(last_history_clear!=g.historyclears){
			history_qhead=0;
			last_history_clear=g.historyclears;
		}
		//ok, now check if any of the added edges allow for a decrease in distance.
		for (int i = history_qhead;i<g.history.size();i++){
			assert(g.history[i].addition); //NOTE: Currently, this is glitchy in some circumstances - specifically, ./modsat -rinc=1.05 -rnd-restart  -conflict-shortest-path  -no-conflict-min-cut   -rnd-init -rnd-seed=01231 -rnd-freq=0.01 /home/sam/data/gnf/unit_tests/unit_test_17_reduced.gnf can trigger this assertion!
			int u=g.history[i].u;
			int v=g.history[i].v;
			double alt = dist[u]+weight[u] ;
			if(alt< dist[v]){

				if(dist[v]>=INF){
					//this was changed
					changed.push(v);
				}

				dist[v]=alt;
				prev[v]=u;

				if(!q.inHeap(v))
					q.insert(v);
				else
					q.decrease(v);
			}
			/*
			 *
			 *
			//Is this altered code still correct? Well, not for dijkstras, but probably for connectivity
			if(dist[v]>=INF){
				//this was changed
				changed.push(v);

				dist[v]=alt;
				prev[v]=u;

				if(!q.inHeap(v))
					q.insert(v);
			}
			 *
			 */

		}
		history_qhead=g.history.size();

		while(q.size()){
			int u = q.removeMin();
			if(dist[u]==INF)
				break;
			for(int i = 0;i<g.adjacency[u].size();i++){
				if(!g.edgeEnabled( g.adjacency[u][i].id))
					continue;
				int v = g.adjacency[u][i].to;
				int alt = dist[u]+ weight[u];
				if(alt<dist[v]){
					if(dist[v]>=INF){
						//this was changed
						changed.push(v);
					}

					dist[v]=alt;
					prev[v]=u;
					if(!q.inHeap(v))
						q.insert(v);
					else
						q.decrease(v);
				}

			}
		}
		stats_fast_update_time+=cpuTime()-start_time;
	}
	vec<int> & getChanged(){
		return changed;
	}
	void clearChanged(){
		changed.clear();
	}

	void drawFull(){

	}

	void update( ){
		static int iteration = 0;
		int local_it = ++iteration ;
		if(local_it==17513){
			int a =1;
		}
		if(last_modification>0 && g.modifications==last_modification)
				return;
		assert(weight.size()>=g.edges);
/*		while(weight.size()<=g.nodes){
				weight.push(1);
			}*/
		/*if (last_addition==g.additions && last_modification>0){
			//if none of the deletions were to edges that were the previous edge of some shortest path, then we don't need to do anything
			if(last_history_clear!=g.historyclears){
				history_qhead=0;
				last_history_clear=g.historyclears;
			}
			bool need_recompute = false;
			//ok, now check if any of the added edges allow for a decrease in distance.
			for (int i = history_qhead;i<g.history.size();i++){
				assert(!g.history[i].addition);
				int u=g.history[i].u;
				int v=g.history[i].v;
				if(prev[v]==u){
					history_qhead = i-1;
					need_recompute=true;
					//this deletion matters, so we need to recompute.
					break;
				}
			}
			if(!need_recompute){
				//none of these deletions touched any shortest paths, so we can ignore them.

				last_modification=g.modifications;
				last_deletion = g.deletions;
				last_addition=g.additions;

				history_qhead=g.history.size();
				last_history_clear=g.historyclears;

				assert(dbg_uptodate());
				stats_skip_deletes++;
				return;
			}
		}*/

		/*if(last_deletion==g.deletions && last_modification>0  ){
			//Don't need to do anything at all.
			if(last_addition==g.additions){
				last_modification = g.modifications;
				stats_skipped_updates++;
				assert(dbg_uptodate());
				return;
			}
			//we can use a faster, simple dijkstra update method
			updateFast();
			assert(dbg_uptodate());
			return;
		}*/

		stats_full_updates++;
		double startdupdatetime = cpuTime();

		INF=g.nodes+1;
		dist.growTo(g.nodes);
		prev.growTo(g.nodes);
		old_dist.growTo(g.nodes);
		q.clear();
		for(int i = 0;i<g.nodes;i++){
			old_dist[i]=last_modification > 0 ? dist[i]:INF;//this won't work properly if we added nodes...
			dist[i]=INF;
			prev[i]=-1;
		}
		dist[source]=0;
		q.insert(source);
		while(q.size()){
			int u = q.peakMin();
			if(dist[u]==INF)
				break;
			if(old_dist[u]>=INF){
				changed.push(u);
			}
			q.removeMin();
			for(int i = 0;i<g.adjacency[u].size();i++){
				int edge =  g.adjacency[u][i].id;
				if(!g.edgeEnabled(edge))
					continue;

				int v = g.adjacency[u][i].node;
				double alt = dist[u]+ weight[edge];
				if(alt<dist[v]){
					dist[v]=alt;
					prev[v]=u;
					if(!q.inHeap(v))
						q.insert(v);
					else
						q.decrease(v);
				}
			}
		}

		for(int u = 0;u<g.nodes;u++){
		//while(q.size()){
			//iterate through the unreached nodes and check which ones were previously reached

			if(last_modification <=0  || (old_dist[u]<INF && dist[u]>=INF)){
				changed.push(u);
			}
		}
		//}
		assert(dbg_uptodate());

		last_modification=g.modifications;
		last_deletion = g.deletions;
		last_addition=g.additions;

		history_qhead=g.history.size();
		last_history_clear=g.historyclears;

		stats_full_update_time+=cpuTime()-startdupdatetime;;
	}
	bool dbg_path(int to){
#ifdef DEBUG_DIJKSTRA
		assert(connected(to));
		if(to == source){
			return true;
		}
		int p = prev[to];

		if(p<0){
			return false;
		}
		if(p==to){
			return false;
		}

		return dbg_path(p);


#endif
		return true;
	}
	bool dbg_uptodate(){

		return true;
	}

	bool connected_unsafe(int t)const{
		return t<dist.size() && dist[t]<INF;
	}
	bool connected_unchecked(int t)const{
		assert(last_modification==g.modifications);
		return connected_unsafe(t);
	}
	bool connected(int t){
		if(last_modification!=g.modifications)
			update();

		assert(dbg_uptodate());

		return dist[t]<INF;
	}
	int distance(int t){
		if(last_modification!=g.modifications)
					update();
		return dist[t];
	}
	int distance_unsafe(int t){
		if(connected_unsafe(t))
			return dist[t];
		else
			return INF;
	}
	int previous(int t){

		return prev[t];
	}

};
};
#endif /* DIJKSTRA_H_ */