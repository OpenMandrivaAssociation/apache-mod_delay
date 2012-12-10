/********************************************************************
 *	 Copyright (c) 2006, Joachim Zobel
 *	 Author: Joachim Zobel <jz-2006@heute-morgen.de>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * $Id: mod_delay.h 13 2009-09-29 20:31:29Z jo $
*********************************************************************/

#ifndef MOD_DELAY_H
#define MOD_DELAY_H

#include <httpd.h>
#include <apr_optional.h>

APR_DECLARE_OPTIONAL_FN(void, delay_end, (request_rec*) ) ;

#endif
