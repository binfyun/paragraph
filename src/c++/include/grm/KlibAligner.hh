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
 * \brief KlibAligner interface
 *
 * \file KlibAligner.hh
 * \author Roman Petrovski
 * \email rpetrovski@illumina.com
 *
 */

#pragma once

#include <list>
#include <memory>
#include <string>

#include "common/Read.hh"
#include "graphcore/Graph.hh"
#include "graphcore/Path.hh"

namespace grm
{

struct KlibAlignerImpl;

/**
 * Aligns using Klib local alignment against each path and picks the best
 */
class KlibAligner
{
public:
    KlibAligner();

    virtual ~KlibAligner();

    KlibAligner(KlibAligner&& rhs) noexcept;

    KlibAligner& operator=(KlibAligner&& rhs) noexcept;

    /**
     * Set the graph to align to
     * @param g a graph
     * @param paths list of paths
     */
    void setGraph(graphtools::Graph const* g, std::list<graphtools::Path> const& paths);

    /**
     * Align a read to the graph and update the graph_* fields.
     *
     * We will align both the forward and the reverse strand and return
     * the alignment which is unique, or with the better score, or default
     * to the forward strand.
     *
     * @param read read structure
     */
    void alignRead(common::Read& read);

    unsigned attempted() const { return attempted_; }
    unsigned mapped() const { return mapped_; }

private:
    unsigned attempted_ = 0;
    unsigned mapped_ = 0;

    std::unique_ptr<KlibAlignerImpl> impl_;
};
}
