/****************************************************************************************[Solver.h]
 The MIT License (MIT)

 Copyright (c) 2017, Sam Bayless

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
 associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
 NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
 OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 **************************************************************************************************/

#ifndef MONOSAT_FLOWROUTER_H
#define MONOSAT_FLOWROUTER_H

#include "monosat/graph/GraphTheory.h"
#include "monosat/graph/ReachDetector.h"
#include "monosat/mtl/Vec.h"
#include "monosat/mtl/IntMap.h"
#include "monosat/mtl/Heap.h"
#include "monosat/mtl/Alg.h"
#include "monosat/mtl/Rnd.h"
#include "monosat/utils/Options.h"
#include "monosat/core/SolverTypes.h"
#include "monosat/core/Theory.h"
#include "monosat/core/TheorySolver.h"
#include "monosat/core/Config.h"
#include "monosat/dgl/RamalReps.h"
using namespace dgl;
namespace Monosat {
template<typename Weight>
class FlowRouter : public Theory,public MaxflowDetector<Weight>::FlowListener {
    IntSet<> edgevars;
    int theory_index=-1;
    struct Net{
        //vec<int> members;
        vec<ReachDetector<Weight>*> detectors;
        vec<Lit> reach_lits;
        //vec<int> dest_edges;
        vec<Lit> dest_edgelits;
        //int disconnectedEdge=-1;
        Lit disconnectedEdgeLit=lit_Undef;
        int n_satisifed = 0;
    };
    Solver * S;
    GraphTheorySolver<Weight> *g_theory=nullptr;
    MaxflowDetector<Weight> * maxflow_detector=nullptr;
    vec<Net> nets;

    int sourceNode;
    int destNode;
    Lit flowLit;
    int routerID=-1;
    vec<Lit> enabled_routing_lits;
    vec<Lit> disabled_routing_lits;
    vec<Lit> inner_conflict;
    long stats_flow_decisions=0;
    long stats_flow_conflicts=0;

    struct NetHeurisitc{
        //vec<Reach*> reaches;
        Reach* reach;
        int source;
        int outer_dest;
        IntSet<> destinations;

    };
    vec<NetHeurisitc> dest_sets;
    DynamicGraph<Weight> heuristic_graph;
    //UnweightedRamalReps<Weight> * reach=nullptr;
public:
    FlowRouter(Solver * S,GraphTheorySolver<Weight> * g,int sourceNode,int destNode,Lit maxflowLit);

    int getRouterID(){
        return routerID;
    }

    void addNet(Lit disabledEdge,vec<Lit> & dest_edges, vec<Lit> & reach_lits);


    void edgeFlowChange(int edgeID,const Weight & flow) override{
        if(flow>0){
            heuristic_graph.enableEdge(edgeID);
        }else{
            heuristic_graph.disableEdge(edgeID);
        }
    }


    void printStats(int detailLevel = 0) override{
        printf("Flow Router:\n");
        printf("\tFlow conflicts: %d\n",stats_flow_conflicts);
        printf("\tFlow decisions: %d\n",stats_flow_decisions);
    }
    bool propagateTheory(vec<Lit> & conflict) override{
        return propagateTheory(conflict,false);
    }
    bool propagateTheory(vec<Lit> & conflict, bool solve);
    bool solveTheory(vec<Lit> & conflict) override{
        return propagateTheory(conflict,true);
    }

    inline int getTheoryIndex()const {
        return theory_index;
    }
    inline void setTheoryIndex(int id) {
        theory_index = id;
    }
    inline void newDecisionLevel() {

    }
    inline void backtrackUntil(int untilLevel){

    }
    inline int decisionLevel() {
        return S->decisionLevel();
    }
    inline void undecideTheory(Lit l){

    }
    void enqueueTheory(Lit l) {

    }


    Lit decideTheory(CRef &decision_reason) override;

    class OpportunisticHeuristic :public Heuristic{
        FlowRouter * router;
        GraphTheorySolver<Weight> *outer;
        ReachDetector<Weight> * r;
        LSet to_decide;


