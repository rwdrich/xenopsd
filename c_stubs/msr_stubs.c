/*
 * Copyright (C) Citrix Systems Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; version 2.1 only. with the special
 * exception on linking described in file LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xenctrl.h>

#include <caml/mlvalues.h>
#include <caml/memory.h>
#include <caml/alloc.h>
#include <caml/custom.h>
#include <caml/fail.h>
#include <caml/signals.h>
#include <caml/callback.h>
#include <caml/unixsupport.h>

#define _H(__h) ((xc_interface *)(__h))
#define _D(__d) ((uint32_t)Int_val(__d))

#define CAML_MSR_T_SIZE 5       /* fields in Xen.Msr.t */
#define CAML_MSR_VERSION 1.0

/* From xenctrl_stubs */
#define ERROR_STRLEN 1024
#define POLICY_DELIMITER ","

// A collection of C bindings for MSR specific operations

// Type definitions
typedef struct serialised_policy {
    int policy_version;
    int max_leaves;
    int max_msrs;
    char* leaves;
    char* msrs;
} serialised_policy_t;

// Need xen definition of  apolicy

#define POLICY_VERSION      = 0
#define POLICY_MAX_LEAVES   = 1
#define POLICY_MAX_MSRS     = 2
#define POLICY_CAML_LEAVES  = 3
#define POLICY_CAML_MSRS    = 4


// Helper functions
static void raise_unix_errno_msg(int err_code, const char *err_msg)
{
    CAMLparam0();
    value args[] = { unix_error_of_code(err_code), caml_copy_string(err_msg) };
    caml_raise_with_args(*caml_named_value("Xenctrlext.Unix_error"), sizeof(args)/sizeof(args[0]), args);
    CAMLnoreturn;
}

static void failwith_xc(xc_interface *xch)
{
    static char error_str[XC_MAX_ERROR_MSG_LEN + 6];
    int real_errno = -1;
    if (xch) {
        const xc_error *error = xc_get_last_error(xch);
        if (error->code == XC_ERROR_NONE) {
            real_errno = errno;
            snprintf(error_str, sizeof(error_str), "%d: %s", errno, strerror(errno));
        } else {
            real_errno = error->code;
            snprintf(error_str, sizeof(error_str), "%d: %s: %s",
                    error->code,
                    xc_error_code_to_desc(error->code),
                    error->message);
        }
    } else {
        snprintf(error_str, sizeof(error_str), "Unable to open XC interface");
    }
    raise_unix_errno_msg(real_errno, error_str);
}


CAMLprim value
serialise_policy(value xch, xen_cpuid_leaf_t* leaves, xen_msr_entry_t* msrs)
{
    CAMLparam1(xch);
    CAMLlocal3(policy, caml_leaves, caml_msrs);
    uint32_t max_leaves = 0;
    uint32_t max_msrs   = 0;

    if (xc_get_cpu_policy_size(_H(xch), &max_leaves, &max_msrs))
        failwith_xc(_H(xch));

    policy = caml_alloc_tuple(CAML_MSR_T_SIZE);

    caml_leaves = caml_alloc_string(max_leaves * sizeof(xen_cpuid_leaf_t));
    caml_msrs =   caml_alloc_string(max_msrs   * sizeof(xen_msr_entry_t ));

    memcpy(String_val(caml_leaves), leaves, max_leaves * sizeof(xen_cpuid_leaf_t));
    memcpy(String_val(caml_msrs),   msrs,   max_msrs   * sizeof(xen_msr_entry_t));

    Store_field(policy, POLICY_VERSION,     CAML_MSR_VERSION);
    Store_field(policy, POLICY_MAX_LEAVES,  max_leaves);
    Store_field(policy, POLICY_MAX_MSRS,    max_msrs);
    Store_field(policy, POLICY_CAML_LEAVES, caml_leaves);
    Store_field(policy, POLICY_CAML_MSRS,   caml_msrs);

    CAMLreturn(policy);
}

