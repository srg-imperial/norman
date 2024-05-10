#pragma once

#include <memory>

#include <clang/Tooling/Tooling.h>

#include "Config.h"

std::unique_ptr<clang::tooling::FrontendActionFactory> newNormaliseFrontendFactory(bool& rewritten, Config& config);