        Lit reach_lit = lit_Undef;
        int last_over_modification=-1;
        int last_over_addition=-1;
        int last_over_deletion=-1;
        int over_history_qhead=0;
        int last_over_history_clear=0;

        int last_under_history_clear=0;
        int under_history_qhead=0;
        int last_under_modification=-1;
        int last_under_addition=-1;
        int last_under_deletion=-1;

        DynamicGraph<Weight> & g_over;
        DynamicGraph<Weight> & g_under;
        DynamicGraph<Weight> & g_h;
        IntSet<int> path_edges;
        bool path_is_cut=false;
        int dest_node=-1;

        int current_path_dest= -1;
    public:

        int netID;
        int dest;

        OpportunisticHeuristic(FlowRouter * router,int netNum, ReachDetector<Weight> * rd,int dest,Lit reach_lit):router(router),outer(rd->outer),netID(netNum),dest(dest),r(rd),g_over(rd->g_over),g_under(rd->g_under),g_h(router->heuristic_graph),reach_lit(reach_lit){

        }




        void computePath(int to){
            path_is_cut=false;
            current_path_dest = to;
            Reach * reach= router->dest_sets[netID].reach;
            auto * over_reach =reach;
            auto * over_path = reach;

            static int iter = 0;
            if(++iter==109){
                int a=1;
            };



            assert(over_path);
            assert(over_reach);

            if(opt_graph_use_cache_for_decisions==1){
                over_path->clearCache();
            }

            to_decide.clear();
            path_edges.clear();

            Lit l = reach_lit;
            assert (l != lit_Undef);


            if (outer->value(l) == l_True) {

                if (over_path->connected(to)) {

                    to_decide.clear();


                    assert(over_path->connected(to));			//Else, we would already be in conflict
                    int p = to;
                    int last_edge = -1;
                    int last = to;
                    over_path->update();


                    while(p!=r->source){
                        last = p;
                        assert(p != r->source);
                        last_edge = over_path->incomingEdge(p);
                        if(outer->g_over.hasEdge(last_edge)) {
                            path_edges.insert(last_edge);
                            Var edge_var = outer->getEdgeVar(last_edge);
                            if (outer->value(edge_var) == l_Undef) {
                                to_decide.push(mkLit(edge_var, false));
                            }
                        }
                        int prev = over_path->previous(p);
                        p = prev;

                    }
                }

            }

        }

