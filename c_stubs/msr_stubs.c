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

#define POLICY_VERSION      0
#define POLICY_MAX_LEAVES   1
#define POLICY_MAX_MSRS     2
#define POLICY_CAML_LEAVES  3
#define POLICY_CAML_MSRS    4

// A collection of C bindings for MSR specific operations

#ifndef XEN_CPUID_NO_SUBLEAF
typedef struct xen_cpuid_leaf {
#define XEN_CPUID_NO_SUBLEAF 0xffffffffu
    uint32_t leaf, subleaf;
    uint32_t a, b, c, d;
} xen_cpuid_leaf_t;
DEFINE_XEN_GUEST_HANDLE(xen_cpuid_leaf_t);

typedef struct xen_msr_entry {
    uint32_t idx;
    uint32_t flags; /* Reserved MBZ. */
    uint64_t val;
} xen_msr_entry_t;
DEFINE_XEN_GUEST_HANDLE(xen_msr_entry_t);
#define XEN_SYSCTL_cpu_policy_host 1
#endif

__attribute__((weak))
    int xc_get_cpu_policy_size(xc_interface *xch, uint32_t *nr_leaves,
                           uint32_t *nr_msrs);
__attribute__((weak))
    int xc_get_system_cpu_policy(xc_interface *xch, uint32_t index,
                             uint32_t *nr_leaves, xen_cpuid_leaf_t *leaves,
                                             uint32_t *nr_msrs, xen_msr_entry_t *msrs);
#define MSR_ARCH_CAPABILITIES 0x10a


// Type definitions
typedef struct policy {
    xen_cpuid_leaf_t* leaves;
    xen_msr_entry_t* msrs;
} policy_t;

typedef struct policy_compatibility {
    char* p;
    bool eq;
    char* reason;
} policy_compatibility_t;


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

CAMLprim value stub_xc_cpu_policy_get_system(value xch, value idx, value policy)
{
    CAMLparam3(xch, idx, policy);
    //int retval = xc_cpu_policy_get_system(_H(xch), Int_val(idx), &policy);
    int retval = 0;
    if (retval)
        failwith_xc(_H(xch));
    CAMLreturn(policy);
}

// Given two policies, return the intersection of their compatibility as a policy
CAMLprim value stub_cpu_policy_calc_compatible(value xch, value left, value right)
{
    CAMLparam3(xch, left, right);
    //int retval = xc_cpu_policy_calc_compatible(_H(xch), &left, &right);
    int retval = 0;
    if (retval)
        failwith_xc(_H(xch));
    CAMLreturn(left);
}

// Given two policies, return if they are compatible
// policy -> policy -> (policy, bool, string option) perhaps
CAMLprim value stub_cpu_policy_is_compatible(value xch, value left, value right)
{
    CAMLparam3(xch, left, right);
    uint32_t max_leaves = 0;
    uint32_t max_msrs = 0;
    //if (xc_get_cpu_policy_size(_H(xch), &max_leaves, &max_msrs))
    //    failwith_xc(_H(xch));

    //int retval = xc_cpu_policy_is_compatible(_H(xch), &left, &right);
    int retval = 0;
    if (retval)
        failwith_xc(_H(xch));

    // Possibly need to construct a record return here
    CAMLreturn(left);
}

// Given a policy, return an upgraded policy
CAMLprim value stub_upgrade_cpu_policy(value xch, value policy)
{
    CAMLparam2(xch, policy);

    //int retval = xc_upgrade_cpu_policy(_H(xch), &policy);
    int retval = 0;
    if (retval)
        failwith_xc(_H(xch));
    // Xen should put the return into the passed in policy
    CAMLreturn(policy);
}

