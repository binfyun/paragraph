// -*- mode: c++; indent-tabs-mode: nil; -*-
//
// Copyright (c) 2017 Illumina, Inc.
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.

// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

/**
 * \summary Read filter based on alignment quality
 *
 * \file BadAlign.hh
 * \author Sai Chen & Egor Dolzhenko & Peter Krusche
 * \email schen6@illumina.com & edolzhenko@illumina.com & pkrusche@illumina.com
 *
 */

#include <algorithm>

#include "graphalign/GraphAlignmentOperations.hh"
#include "paragraph/ReadFilter.hh"

#include "common/Error.hh"

namespace paragraph
{

using graphtools::Graph;
using graphtools::GraphAlignment;
using graphtools::decodeGraphAlignment;

namespace readfilters
{
    class BadAlign : public ReadFilter
    {
    public:
        explicit BadAlign(Graph const* graph, double bad_align_frac)
            : graph_(graph)
            , bad_align_frac_(bad_align_frac)
        {
            assert(graph_ != nullptr);
        }

        std::pair<bool, std::string> filterRead(common::Read const& r) override
        {
            const GraphAlignment mapping = decodeGraphAlignment(r.graph_pos(), r.graph_cigar(), graph_);
            size_t query_clipped = 0;
            for (auto const& aln : mapping)
            {
                query_clipped += aln.numClipped();
            }
            const auto query_aligned = mapping.queryLength() - query_clipped;
            const bool is_bad = query_aligned < round(bad_align_frac_ * (mapping.queryLength()));
            return { is_bad, is_bad ? "bad_align" : "" };
        }

    private:
        Graph const* graph_;
        double bad_align_frac_;
    };
}
}