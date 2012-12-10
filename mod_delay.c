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
 * $Id: mod_delay.c 18 2009-11-02 18:59:39Z jo $
*********************************************************************/

/*
 	mod_delay

	is an output filter that does buffering until it is told to stop
	this (by a note). It also enables the sending of 304 by delaying the
	call to ap_meets_conditions until then.

  You can end delaying by sending a note with the key delay_end. This 
  can be done from e.g. mod_perl. Sending the note switches 
  of buffering, but does not enable calling ap_meets_conditions. To
  call ap_meets_conditions yourself you need to call the modules optional 
  delay_end function.

  Calling delay_end should be _followed_ by calling ap_meets_conditions.
  See mod_sqil.c for an example. Calling ap_meets_conditions before
  will not have any effect. 

  You should be aware that sucessfully using mod_delay requires that
  1. the delay filter is in the output chain and
  2. a module in the lower part of the output chain calls delay_end.
  If 1. is missing, there will simply be no delaying. 
  If 2. is missing, all output will be delayed to the end of the response.
  This may have very unpleasant effects when offering large files for 
  download. 

	The module exports one optional functions and has no configuration.
	It registers as an output filter named delay.
 */

#include <ctype.h>

#include <http_protocol.h>
#include <http_config.h>
#include <util_filter.h>
#include <http_log.h>

#include "mod_delay.h"


/* The notes key that ends delaying. */
static const char *END_NOTE = "delay_end";

/* The mask for no_local_copy */
static const unsigned MASK_NLC = 0x10;

/* The delay filter context */
typedef struct delay_ctx
{
    /* The delayed output */
    apr_bucket_brigade *bb;
    /* Are we delaying */
    int delay_flag;
    /* store flag from request */
    int no_local_copy;
} delay_ctx;

/*+
 * Get the delay flag
 * @param r The request
 * @return The delay flag (0, 1)
 */
static int delay_get_flag(delay_ctx * dcx)
{
    return dcx->delay_flag;
}

/*+
 * Ends delaying by sending a note.
 * @param r The request
 */
static void delay_end(request_rec * r)
{
    apr_table_set(r->notes, END_NOTE, "");

    /* restore no_local_copy */
    r->no_local_copy &= ~MASK_NLC;

    return;
}

/*+
 * Checks for a note to see if delay has ended.
 * @param r The request
 */
static void delay_end_check(ap_filter_t * f)
{
    request_rec *r = f->r;
    delay_ctx *dcx = f->ctx;

    if (apr_table_get(r->notes, END_NOTE)
        /* Avoid multiple info messages */
        && dcx->delay_flag) {
        /* we stop delaying */
        dcx->delay_flag = 0;
        if ((r->no_local_copy & ~MASK_NLC) != dcx->no_local_copy) {
            ap_log_rerror(APLOG_MARK, APLOG_WARNING, 0, r,
                          "no_local_copy has been modified.");
            /* Restore nlc anyway */
            r->no_local_copy = dcx->no_local_copy;
        }

        ap_log_rerror(APLOG_MARK, APLOG_INFO, 0, r, "Delay ended.");
    }

}

/*+
 * Initializes the delay flag with 1.
 * @param r The request
 */
static void delay_flag_init(ap_filter_t * f)
{
    delay_ctx *dcx = f->ctx;

    /* we delay */
    dcx->delay_flag = 1;

    /* save nlc flag */
    dcx->no_local_copy = f->r->no_local_copy;
    /* and set it to true to stop others */
    f->r->no_local_copy |= MASK_NLC;
}

/*+
 * Initializes the delay filter
 * @param f The filter
 * @return OK 
 */
static int delay_filter_init(ap_filter_t * f)
{
    delay_ctx *fctx = f->ctx = apr_pcalloc(f->r->pool, sizeof(delay_ctx));
    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, f->r,
                          "Initialising delay filter.");

    fctx->bb = apr_brigade_create(f->r->pool, f->c->bucket_alloc);

    delay_flag_init(f);

    return OK;
}

/*+
 * The delay filtering function
 * @param f The filter
 * @param bb This runs bucket brigade
 * @return  
 */
static int delay_filter(ap_filter_t * f, apr_bucket_brigade * bb)
{
    delay_ctx *ctxt = f->ctx;
    apr_bucket *b;
    int last = 0;

    ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, f->r, "Delay filter entered.");

    if (!ctxt)
        return ap_pass_brigade(f->next, bb);

    /* Fill flag from note */
    delay_end_check(f);

    if (delay_get_flag(ctxt)) {
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, f->r, "Delaying brigade.");
        /* Is this the last brigade ? */
        for (b = APR_BRIGADE_FIRST(bb);
            b != APR_BRIGADE_SENTINEL(bb); b = APR_BUCKET_NEXT(b)) {
            apr_bucket_setaside(b, f->c->pool);
            if (APR_BUCKET_IS_EOS(b)) {
                ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, f->r,
                              "Found eos bucket.");
                last = 1;
            }
        }

        /* 
         * As long as delay is turned on we store all brigades
         * in the filter context
         */
        APR_BRIGADE_CONCAT(ctxt->bb, bb);
    }
  /*
   * This is _not_ an else since the delay
   * flag may have changed during the execution of
   * the above code.
   */
    if (last || !delay_get_flag(ctxt)) {
        if (ctxt->bb) {
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, f->r,
                          "Sending stored brigade.");
            /*
             * We send out the stored brigades
             */
            APR_BRIGADE_PREPEND(bb, ctxt->bb);
            ctxt->bb = NULL;
        }
        else {
            ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, f->r,
                          "Passing brigade.");
        }
    }

    if (APR_BRIGADE_EMPTY(bb)) { 
        ap_log_rerror(APLOG_MARK, APLOG_DEBUG, 0, f->r,
                          "Dropping empty brigade.");
        return APR_SUCCESS;
    } else {
        return ap_pass_brigade(f->next, bb);
    }
}

static void delay_hooks(apr_pool_t * p)
{

    ap_register_output_filter("delay", delay_filter,
                              delay_filter_init, AP_FTYPE_RESOURCE);

    APR_REGISTER_OPTIONAL_FN(delay_end);
}

module AP_MODULE_DECLARE_DATA delay_module = {
    STANDARD20_MODULE_STUFF,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    delay_hooks
};
