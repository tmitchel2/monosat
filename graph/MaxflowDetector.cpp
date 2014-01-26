/*
 * MaxflowDetector.c
 *
 *  Created on: 2014-01-08
 *      Author: sam
 */



#include "MaxflowDetector.h"
#include "GraphTheory.h"
#include "EdmondsKarpAdj.h"
MaxflowDetector::MaxflowDetector(int _detectorID, GraphTheorySolver * _outer,  DynamicGraph<PositiveEdgeStatus> &_g,DynamicGraph<NegativeEdgeStatus> &_antig, int from, int _target,double seed):
Detector(_detectorID),outer(_outer),over_graph(_g),g(_g),antig(_antig),source(from),target(_target),rnd_seed(seed),positive_detector(NULL),negative_detector(NULL){


	positive_detector = new EdmondsKarpAdj<vec<int>,PositiveEdgeStatus>(_g,outer->edge_weights);
	negative_detector = new EdmondsKarpAdj<vec<int>,NegativeEdgeStatus>(_antig,outer->edge_weights);

	//for(int i = 0;i<g)
	//positive_detector->setAllEdgeCapacities(1);
	//negative_detector->setAllEdgeCapacities(1);
	first_reach_var = var_Undef;
	reach_marker=outer->newReasonMarker(getID());
	non_reach_marker=outer->newReasonMarker(getID());
	forced_reach_marker=outer->newReasonMarker(getID());
}



void MaxflowDetector::addFlowLit(int maxflow, Var reach_var){
	g.invalidate();
	antig.invalidate();
	if(first_reach_var==var_Undef){
		first_reach_var=reach_var;
	}else{
		assert(reach_var>first_reach_var);
	}

	assert(maxflow>=0);



	//while( dist_lits[to].size()<=within_steps)
	//	dist_lits[to].push({lit_Undef,-1});

	while(outer->S->nVars()<=reach_var)
		outer->S->newVar();

	Lit reachLit=mkLit(reach_var,false);
	bool found=false;


		flow_lits.push();
		flow_lits.last().l = reachLit;
		flow_lits.last().max_flow=maxflow;
		while(reach_lit_map.size()<= reach_var- first_reach_var ){
			reach_lit_map.push(-1);
		}

		reach_lit_map[reach_var-first_reach_var]=flow_lits.size()-1;




}






