/**
 * @file AnalysisConfig.h
 * @brief Configuration structure for basic network analysis
 */

#ifndef ANALYSIS_CONFIG_H
#define ANALYSIS_CONFIG_H

#include <string>
#include <thread>

/**
 * @brief Configuration structure for basic network analysis
 */
struct AnalysisConfig {
    int numPermutations;
    bool enablePruning;
    bool saveDetailedResults;
    std::string outputDirectory;
    unsigned int randomSeed;
    
    bool enableParallelExecution;
    bool enableParallelCalculations;
    bool enableParallelPermutations;
    int maxThreads;
    bool enablePerformanceMonitoring;
    
    bool enableStatisticalAnalysis;
    std::string goTermName;
    std::string goDomain;
    
    int batchSize;

    bool randomPruning;

    /**
     * @brief Default constructor with sensible defaults
     */
    AnalysisConfig() :
        numPermutations(1000),
        enablePruning(true),
        saveDetailedResults(false),
        outputDirectory("./output"),
        randomSeed(std::random_device{}()),
        enableParallelExecution(true),
        enableParallelCalculations(true),
        enableParallelPermutations(true),
        maxThreads(std::thread::hardware_concurrency()),
        enablePerformanceMonitoring(false),
        enableStatisticalAnalysis(false),
        goTermName(""),
        goDomain(""),
        batchSize(50),
        randomPruning(false)
    {
    }
};

#endif