            bool needsRecompute(int to){
                if (to!=current_path_dest)
                    return true;
                if (outer->value(reach_lit) != l_False && path_is_cut) {
                    return true;
                }else if (outer->value(reach_lit) != l_True && !path_is_cut) {
                    return true;
                }


                //check if any edges on the path have been removed since the last update
                if (last_over_modification > 0 && g_over.modifications == last_over_modification && last_under_modification > 0 && g_under.modifications == last_under_modification){
                    return false;
                }
                if (last_over_modification <= 0 || g_over.changed() || last_under_modification <= 0 || g_under.changed()) {//Note for the future: there is probably room to improve this further.
                    return true;
                }

                if(!path_edges.size()){
                    return true;
                }

                if (last_over_history_clear != g_over.historyclears || last_under_history_clear != g_under.historyclears) {
                    over_history_qhead = g_over.historySize();
                    last_over_history_clear = g_over.historyclears;
                    under_history_qhead = g_under.historySize();
                    last_under_history_clear = g_under.historyclears;
                    to_decide.clear();
                    for(int edgeID:path_edges){
                        if(!path_is_cut) {
                            if (!g_over.edgeEnabled(edgeID))
                                return true;
                            Var edge_var = outer->getEdgeVar(edgeID);
                            Lit l = mkLit(edge_var, false);

                            if (outer->value(edge_var) == l_Undef) {

                                to_decide.push(l);
                            }
                        }else{
                            if (g_under.edgeEnabled(edgeID))
                                return true;
                            Var edge_var = outer->getEdgeVar(edgeID);
                            Lit l = mkLit(edge_var, true);
                            if (outer->value(edge_var) == l_Undef) {

                                to_decide.push(l);
                            }
                        }
                    }
                }

                for (int i = over_history_qhead; i < g_over.historySize(); i++) {
                    int edgeid = g_over.getChange(i).id;

                    if (g_over.getChange(i).addition && g_over.edgeEnabled(edgeid)) {
                        if(!path_is_cut) {

                        }else{
                            Var edge_var = outer->getEdgeVar(edgeid);
                            Lit l = mkLit(edge_var, true);
                            if (outer->value(edge_var) == l_Undef) {

                                to_decide.push(mkLit(edge_var, true));
                            }
                        }
                    } else if (!g_over.getChange(i).addition && !g_over.edgeEnabled(edgeid)) {
                        if(!path_is_cut) {
                            if (path_edges.has(edgeid)) {
                                return true;
                            }
                        }else{

                        }
                    }
                }

                for (int i = under_history_qhead; i < g_under.historySize(); i++) {
                    int edgeid = g_under.getChange(i).id;

                    if (path_edges.has(edgeid)) {
                        if (!g_under.getChange(i).addition && !g_under.edgeEnabled(edgeid)) {
                            if (!path_is_cut) {
                                Var edge_var = outer->getEdgeVar(edgeid);
                                Lit l = mkLit(edge_var, false);
                                if (outer->value(edge_var) == l_Undef) {

                                    to_decide.push(mkLit(edge_var, false));
                                }
                            }else {

                            }
                        } else if (g_under.getChange(i).addition && g_under.edgeEnabled(edgeid)) {
                            if (!path_is_cut) {

                            }else {
                                if (path_edges.has(edgeid)) {
                                    return true;
                                }
                            }
                        }
                    }
                }

                last_over_modification = g_over.modifications;
                last_over_deletion = g_over.deletions;
                last_over_addition = g_over.additions;
                over_history_qhead = g_over.historySize();
                last_over_history_clear = g_over.historyclears;

                last_under_modification = g_under.modifications;
                last_under_deletion = g_under.deletions;
                last_under_addition = g_under.additions;
                under_history_qhead = g_under.historySize();
                last_under_history_clear = g_under.historyclears;
                return false;
            }


