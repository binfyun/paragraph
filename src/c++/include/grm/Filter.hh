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
 * \brief Filter interface
 *
 * \file Filter.hh
 * \author Roman Petrovski
 * \email rpetrovski@illumina.com
 *
 */

#pragma once
#include <functional>

#include "common/Read.hh"

namespace grm
{
typedef std::function<bool(common::Read&)> ReadFilter;
}