void MaxflowDetector::buildMaxFlowTooHighReason(int flow,vec<Lit> & conflict){
			//drawFull();


			double starttime = cpuTime();


		tmp_cut.clear();
			int actual_flow = positive_detector->maxFlow(source, target);

			//just collect the set of edges which have non-zero flow, and return them
			//(Or, I could return a cut, probably)
			for(int i = 0;i<outer->edge_list.size();i++){
				if(positive_detector->getEdgeFlow(i)>0){
					Var v = outer->edge_list[i].v;
					assert(outer->S->value(v)==l_True);
					conflict.push(mkLit(v,true));
				}
			}


			/*	if(!outer->dbg_reachable(source,node)){
				outer->drawFull();
				assert(false);
			}
			assert(d.connected_unchecked(node));

			for(int i = 0;i<tmp_cut.size();i++){
				int edgeid = tmp_cut[i].id;
				Var v = outer->edge_list[edgeid].v;
				Lit l = mkLit(v,false);
				assert(outer->S->value(l)==l_True);
				conflict.push(~l);
			}
			int u = node;
			int p;
*/
			outer->num_learnt_paths++;
			outer->learnt_path_clause_length+= (conflict.size()-1);
			double elapsed = cpuTime()-starttime;
			outer->pathtime+=elapsed;

		}
		void MaxflowDetector::buildMaxFlowTooLowReason(int maxflow,vec<Lit> & conflict){
			static int it = 0;
			++it;
			double starttime = cpuTime();

			//drawFull( non_reach_detectors[detector]->getSource(),u);
			//assert(outer->dbg_distance( source,u));

			//The reason why we can't reach this assignment is a cut through the disabled edges in the residual graph from the overapprox.
			//we could search for a min cut, but instead we will just step back in the RESIDUAL graph, from u to s, collecting disabled edges.
			seen.clear();
			seen.growTo(outer->nNodes());
			visit.clear();
			int foundflow = negative_detector->maxFlow(source,target);
			visit.push(target);
			for(int k = 0;k<visit.size();k++){
				int u = visit[k];
				for(int i = 0;i<outer->inv_adj[u].size();i++){
					int p = outer->inv_adj[u][i].from;
					if(!seen[p]){
						seen[p]=true;
						int v = outer->inv_adj[u][i].v;
						assert( outer->inv_adj[u][i].to==u);
						if(outer->S->value(v)!=l_False){
							//this is an enabled edge in the overapprox
							int edgeid = outer->getEdgeID(v);
							int residual_capacity = negative_detector->getEdgeResidualCapacity(edgeid);
							if(residual_capacity>0){
								visit.push(p);
							}
						}else{
							//this is a disabled edge, and we can add it to the cut.
							//we're going to assume the edge has non-zero capacity here, otherwise we could exclude it (but it shouldn't really even be in this graph in that case, anyways).
							conflict.push(mkLit(v,false));
						}
					}
					//pred
				}
			}


			 outer->num_learnt_cuts++;
			 outer->learnt_cut_clause_length+= (conflict.size()-1);

			double elapsed = cpuTime()-starttime;
			 outer->mctime+=elapsed;

		}


		void MaxflowDetector::buildReason(Lit p, vec<Lit> & reason, CRef marker){


				if(marker==reach_marker){
					reason.push(p);
				//	double startpathtime = cpuTime();

					/*Dijkstra & detector = *reach_detectors[d]->positive_dist_detector;
					//the reason is a path from s to p(provided by d)
					//p is the var for a reachability detector in dijkstra, and corresponds to a node
					detector.update();
					Var v = var(p);
					int u =reach_detectors[d]->getNode(v); //reach_detectors[d]->reach_lit_map[v];
					assert(detector.connected(u));
					int w;
					while(( w= detector.previous(u)) > -1){
						Lit l = mkLit( edges[w][u].v,true );
						assert(S->value(l)==l_False);
						reason.push(l);
						u=w;
					}*/
					Var v = var(p);
					int flow = flow_lits[reach_lit_map[v-first_reach_var]].max_flow;
					//int u =getNode(v);
					buildMaxFlowTooHighReason(flow,reason);


					//double elapsed = cpuTime()-startpathtime;
				//	pathtime+=elapsed;
				}else if(marker==non_reach_marker){
					reason.push(p);

					//the reason is a cut separating p from s;
					//We want to find a min-cut in the full graph separating, where activated edges (ie, those still in antig) are weighted infinity, and all others are weighted 1.

					//This is a cut that describes a minimal set of edges which are disabled in the current graph, at least one of which would need to be activated in order for s to reach p
					//assign the mincut edge weights if they aren't already assigned.


					Var v = var(p);
					int flow = flow_lits[reach_lit_map[v-first_reach_var]].max_flow;
					//int t = getNode(v); // v- var(reach_lits[d][0]);
					buildMaxFlowTooLowReason(flow,reason);

				}else{
					assert(false);
				}
		}

		bool MaxflowDetector::propagate(vec<Assignment> & trail,vec<Lit> & conflict){


		double startdreachtime = cpuTime();

		double reachUpdateElapsed = cpuTime()-startdreachtime;
		outer->reachupdatetime+=reachUpdateElapsed;

		double startunreachtime = cpuTime();

		double unreachUpdateElapsed = cpuTime()-startunreachtime;
		outer->unreachupdatetime+=unreachUpdateElapsed;
		for(int j = 0;j<flow_lits.size();j++){

				DistLit f = flow_lits[j];
				Lit l = f.l;
				int maxflow = f.max_flow;
				//int u = getNode(var(l));
				int over_maxflow;
				int under_maxflow;

				if((under_maxflow=positive_detector->maxFlow(source,target))>=maxflow){
					if(outer->S->value(l)==l_True){
						//do nothing
					}else if(outer->S->value(l)==l_Undef){
						trail.push(Assignment(false,true,detectorID,0,var(l)));
						outer->S->uncheckedEnqueue(l,reach_marker) ;

					}else if(outer->S->value(l)==l_False){
						conflict.push(l);
						buildMaxFlowTooHighReason(maxflow,conflict);
						return false;
					}

				}else if((over_maxflow = negative_detector->maxFlow(source,target))<maxflow){
					if(outer->S->value(l)==l_False){
						//do nothing
					}else if(outer->S->value(l)==l_Undef){
						trail.push(Assignment(false,false,detectorID,0,var(l)));
						outer->S->uncheckedEnqueue(~l,reach_marker) ;

					}else if(outer->S->value(l)==l_True){
						conflict.push(~l);
						buildMaxFlowTooLowReason(maxflow,conflict);
						return false;
					}

				}



		}

			return true;
		}

