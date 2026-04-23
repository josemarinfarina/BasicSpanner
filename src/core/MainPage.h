/**
 * @file MainPage.h
 * @brief Main page documentation for BasicSpanner.
 *
 * This file contains only documentation and is not meant to be compiled.
 * It exists solely to provide the main page in Doxygen.
 */

#ifndef MAIN_PAGE_H
#define MAIN_PAGE_H

/** @mainpage BasicSpanner Documentation
 *
 * @section intro_sec Welcome to BasicSpanner
 *
 * BasicSpanner is a desktop application for the rapid analysis of networks
 * through extreme graph simplification. Given a set of seeds, it computes
 * the basic network: a very strict graph spanner that contains the seeds
 * together with the minimal number of connectors required to preserve all
 * pairwise seed distances of the original graph.
 *
 * @section quick_start Quick Start
 *
 * - New to BasicSpanner? Start with the
 *   @ref a_tutorials_main_page "step-by-step tutorials".
 * - API reference: browse the Classes and Files sections.
 * - Detailed information: see the project README.
 *
 * @section main_features Key Features
 *
 * - Computation of basic networks: minimal subgraphs that preserve every
 *   geodesic distance among seed nodes.
 * - Stackable graph filters (degree range, betweenness centrality, largest
 *   connected component, isolated-node removal, name patterns).
 * - User-defined or randomly generated seed sets with permutation and
 *   batch replicates.
 * - Parallel CPU execution and optional pruning.
 * - Real-time connector-count histogram with summary statistics.
 * - Interactive visualization and export in standard formats (CSV, plain
 *   text, Gephi-compatible node metadata).
 *
 * @section getting_started Getting Started
 *
 * 1. Build the project following the instructions in the README.
 * 2. Prepare an input graph file (TXT, CSV or TSV).
 * 3. Load the network, apply optional filters and select or generate
 *    seeds.
 * 4. Configure replicates, threads and pruning, and run the analysis.
 * 5. Inspect the resulting basic network, node metrics and connector
 *    distribution.
 *
 * @section documentation_sections Documentation Sections
 *
 * - @ref about_page "About BasicSpanner" - comprehensive information about
 *   the software and the basic network concept.
 * - @ref a_tutorials_main_page "Tutorials" - step-by-step guides with
 *   worked examples.
 * - Classes, Files and Namespaces - API reference generated from the
 *   source code.
 *
 * @section citation Citation
 *
 * If you use BasicSpanner in your work, please cite:
 *
 * Marin J, Marin I. BasicSpanner. Zenodo (2026).
 * https://doi.org/10.5281/zenodo.19697430
 */

#endif
