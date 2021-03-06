// -*- mode: c++; indent-tabs-mode: nil; -*-
//
// Paragraph
// Copyright (c) 2016-2019 Illumina, Inc.
// All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// You may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//		http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied
// See the License for the specific language governing permissions and limitations
//
//

/**
 * \brief Workflow implementation
 *
 * \file Workflow.cpp
 * \author Roman Petrovski
 * \email rpetrovski@illumina.com
 *
 */

#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>

#include <boost/filesystem.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>

#include "common/JsonHelpers.hh"
#include "common/Threads.hh"
#include "paragraph/Disambiguation.hh"
#include "paragraph/Workflow.hh"

#include "common/Error.hh"

namespace paragraph
{

Workflow::Workflow(
    bool jointInputs, const InputPaths& inputPaths, const InputPaths& inputIndexPaths,
    const std::vector<std::string>& graph_spec_paths, const std::string& output_file_path,
    const std::string& output_folder_path, bool gzipOutput, const Parameters& parameters,
    const std::string& reference_path, const std::string& target_regions)
    : graphSpecPaths_(graph_spec_paths)
    , outputFilePath_(output_file_path)
    , outputFolderPath_(output_folder_path)
    , gzipOutput_(gzipOutput)
    , parameters_(parameters)
    , referencePath_(reference_path)
    , targetRegions_(target_regions)
{
    if (jointInputs)
    {
        unprocessedInputs_.push_back(Input(inputPaths, inputIndexPaths, std::begin(graphSpecPaths_)));
    }
    else
    {
        for (std::size_t i = 0; inputPaths.size() > i; ++i)
        {
            const auto& inputPath = inputPaths[i];
            const auto& inputIndexPath = inputIndexPaths.at(i);
            unprocessedInputs_.push_back(
                Input(InputPaths(1, inputPath), InputPaths(1, inputIndexPath), std::begin(graphSpecPaths_)));
        }
    }
}

static void dumpOutput(const std::string& output, std::ostream& os, const std::string& file)
{
    os << output;
    if (!os)
    {
        error("ERROR: Failed to write output to '%s' error: '%s'", file.c_str(), std::strerror(errno));
    }
}

std::string Workflow::processGraph(
    const std::string& graphSpecPath, const Parameters& parameters, const InputPaths& inputPaths,
    std::vector<common::BamReader>& readers)
{
    common::ReadBuffer allReads;
    for (common::BamReader& reader : readers)
    {
        common::extractReads(
            reader, parameters.target_regions(), (int)(parameters.max_reads()), parameters.longest_alt_insertion(),
            allReads);
    }
    Json::Value outputJson = alignAndDisambiguate(parameters, allReads);
    if (inputPaths.size() == 1)
    {
        outputJson["bam"] = inputPaths.front();
    }
    else
    {
        outputJson["bam"] = Json::arrayValue;
        for (const auto& inputPath : inputPaths)
        {
            outputJson["bam"].append(inputPath);
        }
    }
    return common::writeJson(outputJson);
}

void Workflow::makeOutputFile(const std::string& output, const std::string& graphSpecPath)
{
    const boost::filesystem::path inputPath(graphSpecPath);
    boost::filesystem::path outputPath = boost::filesystem::path(outputFolderPath_) / inputPath.filename();
    if (gzipOutput_)
    {
        outputPath += ".gz";
    }
    boost::iostreams::basic_file_sink<char> of(outputPath.string());
    if (!of.is_open())
    {
        error("ERROR: Failed to open output file '%s'. Error: '%s'", outputPath.string().c_str(), std::strerror(errno));
    }

    boost::iostreams::filtering_ostream fos;
    if (gzipOutput_)
    {
        fos.push(boost::iostreams::gzip_compressor());
    }
    fos.push(of);

    dumpOutput(output, fos, outputPath.string());
}

void Workflow::processGraphs(std::ostream& outputFileStream)
{
    for (Input& input : unprocessedInputs_)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        while (graphSpecPaths_.end() != input.unprocessedGraphs_)
        {
            const std::string& graphSpecPath = *(input.unprocessedGraphs_++);
            std::string output;
            ASYNC_BLOCK_WITH_CLEANUP([this](bool failure) { terminate_ |= failure; })
            {
                std::vector<common::BamReader> readers;
                //        for (const std::string& inputPath : input.inputPaths_)
                for (size_t i = 0; i != input.inputPaths_.size(); ++i)
                {
                    //            LOG()->info("Opening {} with {}", inputPath, referencePath_);
                    //            readers.push_back(common::BamReader(inputPath, referencePath_));
                    const auto& bamPath = input.inputPaths_[i];
                    const auto& bamIndexPath = input.inputIndexPaths_[i];
                    LOG()->info("Opening {}/{} with {}", bamPath, bamIndexPath, referencePath_);
                    readers.emplace_back(bamPath, bamIndexPath, referencePath_);
                }
                if (terminate_)
                {
                    LOG()->warn("terminating");
                    break;
                }
                common::unlock_guard<std::mutex> unlock(mutex_);
                Parameters parameters = parameters_;
                LOG()->info("Loading parameters {}", graphSpecPath);
                parameters.load(graphSpecPath, referencePath_, targetRegions_);
                LOG()->info("Done loading parameters");

                output = processGraph(graphSpecPath, parameters, input.inputPaths_, readers);

                if (!outputFolderPath_.empty())
                {
                    makeOutputFile(output, graphSpecPath);
                }
            }

            if (!outputFilePath_.empty())
            {
                if (firstPrinted_)
                {
                    outputFileStream << ',';
                }
                dumpOutput(output, outputFileStream, outputFilePath_);
                firstPrinted_ = true;
            }
        }
    }
}

void Workflow::run()
{
    boost::iostreams::filtering_ostream fos;
    if (gzipOutput_)
    {
        fos.push(boost::iostreams::gzip_compressor());
    }
    if (!outputFilePath_.empty())
    {
        if ("-" != outputFilePath_)
        {
            LOG()->info("Output file path: {}", outputFilePath_);
            boost::iostreams::basic_file_sink<char> of(outputFilePath_);
            if (!of.is_open())
            {
                error(
                    "ERROR: Failed to open output file '%s'. Error: '%s'", outputFilePath_.c_str(),
                    std::strerror(errno));
            }
            fos.push(of);
        }
        else
        {
            LOG()->info("Output to stdout");
            fos.push(std::cout);
        }
    }

    if (!outputFilePath_.empty() && 1 < graphSpecPaths_.size())
    {
        fos << "[";
    }

    common::CPU_THREADS(parameters_.threads()).execute([this, &fos]() { processGraphs(fos); });

    if (!outputFilePath_.empty() && 1 < graphSpecPaths_.size())
    {
        fos << "]\n";
    }
}

} /* namespace workflow */