        Lit decideTheory(CRef & decision_reason) override{
            decision_reason = CRef_Undef;
            NetHeurisitc & n = router->dest_sets[netID];
            Reach * reach= router->dest_sets[netID].reach;
            if (opt_flow_router_heuristic==2 && router->nets[netID].n_satisifed <=1){
                //suppress flow based decisions if only 1 path is left to route.
                return lit_Undef;
            }

            /*if(opt_verb>=2){
                g_h.drawFull(false,true);
            }*/
            if (reach->connected(n.outer_dest)){
                //step back from outer dest
                int prev = reach->previous(n.outer_dest);
                assert(prev>=0);
                if(n.destinations.has(prev)){
                    //this is a valid path, that we can extract out of the network flow solution

                    int to = prev;


                    if (outer->value(reach_lit)==l_Undef){
                        return lit_Undef;//if the reach lit is unassigned, do not make any decisions here
                    }
                    static int iter = 0;
                    if(++iter==11){
                        int a=1;
                    }

                    if(needsRecompute(to)){
                        r->stats_heuristic_recomputes++;
                        computePath(to);

                        last_over_modification = g_over.modifications;
                        last_over_deletion = g_over.deletions;
                        last_over_addition = g_over.additions;
                        over_history_qhead = g_over.historySize();
                        last_over_history_clear = g_over.historyclears;

                        last_under_modification = g_under.modifications;
                        last_under_deletion = g_under.deletions;
                        last_under_addition = g_under.additions;
                        under_history_qhead = g_under.historySize();
                        last_under_history_clear = g_under.historyclears;

                    }
    #ifdef DEBUG_GRAPH
                    for(Lit l:to_decide){
                assert(outer->value(l)!=l_False);
            }

            if(!path_is_cut){
                for(int edgeID:path_edges){
                    assert(g_over.edgeEnabled(edgeID));
                    Lit l = mkLit(outer->getEdgeVar(edgeID));
                    if(outer->value(l)==l_Undef){
                        assert(to_decide.contains(l));
                    }
                }

            }else{
                for(int edgeID:path_edges){
                    assert(!g_under.edgeEnabled(edgeID));
                    Lit l = mkLit(outer->getEdgeVar(edgeID));
                    if(outer->value(l)==l_Undef){
                        assert(to_decide.contains(~l));
                    }
                }
            }
    #endif

                    if (to_decide.size()) {//  && last_decision_status == over_path->numUpdates() the numUpdates() monitoring strategy doesn't work if the constraints always force edges to be assigned false when other edges
                        // are assigned true, as the over approx graph will always register as having been updated.
                        //instead, check the history of the dynamic graph to see if any of the edges on the path have been assigned false, and only recompute in that case.
                        while (to_decide.size()) {
                            Lit l = to_decide.last();

                            if (outer->value(l) == l_Undef) {
                                //stats_decide_time += rtime(2) - startdecidetime;
                                router->stats_flow_decisions++;
                                return l;
                            }else if(outer->value(l)==l_True){
                                //only pop a decision that was actually made
                                to_decide.pop();
                            }else if(outer->value(l)==l_False){
                                //is this even a reachable state? it probably shouldn't be.
                                to_decide.clear();
                                path_edges.clear();
                                //needs recompute!
                                return decideTheory(decision_reason);
                            }
                        }
                    }
                    }
                }
                return lit_Undef;
            }



    };
};


template<typename Weight>
FlowRouter<Weight>::FlowRouter(Solver * S,GraphTheorySolver<Weight> * g,int sourceNode,int destNode,Lit maxflowLit):S(S),g_theory(g),sourceNode(sourceNode),destNode(destNode),flowLit(maxflowLit){

    S->addTheory(this);

    routerID = getTheoryIndex();
    Theory * t = S->theories[S->getTheoryID(maxflowLit)];
   //this is really a mess...
   //and should be done more safely in the future...
   //This code is recovering the reachability detector associated with each net member
   GraphTheorySolver<Weight> * g2 = (GraphTheorySolver<Weight> *) t;
   Detector * d =  g2->detectors[g2->getDetector(var(S->getTheoryLit(maxflowLit)))];
   assert(d);
   assert(strcmp(d->getName(),"Max-flow Detector")==0);
   MaxflowDetector<Weight> * rd = (MaxflowDetector<Weight> *) d;
   this->maxflow_detector = rd;

    if(opt_flow_router_heuristic){
        while(heuristic_graph.nodes()<maxflow_detector->g_over.nodes()){
            heuristic_graph.addNode();
        }
        for(int i = 0;i<maxflow_detector->g_over.edges();i++){
            int from = maxflow_detector->g_over.getEdge(i).from;
            int to = maxflow_detector->g_over.getEdge(i).to;
            int edgeID = heuristic_graph.addEdge(from,to);
            heuristic_graph.setEdgeEnabled(edgeID,false);
        }

        rd->attachFlowListener(this);

    }
}

template<typename Weight>
void FlowRouter<Weight>::addNet(Lit disabledEdge,vec<Lit> & dest_edges, vec<Lit> & reach_lits){
    if(!S->okay())
        return;
    /*
     *   struct Net{
        //vec<int> members;
        ReachDetector<Weight> * det;
        vec<Lit> member_lits;
        //vec<int> dest_edges;
        vec<Lit> dest_edgelits;
        //int disconnectedEdge=-1;
        Lit disconnectedEdgeLit=lit_Undef;
    };
     */
    nets.push();
    dest_sets.push();
    int common_source=-1;


    nets.last().disconnectedEdgeLit = disabledEdge;
    dest_edges.copyTo(nets.last().dest_edgelits);
    for(Lit l:dest_edges){
        S->setDecisionVar(var(l),false);
    }
    S->setDecisionVar(var(disabledEdge),false);

    if(opt_flow_router_heuristic>0){
        int outer_dest = heuristic_graph.addNode();
        dest_sets.last().outer_dest=outer_dest;

    }

    for(Lit l:reach_lits){
        nets.last().reach_lits.push(l);

        assert(S->hasTheory(l));
        Theory * t = S->theories[S->getTheoryID(l)];
        //this is really a mess...
        //and should be done more safely in the future...
        //This code is recovering the reachability detector associated with each net member
        GraphTheorySolver<Weight> * g = (GraphTheorySolver<Weight> *) t;
        Detector * d =  g->detectors[g->getDetector(var(S->getTheoryLit(l)))];
        assert(d);
        assert(strcmp(d->getName(),"Reachability Detector")==0);
        ReachDetector<Weight> * rd = (ReachDetector<Weight> *) d;
        nets.last().detectors.push(rd);

        int source = rd->source;
        if(common_source<0){
            common_source=source;
        }else{
            assert(source==common_source);
        }
        int dest = rd->getReachNode(S->getTheoryLit(l));

        assert(dest>=0);
        if(opt_flow_router_heuristic>0){
            int outer_dest = dest_sets.last().outer_dest;
            heuristic_graph.addEdge(dest,outer_dest);
            dest_sets.last().destinations.insert(dest);
            rd->attachSubHeuristic(new OpportunisticHeuristic(this,nets.size()-1,rd,dest,l),dest);
        }

    }
    if(opt_flow_router_heuristic>0){
        dest_sets.last().source = common_source;
        dest_sets.last().reach = new UnweightedRamalReps<Weight>(common_source, heuristic_graph);
    }



}

template<typename Weight>
Lit FlowRouter<Weight>::decideTheory(CRef &decision_reason){
#ifdef DEBUG_GRAPH
    for(int i = 0;i<maxflow_detector->g_over.edges();i++){
        Weight w = maxflow_detector->overapprox_conflict_detector->getEdgeFlow(i);
        if(w>0){
            assert(heuristic_graph.edgeEnabled(i));
        }else{
            assert(!heuristic_graph.edgeEnabled(i));
        }
    }
#endif

}

template<typename Weight>
bool FlowRouter<Weight>::propagateTheory(vec<Lit> &conflict, bool solve) {
    //for each net to be routed, pick one unrouted endpoint (if any).
    //connect it to destination in g.
    static int iter = 0;
    ++iter;

    if(!maxflow_detector->propagate(conflict)){
        return false;
    }

   /* if(S->decisionLevel()==0)
        return true;//don't do anything at level 0*/


    bool has_level_decision=false;
    int lev = S->decisionLevel();
    enabled_routing_lits.clear();
    disabled_routing_lits.clear();
    inner_conflict.clear();

    DynamicGraph<Weight> & g = g_theory->g_over;
    //vec<int> routing_edges;

    //nets that are completely routed are connected directly to the dest node; if all are connected then do nothing
    for(int j = 0;j<nets.size();j++){
        Net & net = nets[j];
        net.n_satisifed = 0;
        //check if any members of this detector are unrouted
        Lit unrouted = lit_Undef;
        int unrouted_n = -1;
        for(int i = 0;i<net.dest_edgelits.size();i++){
            ReachDetector<Weight> * r = net.detectors[i];
            Lit l = net.reach_lits[i];
            if(S->value(l)==l_True) {
                Lit edge_lit = net.dest_edgelits[i];
                //find an endpoint that is not yet connected in the under approx graph, or arbitrarily use the last one if all of them are connected
                if ((i==net.dest_edgelits.size()-1)  || ( !r->isConnected(S->getTheoryLit(l),false) && unrouted==lit_Undef)){
                	unrouted = edge_lit;
					unrouted_n = i;
                    //assert(S->value(net.disconnectedEdgeLit)!=l_True);
                    if(S->value(edge_lit)==l_Undef){
						enabled_routing_lits.push(edge_lit);
                        if(!has_level_decision){
                            has_level_decision=true;
                            S->newDecisionLevel();
                        }
						S->enqueue((edge_lit),CRef_Undef);
                	}
                }else{
                    net.n_satisifed++;
                	if(S->value(edge_lit)==l_Undef){
                    disabled_routing_lits.push(~edge_lit);
                    //g_theory->enqueue(~S->getTheoryLit(edge_lit),CRef_Undef);
                        if(!has_level_decision){
                            has_level_decision=true;
                            S->newDecisionLevel();
                        }
                        S->enqueue(~(edge_lit),CRef_Undef);
                	}
                }
            }
        }
        assert(unrouted!=lit_Undef);
        /*
        if(unrouted==lit_Undef){
            //connect source to dest directly
            //g.enableEdge(net.disconnectedEdge);
        	if(S->value(net.disconnectedEdgeLit)==l_Undef){
                if(!has_level_decision){
                    has_level_decision=true;
                    S->newDecisionLevel();
                }
        		S->enqueue((net.disconnectedEdgeLit),CRef_Undef);
        		enabled_routing_lits.push(net.disconnectedEdgeLit);
        	}else{
                assert(S->value(net.disconnectedEdgeLit)==l_True);
            }
            //routing_edges.push(net.disconnectedEdgeLit);
        }*//*else{
            //connect unrouted to dest
            //int edge_ID = net.dest_edges[unrouted_n];
            //g.enableEdge(edgeID);
            //Lit l = net.dest_edgelits[unrouted_n];
            //routing_edges.push(edge_ID);
        	if(S->value(net.disconnectedEdgeLit)==l_Undef){
                if(!has_level_decision){
                    has_level_decision=true;
                    S->newDecisionLevel();
                }
        		S->enqueue(~(net.disconnectedEdgeLit),CRef_Undef);
        		disabled_routing_lits.push(~net.disconnectedEdgeLit);
        	}else{
                assert(S->value(net.disconnectedEdgeLit)==l_True);
            }
        }*/
    }
    inner_conflict.clear();
    conflict.clear();
    //assert(lev>=1);
    if(!maxflow_detector->propagate(inner_conflict)){
        //if there is a conflict, remove any of the edge lits that were assigned here
        for(int i = 0;i<inner_conflict.size();i++){
            Lit l = inner_conflict[i];
            assert(l!=lit_Undef);
            if(edgevars.has(var(l))){
                int a=1;
                //drop this literal
            }else if (S->level(var(l))>lev){
                int a=1;
                //drop this literal.
                //Note that in addition to edge literals assigned temporarily above, it may be possible for the solver to
                //assign other literals through bcp.
                //I _believe_ that any such literals are safe to remove from the learnt clause.
            }else{
                conflict.push(l);
            }
        }
        S->cancelUntil(lev);
        for(Lit l : enabled_routing_lits){
            //g_theory->backtrackAssign(S->getTheoryLit(l));
            //S->unsafeUnassign(l);
            assert(S->value(l)==l_Undef);
        }

        for(Lit l : disabled_routing_lits){
            //g_theory->backtrackAssign(S->getTheoryLit(l));
            //S->unsafeUnassign(l);
            assert(S->value(l)==l_Undef);
        }
        for(Lit l:conflict){
            assert(S->value(l)!=l_Undef);
        }
        stats_flow_conflicts++;
        return false;
    }
    maxflow_detector->collectChangedEdges();//for edge collection here to keep the heuristics up to date
    /*if(opt_verb>=2){
        heuristic_graph.drawFull(false,true);
    }*/
    if(!solve) {
        S->cancelUntil(lev);

        for (Lit l : enabled_routing_lits) {
            //S->unsafeUnassign(l);
            //g_theory->backtrackAssign(S->getTheoryLit(l));
            assert(S->value(l) == l_Undef);
        }

        for (Lit l : disabled_routing_lits) {
            //S->unsafeUnassign(l);
            //g_theory->backtrackAssign(S->getTheoryLit(l));
            assert(S->value(l) == l_Undef);
        }
    }else{
        int a =1;
    }
    return true;

}


};

#endif //MONOSAT_FLOWROUTER_H