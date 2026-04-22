/**
 * @file TutorialsDoc.h
 * @brief Documentation-only file containing the tutorials for BasicSpanner.
 * 
 * This file contains only documentation and is not meant to be compiled.
 * It exists solely to provide the tutorials page in Doxygen.
 */

#ifndef TUTORIALS_DOC_H
#define TUTORIALS_DOC_H

/** @page a_tutorials_main_page Tutorials Overview
 * @brief Step-by-step guides for using BasicSpanner.
 *
 * This section provides detailed tutorials to help you understand the BasicSpanner algorithm and software. Each tutorial walks through the process of identifying a Basic Network for an example graph and a given set of seed nodes.
 *
 * @section tutorial1_section Tutorial 1: test.txt (Seeds: 1, 2, 3)
 *
 * This tutorial demonstrates the process of finding the Basic Network for the graph defined in `tests/test.txt` using seed nodes **1, 2, and 3**.
 *
 * @subsection tutorial1_original Original Graph and Seeds
 *
 * The input graph (`tests/test.txt`) is defined by the following edges:
 * ```
 * 1 4
 * 4 2
 * 2 5
 * 5 3
 * 1 6
 * 6 7
 * 7 3
 * 4 6
 * 5 7
 * 2 7
 * ```
 * The selected seed nodes are **1, 2, and 3**.
 *
 * @image html test1_original_graph_seeds123.png "Figure 1: Original graph from tests/test.txt. Seed nodes 1, 2, and 3 are highlighted." width=500px
 * (Placeholder for Figure 1: Visualization of the original graph with seeds 1, 2, 3 highlighted.)
 *
 * @subsection tutorial1_geodesics Original Geodesic Distances
 *
 * The geodesic distances between seed nodes in the original graph are:
 * - d(1,2) = 2 (via path 1-4-2)
 * - d(2,3) = 2 (via paths 2-5-3 and 2-7-3)  
 * - d(1,3) = 3 (via path 1-6-7-3)
 *
 * @subsection tutorial1_step1 Step 1: Identifying Connectors for d(1,2)=2
 *
 * For the geodesic distance d(1,2) = 2, the path 1-4-2 is the only geodesic. Node 4 is essential as a connector.
 *
 * @image html test1_step1_connector4.png "Figure 2: Step 1 - Node 4 identified as essential connector for preserving d(1,2)=2." width=500px
 * (Placeholder for Figure 2: Graph highlighting the path 1-4-2 with node 4 marked as a connector.)
 *
 * @subsection tutorial1_step2 Step 2: Identifying Connectors for d(2,3)=2
 *
 * For d(2,3) = 2, there are two geodesic paths: 2-5-3 and 2-7-3. Both nodes 5 and 7 are potential connectors, but we need at least one to preserve the distance.
 *
 * @image html test1_step2_connectors57.png "Figure 3: Step 2 - Nodes 5 and 7 as potential connectors for d(2,3)=2." width=500px
 * (Placeholder for Figure 3: Graph showing both paths 2-5-3 and 2-7-3.)
 *
 * @subsection tutorial1_step3 Step 3: Identifying Connectors for d(1,3)=3
 *
 * For d(1,3) = 3, the path 1-6-7-3 requires connectors 6 and 7. Since we already need either 5 or 7 from the previous step, node 7 serves both purposes efficiently.
 *
 * @image html test1_step3_connectors67.png "Figure 4: Step 3 - Nodes 6 and 7 needed for d(1,3)=3." width=500px
 * (Placeholder for Figure 4: Graph highlighting path 1-6-7-3.)
 *
 * @subsection tutorial1_result Final Basic Network for test.txt
 *
 * The minimal Basic Network includes:
 * - **Seeds**: 1, 2, 3
 * - **Connectors**: 4, 6, 7
 *
 * @image html test1_basic_network_final.png "Figure 5: Final Basic Network with seeds (red) and connectors (green)." width=500px
 * (Placeholder for Figure 5: Final Basic Network showing only essential nodes and edges.)
 *
 * @subsection tutorial1_validation Validation
 *
 * The Basic Network preserves all original geodesic distances:
 * - d(1,2) = 2 (OK) (via 1-4-2)
 * - d(2,3) = 2 (OK) (via 2-7-3)
 * - d(1,3) = 3 (OK) (via 1-6-7-3)
 *
 * @section tutorial2_section Tutorial 2: test2.txt (Seeds: 1, 2, 3, 4, 5)
 *
 * This tutorial demonstrates the process for a more complex graph in `tests/test2.txt` with seed nodes **1, 2, 3, 4, and 5**.
 *
 * @subsection tutorial2_original Original Graph and Seeds
 *
 * The input graph (`tests/test2.txt`) is defined by the following edges:
 * ```
 * 1 10
 * 10 2
 * 2 11
 * 11 3
 * 1 12
 * 12 13
 * 13 3
 * 2 14
 * 14 4
 * 3 15
 * 15 4
 * 4 20
 * 20 21
 * 21 5
 * 3 22
 * 22 5
 * 13 14
 * 10 12
 * 11 14
 * 15 22
 * ```
 * The selected seed nodes are **1, 2, 3, 4, and 5**.
 *
 * @image html test2_original_graph_seeds12345.png "Figure 6: Original graph from tests/test2.txt with seeds 1, 2, 3, 4, 5 highlighted." width=600px
 * (Placeholder for Figure 6: Visualization of the complex graph with all 5 seeds highlighted.)
 *
 * @subsection tutorial2_geodesics Original Geodesic Distances
 *
 * Key geodesic distances in the original graph:
 * - d(1,2) = 2 (via 1-10-2)
 * - d(1,3) = 3 (via 1-12-13-3 and 1-10-2-11-3)
 * - d(1,4) = 4 (multiple paths)
 * - d(1,5) = 5 (multiple paths)
 * - d(2,3) = 2 (via 2-11-3)
 * - d(2,4) = 2 (via 2-14-4)
 * - d(2,5) = 4 (multiple paths)
 * - d(3,4) = 2 (via 3-15-4)
 * - d(3,5) = 2 (via 3-22-5)
 * - d(4,5) = 3 (via 4-20-21-5)
 *
 * @subsection tutorial2_process Analysis Process
 *
 * Due to the complexity, the algorithm systematically identifies all nodes that lie on geodesic paths between any pair of seed nodes:
 *
 * **Essential connectors identified:**
 * - **10**: Required for d(1,2) = 2
 * - **11**: Required for d(2,3) = 2
 * - **12, 13**: Required for alternative d(1,3) = 3 path
 * - **14**: Required for d(2,4) = 2
 * - **15**: Required for d(3,4) = 2
 * - **20, 21**: Required for d(4,5) = 3
 * - **22**: Required for d(3,5) = 2
 *
 * @image html test2_connectors_identified.png "Figure 7: All essential connectors identified for the complex graph." width=600px
 * (Placeholder for Figure 7: Graph with all connector nodes highlighted.)
 *
 * @subsection tutorial2_result Final Basic Network for test2.txt
 *
 * The Basic Network includes:
 * - **Seeds**: 1, 2, 3, 4, 5
 * - **Connectors**: 10, 11, 12, 13, 14, 15, 20, 21, 22
 *
 * @image html test2_basic_network_final.png "Figure 8: Final Basic Network for the complex example." width=600px
 * (Placeholder for Figure 8: Final Basic Network with seeds and connectors clearly marked.)
 *
 * @subsection tutorial2_validation Validation
 *
 * The algorithm verifies that all pairwise geodesic distances between seeds are preserved in the resulting Basic Network, confirming the correctness of the identified connector set.
 *
 * @section conclusion_section Conclusion
 *
 * These tutorials demonstrate how the BasicSpanner algorithm systematically
 * identifies the minimal set of connector nodes required to preserve geodesic
 * distances between seed nodes. The complexity of the required Basic Network
 * depends on:
 *
 * - The number of seed nodes
 * - The topology of the original graph
 * - The distribution of geodesic paths
 * - The presence of alternative paths between seeds
 *
 * For more information on the theoretical foundation, see the accompanying
 * documentation.
 */

#endif