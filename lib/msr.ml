type leaf = string

type policy = {
    version : int;
    max_leaves : int;
    max_msrs : int;
    leaves : leaf;
    msrs : leaf;
  }

let version policy = policy.version

let cpu_count policy = policy.max_leaves

let msr_count policy = policy.max_msrs

let to_string policy = (
  cpu_count policy,
  msr_count policy,
  version policy,
  String.escaped policy.leaves,
  String.escaped policy.msrs
)
