/**
 * @file AboutPage.h
 * @brief About BasicSpanner page documentation.
 *
 * This file contains only documentation and is not meant to be compiled.
 * It exists solely to provide detailed information about BasicSpanner.
 */

#ifndef ABOUT_PAGE_H
#define ABOUT_PAGE_H

/** @page about_page About BasicSpanner
 * @brief Comprehensive information about the BasicSpanner software and algorithm.
 *
 * @section about_intro Introduction
 *
 * BasicSpanner is a desktop application for the rapid analysis of networks
 * through extreme graph simplification. Given a set of nodes of interest
 * (seeds), BasicSpanner computes a very strict graph spanner, the basic
 * network, which contains those seeds together with the minimal number of
 * additional nodes (connectors) required to preserve all pairwise distances
 * that the seeds had in the original network.
 *
 * The tool is designed to support network-based structural and functional
 * analyses in arbitrary undirected graphs, and to complement conventional
 * techniques such as community detection or shortest-path analysis.
 * BasicSpanner is implemented in C++ using the Qt framework and is available
 * for Linux and Windows.
 *
 * @section about_core_concept Core Concept: Basic Networks
 *
 * A basic network is the strictest distance preserver that can be derived
 * from an undirected graph: an induced subgraph with the minimal number of
 * units in which the geodesic distances among every pair of selected nodes
 * (seeds) are exactly the same as in the original graph.
 *
 * The nodes of a basic network are of two kinds:
 *
 * 1. Seeds: a predefined set of nodes of interest.
 * 2. Connectors: the minimal additional nodes required to preserve every
 *    seed-to-seed geodesic distance.
 *
 * Seeds and connectors together are the basic units of the network. Because
 * the construction is driven by an explicit distance criterion, the basic
 * network avoids the ambiguities associated with module or community
 * definitions, and provides an objective summary of how a set of nodes is
 * related within a larger graph.
 *
 * @section about_algorithm Algorithmic Strategy
 *
 * Computing the exact basic network by enumerating all combinations of
 * connectors is infeasible for realistic graphs. BasicSpanner therefore
 * implements a heuristic strategy whose core logic is:
 *
 * 1. Seed nodes are marked as basic units.
 * 2. All pairwise geodesic distances among seeds are computed in the
 *    original graph.
 * 3. Seed+n units are identified iteratively: internal nodes of seed-to-seed
 *    geodesics are classified as seed+1, seed+2, ..., seed+n. Units that are
 *    irreplaceable for preserving at least one seed-to-seed geodesic are
 *    promoted to basic units.
 * 4. Paths between seeds are evaluated and pruned: paths traversing basic
 *    or highly reused seed+n units are preferred, while paths through
 *    non-basic, sparsely reused units are eliminated whenever doing so does
 *    not alter any seed-to-seed distance.
 * 5. To mitigate convergence to suboptimal, locally minimal subgraphs, the
 *    algorithm is executed over multiple randomized orderings. Replicates
 *    can either be permutations of a user-defined seed list or independent
 *    batches of randomly drawn seeds.
 * 6. An optional pruning pass removes redundant connectors from tied basic
 *    networks while preserving every seed-to-seed distance. Pruning is
 *    generally required to reach the optimal basic network.
 *
 * @section about_features Software Features
 *
 * BasicSpanner exposes the full workflow through a single graphical
 * interface:
 *
 * - Import of delimited text files (TXT, CSV, TSV) with configurable column
 *   and separator selection; self-loops and isolated nodes are detected and
 *   reported on import.
 * - A stackable filter system that combines criteria such as degree range
 *   (useful to discard high-degree hubs), betweenness centrality, the
 *   largest connected component, isolated-node removal and name patterns.
 * - Seed selection from a user-provided plain-text file or through random
 *   generation from the current node list.
 * - Configurable number of replicates (permutations of user seeds or
 *   batches of random seeds), number of parallel CPU threads and optional
 *   pruning.
 * - A real-time connector-count histogram with summary statistics (mean,
 *   median, standard deviation, range) and per-replicate results.
 * - Interactive network viewer with multiple layout algorithms (Circular,
 *   Yifan Hu, Force Atlas, Spring Force, Hierarchical, Grid, Random) and
 *   node coloring by type, degree or centrality.
 * - Export of basic network edges (CSV or plain text), node-level metrics
 *   and a Gephi-compatible node metadata file.
 *
 * @section about_applications Purpose and Applications
 *
 * The characterization of basic networks supports several analytical goals:
 *
 * - Identify significant connectors: nodes that, without being seeds, are
 *   critical for linking them and may reveal functional relationships that
 *   are not evident in the original graph.
 * - Assess the relatedness of a seed set: comparing the number of
 *   connectors observed for a user-defined seed set against the
 *   distribution obtained from random seed sets of equal size provides a
 *   statistical measure of how closely related the chosen seeds are in the
 *   network.
 * - Complement community and enrichment analyses: basic networks can
 *   highlight structural associations that conventional techniques fail to
 *   detect and can point out inconsistencies or omissions in existing
 *   annotations of the underlying network.
 * - Detect graph inconsistencies: when a single seed dramatically increases
 *   the number of required connectors, it often indicates that the seed is
 *   poorly related to the rest or that the underlying graph contains
 *   missing or erroneous data.
 *
 * The method is applicable to any undirected network regardless of domain.
 *
 * @section about_usage Usage
 *
 * Detailed instructions on compiling and running the software, as well as
 * input file formats, can be found in the accompanying documentation.
 *
 * Step-by-step guides are available in the
 * @ref a_tutorials_main_page "Tutorials section".
 *
 * A typical analysis involves:
 *
 * 1. Loading an input network.
 * 2. Optionally applying filters to the input graph.
 * 3. Providing a seed set or generating seeds at random.
 * 4. Configuring the number of replicates, threads and pruning.
 * 5. Running the analysis and inspecting the resulting basic network and
 *    connector count distribution.
 *
 * @section about_citation Citation
 *
 * If you use BasicSpanner in your work, please cite:
 *
 * Marin J, Marin I. BasicSpanner. Zenodo (2026).
 * https://doi.org/10.5281/zenodo.19697430
 *
 * @section about_license License
 *
 * Please refer to the LICENSE file for information on licensing and usage
 * rights.
 */

#endif