bool MaxflowDetector::checkSatisfied(){

				for(int j = 0;j< flow_lits.size();j++){

						Lit l = flow_lits[j].l;
						int dist = flow_lits[j].max_flow;

						if(l!=lit_Undef){
							//int node =getNode(var(l));

							if(outer->S->value(l)==l_True){
								if(positive_detector->maxFlow(source,target)<dist){
									return false;
								}
							}else if (outer->S->value(l)==l_False){
								if( negative_detector->maxFlow(source,target)>=dist){
									return false;
								}
							}else{
								if(positive_detector->maxFlow(source,target)>=dist){
									return false;
								}
								if(!negative_detector->maxFlow(source,target)<dist){
									return false;
								}
							}
						}

				}
	return true;
}
Lit MaxflowDetector::decide(){
	/*MaxflowDetector *r =this;
	Distance<MaxflowDetector::DetectorStatus,NegativeEdgeStatus> * over = (Distance<MaxflowDetector::DetectorStatus,NegativeEdgeStatus>*) r->negative_detector;

	Distance<MaxflowDetector::DetectorStatus,PositiveEdgeStatus> * under = (Distance<MaxflowDetector::DetectorStatus,PositiveEdgeStatus>*) r->positive_detector;

	//we can probably also do something similar, but with cuts, for nodes that are decided to be unreachable.

	//ok, for each node that is assigned reachable, but that is not actually reachable in the under approx, decide an edge on a feasible path

	//this can be obviously more efficient
	//for(int j = 0;j<nNodes();j++){
	for(int k = 0;k<flow_lits.size();k++){
		for(int n = 0;n<flow_lits[k].size();n++){
			Lit l = flow_lits[k][n].l;
			int min_dist = flow_lits[k][n].min_distance;
			if(l==lit_Undef)
				continue;
			int j = r->getNode(var(l));
			if(outer->S->value(l)==l_True){
				//if(S->level(var(l))>0)
				//	continue;

				assert(over->distance(j)<=min_dist);//else we would already be in conflict before this decision was attempted!
				if(under->distance(j)>min_dist){
					//then lets try to connect this
					static vec<bool> print_path;

					assert(over->connected(j));//Else, we would already be in conflict


					int p =j;
					int last=j;
					//if(!opt_use_random_path_for_decisions)
					{
						//ok, read back the path from the over to find a candidate edge we can decide
						//find the earliest unconnected node on this path
						over->update();
						 p = j;
						 last = j;
						 int dist = 0;
						while(under->distance(p)>=min_dist-dist){

							last=p;
							assert(p!=r->source);
							int prev = over->previous(p);
							assert(over->distance(p)<=min_dist-dist);
							dist+=1;//should really be weighted
							p = prev;

						}
					}else{
					//This won't work (without modification) because we need to constrain these paths to ones of maximum real distance < min_dist.
						//Randomly re-weight the graph sometimes
						if(drand(rnd_seed)<opt_decide_graph_re_rnd){

							for(int i=0;i<outer->g.nodes;i++){
									 double w = drand(rnd_seed);
									 w-=0.5;
									 w*=w;
									 //printf("%f (%f),",w,rnd_seed);
									 rnd_path->setWeight(i,w);
								 }
						}
						 rnd_path->update();
						//derive a random path in the graph
						 p = j;
						 last = j;
						 assert( rnd_path->connected(p));
						while(!under->connected(p)){

							last=p;
							assert(p!=source);
							int prev = rnd_path->previous(p);
							p = prev;
							assert(p>=0);
						}

					}



					//ok, now pick some edge p->last that will connect p to last;
					assert(!under->connected(last));
					assert(under->connected(p));

					assert(over->connected(last));
					assert(over->connected(p));

					for(int k = 0;k<outer->antig.adjacency[p].size();k++){
						int to = outer->antig.adjacency[p][k].node;
						if (to==last){
							Var v =outer->edge_list[ outer->antig.adjacency[p][k].id].v;
							if(outer->S->value(v)==l_Undef){
								return mkLit(v,false);
							}else{
								assert(outer->S->value(v)!=l_True);
							}
						}
					}

				}
			}
		}
	}*/
	return lit_Undef;
};


