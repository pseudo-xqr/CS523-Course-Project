/***************************************************************************
 *
 *   BSD LICENSE
 * 
 *   Copyright(c) 2007-2023 Intel Corporation. All rights reserved.
 *   All rights reserved.
 * 
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 * 
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 *  version: QAT20.L.1.2.30-00078
 *
 ***************************************************************************/

/**
 ***************************************************************************
 * @file lac_sal_ctrl.h
 *
 * @ingroup SalCtrl
 *
 * Functions to register and deregister qat and service controllers with ADF.
 *
 ***************************************************************************/

#ifndef LAC_SAL_CTRL_H
#define LAC_SAL_CTRL_H

/*******************************************************************
 * @ingroup SalCtrl
 * @description
 *    This function is used to check whether the service component
 *    has been successfully started.
 *
 * @context
 *      This function is called from the icp_sal_userStart() function.
 *
 * @assumptions
 *      None
 * @sideEffects
 *      None
 * @reentrant
 *      No
 * @threadSafe
 *      Yes
 *
 ******************************************************************/

CpaStatus SalCtrl_AdfServicesStartedCheck(void);

/*******************************************************************
 * @ingroup SalCtrl
 * @description
 *    This function is used to check whether the user's parameter
 *    for concurrent request is valid.
 *
 * @context
 *      This function is called when crypto or compression is init
 *
 * @assumptions
 *      None
 * @sideEffects
 *      None
 * @reentrant
 *      Yes
 * @threadSafe
 *      Yes
 *
 ******************************************************************/
CpaStatus validateConcurrRequest(Cpa32U numConcurrRequests);

/*******************************************************************
 * @ingroup SalCtrl
 * @description
 *    This function is used to register adf services
 *
 * @context
 *      This function is called from do_userStart() function
 *
 * @assumptions
 *      None
 * @sideEffects
 *      None
 * @reentrant
 *      Yes
 * @threadSafe
 *      Yes
 *
 ******************************************************************/
CpaStatus SalCtrl_AdfServicesRegister(void);

/*******************************************************************
 * @ingroup SalCtrl
 * @description
 *    This function is used to unregister adf services.
 *
 * @context
 *      This function is called from do_userStart() function
 *
 * @assumptions
 *      None
 * @sideEffects
 *      None
 * @reentrant
 *      Yes
 * @threadSafe
 *      Yes
 *
 ******************************************************************/
CpaStatus SalCtrl_AdfServicesUnregister(void);

#endif
