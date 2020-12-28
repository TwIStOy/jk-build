// Copyright (c) 2020 Hawtian Wang
//

#include "args.hxx"

namespace jk::cli {

void LinkScript(args::Subparser &parser) {
  args::ValueFlag<std::string> verbose(parser, "VERBOSE", "verbose",
                                       {"verbose"}, args::Options::Required);
}

}  // namespace jk::cli
