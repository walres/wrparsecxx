/**
 * \file Config.h
 *
 * \brief Platform-specific definitions for the wrparsecxx library
 *
 * \copyright
 * \parblock
 *
 *   Copyright 2014-2016 James S. Waller
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 * \endparblock
 */
#ifndef WRPARSECXX_CONFIG_H
#define WRPARSECXX_CONFIG_H

#include <wrutil/Config.h>


#if WR_WINDOWS
#       ifdef wrparsecxx_EXPORTS
#               define WRPARSECXX_API __declspec(dllexport)
#       elif defined(wrparsecxx_IMPORTS)
#               define WRPARSECXX_API __declspec(dllimport)
#       else
#               define WRPARSECXX_API
#       endif
#elif WR_HAVE_ELF_VISIBILITY_ATTR
#       ifdef wrparsecxx_EXPORTS
#               define WRPARSECXX_API __attribute__((visibility("default")))
#       else
#               define WRPARSECXX_API
#       endif
#else
#       define WRPARSECXX_API
#endif


#endif // !WRPARSECXX_CONFIG_H