CAMLprim value
deserialise_policy(value xch, xen_cpuid_leaf_t* leaves, xen_msr_entry_t* msrs, value p)
{
    CAMLparam2(xch, p);
    uint32_t max_leaves = 0;
    uint32_t max_msrs = 0;

    if (xc_get_cpu_policy_size(_H(xch), &max_leaves, &max_msrs))
        failwith_xc(_H(xch));

    // These were passed in, so assume already allocated?
    // xen_cpuid_leaf_t *leaves = calloc(max_leaves, sizeof(xen_cpuid_leaf_t));
    // xen_msr_entry_t *msrs = calloc(max_leaves, sizeof(xen_cpuid_leaf_t));

    memcpy(leaves,  p.leaves, max_leaves);
    memcpy(msrs,    p.msrs, max_msrs);

    return 0;
}

// Hypercalls
CAMLprim value
stub_xenctrlext_get_max_nr_cpus(value xch)
{
    CAMLparam1(xch);
    xc_physinfo_t c_physinfo;
    int r;

    caml_enter_blocking_section();
    r = xc_physinfo(_H(xch), &c_physinfo);
    caml_leave_blocking_section();

    if (r)
        failwith_xc(_H(xch));

    CAMLreturn(Val_int(c_physinfo.max_cpu_id + 1));
}

CAMLprim value
stub_xc_cpu_policy_get_system(value xch, value idx, value arr)
{
    CAMLparam3(xch, idx, arr);
    int retval = xc_cpu_policy_get_system(_H(xch), Int_val(idx), &arr);
    if (retval)
        failwith_xc(_H(xch));
    CAMLreturn(Val_int(arr));
}

// Given two policies, return the intersection of their compatibility as a policy
CAMLprim value
stub_cpu_policy_calc_compatible(value xch, value left, value right)
{
    CAMLparam3(xch, left, right);
    int retval = xc_cpu_policy_calc_compatible(_H(xch), &left, &right);
    if (retval)
        failwith_xc(_H(xch));
    CAMLreturn(Val_int(left));
}

// Given two policies, return if they are compatible
CAMLprim value
stub_cpu_policy_is_compatible(value xch, value left, value right)
{
    CAMLparam3(xch, left, right);
    uint32_t max_leaves = 0;
    uint32_t max_msrs = 0;
    if (xc_get_cpu_policy_size(_H(xch), &max_leaves, &max_msrs))
        failwith_xc(_H(xch));

    xen_cpuid_leaf_t *leaves_left = calloc(max_leaves, sizeof(xen_cpuid_leaf_t));
    xen_msr_entry_t *msrs_left    = calloc(max_leaves, sizeof(xen_cpuid_leaf_t));

    xen_cpuid_leaf_t *leaves_right = calloc(max_leaves, sizeof(xen_cpuid_leaf_t));
    xen_msr_entry_t *msrs_right    = calloc(max_leaves, sizeof(xen_cpuid_leaf_t));

    deserialise_policy(xch, leaves_left, msrs_left, left);
    deserialise_policy(xch, leaves_right, msrs_right, right);

    left_policy  = {leaves_left, msrs_left};
    right_policy = {leaves_right, msrs_right};
    int retval = xc_cpu_policy_is_compatible(_H(xch), &left_policy, &right_policy);
    CAMLreturn(Val_int(retval));
}

// Given a serialised policy, return a serialised upgraded policy
CAMLprim serialised_policy_t
stub_upgrade_cpu_policy(value xch, serialised_policy_t policy)
{
    CAMLparam1(xch);
    uint32_t max_leaves = 0;
    uint32_t max_msrs = 0;

    if (xc_get_cpu_policy_size(_H(xch), &max_leaves, &max_msrs))
        failwith_xc(_H(xch));

    xen_cpuid_leaf_t *leaves = calloc(max_leaves, sizeof(xen_cpuid_leaf_t));
    xen_msr_entry_t *msrs = calloc(max_leaves, sizeof(xen_cpuid_leaf_t));

    deserialise_policy(xch, leaves, msrs, policy);

    int retval = xc_upgrade_cpu_policy(_H(xch), leaves,  msrs);
    if (retval)
        failwith_xc(_H(xch));

    policy = serialise_policy(xch, leaves, msrs);

    CAMLreturn(policy);
}

